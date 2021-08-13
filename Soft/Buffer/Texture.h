#pragma once

#include <iostream>
#include <cstdlib>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "../Display/Color.h"
#include "FrameBuffer.h"
#include "../Math/Transform.h"

#include "../stb_image/stb_image.h"
#include "../stb_image/stb_image_write.h"

class Texture:
	public FrameBuffer<glm::vec3>
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

	glm::vec4 get(float u, float v);
	glm::vec4 getSpherical(const glm::vec3 &uv);

	void setFilterType(FilterType type) { filterType = type; }

	int texWidth() const { return width; }
	int texHeight() const { return height; }

public:
	FilterType filterType = FilterType::LINEAR;
};