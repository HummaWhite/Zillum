#ifndef BVH_NODE_H
#define BVH_NODE_H

#include <memory>

#include "../Bound/AABB.h"
#include "../Hittable/HittableShapes.h"
#include "../Light/Lights.h"
#include "../Shape/Shapes.h"

template<typename H>
class BVHnode
{
public:
	BVHnode(): offset(0), primCount(0) {};

	BVHnode(const AABB& _box, int _offset, int _primCount, int _splitAxis):
		box(_box), offset(_offset), primCount(_primCount), splitAxis(_splitAxis) {}

	bool hit(const Ray &ray, float &tMin, float &tMax)
	{
		return box.hit(ray, tMin, tMax);
	}

	bool isLeaf() const { return primCount == 1; }

public:
	int offset;
	int primCount;
	AABB box;
	BVHnode *lch = nullptr, *rch = nullptr;
	int splitAxis;
};

#endif
