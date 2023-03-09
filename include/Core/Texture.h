#pragma once

#include <iostream>
#include <cstdlib>
#include <memory>

#include "stbIncluder.h"
#include "Utils/Buffer2D.h"
//#include "../Utils/File.h"
#include "Color.h"
#include "Transform.h"

enum TextureFilterType {
	Nearest, Linear
};

template<typename T>
class ColorMap {
public:
	virtual T get(float u, float v) = 0;
	T get(const Vec2f &uv) { return get(uv.x, uv.y); }
};

using ColorMap1f = ColorMap<float>;
using ColorMap3f = ColorMap<Vec3f>;
using ColorMapSpec = ColorMap<Spectrum>;
using ColorMap1fPtr = std::shared_ptr<ColorMap1f>;
using ColorMap3fPtr = std::shared_ptr<ColorMap3f>;
using ColorMapSpecPtr = std::shared_ptr<ColorMapSpec>;

template<typename T>
class SingleColor : public ColorMap<T> {
public:
	SingleColor(const T &color):
		mColor(color) {}

	T get(float u, float v) { return mColor; }

private:
	T mColor;
};

using SingleColor1f = SingleColor<float>;
using SingleColor3f = SingleColor<Vec3f>;
using SingleColorSpec = SingleColor<Spectrum>;

template<typename T>
class Texture : public Buffer2D<T>, public ColorMap<T> {
public:
	T get(float u, float v) {
		if (Math::isNan(u) || Math::isNan(v)) {
			return Vec4f(0.0f);
		}
		u = glm::fract(u);
		v = glm::fract(v);

		auto &width = this->width;
		auto &height = this->height;

		float x = (width - 1) * u;
		float y = (height - 1) * v;

		int u1 = (int)(x);
		int v1 = (int)(y);
		int u2 = (int)(x + 1.0f);
		int v2 = (int)(y + 1.0f);

		if (mFilterType == TextureFilterType::Nearest) {
			int pu = (u2 - x) > (x - u1) ? u2 : u1;
			int pv = (v2 - y) > (y - v1) ? v2 : v1;

			pu = (pu + width) % width;
			pv = (pv + height) % height;

			return (*this)(pu, pv);
		}
		else if (mFilterType == TextureFilterType::Linear) {
			u1 = (u1 + width) % width;
			v1 = (v1 + height) % height;
			u2 = (u2 + width) % width;
			v2 = (v2 + height) % height;

			T c1 = (*this)(u1, v1);
			T c2 = (*this)(u2, v1);
			T c3 = (*this)(u1, v2);
			T c4 = (*this)(u2, v2);

			float lx = x - (int)x;
			float ly = y - (int)y;

			return glm::mix(glm::mix(c1, c2, lx), glm::mix(c3, c4, lx), ly);
		}
		else {
			return T(0.0f);
		}
	}

	T getSpherical(const glm::vec3 &uv) {
		if (Math::isNan(uv.x) || Math::isNan(uv.y) || Math::isNan(uv.z)) {
			std::cout << "Texture::getSpherical: Invalid uv with NAN(s)\n";
			return get(0.0f, 0.0f);
		}
		Vec2f planeUV = Transform::sphereToPlane(glm::normalize(uv));
		return get(planeUV.x, planeUV.y);
	}

	// void write(const File::path &path)
	// {
	// 	int size = width * height;
	// 	RGB24 *data = new RGB24[size];
	// 	for (int i = 0; i < size; i++)
	//    	 	data[i] = RGB24::swapRB(RGB24((*this)[i]));
	// 	stbi_write_png(path.generic_string().c_str(), width, height, 3, data, width * 3);
	// 	delete[] data;
	// }

	void setFilterType(TextureFilterType type) { mFilterType = type; }

	int texWidth() const { return this->width; }
	int texHeight() const { return this->height; }

public:
	TextureFilterType mFilterType = TextureFilterType::Linear;
};

using Texture1f = Texture<float>;
using Texture3f = Texture<Vec3f>;
using TextureSpec = Texture<Spectrum>;
using Texture1fPtr = std::shared_ptr<Texture1f>;
using Texture3fPtr = std::shared_ptr<Texture3f>;
using TextureSpecPtr = std::shared_ptr<TextureSpec>;

NAMESPACE_BEGIN(TextureLoader)

static Texture3fPtr fromU8x3(const char *filePath, bool linearize = false) {
	Texture3fPtr tex = std::make_shared<Texture3f>();
	std::cout << "Texture::loading RGB: " << filePath << std::endl;

	int width, height, bits;
	uint8_t *data = stbi_load(filePath, &width, &height, &bits, 3);

	if (data == nullptr) {
		std::cout << "Texture::error loading" << std::endl;
		exit(-1);
	}

	tex->init(width, height);
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			Vec3f color = (*(RGB24*)&data[(j * width + i) * 3]).toVec3();
			if (linearize) {
				color = glm::pow(color, Vec3f(2.2f));
			}
			(*tex)(i, j) = color;
		}
	}

	if (data != nullptr)
		stbi_image_free(data);
	return tex;
}

static Texture3fPtr fromF32x3(const char *filePath) {
	Texture3fPtr tex = std::make_shared<Texture3f>();
	std::cout << "Texture::loading HDR: " << filePath << std::endl;

	int width, height, bits;
	float *data = stbi_loadf(filePath, &width, &height, &bits, 0);

	if (data == nullptr) {
		std::cout << "Texture::error loading" << std::endl;
		exit(-1);
	}

	tex->init(width, height);
	memcpy(tex->bufPtr(), data, width * height * sizeof(Vec3f));

	if (data != nullptr) {
		stbi_image_free(data);
	}
	return tex;
}

NAMESPACE_END(ColorMapLoader)