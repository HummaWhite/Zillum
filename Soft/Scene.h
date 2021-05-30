#pragma once

#include <vector>
#include <memory>

#include "Hittable/Shapes.h"
#include "Hittable/Light.h"
#include "Environment/Environments.h"
#include "Accelerator/BVH.h"
#include "Camera.h"
#include "ObjReader.h"

enum class LightSelectStrategy
{
	ByPower, Uniform
};

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
			float pdf = lt->getRgbPower();
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
			weight = lt->getRadiance(y, -Wi) * Math::satDot(N, -Wi) * lt->surfaceArea() / (dist * dist);
		}
		return {Wi, weight, lt->pdfLi(x, y)};
	}

	LightSample sampleOneLight(const glm::vec3 &x)
	{
		if (lights.size() == 0) return INVALID_LIGHT_SAMPLE;
		bool sampleByPower = lightSelectStrategy == LightSelectStrategy::ByPower;
		int index = sampleByPower ? lightDistrib.sample() : uniformInt<int>(0, lights.size() - 1);

		auto lt = lights[index];
		glm::vec3 y = lt->getRandomPoint();
		glm::vec3 Wi = glm::normalize(y - x);
		glm::vec3 N = lt->surfaceNormal(y);
		float cosTheta = glm::dot(N, -Wi);

		if (cosTheta <= 1e-6f) return INVALID_LIGHT_SAMPLE;

		float dist = glm::distance(x, y);
		float pdf = dist * dist / (lt->surfaceArea() * cosTheta);

		Ray lightRay(x + Wi * 1e-4f, Wi);
		float testDist = dist - 1e-4f - 1e-6f;

		if (bvh->testIntersec(lightRay, testDist) || pdf < 1e-8f) return INVALID_LIGHT_SAMPLE;

		glm::vec3 weight = lt->getRadiance(y, -Wi);
		float pdfSample = sampleByPower ? lt->getRgbPower() / lightDistrib.sum() : 1.0f / lights.size();
		pdf *= pdfSample;
		return { Wi, weight / pdf, pdf };
	}

	LightSample sampleEnvironment(const glm::vec3 &x)
	{
		auto [Wi, pdf] = env->importanceSample();

		Ray ray(x + Wi * 1e-4f, Wi);
		float tmp = 1e6;
		if (quickIntersect(ray, tmp)) return INVALID_LIGHT_SAMPLE;

		auto rad = env->getRadiance(Wi);
		return { Wi, rad / pdf, pdf };
	}

	LightSample sampleLightAndEnv(const glm::vec3 &x)
	{	
		float r = uniformFloat();
		float pdfSampleLight = 0.0f;
		
		if (lights.size() > 0)
		{
			pdfSampleLight = lightAndEnvStrategy == LightSelectStrategy::ByPower ?
				lightDistrib.sum() / powerlightAndEnv() : 0.5f;
		}

		bool sampleLight = r < pdfSampleLight;
		float pdfSelect = sampleLight ? pdfSampleLight : 1.0f - pdfSampleLight;

		auto [Wi, coef, pdf] = sampleLight ? sampleOneLight(x) : sampleEnvironment(x);
		return { Wi, coef / pdfSelect, pdf * pdfSelect };
	}

	float pdfSelectLight(Light *lt)
	{
		float fstPdf = lightSelectStrategy == LightSelectStrategy::ByPower ?
			lt->getRgbPower() / lightDistrib.sum() :
			1.0f / lights.size();

		float sndPdf = lightAndEnvStrategy == LightSelectStrategy::ByPower ?
			lightDistrib.sum() / powerlightAndEnv():
			0.5f;

		return fstPdf * sndPdf;
	}

	float pdfSelectEnv()
	{
		return lightAndEnvStrategy == LightSelectStrategy::ByPower ?
			env->power() / (lightDistrib.sum() + env->power()) :
			0.5f;
	}

	float powerlightAndEnv()
	{
		return lightDistrib.sum() + env->power();
	}

	void buildScene()
	{
		bvh = std::make_shared<BVH>(hittables);
		auto [maxDepth, avgDepth] = bvh->dfsDetailed();
		std::cout << "[BVH] TreeSize: " << bvh->size() << "  MaxDepth: " << maxDepth << "  AvgDepth: " << avgDepth << "\n";
		setupLightSampleTable();
	}

	std::pair<float, std::shared_ptr<Hittable>> closestHit(const Ray &ray)
	{
		return bvh->closestHit(ray);
	}

	bool quickIntersect(const Ray &ray, float dist)
	{
		return bvh->testIntersec(ray, dist);
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
	LightSelectStrategy lightSelectStrategy = LightSelectStrategy::ByPower;
	LightSelectStrategy lightAndEnvStrategy = LightSelectStrategy::Uniform;
};
