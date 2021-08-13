#pragma once

#include <Windows.h>
#include <iostream>
#include <cmath>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

struct RGB24
{
	RGB24() = default;
	RGB24(uint8_t _r, uint8_t _g, uint8_t _b) :
		r(glm::clamp<uint8_t>(_r, 0, 255u)),
		g(glm::clamp<uint8_t>(_g, 0, 255u)),
		b(glm::clamp<uint8_t>(_b, 0, 255u)) {}
	RGB24(glm::vec3 color);

	glm::vec3 toVec3() { return glm::vec3(r, g, b) / 255.0f; }
	glm::vec4 toVec4() { return glm::vec4(glm::vec3(r, g, b) / 255.0f, 1.0f); }

	static RGB24 swapRB(RGB24 c);
	
	uint8_t r, g, b;
};

struct RGBA32
{
	uint8_t r, g, b, a;
};