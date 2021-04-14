#pragma once

#include <Windows.h>
#include <iostream>
#include <cmath>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

struct RGB24
{
	RGB24() {}
	RGB24(BYTE _r, BYTE _g, BYTE _b):
		r(_r), g(_g), b(_b) 
	{
		r = glm::clamp((int)_r, 0, 255);
		g = glm::clamp((int)_g, 0, 255);
		b = glm::clamp((int)_b, 0, 255);
	}

	RGB24(glm::vec3 color)
	{
		glm::vec3 c = glm::clamp(color, glm::vec3(0.0f), glm::vec3(1.0f));
		c *= 255;
		*this = RGB24(c.x, c.y, c.z);
	}

	glm::vec3 toVec3()
	{
		return glm::vec3(r, g, b) / 255.0f;
	}

	glm::vec4 toVec4()
	{
		return glm::vec4(glm::vec3(r, g, b) / 255.0f, 1.0f);
	}

	static RGB24 swapRB(RGB24 c)
	{
		std::swap(c.r, c.b);
		return c;
	}
	
	BYTE r, g, b;
};

struct RGBA32
{
	BYTE r, g, b, a;
};
