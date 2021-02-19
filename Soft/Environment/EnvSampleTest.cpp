#pragma GCC optimize(3, "Ofast", "inline")

#include <algorithm>
#include <iostream>
#include <memory>
#include <random>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

typedef std::uniform_real_distribution<float> UniformFloat;

std::default_random_engine randGenerator;

template<typename T>
struct Vec3
{
	Vec3() {}

	Vec3(T v): x(v), y(v), z(v) {}

	Vec3(T x, T y, T z): x(x), y(y), z(z) {}

	T& operator [] (int dim)
	{
		if (dim < 0 || dim > 2) throw "Out of bound";
		return *(&x + dim);
	}

	T x, y, z;
};

template<typename T>
struct Image
{
	Image() {}
	
	Image(int w, int h): w(w), h(h)
	{
		buf = new T[w * h];
	}

	Image(const Image &img): w(img.w), h(img.h)
	{
		if (img.buf == nullptr) return;
		buf = new T[w * h];
		memcpy(buf, img.buf, w * h * sizeof(T));
	}

	~Image()
	{
		if (buf != nullptr) delete[] buf;
	}

	T& operator () (int i, int j)
	{
		return buf[j * w + i];
	}

	void clear()
	{
		if (buf != nullptr) memset(buf, 0, w * h * sizeof(T));
	}

	T *buf = nullptr;
	int w, h;
};

struct CdfTable: Image<float>
{
	CdfTable(const Image<Vec3<float>> &img):
		Image<float>(img.w, img.h)
	{
		sumRow = new float[h];
		int count = img.w * img.h;

		for (int i = 0; i < count; i++)
		{
			float theta = 3.141592653589793f * (float)(i / w) / h;
			buf[i] = img.buf[i].x * 0.299f + img.buf[i].y * 0.587f + img.buf[i].z * 0.114f;
			buf[i] *= std::sin(theta);
		}

		for (int i = 0; i < h; i++)
		{
			for (int j = 1; j < w; j++)
			{
				(*this)(j, i) += (*this)(j - 1, i);
			}
		}

		sumRow[0] = buf[w - 1];
		for (int i = 1; i < h; i++)
		{
			sumRow[i] = sumRow[i - 1] + (*this)(w - 1, i);
		}
	}

	~CdfTable()
	{
		delete[] sumRow;
	}

	Vec3<float> sample()
	{
		int col, row;
		float rowVal = UniformFloat(0.0f, sumRow[h - 1])(randGenerator);

		int l = 0, r = h - 1;
		while (l != r)
		{
			int m = (l + r) >> 1;
			if (rowVal >= sumRow[m]) l = m + 1;
			else r = m;
		}
		row = l;

		float colVal = UniformFloat(0.0f, (*this)(w - 1, row))(randGenerator);
		l = 0, r = w - 1;
		while (l != r)
		{
			int m = (l + r) >> 1;
			if (colVal >= (*this)(m, row)) l = m + 1;
			else r = m;
		}
		col = l;

		//std::cout << rowVal << " " << colVal << " " << row << " " << col << "\n";

		return Vec3<float>((float)col, (float)row, 0.0f);
	}

	float *sumRow = nullptr;
};

template<typename T>
Image<Vec3<uint8_t>> toRGB24(const Image<T> &img, Vec3<float> (*mapping)(T))
{
	Image<Vec3<uint8_t>> newImg(img.w, img.h);

	int count = img.w * img.h;
	for (int i = 0; i < count; i++)
	{
		Vec3<float> mapped = mapping(img.buf[i]);
		newImg.buf[i].x = std::clamp(255.0f * mapped.x, 0.0f, 255.0f);
		newImg.buf[i].y = std::clamp(255.0f * mapped.y, 0.0f, 255.0f);
		newImg.buf[i].z = std::clamp(255.0f * mapped.z, 0.0f, 255.0f);
	}

	return newImg;
}

Image<Vec3<float>> loadImage(const std::string &path)
{
	Image<Vec3<float>> img;
	img.buf = (Vec3<float>*)stbi_loadf(path.c_str(), &img.w, &img.h, nullptr, 0);
	return img;
}

void saveImage(const Image<Vec3<uint8_t>> &img, const std::string& path)
{
	stbi_write_png(path.c_str(), img.w, img.h, 3, (float*)img.buf, img.w * 3);
}

int main()
{
	Image<Vec3<float>> img = loadImage("010.hdr");
	
	Image<Vec3<uint8_t>> res = toRGB24<Vec3<float>>(img,
			[](Vec3<float> v)
			{
				auto map = [](float x) { return std::pow(1.0 - std::exp(-x), 1.0f / 2.2f); };
				return Vec3<float>(map(v.x), map(v.y), map(v.z));
			}
			);

	CdfTable table(img);
	const int outerSize = 6, innerSize = 5;
	for (int i = 0; i < 1000; i++)
	{
		auto sample = table.sample();
		int x = sample.x, y = sample.y;

		for (int j = std::max(0, x - outerSize); j <= std::min(img.w - 1, x + outerSize); j++)
		{
			for (int k = std::max(0, y - outerSize); k <= std::min(img.h - 1, y + outerSize); k++)
			{
				res(j, k) = (j <= x - innerSize || j >= x + innerSize || k <= y - innerSize || k >= y + innerSize) ? Vec3<uint8_t>(0) : Vec3<uint8_t>(255);
			}
		}
	}
	saveImage(res, "g.png");
}
