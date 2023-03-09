#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <vector>
#include <cstdio>

#include "glmIncluder.h"
#include "tiny_obj_loader.h"

namespace ObjReader {
	struct VertexInfo {
		std::vector<Vec3f> vertices;
		std::vector<Vec2f> texcoords;
		std::vector<Vec3f> normals;
	};

	static VertexInfo readFile(const char* filePath) {
		VertexInfo data;
		std::cout << "Loading Obj: " << filePath << std::endl;

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string errStr;
		tinyobj::LoadObj(&attrib, &shapes, &materials, &errStr, filePath);

		bool hasTexcoord = attrib.texcoords.size() != 0;

		for (const auto &shape : shapes) {
			for (auto idx : shape.mesh.indices) {
				data.vertices.push_back(*reinterpret_cast<Vec3f*>(attrib.vertices.data() + idx.vertex_index * 3));
				data.normals.push_back(*reinterpret_cast<Vec3f*>(attrib.normals.data() + idx.normal_index * 3));
				if (!hasTexcoord) {
					continue;
				}
				data.texcoords.push_back(*reinterpret_cast<Vec2f*>(attrib.texcoords.data() + idx.texcoord_index * 2));
			}
		}
		return data;
	}
};
