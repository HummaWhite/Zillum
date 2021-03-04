#ifndef ENVIRONMENTS_H
#define ENVIRONMENTS_H

#include "Environment.h"
#include "../Texture.h"

class EnvSingleColor:
	public Environment
{
public:
	EnvSingleColor(const glm::vec3 &color):
		radiance(color) {}

	glm::vec3 getRadiance(const glm::vec3 &dir)
	{
		return radiance;
	}

private:
	glm::vec3 radiance;
};

class EnvSphereMapHDR:
	public Environment
{
public:
	const glm::vec3 BRIGHTNESS = glm::vec3(0.299f, 0.587f, 0.114f);

public:
	EnvSphereMapHDR(const char *filePath)
	{
		sphereMap.loadFloat(filePath);
		w = sphereMap.texWidth();
		h = sphereMap.texHeight();
		cdfTable = new float[w * h];
		sumRow = new float[h];

		for (int i = 0; i < w; i++)
		{
			for (int j = 0; j < h; j++)
			{
				(*this)(i, j) = glm::dot(sphereMap(i, j), BRIGHTNESS) * glm::sin((float)(j + 0.5f) / h * Math::Pi);
			}
		}

		for (int i = 0; i < h; i++)
		{
			for (int j = 1; j < w; j++)
			{
				(*this)(j, i) += (*this)(j - 1, i);
			}
		}

		sumRow[0] = (*this)(w - 1, 0);
		for (int i = 1; i < h; i++)
		{
			sumRow[i] = sumRow[i - 1] + (*this)(w - 1, i);
		}
	}

	~EnvSphereMapHDR()
	{
		if (cdfTable != nullptr) delete[] cdfTable;
		if (sumRow != nullptr) delete[] sumRow;
	}

	float& operator () (int i, int j)
	{
		return cdfTable[j * w + i];
	}

	glm::vec3 getRadiance(const glm::vec3 &dir)
	{
		return sphereMap.getSpherical(dir);
	}

	std::pair<glm::vec3, float> importanceSample()
	{
		RandomGenerator rg;

		int col, row;
		float rowVal = rg.get(0.0f, sumRow[h - 1]);

		int l = 0, r = h - 1;
		while (l != r)
		{
			int m = (l + r) >> 1;
			if (rowVal >= sumRow[m]) l = m + 1;
			else r = m;
		}
		row = l;

		float colVal = rg.get(0.0f, (*this)(w - 1, row));
		l = 0, r = w - 1;
		while (l != r)
		{
			int m = (l + r) >> 1;
			if (colVal >= (*this)(m, row)) l = m + 1;
			else r = m;
		}
		col = l;

		float sinTheta = glm::sin(Math::Pi * (row + 0.5f) / h);
		auto Wi = Transform::planeToSphere(glm::vec2((col + 0.5f) / w, (row + 0.5f) / h));
		float pdf = glm::dot(glm::vec3(sphereMap.getSpherical(Wi)), BRIGHTNESS) / sumRow[h - 1] * float(w * h) * 0.25f * Math::PiInv;

		return { Wi, pdf };
	}

	float pdfLi(const glm::vec3 &Wi) override
	{
		return glm::dot(glm::vec3(sphereMap.getSpherical(Wi)), BRIGHTNESS) / sumRow[h - 1];
	}

private:
	Texture sphereMap;
	int w, h;
	float *cdfTable = nullptr;
	float *sumRow = nullptr;
};

class EnvTest:
	public Environment
{
public:
	EnvTest(const glm::vec3 &color, int _row, int _col):
		radiance(color), row(_row), col(_col) {}

	glm::vec3 getRadiance(const glm::vec3 &dir)
	{
		glm::vec2 uv = Transform::sphereToPlane(dir);

		int r = (int)(uv.x * row);
		int c = (int)(uv.y * col);

		return (r & 1) ^ (c & 1) ? radiance : glm::vec3(0.0f);
	}

private:
	glm::vec3 radiance;
	int row, col;
};

#endif
