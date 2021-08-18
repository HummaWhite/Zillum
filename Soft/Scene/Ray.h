#pragma once

#include <iostream>
#include <cmath>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

struct Ray
{
	Ray() = default;

	Ray(const glm::vec3 &_ori, const glm::vec3 &_dir):
		ori(_ori), dir(glm::normalize(_dir)){}

	glm::vec3 get(float t) const { return ori + dir * t; }

	glm::vec3 ori;
	glm::vec3 dir;
};
