#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "Buffer.h"

template<typename T>
struct FrameBuffer:
	Buffer<T>
{
	FrameBuffer() {}

	FrameBuffer(int w, int h)
	{
		init(w, h);
	}

	~FrameBuffer() {}

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
		return (*((Buffer<T>*)this)) [j * width + i];
	}

	int width = 0;
	int height = 0;
};

#endif
