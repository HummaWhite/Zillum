#ifndef MATH_H
#define MATH_H

#include <iostream>
#include <cmath>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"
#include "RandomGenerator.h"

namespace Math
{
	static glm::vec2 sphereToPlane(const glm::vec3 &uv)
	{
		float pi = glm::pi<float>();

		float theta = glm::atan(uv.y, uv.x);
		if (theta < 0.0f) theta += pi * 2.0f;
		
		float phi = glm::atan(glm::length(glm::vec2(uv)), uv.z);
		if (phi < 0.0f) phi += pi * 2.0f;

		return { theta / (pi * 2.0f), phi / pi };
	}

	static glm::vec3 planeToSphere(const glm::vec2 &uv)
	{
		float theta = uv.x * glm::pi<float>() * 2.0f;
		float phi = uv.y * glm::pi<float>();
		return { cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi) };
	}

	static float radicalInverseVDC(unsigned int bits)
	{
		bits = (bits << 16u) | (bits >> 16u);
    	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    	return float(bits) * 2.3283064365386963e-10;
	}

	static glm::vec2 hammersley(unsigned int i, unsigned int n)
	{
		return glm::vec2((float)i / (float)n, radicalInverseVDC(i));
	}
}

#endif
