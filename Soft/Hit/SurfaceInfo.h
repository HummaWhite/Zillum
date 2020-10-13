#ifndef SURFACE_INFO_H
#define SURFACE_INFO_H

#include <iostream>
#include <memory>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "../Material/Material.h"

struct SurfaceInfo
{
	SurfaceInfo(const glm::vec2 &_texCoord, const glm::vec3 &_norm, std::shared_ptr<Material> _material):
		texCoord(_texCoord), norm(_norm), material(_material) {}

	glm::vec2 texCoord;
	glm::vec3 norm;
	std::shared_ptr<Material> material;
};

#endif
