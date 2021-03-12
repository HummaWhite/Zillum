#ifndef PIECEWISE_INDEPENDENT_2D_H
#define PIECEWISE_INDEPENDENT_2D_H

#include "Piecewise1D.h"

class PiecewiseIndependent2D
{
public:
    PiecewiseIndependent2D() {}

    PiecewiseIndependent2D(float *pdf, int width, int height)
    {
        float sum = 0.0f;
        std::vector<float> colDistrib(height);
        for (int i = 0; i < height; i++)
        {
            float sumRow = 0.0f;
            for (int j = 0; j < width; j++) sumRow += pdf[i * width + j];

            std::vector<float> table(pdf + i * width, pdf + (i + 1) * width);
            Piecewise1D rowDistrib(table, sumRow);
            rowTables.push_back(rowDistrib);
            colDistrib[i] = sumRow;
            sum += sumRow;
        }
        colTable = Piecewise1D(colDistrib, sum);
    }

    std::pair<int, int> sample()
    {
        int row = colTable.sample();
        return std::pair<int, int>(rowTables[row].sample(), row);
    }

    float sum() const
    {
        return colTable.sum();
    }

private:
    std::vector<Piecewise1D> rowTables;
    Piecewise1D colTable;
};

#endif