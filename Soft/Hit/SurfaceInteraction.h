#ifndef SURFACE_INTERACTION_H
#define SURFACE_INTERACTION_H

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

struct SurfaceInteraction
{
	SurfaceInteraction(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N):
		Wo(Wo), Wi(Wi), N(N) {}

	glm::vec3 Wo;
	glm::vec3 Wi;
	glm::vec3 N;
	glm::vec2 uv;
};

#endif
