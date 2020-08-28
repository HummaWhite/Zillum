#ifndef BUFFER_H
#define BUFFER_H

#include <Windows.h>
#include <cstdlib>
#include <algorithm>

template<typename T>
struct Buffer
{
	Buffer(): data(nullptr) {}

	Buffer(int count)
	{
		init(count);
	}

	~Buffer()
	{
		release();
	}

	virtual void init(int count)
	{
		if (data != nullptr) return;
		data = new T[count];
		this->count = count;
	}

	virtual void release()
	{
		if (data == nullptr) return;
		delete[] data;
		data = nullptr;
		count = 0;
	}

	virtual void resize(int count)
	{
		release();
		init(count);
	}

	void fill(T val)
	{
		std::fill(data, data + count, val);
	}

	void load(const void *src, size_t byteSize, int offset = 0)
	{
		if (data == nullptr)
		{
			std::cout << "Buffer::Error: buffer not initialized" << std::endl;
			exit(-1);
		}
		memcpy(data + offset, src, byteSize);
	}

	void copy(Buffer& buffer)
	{
		init(buffer.count);
		memcpy(data, buffer.ptr(), count * sizeof(T));
	}

	T& operator [] (int index)
	{
		return data[index];
	}

	void* bufPtr()
	{
		return (void*)data;
	}

	T* ptr()
	{
		return data;
	}

	T *data = nullptr;
	int count = 0;
};

#endif
