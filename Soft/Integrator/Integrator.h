#ifndef INTEGRATOR_H
#define INTEGRATOR_H

#include <thread>

#include "../FrameBuffer.h"
#include "../Color.h"
#include "../Camera.h"
#include "../Texture.h"
#include "../Hittable/HittableShapes.h"
#include "../Shape/Shapes.h"
#include "../Light/Lights.h"
#include "../Material/Materials.h"
#include "../Math/Math.h"
#include "../Environment/Environments.h"
#include "../Accelerator/BVH.h"
#include "../ObjReader.h"
#include "../Scene.h"

class Integrator
{
public:
	Integrator(int width, int height, int maxSpp):
		width(width), height(height), maxSpp(maxSpp), maxThreads(std::thread::hardware_concurrency())
	{
		resultBuffer.init(width, height);
		resultBuffer.fill(glm::vec3(0.0f));
	}

	void setScene(std::shared_ptr<Scene> scene) { this->scene = scene; }

	FrameBuffer<glm::vec3>& result() { return resultBuffer; }

	virtual void render() = 0;

	void render(int spps);

protected:
	inline Ray getRay(float x, float y)
	{
		auto cam = scene->camera;
		glm::vec3 rayDir = (cam->front() + (cam->up() * y + cam->right() * ((float)width / height) * x) * (float)tan(glm::radians(cam->FOV() / 2.0))) * cam->nearPlane();
		return { cam->pos(), glm::normalize(rayDir) };
	}

public:
	bool modified = false;

protected:
	const int maxThreads;
	const int maxSpp;
	int curSpp = 0;

	int width, height;

	std::shared_ptr<Scene> scene;
	FrameBuffer<glm::vec3> resultBuffer;
};

#endif
