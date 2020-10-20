#ifndef TEXTURE_H
#define TEXTURE_H

#include <iostream>
#include <cstdlib>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "stb_image/stb_image.h"

#include "Color.h"
#include "FrameBuffer.h"
#include "Math/Math.h"

class Texture
{
public:
	~Texture()
	{
		if (texRGB24 != nullptr) delete texRGB24;
		if (texFloat != nullptr) delete texFloat;
	}

	void loadRGB24(const char *filePath)
	{
		std::cout << "Loading Texture: " << filePath << std::endl;

		int width, height, bits;
		BYTE *data = stbi_load(filePath, &width, &height, &bits, 3);

		if (data == nullptr)
		{
			std::cout << "Error loading texture" << std::endl;
			exit(-1);
		}

		texRGB24 = new FrameBuffer<RGB24>(width, height);
		memcpy(texRGB24->ptr(), data, width * height * sizeof(glm::vec3));

		if (data != nullptr) stbi_image_free(data);
	}

	void loadFloat(const char *filePath)
	{
		std::cout << "Loading Texture: " << filePath << std::endl;

		int width, height, bits;
		float *data = stbi_loadf(filePath, &width, &height, &bits, 0);

		if (data == nullptr)
		{
			std::cout << "Error loading texture" << std::endl;
			exit(-1);
		}

		texFloat = new FrameBuffer<glm::vec3>(width, height);
		memcpy(texFloat->ptr(), data, width * height * sizeof(glm::vec3));

		if (data != nullptr) stbi_image_free(data);
	}

	glm::vec4 get(float u, float v)
	{
		if (texRGB24 != nullptr) return getRGB24(u, v);
		else return getFloat(u, v);
	}

	glm::vec4 getSpherical(const glm::vec3 &uv)
	{
		if (glm::isnan(uv.x) || glm::isnan(uv.y) || glm::isnan(uv.z)) return get(0.0f, 0.0f);
		glm::vec2 planeUV = Math::sphereToPlane(uv);
    	return get(planeUV.x, planeUV.y);
	}

	void setFilterType(int type)
	{
		filterType = type;
	}

public:
	enum
	{
		NEAREST = 0,
		LINEAR
	};

private:
	glm::vec4 getRGB24(float u, float v)
	{
		glm::vec4 res(1.0f);
		FrameBuffer<RGB24> *tex = texRGB24;

		float x = (tex->width - 1) * u;
		float y = (tex->height - 1) * v;

		int u1 = (int)(x);
		int v1 = (int)(y);
		int u2 = (int)(x + 1.0f);
		int v2 = (int)(y + 1.0f);

		if (filterType == NEAREST)
		{
			int pu = (u2 - x) > (x - u1) ? u2 : u1;
			int pv = (v2 - y) > (y - v1) ? v2 : v1;

			pu = (pu + tex->width) % tex->width;
			pv = (pv + tex->height) % tex->height;

			return (*tex)(pu, pv).toVec4();
		}
		else if (filterType == LINEAR)
		{
			u1 = (u1 + tex->width) % tex->width;
			v1 = (v1 + tex->height) % tex->height;
			u2 = (u2 + tex->width) % tex->width;
			v2 = (v2 + tex->height) % tex->height;

			glm::vec4 c1 = (*tex)(u1, v1).toVec4();
			glm::vec4 c2 = (*tex)(u2, v1).toVec4();
			glm::vec4 c3 = (*tex)(u1, v2).toVec4();
			glm::vec4 c4 = (*tex)(u2, v2).toVec4();

			float lx = x - (int)x;
			float ly = y - (int)y;

			return Math::lerp(Math::lerp(c1, c2, lx), Math::lerp(c3, c4, lx), ly);
		}
		else return glm::vec4(0.0f);
	}

	glm::vec4 getFloat(float u, float v)
	{
		glm::vec4 res(1.0f);
		FrameBuffer<glm::vec3> *tex = texFloat;

		float x = (tex->width - 1) * u;
		float y = (tex->height - 1) * v;

		int u1 = (int)(x);
		int v1 = (int)(y);
		int u2 = (int)(x + 1.0f);
		int v2 = (int)(y + 1.0f);

		if (filterType == NEAREST)
		{
			int pu = (u2 - x) > (x - u1) ? u2 : u1;
			int pv = (v2 - y) > (y - v1) ? v2 : v1;

			pu = (pu + tex->width) % tex->width;
			pv = (pv + tex->height) % tex->height;

			return glm::vec4((*tex)(pu, pv), 1.0f);
		}
		else if (filterType == LINEAR)
		{
			u1 = (u1 + tex->width) % tex->width;
			v1 = (v1 + tex->height) % tex->height;
			u2 = (u2 + tex->width) % tex->width;
			v2 = (v2 + tex->height) % tex->height;

			glm::vec3 c1 = (*tex)(u1, v1);
			glm::vec3 c2 = (*tex)(u2, v1);
			glm::vec3 c3 = (*tex)(u1, v2);
			glm::vec3 c4 = (*tex)(u2, v2);

			float lx = x - (int)x;
			float ly = y - (int)y;

			return glm::vec4(Math::lerp(Math::lerp(c1, c2, lx), Math::lerp(c3, c4, lx), ly), 1.0f);
		}
		else return glm::vec4(0.0f);
	}

private:
	FrameBuffer<RGB24> *texRGB24 = nullptr;
	FrameBuffer<glm::vec3> *texFloat = nullptr;

	int filterType = LINEAR;
};

#endif
