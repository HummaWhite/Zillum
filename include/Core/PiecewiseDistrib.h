#pragma once

#include "Math.h"

#include <array>
#include <queue>

class Piecewise1D {
public:
    Piecewise1D() = default;
    Piecewise1D(const std::vector<float> &distrib);

    int sample(const Vec2f &u);
    float sum() const { return sumDistrib; }
    std::vector<std::pair<int, float>> getTable() const { return table; }

private:
    typedef std::pair<int, float> Element;

private:
    std::vector<Element> table;
    float sumDistrib;
};

class PiecewiseIndependent2D {
public:
    PiecewiseIndependent2D() = default;
    PiecewiseIndependent2D(float *pdf, int width, int height);

    std::pair<int, int> sample(const Vec2f &u1, const Vec2f &u2);
    float sum() const { return colTable.sum(); }

private:
    std::vector<Piecewise1D> rowTables;
    Piecewise1D colTable;
};