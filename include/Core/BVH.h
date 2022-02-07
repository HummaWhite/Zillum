#pragma once

#include <algorithm>
#include <optional>
#include <stack>
#include <list>

#include "Hittable.h"
#include "AABB.h"

enum class BVHSplitMethod { SAH, Middle, EqualCounts, HLBVH };

const int BVHLeafMark = 0x80000000;

struct BVHNode
{
	AABB bound;
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

	int size() const { return mTreeSize; }
	AABB box() const { return mTree[0].bound; }

private:
	void quickBuild(std::vector<HittableInfo> &primInfo, const AABB &rootExtent);
	void standardBuild(std::vector<HittableInfo> &primInfo, const AABB &rootExtent);
	void buildHitTable();
	
private:
	int mTreeSize = 0;
	BVHSplitMethod mSplitMethod;

	std::vector<BVHNode> mTree;
	std::vector<BVHTableElement> mHitTables[6];
};