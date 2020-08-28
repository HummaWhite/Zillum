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

typedef FrameBuffer<RGB24> TextureRGB24;
typedef FrameBuffer<RGBA32> TextureRGBA32;
typedef FrameBuffer<float> TextureFloat;

enum
{
	LINEAR = 0,
	NEAREST
} TextureFilterType;

template<typename T>
T lerp(T x, T y, float a)
{
	return x + (y - x) * a;
}

namespace Texture
{
	BYTE* loadTexture(const char *filePath, int channels, int& width, int &height)
	{
		std::cout << "Loading Texture: " << filePath << std::endl;

		int bits;
		BYTE *data = stbi_load(filePath, &width, &height, &bits, channels);

		if (data == nullptr)
		{
			std::cout << "Error loading texture" << std::endl;
			exit(-1);
		}

		return data;
	}

	void load(TextureRGB24& tex, const char *filePath)
	{
		int width, height;
		BYTE *data = loadTexture(filePath, 3, width, height);

		tex.init(width, height);

		memcpy(tex.ptr(), data, width * height * sizeof(RGB24));
		stbi_image_free(data);
	}

	/*TextureRGBA32 loadRGBA32(const char *filePath)
	{
		TextureRGBA32 tex;
		int width, height;
		BYTE *data = loadTexture(filePath, 4, width, height);

		tex.init(width, height);

		memcpy(tex.ptr(), data, width * height * 4 * sizeof(BYTE));
		stbi_image_free(data);

		return tex;
	}*/
}

inline glm::vec4 texture(TextureRGB24* tex, glm::vec2 uv, int filterType)
{
	if (tex == nullptr) return glm::vec4(0.0f);

	glm::vec4 res(1.0f);

	float x = (tex->width - 1) * uv.x;
	float y = (tex->height - 1) * uv.y;

	int u1 = (int)(x);
	int v1 = (int)(y);
	int u2 = (int)(x + 1.0f);
	int v2 = (int)(y + 1.0f);

	if (filterType == NEAREST)
	{
		int u = (u2 - x) > (x - u1) ? u2 : u1;
		int v = (v2 - y) > (y - v1) ? v2 : v1;
		
		u = (u + tex->width) % tex->width;
		v = (v + tex->height) % tex->height;

		return (*tex)(u, v).toVec4();
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

		return lerp(lerp(c1, c2, lx), lerp(c3, c4, lx), ly);
	}
}

#endif
