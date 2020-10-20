#ifndef BVH_H
#define BVH_H

#include <algorithm>
#include <tuple>

#include "BVHnode.h"

enum class BVHSplitMethod { SAH, Middle, EqualCounts };

template<typename H>
class BVH
{
public:
	struct DepthInfo
	{
		int maxDepth = 0;
		float avgDepth = 0.0f;
	};

public:
	BVH() {}

	BVH(const std::vector<std::shared_ptr<H>> &_hittables, BVHSplitMethod method = BVHSplitMethod::SAH):
		hittables(_hittables), splitMethod(method)
	{
		if (hittables.size() == 0) return;
		std::vector<HittableInfo> hittableInfo(hittables.size());

		AABB bound;
		for (int i = 0; i < hittables.size(); i++)
		{
			AABB box = hittables[i]->bound();
			hittableInfo[i] = { box, box.centroid(), i };
			bound = AABB(bound, box);
		}

		buildRecursive(root, hittableInfo, bound, 0, hittableInfo.size() - 1);

		std::vector<std::shared_ptr<H>> orderedPrims(hittableInfo.size());
		for (int i = 0; i < hittableInfo.size(); i++)
		{
			orderedPrims[i] = hittables[hittableInfo[i].index];
		}

		hittables = orderedPrims;
	}

	~BVH()
	{
		//destroyRecursive(root);
	}

	void makeCompact()
	{
		makeCompact(root);
	}

	std::shared_ptr<H> closestHit(const Ray &ray, float &tMin, float &tMax)
	{
		return closestHit(root, ray, tMin, tMax);
	}

	void dfs()
	{
		dfs(root, 1);
	}

	int size() const { return treeSize; }

	DepthInfo dfsDetailed()
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
	void buildRecursive(BVHnode<H> *&k, std::vector<HittableInfo> &hittableInfo, const AABB &nodeBound, int l, int r)
	{
		// [l, r]
		int dim = (l == r) ? -1 : nodeBound.maxExtent();
		k = new BVHnode<H>(nodeBound, l, r - l + 1, dim);
		treeSize++;

		//std::cout << l << "  " << r << "  SplitAxis: " << dim << "\n";
		if (l == r) return;

		auto cmp =
			dim == 0 ?
				[](const HittableInfo &a, const HittableInfo &b)
				{	
					return a.centroid.x < b.centroid.x;
				} :
			dim == 1 ?
				[](const HittableInfo &a, const HittableInfo &b)
				{
					return a.centroid.y < b.centroid.y;
				} :
				[](const HittableInfo &a, const HittableInfo &b)
				{
					return a.centroid.z < b.centroid.z;
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
					float mid =
						dim == 0 ? nodeCentroid.x :
						dim == 1 ? nodeCentroid.y :
						nodeCentroid.z;
					for (m = l; m < r - 1; m++)
					{
						float tmp =
							dim == 0 ? hittableInfo[m].centroid.x :
							dim == 1 ? hittableInfo[m].centroid.y :
							hittableInfo[m].centroid.z;
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

	void makeCompact(BVHnode<H> *k)
	{
	}

	void destroyRecursive(BVHnode<H> *k)
	{
		if (k == nullptr) return;

		if (k->lch != nullptr)
		{
			if (k->lch->isLeaf()) delete k;
			else destroyRecursive(k->lch);
		}
		if (k->rch != nullptr)
		{
			if (k->rch->isLeaf()) delete k;
			else destroyRecursive(k->rch);
		}
	}

	std::shared_ptr<H> closestHit(BVHnode<H> *k, const Ray &ray, float &tMin, float &tMax)
	{
		if (k == nullptr) return nullptr;

		//std::cout << k->offset << " " << k->primCount << std::endl;

		if (k == root && !k->hit(ray, tMin, tMax)) return nullptr;

		if (k->isLeaf())
		{
			HitInfo hInfo = hittables[k->offset]->closestHit(ray);
			if (!hInfo.hit) return nullptr;
			tMin = hInfo.dist;
			return hittables[k->offset];
		}

		bool lhit = false, rhit = false;
		float lMin, lMax, rMin, rMax;

		if (k->lch != nullptr) lhit = k->lch->box.hit(ray, lMin, lMax);
		if (k->rch != nullptr) rhit = k->rch->box.hit(ray, rMin, rMax);

		//左右包围盒都相交，进而检查两个子结点内的形状是否真的与光线相交
		//（有可能光线与包围盒相交而不与包围盒内的形状相交）
		if (lhit && rhit)
		{
			auto lRes = closestHit(k->lch, ray, lMin, lMax);
			auto rRes = closestHit(k->rch, ray, rMin, rMax);

			//左右子树都有形状与光线相交，取近
			if (lRes != nullptr && rRes != nullptr)
			{
				if (lMin < rMin)
				{
					tMin = lMin, tMax = lMax;
					return lRes;
				}
				else
				{
					tMin = rMin, tMax = rMax;
					return rRes;
				}
			}
			
			if (lRes != nullptr)
			{
				tMin = lMin, tMax = lMax;
				return lRes;
			}

			if (rRes != nullptr)
			{
				tMin = rMin, tMax = rMax;
				return rRes;
			}

			return nullptr;
		}

		if (lhit) return closestHit(k->lch, ray, tMin, tMax);
		if (rhit) return closestHit(k->rch, ray, tMin, tMax);

		return nullptr;
	}

	void dfs(BVHnode<H> *k, int depth)
	{
		if (k == nullptr) return;
		std::cout << depth << "  " << k->offset << " " << k->primCount << " " << k->splitAxis << std::endl;

		if (k->lch != nullptr) dfs(k->lch, depth + 1);
		if (k->rch != nullptr) dfs(k->rch, depth + 1);
	}

	int dfsDetailed(BVHnode<H> *k, int depth, int &sumDepth)
	{
		if (k == nullptr) return 0;

		sumDepth += depth;
		int lDep = dfsDetailed(k->lch, depth + 1, sumDepth);
		int rDep = dfsDetailed(k->rch, depth + 1, sumDepth);

		return std::max(lDep, rDep) + 1;
	}
	
private:
	const int maxHittablesInNode = 1;
	std::vector<std::shared_ptr<H>> hittables;

	BVHnode<H> *root = nullptr;
	int treeSize = 0;
	BVHSplitMethod splitMethod;
};

#endif
