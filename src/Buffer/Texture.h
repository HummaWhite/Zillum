#pragma once

#include <iostream>
#include <cstdlib>

#include "../Display/Color.h"
#include "Buffer2D.h"
#include "../Math/Transform.h"

#include "../stb_image/stb_image.h"
#include "../stb_image/stb_image_write.h"

class Texture:
	public Buffer2D<glm::vec3>
{
public:
	enum FilterType
	{
		NEAREST = 0,
		LINEAR
	};

public:
	void loadRGB24(const char *filePath);
	void loadFloat(const char *filePath);

	Vec4f get(float u, float v);
	Vec4f getSpherical(const glm::vec3 &uv);

	void setFilterType(FilterType type) { filterType = type; }

	int texWidth() const { return width; }
	int texHeight() const { return height; }

public:
	FilterType filterType = FilterType::LINEAR;
};