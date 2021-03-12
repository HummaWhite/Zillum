#ifndef ENVIRONMENTS_H
#define ENVIRONMENTS_H

#include "Environment.h"
#include "../Texture.h"
#include "../Math/PiecewiseIndependent2D.h"

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

		float *pdf = new float[w * h];
		for (int j = 0; j < h; j++)
		{
			for (int i = 0; i < w; i++)
			{
				pdf[j * w + i] = glm::dot(sphereMap(i, j), BRIGHTNESS) * glm::sin((float)(j + 0.5f) / h * Math::Pi);
			}
		}

		distrib = PiecewiseIndependent2D(pdf, w, h);
		delete[] pdf;
	}

	glm::vec3 getRadiance(const glm::vec3 &dir)
	{
		return sphereMap.getSpherical(dir);
	}

	std::pair<glm::vec3, float> importanceSample()
	{
		auto [col, row] = distrib.sample();

		float sinTheta = glm::sin(Math::Pi * (row + 0.5f) / h);
		auto Wi = Transform::planeToSphere(glm::vec2((col + 0.5f) / w, (row + 0.5f) / h));
		//float pdf = getPortion(Wi) * float(w * h) * 0.5f * Math::square(Math::PiInv) / sinTheta;
		float pdf = pdfLi(Wi);

		return { Wi, pdf };
	}

	float pdfLi(const glm::vec3 &Wi) override
	{
		float sinTheta = glm::asin(glm::abs(Wi.z));
		if (sinTheta < 1e-6f) return 0.0f;
		return getPortion(Wi) * float(w * h) * 0.5f * Math::square(Math::PiInv) /* sinTheta*/;
		//return 0.25 * Math::PiInv;
	}

private:
	float getPortion(const glm::vec3 &Wi)
	{
		return glm::dot(glm::vec3(sphereMap.getSpherical(Wi)), BRIGHTNESS) / distrib.sum();
	}

private:
	Texture sphereMap;
	int w, h;
	PiecewiseIndependent2D distrib;
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
