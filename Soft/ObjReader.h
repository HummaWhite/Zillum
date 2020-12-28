#ifndef OBJREADER_H
#define OBJREADER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdio>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace ObjReader
{
	struct VertexInfo
	{
		std::vector<glm::vec3> vertices;
		std::vector<glm::vec2> texCoords;
		std::vector<glm::vec3> normals;
	};

	VertexInfo readFile(const char* filePath)
	{
		VertexInfo data;
		std::fstream file(filePath);
		std::cout << "Loading Obj: " << filePath << std::endl;

		if (!file.is_open())
		{
			std::cout << "Error loading Obj" << std::endl;
			return data;
		}

		std::vector<glm::vec3> points;
		std::vector<glm::vec2> texCoords;
		std::vector<glm::vec3> normals;

		std::string line;
		while (std::getline(file, line))
		{
			if (line.empty()) continue;
			if (line[0] != 'v' && line[0] != 'f') continue;

			std::stringstream ss;
			ss << line;

			std::string type;
			ss >> type;

			if (type == "f")
			{
				int indexP[3], indexT[3], indexN[3];

				const char *buf = line.c_str();
				bool withTexCoord = true;

				for (int i = 0; i < line.length(); i++)
				{
					if (i == line.length() - 1) break;
					if (line[i] == '/' && line[i + 1] == '/')
					{
						withTexCoord = false;
						break;
					}
				}

				//std::cout << "##\n";

				if (withTexCoord)
				{
					sscanf
					(
						buf + 1,
						"%d/%d/%d %d/%d/%d %d/%d/%d",
						&indexP[0], &indexT[0], &indexN[0],
						&indexP[1], &indexT[1], &indexN[1],
						&indexP[2], &indexT[2], &indexN[2]
					);
				}
				else
				{
					sscanf
					(
					 	buf + 1,
						"%d//%d %d//%d %d//%d",
						&indexP[0], &indexN[0],
						&indexP[1], &indexN[1],
						&indexP[2], &indexN[2]
					);
				}

				for (int i = 0; i < 3; i++)
				{
					data.vertices.push_back(points[indexP[i] - 1]);
					data.texCoords.push_back(withTexCoord ? texCoords[indexT[i] - 1] : glm::vec2(0.0f));
					data.normals.push_back(normals[indexT[i] - 1]);
				}
			}
			else if (type == "v")
			{
				glm::vec3 v;
				ss >> v.x >> v.y >> v.z;
				points.push_back(v);
			}
			else if (type == "vt")
			{
				glm::vec2 v;
				ss >> v.x >> v.y;
				texCoords.push_back(v);
			}
			else if (type == "vn")
			{
				glm::vec3 v;
				ss >> v.x >> v.y >> v.z;
				normals.push_back(v);
			}
		}

		return data;
	}
};

#endif
