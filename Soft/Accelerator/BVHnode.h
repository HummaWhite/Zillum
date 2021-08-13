#pragma once

#include <memory>

#include "AABB.h"
#include "../Scene/Shapes.h"

class BVHNode
{
public:
	BVHNode() = default;
	BVHNode(const AABB& _box, int _offset, int _primCount, int _splitAxis):
		box(_box), offset(_offset), primCount(_primCount), splitAxis(_splitAxis) {}

	inline BoxHit hit(const Ray &ray) { return box.hit(ray); }
	inline bool isLeaf() const { return primCount == 1; }

public:
	int offset = 0;
	int primCount = 0;
	AABB box;
	BVHNode *lch = nullptr, *rch = nullptr;
	int splitAxis;
};

struct BVHNodeCompact
{
	AABB box;
	std::shared_ptr<Hittable> hittable;
};