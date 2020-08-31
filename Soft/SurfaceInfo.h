#ifndef SURFACE_INFO_H
#define SURFACE_INFO_H

#include <iostream>
#include <memory>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "Material/Material.h"

struct SurfaceInfo
{
	SurfaceInfo(const glm::vec3 &_pos, const glm::vec3 &_norm, std::shared_ptr<Material> _material):
		pos(_pos), norm(_norm), material(_material) {}

	glm::vec3 pos;
	glm::vec3 norm;

	std::shared_ptr<Material> material;
};

#endif
