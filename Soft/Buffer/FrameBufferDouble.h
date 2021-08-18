#pragma once

#include "Buffer2D.h"

template<typename T>
class FrameBufferDouble
{
public:
	FrameBufferDouble() {}

	FrameBufferDouble(int w, int h)
	{
		buf[0].init(w, h);
		buf[1].init(w, h);
	}

	void init(int w, int h)
	{
		buf[0].init(w, h);
		buf[1].init(w, h);
	}

	void release()
	{
		buf[0].release();
		buf[1].release();
	}

	void resize(int w, int h)
	{
		buf[0].resize(w, h);
		buf[1].resize(w, h);
	}

	void fill(T val)
	{
		buf[index].fill(val);
	}

	int width() { return buf[0].width; }
	int height() { return buf[0].height; }

	T& operator () (int i, int j)
	{
		return buf[index](i, j);
	}

	T& operator [] (int i)
	{
		return buf[index][i];
	}

	Buffer2D<T>& getCurrentBuffer()
	{
		return buf[index];
	}

	void swap()
	{
		index ^= 1;
	}

private:
	Buffer2D<T> buf[2];
	int index = 0;
};
