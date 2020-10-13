#ifndef BVH_H
#define BVH_H

#include <algorithm>
#include <tuple>

#include "BVHnode.h"

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

	BVH(const std::vector<std::shared_ptr<H>> &_hittables):
		hittables(_hittables)
	{
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
		auto res = dfsDetailed(root, 1);
		res.avgDepth /= treeSize;
		return res;
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

		//glm::vec3 vol = nodeBound.pMax - nodeBound.pMin;
		//std::cout << l << "  " << r << "\n";
		//std::cout << vol.x << "  " << vol.y << "  " << vol.z << "\n";
		if (l == r) return;

		//std::cout << "SplitAxis: " << dim << "\n";

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
		
		AABB boundInfo[hittableCount];
		AABB boundInfoRev[hittableCount];

		boundInfo[0] = hittableInfo[l].bound;
		boundInfoRev[hittableCount - 1] = hittableInfo[r].bound;

		for (int i = 1; i < hittableCount; i++)
		{
			boundInfo[i] = AABB(boundInfo[i - 1], hittableInfo[l + i].bound);
			boundInfoRev[hittableCount - 1 - i] = AABB(boundInfoRev[hittableCount - i], hittableInfo[r - i].bound);
		}
		
		int m = l;
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

		buildRecursive(k->lch, hittableInfo, boundInfo[m - l], l, m);
		buildRecursive(k->rch, hittableInfo, boundInfoRev[m + 1 - l], m + 1, r);
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

	DepthInfo dfsDetailed(BVHnode<H> *k, int depth)
	{
		if (k == nullptr) return DepthInfo();

		DepthInfo lRes = dfsDetailed(k->lch, depth + 1);
		DepthInfo rRes = dfsDetailed(k->rch, depth + 1);

		int curDepth = std::max(lRes.maxDepth, rRes.maxDepth) + 1;
		return DepthInfo{ curDepth, lRes.avgDepth + rRes.avgDepth + curDepth };
	}
	
private:
	const int maxHittablesInNode = 1;
	std::vector<std::shared_ptr<H>> hittables;

	BVHnode<H> *root = nullptr;
	int treeSize = 0;
};

#endif
