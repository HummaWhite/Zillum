#pragma once

#include <algorithm>
#include <optional>
#include <stack>
#include <list>

#include "../Scene/Hittable.h"
#include "AABB.h"

enum class BVHSplitMethod { SAH, Middle, EqualCounts, HLBVH };

const int BVHLeafMark = 0x80000000;

struct BVHNode
{
	AABB box;
	HittablePtr hittable;
	int size;
};

struct BVHTableElement
{
	int misNext;
	int nodeIndex;
};

struct HittableInfo
{
	AABB bound;
	Vec3f centroid;
	HittablePtr hittable;
};

class BVH
{
public:
	BVH() = default;
	BVH(const std::vector<HittablePtr> &hittables, BVHSplitMethod method = BVHSplitMethod::SAH);

	bool testIntersec(const Ray &ray, float dist);
	std::pair<float, HittablePtr> closestHit(const Ray &ray);

	int size() const { return treeSize; }
	AABB box() const { return tree[0].box; }

private:
	void quickBuild(std::vector<HittableInfo> &primInfo, const AABB &rootExtent);
	void standardBuild(std::vector<HittableInfo> &primInfo, const AABB &rootExtent);
	void buildHitTable();
	
private:
	int treeSize = 0;
	BVHSplitMethod splitMethod;

	std::vector<BVHNode> tree;
	std::vector<BVHTableElement> hitTables[6];
};