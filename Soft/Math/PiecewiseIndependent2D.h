#pragma once

#include "Piecewise1D.h"

class PiecewiseIndependent2D
{
public:
    PiecewiseIndependent2D() {}

    PiecewiseIndependent2D(float *pdf, int width, int height)
    {
        std::vector<float> colDistrib(height);
        for (int i = 0; i < height; i++)
        {
            std::vector<float> table(pdf + i * width, pdf + (i + 1) * width);
            Piecewise1D rowDistrib(table);
            rowTables.push_back(rowDistrib);
            colDistrib[i] = rowDistrib.sum();
        }
        colTable = Piecewise1D(colDistrib);
    }

    std::pair<int, int> sample(const glm::vec2 &u1, const glm::vec2 &u2)
    {
        int row = colTable.sample(u1);
        return std::pair<int, int>(rowTables[row].sample(u2), row);
    }

    float sum() const
    {
        return colTable.sum();
    }

private:
    std::vector<Piecewise1D> rowTables;
    Piecewise1D colTable;
};
