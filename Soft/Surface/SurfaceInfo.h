#pragma once

#include <iostream>
#include <memory>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "../Material/Material.h"

struct SurfaceInfo
{
	SurfaceInfo(const glm::vec2 &uv, const glm::vec3 &N, MaterialPtr mat):
		uv(uv), N(N), mat(mat) {}

	glm::vec2 uv;
	glm::vec3 N;
	MaterialPtr mat;
};
