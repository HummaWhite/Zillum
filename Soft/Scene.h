#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <memory>

#include "Shape/Shapes.h"
#include "Light/Light.h"
#include "Environment/Environments.h"
#include "Accelerator/BVH.h"
#include "Camera.h"
#include "ObjReader.h"

struct SceneHitInfo
{
	enum Type { NONE = 0, SHAPE, LIGHT };
	Type type;
	float dist;
	std::shared_ptr<Shape> shape;
	std::shared_ptr<Light> light;
};

class Scene
{
public:
	Scene() {}

	Scene(const std::vector<std::shared_ptr<Shape>> &shapeList, const std::vector<std::shared_ptr<Light>> &lightList, std::shared_ptr<Environment> environment, std::shared_ptr<Camera> camera):
		shapeList(shapeList), lightList(lightList), environment(environment), camera(camera) {}

	void buildBVH()
	{
		shapeBVH = std::make_shared<BVH<Shape>>(shapeList);
		lightBVH = std::make_shared<BVH<Light>>(lightList);

		auto shapeBVHInfo = shapeBVH->dfsDetailed();
		std::cout << "BVHShapes::  TreeSize: " << shapeBVH->size() << "  MaxDepth: " << shapeBVHInfo.maxDepth << "  AvgDepth: " << shapeBVHInfo.avgDepth << "\n";
		auto lightBVHInfo = lightBVH->dfsDetailed();
		std::cout << "BVHLights::  TreeSize: " << lightBVH->size() << "  MaxDepth: " << lightBVHInfo.maxDepth << "  AvgDepth: " << lightBVHInfo.avgDepth << "\n";
		lightBVH->makeCompact();
		shapeBVH->makeCompact();
	}

	SceneHitInfo closestHit(const Ray &ray)
	{
		float minDistShape = 1000.0f, minDistLight = 1000.0f;
		auto shape = shapeBVH->closestHit(ray, minDistShape, false);
		auto light = lightBVH->closestHit(ray, minDistLight, false);

		auto ret = SceneHitInfo{ SceneHitInfo::NONE, 1000.0f, shape, light };
		if (shape != nullptr)
		{
			if (light != nullptr && minDistLight < minDistShape)
			{
				ret.type = SceneHitInfo::LIGHT;
				ret.dist = minDistLight;
			}
			else
			{
				ret.type = SceneHitInfo::SHAPE;
				ret.dist = minDistShape;
			}
		}
		else if (light != nullptr)
		{
			ret.type = SceneHitInfo::LIGHT;
			ret.dist = minDistLight;
		}

		return ret;
	}

public:
	std::vector<std::shared_ptr<Shape>> shapeList;
	std::vector<std::shared_ptr<Light>> lightList;
	std::shared_ptr<Environment> environment;
	std::shared_ptr<Camera> camera;

	std::shared_ptr<BVH<Shape>> shapeBVH;
	std::shared_ptr<BVH<Light>> lightBVH;
};

#endif
