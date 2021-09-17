#pragma once

#include "Buffer.h"

#include "../glm/glmIncluder.h"

template<typename T>
struct Buffer2D:
	Buffer<T>
{
	Buffer2D() = default;

	Buffer2D(int w, int h)
	{
		init(w, h);
	}

	~Buffer2D() = default;

	void init(int w, int h)
	{
		Buffer<T>::init(w * h);
		width = w, height = h;
	}

	void release()
	{
		Buffer<T>::release();
		width = height = 0;
	}

	void resize(int w, int h)
	{
		Buffer<T>::resize(w * h);
		init(w, h);
	}

	T& operator () (int i, int j)
	{
		return this->data[j * width + i];
	}

	T& operator () (Vec2f uv)
	{
		Vec2i iuv(uv * Vec2f(width, height));
		return this->data[iuv.y * width + iuv.x];
	}

	int width = 0;
	int height = 0;
};
