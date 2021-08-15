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
	BVH() = default;
	BVH(const std::vector<HittablePtr> &_hittables, BVHSplitMethod method = BVHSplitMethod::SAH);
	~BVH() = default;

	bool testIntersec(const Ray &ray, float dist);
	std::pair<float, HittablePtr> closestHit(const Ray &ray);
	void dfs() { dfs(root, 1); }
	std::pair<int, float> dfsDetailed();
	AABB box() const;

	inline int size() const { return treeSize; }

private:
	struct HittableInfo
	{
		AABB bound;
		glm::vec3 centroid;
		int index;
	};

private:
	void radixSort16(HittableInfo *a, int count, int dim);
	void buildRecursive(BVHNode *&k, std::vector<HittableInfo> &hittableInfo, const AABB &nodeBound, int l, int r);
	void destroyRecursive(BVHNode *&k);
	void makeCompact();
	void buildHitTable();

	void dfs(BVHNode *k, int depth);
	int dfsDetailed(BVHNode *k, int depth, int &sumDepth);
	
private:
	const int maxHittablesInNode = 1;
	std::vector<HittablePtr> hittables;

	BVHNode *root = nullptr;
	int treeSize = 0;
	BVHSplitMethod splitMethod;

	BVHNodeCompact *compactNodes;
	BVHTransTableElement *transTables[6];
};