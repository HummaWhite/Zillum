#pragma once

#include <memory>

#include "AABB.h"
#include "../Hittable/Shapes.h"

class BVHNode
{
public:
	BVHNode(): offset(0), primCount(0) {};

	BVHNode(const AABB& _box, int _offset, int _primCount, int _splitAxis):
		box(_box), offset(_offset), primCount(_primCount), splitAxis(_splitAxis) {}

	inline BoxHit hit(const Ray &ray)
	{
		return box.hit(ray);
	}

	inline bool isLeaf() const { return primCount == 1; }

public:
	int offset;
	int primCount;
	AABB box;
	BVHNode *lch = nullptr, *rch = nullptr;
	int splitAxis;
};

struct BVHNodeCompact
{
	AABB box;
	std::shared_ptr<Hittable> hittable;
};