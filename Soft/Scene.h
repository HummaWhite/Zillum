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
