#pragma once

#include <algorithm>
#include <optional>
#include <stack>
#include <list>

#include "BVHnode.h"

enum class BVHSplitMethod { SAH, Middle, EqualCounts, HLBVH };

struct BVHTransTableElement
{
	int misNext;
	int nodeIndex;
};

class BVH
{
public:
	BVH() {}

	BVH(const std::vector<std::shared_ptr<Hittable>> &_hittables, BVHSplitMethod method = BVHSplitMethod::SAH):
		hittables(_hittables), splitMethod(method)
	{
		if (hittables.size() == 0) return;
		std::vector<HittableInfo> hittableInfo(hittables.size());

		AABB bound = hittables[0]->bound();
		hittableInfo[0] = { bound, bound.centroid(), 0 };
		for (int i = 1; i < hittables.size(); i++)
		{
			AABB box = hittables[i]->bound();
			hittableInfo[i] = { box, box.centroid(), i };
			bound = AABB(bound, box);
		}

		treeSize = hittables.size() * 2 - 1;
		buildRecursive(root, hittableInfo, bound, 0, hittableInfo.size() - 1);

		std::vector<std::shared_ptr<Hittable>> orderedPrims(hittableInfo.size());
		for (int i = 0; i < hittableInfo.size(); i++)
		{
			orderedPrims[i] = hittables[hittableInfo[i].index];
		}

		hittables = orderedPrims;
		makeCompact();
		buildHitTable();
		destroyRecursive(root);
	}

	~BVH()
	{
	}

	inline bool testIntersec(const Ray &ray, float dist)
	{
		if (treeSize == 0) return false;
		BVHTransTableElement *table = transTables[Math::cubeMapFace(-ray.dir)];

		int k = 0;
		while (k != treeSize)
		{
			auto [box, hittable] = compactNodes[table[k].nodeIndex];
			auto [boxHit, tMin, tMax] = box.hit(ray);

			if (!boxHit || (boxHit && tMin > dist))
			{
				k = table[k].misNext;
				continue;
			}

			if (hittable != nullptr)
			{
				auto t = hittable->closestHit(ray);
				if (t.has_value())
				{
					if (t.value() < dist) return true;
				}
			}
			k++;
		}
		return false;
	}

	inline std::pair<float, std::shared_ptr<Hittable>> closestHit(const Ray &ray)
	{
		if (treeSize == 0) return { 0.0f, nullptr };
		float dist = 1e8f;
		std::shared_ptr<Hittable> hit;
		BVHTransTableElement *table = transTables[Math::cubeMapFace(-ray.dir)];

		int k = 0;
		while (k != treeSize)
		{
			auto [box, hittable] = compactNodes[table[k].nodeIndex];
			auto [boxHit, tMin, tMax] = box.hit(ray);

			if (!boxHit || (boxHit && tMin > dist))
			{
				k = table[k].misNext;
				continue;
			}

			if (hittable != nullptr)
			{
				auto t = hittable->closestHit(ray);
				if (t.has_value())
				{
					if (t.value() < dist)
					{
						dist = t.value();
						hit = hittable;
					}
				}
			}
			k++;
		}
		return { dist, hit };
	}

	inline void dfs()
	{
		dfs(root, 1);
	}

	inline int size() const { return treeSize; }

	std::pair<int, float> dfsDetailed()
	{
		int sumDepth = 0;
		int maxDepth = dfsDetailed(root, 1, sumDepth);
		return { maxDepth, (float)sumDepth / treeSize };
	}

private:
	struct HittableInfo
	{
		AABB bound;
		glm::vec3 centroid;
		int index;
	};

private:
	void buildRecursive(BVHNode *&k, std::vector<HittableInfo> &hittableInfo, const AABB &nodeBound, int l, int r)
	{
		int dim = (l == r) ? -1 : nodeBound.maxExtent();
		k = new BVHNode(nodeBound, l, (r - l) * 2 + 1, dim);

		if (l == r) return;

		auto cmp = [dim, this](const HittableInfo &a, const HittableInfo &b)
		{
			return Math::vecElement(a.centroid, dim) < Math::vecElement(b.centroid, dim);
		};

		std::sort(hittableInfo.begin() + l, hittableInfo.begin() + r, cmp);
		int hittableCount = r - l + 1;

		if (hittableCount == 2)
		{
			buildRecursive(k->lch, hittableInfo, hittableInfo[l].bound, l, l);
			buildRecursive(k->rch, hittableInfo, hittableInfo[r].bound, r, r);
			return;
		}

		AABB *boundInfo = new AABB[hittableCount];
		AABB *boundInfoRev = new AABB[hittableCount];

		boundInfo[0] = hittableInfo[l].bound;
		boundInfoRev[hittableCount - 1] = hittableInfo[r].bound;

		for (int i = 1; i < hittableCount; i++)
		{
			boundInfo[i] = AABB(boundInfo[i - 1], hittableInfo[l + i].bound);
			boundInfoRev[hittableCount - 1 - i] = AABB(boundInfoRev[hittableCount - i], hittableInfo[r - i].bound);
		}

		int m = l;
		switch (splitMethod)
		{
			case BVHSplitMethod::SAH:
				{
					m = l;
					float cost = boundInfo[0].surfaceArea() + boundInfoRev[1].surfaceArea() * (r - l);

					for (int i = 1; i < hittableCount - 1; i++)
					{
						float c = boundInfo[i].surfaceArea() * (i + 1) + boundInfoRev[i + 1].surfaceArea() * (hittableCount - i - 1);
						if (c < cost)
						{
							cost = c;
							m = l + i;
						}
					}
				} break;

			case BVHSplitMethod::Middle:
				{
					glm::vec3 nodeCentroid = nodeBound.centroid();
					float mid = Math::vecElement(nodeCentroid, dim);
					for (m = l; m < r - 1; m++)
					{
						float tmp = Math::vecElement(hittableInfo[m].centroid, dim);
						if (tmp >= mid) break;
					}
				} break;

			case BVHSplitMethod::EqualCounts:
				m = (l + r) >> 1;
				break;

			default: break;
		}

		AABB lBound = boundInfo[m - l];
		AABB rBound = boundInfoRev[m + 1 - l];
		delete[] boundInfo;
		delete[] boundInfoRev;

		buildRecursive(k->lch, hittableInfo, lBound, l, m);
		buildRecursive(k->rch, hittableInfo, rBound, m + 1, r);
	}

	void destroyRecursive(BVHNode *&k)
	{
		if (k == nullptr) return;
		if (!k->isLeaf())
		{
			destroyRecursive(k->lch);
			destroyRecursive(k->rch);
		}
		delete k;
	}

	void makeCompact()
	{
		if (treeSize == 0) return;
		compactNodes = new BVHNodeCompact[treeSize];

		int offset = 0;
		std::stack<BVHNode*> st;
		st.push(root);

		while (!st.empty())
		{
			BVHNode *k = st.top();
			st.pop();

			compactNodes[offset] = { k->box, k->isLeaf() ? hittables[k->offset] : std::shared_ptr<Hittable>() };
			offset++;

			if (k->isLeaf()) continue;
			st.push(k->rch);
			st.push(k->lch);
		}
		std::cout << "[BVH] made compact\n";
	}

	void buildHitTable()
	{
		bool (*cmpFuncs[6])(const glm::vec3 &a, const glm::vec3 &b) =
		{
			[](const glm::vec3 &a, const glm::vec3 &b) { return a.x > b.x; },	// X+
			[](const glm::vec3 &a, const glm::vec3 &b) { return a.x < b.x; },	// X-
			[](const glm::vec3 &a, const glm::vec3 &b) { return a.y > b.y; },	// Y+
			[](const glm::vec3 &a, const glm::vec3 &b) { return a.y < b.y; },	// Y-
			[](const glm::vec3 &a, const glm::vec3 &b) { return a.z > b.z; },	// Z+
			[](const glm::vec3 &a, const glm::vec3 &b) { return a.z < b.z; }	// Z-
		};

		for (int i = 0; i < 6; i++)
		{
			transTables[i] = new BVHTransTableElement[treeSize];
			BVHTransTableElement *table = transTables[i];
			std::stack<std::pair<BVHNode*, int>> st;
			st.push({ root, 0 });

			int index = 0;
			while (!st.empty())
			{
				auto [k, offNodeList] = st.top();
				st.pop();

				table[index] = { index + k->primCount, offNodeList };
				index++;
				if (k->isLeaf()) continue;

				BVHNode *lch = k->lch, *rch = k->rch;
				int loff = offNodeList + 1;
				int roff = loff + lch->primCount;

				if (!cmpFuncs[i](lch->box.centroid(), rch->box.centroid()))
				{
					std::swap(lch, rch);
					std::swap(loff, roff);
				}
				st.push({ rch, roff });
				st.push({ lch, loff });
			}
		}
	}

	void dfs(BVHNode *k, int depth)
	{
		if (k == nullptr) return;
		std::cout << depth << "  " << k->offset << " " << k->primCount << " " << k->splitAxis << std::endl;

		dfs(k->lch, depth + 1);
		dfs(k->rch, depth + 1);
	}

	int dfsDetailed(BVHNode *k, int depth, int &sumDepth)
	{
		if (k == nullptr) return 0;

		sumDepth += depth;
		int lDep = dfsDetailed(k->lch, depth + 1, sumDepth);
		int rDep = dfsDetailed(k->rch, depth + 1, sumDepth);

		return std::max(lDep, rDep) + 1;
	}
	
private:
	const int maxHittablesInNode = 1;
	std::vector<std::shared_ptr<Hittable>> hittables;

	BVHNode *root = nullptr;
	int treeSize = 0;
	BVHSplitMethod splitMethod;

	BVHNodeCompact *compactNodes;
	BVHTransTableElement *transTables[6];
};