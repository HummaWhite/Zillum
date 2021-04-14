#pragma once

#include <vector>
#include <memory>

#include "Hittable/Shapes.h"
#include "Hittable/Light.h"
#include "Environment/Environments.h"
#include "Accelerator/BVH.h"
#include "Camera.h"
#include "ObjReader.h"

class Scene
{
public:
	Scene() {}

	Scene(const std::vector<std::shared_ptr<Hittable>> &hittables, std::shared_ptr<Environment> environment, std::shared_ptr<Camera> camera):
		hittables(hittables), env(environment), camera(camera)
	{
		for (const auto &i : hittables)
		{
			if (i->type() == HittableType::Light)
			{
				lights.push_back(std::shared_ptr<Light>(dynamic_cast<Light*>(i.get())));
			}
		}
	}

	void setupLightSampleTable()
	{
		std::vector<float> lightPdf;
		for (const auto &lt : lights)
		{
			float pdf = Math::rgbBrightness(lt->getPower());
			lightPdf.push_back(pdf);
		}
		lightDistrib = Piecewise1D(lightPdf);
	}

	LightSample lightWeigt(std::shared_ptr<Light> lt, const glm::vec3 &x)
	{
		glm::vec3 y = lt->getRandomPoint();
		glm::vec3 Wi = glm::normalize(y - x);
		float dist = glm::distance(x, y);

		glm::vec3 N = lt->surfaceNormal(y);
		glm::vec3 weight(0.0f);

		Ray lightRay(x + Wi * 1e-4f, Wi);
		if (!quickIntersect(lightRay, dist))
		{
			weight = lt->getRadiance(y, Wi) * Math::satDot(N, -Wi) * lt->surfaceArea() / (dist * dist);
		}
		return {Wi, weight, lt->pdfLi(x, y)};
	}

	LightSample sampleLightUniform(const glm::vec3 &x)
	{
		auto lt = lights[uniformInt<int32_t>(0, lights.size() - 1)];
		glm::vec3 y = lt->getRandomPoint();
		glm::vec3 Wi = glm::normalize(y - x);
		float dist = glm::distance(x, y);

		glm::vec3 N = lt->surfaceNormal(y);
		glm::vec3 weight(0.0f);

		Ray lightRay(x + Wi * 1e-4f, Wi);
		float testDist = dist - 1e-4f - 1e-6f;
		if (!quickIntersect(lightRay, testDist))
		{
			weight = lt->getRadiance(y, Wi) * Math::satDot(N, -Wi) * lt->surfaceArea() / (dist * dist);
		}
		float pdfSample = 1.0f / lights.size();
		return { Wi, weight / pdfSample, lt->pdfLi(x, y) * pdfSample };
	}

	LightSample sampleLightByPower(const glm::vec3 &x)
	{
		auto lt = lights[lightDistrib.sample()];
		glm::vec3 y = lt->getRandomPoint();
		glm::vec3 Wi = glm::normalize(y - x);
		float dist = glm::length(y - x);

		glm::vec3 N = lt->surfaceNormal(y);
		glm::vec3 weight(0.0f);

		Ray lightRay(x + Wi * 1e-4f, Wi);
		float testDist = dist - 1e-4f - 1e-6f;
		if (!quickIntersect(lightRay, testDist))
		{
			weight = lt->getRadiance(y, Wi) * Math::satDot(N, -Wi) * lt->surfaceArea() / (dist * dist);
		}
		float pdfSample = Math::rgbBrightness(lt->getPower()) / lightDistrib.sum();
		return { Wi, weight / pdfSample, lt->pdfLi(x, y) * pdfSample };
	}

	LightSample sampleEnvironment(glm::vec3 &x)
	{
		auto [Wi, pdf] = env->importanceSample();
		auto rad = env->getRadiance(Wi);
		Ray ray(x + Wi * 1e-4f, Wi);
		float tmp = 1e6;
		if (quickIntersect(ray, tmp))
		{
			rad = glm::vec3(0.0f);
			pdf = 1.0f;
		}
		return { Wi, rad / pdf, pdf };
	}

	void buildScene()
	{
		bvh = std::make_shared<BVH>(hittables);
		auto [maxDepth, avgDepth] = bvh->dfsDetailed();
		std::cout << "[BVH] TreeSize: " << bvh->size() << "  MaxDepth: " << maxDepth << "  AvgDepth: " << avgDepth << "\n";
		bvh->makeCompact();
		setupLightSampleTable();
	}

	std::pair<float, std::shared_ptr<Hittable>> closestHit(const Ray &ray)
	{
		float dist = 1e6f;
		auto obj = bvh->closestHit(ray, dist, false);
		return { dist, obj };
	}

	bool quickIntersect(const Ray &ray, float &dist)
	{
		auto obj = bvh->closestHit(ray, dist, true);
		return obj != nullptr;
	}

	void addHittable(std::shared_ptr<Hittable> hittable)
	{
		hittables.push_back(hittable);
	}

	void addLight(std::shared_ptr<Light> light)
	{
		lights.push_back(light);
		hittables.push_back(light);
	}

	void addObjectMesh(const char *path, std::shared_ptr<Transform> transform, std::shared_ptr<Material> material)
	{
		auto [vertices, texcoords, normals] = ObjReader::readFile(path);
		int faceCount = vertices.size() / 3;
		for (int i = 0; i < faceCount; i++)
		{
			glm::vec3 v[] = { vertices[i * 3 + 0], vertices[i * 3 + 1], vertices[i * 3 + 2] };
			glm::vec3 n[] = { normals[i * 3 + 0], normals[i * 3 + 1], normals[i * 3 + 2] };
			glm::vec2 t[3];

			auto tr = std::make_shared<MeshTriangle>(v, t, n);
			tr->setTransform(transform);
			hittables.push_back(std::make_shared<Object>(tr, material));
		}
	}

	void addLightMesh(const char *path, std::shared_ptr<Transform> transform, const glm::vec3 &power)
	{
		auto [vertices, texcoords, normals] = ObjReader::readFile(path);
		int faceCount = vertices.size() / 3;
		for (int i = 0; i < faceCount; i++)
		{
			glm::vec3 v[] = { vertices[i * 3 + 0], vertices[i * 3 + 1], vertices[i * 3 + 2] };
			glm::vec3 n[] = { normals[i * 3 + 0], normals[i * 3 + 1], normals[i * 3 + 2] };
			glm::vec2 t[3];

			auto tr = std::make_shared<Light>(std::make_shared<MeshTriangle>(v, t, n), power, false);

			tr->setTransform(transform);
			hittables.push_back(tr);
			lights.push_back(tr);
		}
	}

public:
	std::vector<std::shared_ptr<Hittable>> hittables;
	std::vector<std::shared_ptr<Light>> lights;
	std::shared_ptr<Environment> env;
	std::shared_ptr<Camera> camera;

	std::shared_ptr<BVH> bvh;
	Piecewise1D lightDistrib;
};
