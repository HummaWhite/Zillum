#pragma once

#include <functional>

#include "Buffer2D.h"
#include "../Core/Color.h"
#include "../../ext/stbIncluder.h"

template<typename T>
void saveImage(const std::string &name, Buffer2D<T> &buffer, std::function<RGB24(T&)> transformFunc)
{
    int w = buffer.width;
    int h = buffer.height;
    int size = w * h;
    RGB24 *data = new RGB24[size];
    for (int i = 0; i < size; i++)
        data[i] = RGB24::swapRB(transformFunc(buffer[i]));
    stbi_write_png(name.c_str(), w, h, 3, data, w * 3);
    delete[] data;
}

template<typename T>
void saveImage(const std::string &name, T *buffer, int w, int h, std::function<RGB24(T&)> transformFunc)
{
    int size = w * h;
    RGB24 *data = new RGB24[size];
    for (int i = 0; i < size; i++)
        data[i] = RGB24::swapRB(transformFunc(buffer[i]));
    stbi_write_png(name.c_str(), w, h, 3, data, w * 3);
    delete[] data;
}