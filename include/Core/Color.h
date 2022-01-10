#pragma once

#include <Windows.h>
#include <iostream>
#include <cmath>

#include "../../ext/glmIncluder.h"

struct RGB24
{
	RGB24() = default;

	RGB24(uint8_t _r, uint8_t _g, uint8_t _b) :
		r(glm::clamp<uint8_t>(_r, 0, 255u)),
		g(glm::clamp<uint8_t>(_g, 0, 255u)),
		b(glm::clamp<uint8_t>(_b, 0, 255u)) {}

	RGB24(Vec3f color)
	{
		Vec3f c = glm::clamp(color, Vec3f(0.0f), Vec3f(1.0f));
    	c *= 255;
    	*this = RGB24(c.x, c.y, c.z);
	}

	Vec3f toVec3() { return Vec3f(r, g, b) / 255.0f; }
	Vec4f toVec4() { return Vec4f(Vec3f(r, g, b) / 255.0f, 1.0f); }

	static RGB24 swapRB(RGB24 c)
	{
		std::swap(c.r, c.b);
    	return c;
	}
	
	uint8_t r, g, b;
};

struct RGBA32
{
	uint8_t r, g, b, a;
};