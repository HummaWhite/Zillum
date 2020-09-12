#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <iostream>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

class Environment
{
public:
	virtual glm::vec3 getRadiance(const glm::vec3 &dir) = 0;
};

#endif
