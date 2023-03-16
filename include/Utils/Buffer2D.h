#pragma once

#include "Buffer.h"

template<typename T>
struct Buffer2D : Buffer<T> {
	Buffer2D() = default;

	Buffer2D(int w, int h) {
		init(w, h);
	}

	~Buffer2D() = default;

	void init(int w, int h) {
		Buffer<T>::init(w * h);
		width = w, height = h;
	}

	void release() {
		Buffer<T>::release();
		width = height = 0;
	}

	void resize(int w, int h) {
		Buffer<T>::resize(w * h);
		init(w, h);
	}

	T& operator () (int i, int j) {
		return this->data[j * width + i];
	}

	const T& operator () (int i, int j) const {
		return this->data[j * width + i];
	}

	T& operator () (float u, float v) {
		int iu = std::min(static_cast<int>(u * width), width - 1);
		int iv = std::min(static_cast<int>(v * height), height - 1);
		return this->data[iv * width + iu];
	}

	const T& operator () (float u, float v) const {
		int iu = std::min(static_cast<int>(u * width), width - 1);
		int iv = std::min(static_cast<int>(v * height), height - 1);
		return this->data[iv * width + iu];
	}

	std::tuple<int, int> convertUV(float u, float v) {
		return { u * width, v * height };
	}

	int width = 0;
	int height = 0;
};
