#ifndef FILTER_H
#define FILTER_H

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "Math/RandomGenerator.h"

namespace Filter
{
	inline glm::vec3 box(const glm::vec2 &sample)
	{
		return glm::vec3(sample - 0.5f, 1.0f);
	}

	inline glm::vec3 tent(const glm::vec2 &sample)
	{
		glm::vec2 s = sample * 2.0f - 1.0f;
		return glm::vec3(s, (1.0f - glm::abs(s.x)) * (1.0f - glm::abs(s.y)));
	}
}

#endif
