#pragma once

#include "Buffer.h"

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

	T& operator () (float u, float v)
	{
		int iu = u * width;
		int iv = v * height;
		return this->data[iv * width + iu];
	}

	int width = 0;
	int height = 0;
};
