#ifndef HITINFO_H
#define HITINFO_H

#include <iostream>
#include <memory>

#include "../Ray.h"
#include "SurfaceInfo.h"

struct HitInfo
{
	HitInfo(bool _hit, float _dist):
		hit(_hit), dist(_dist) {}

	bool hit = false;
	float dist = 0.0f;
};

#endif
