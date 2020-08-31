#ifndef MATERIAL_H
#define MATERIAL_H

#include <iostream>
#include <random>
#include <cmath>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "../Ray.h"

struct Material
{
	virtual Ray scatter(const Ray &rayIn, const glm::vec3 &N) = 0;
	virtual glm::vec3 reflectionRate() = 0;
};

struct SampleMaterial:
	Material
{
	SampleMaterial(const glm::vec3 &_reflRate):
		reflRate(_reflRate) {}

	Ray scatter(const Ray &ray, const glm::vec3 &N)
	{
		glm::vec3 U(0.0f, 0.0f, 1.0f);

		glm::vec3 R = glm::cross(N, U);
		U = glm::cross(R, N);

		glm::mat3 normMatrix(U, R, N);
		glm::vec3 dirRefl = glm::reflect(ray.dir, N);

		std::uniform_real_distribution<float> randF(0.0f, 1.0f);
		std::default_random_engine generator;

		glm::vec3 dirOut(randF(generator), randF(generator), randF(generator));
		dirOut = glm::normalize(normMatrix * dirOut);

		return { ray.ori, dirRefl };
	}

	glm::vec3 reflectionRate() { return reflRate; }

	glm::vec3 reflRate;
};

#endif
