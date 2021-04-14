#pragma once

#include <memory>

#include "AABB.h"
#include "../Hittable/Shapes.h"

class BVHnode
{
public:
	BVHnode(): offset(0), primCount(0) {};

	BVHnode(const AABB& _box, int _offset, int _primCount, int _splitAxis):
		box(_box), offset(_offset), primCount(_primCount), splitAxis(_splitAxis) {}

	inline bool hit(const Ray &ray, float &tMin, float &tMax)
	{
		return box.hit(ray, tMin, tMax);
	}

	inline bool isLeaf() const { return primCount == 1; }

public:
	int offset;
	int primCount;
	AABB box;
	BVHnode *lch = nullptr, *rch = nullptr;
	int splitAxis;
};

struct BVHnodeCompact
{
	AABB box;
	int sizeIndex;
};
