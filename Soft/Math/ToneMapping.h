#pragma once

#include <iostream>
#include <cmath>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

namespace ToneMapping
{
	glm::vec3 reinhard(const glm::vec3 &color);
	glm::vec3 CE(const glm::vec3 &color);
	glm::vec3 filmic(const glm::vec3 &color);
	glm::vec3 ACES(const glm::vec3 &color);
}
