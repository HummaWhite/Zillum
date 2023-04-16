#include "Core/PiecewiseDistrib.h"

Piecewise1D::Piecewise1D(const std::vector<float> &distrib) {
    std::queue<Element> greater, lesser;

    sumDistrib = 0.0f;
    for (auto i : distrib)
        sumDistrib += i;

    for (int i = 0; i < distrib.size(); i++) {
        float scaledPdf = distrib[i] * distrib.size();
        (scaledPdf >= sumDistrib ? greater : lesser).push(Element(i, scaledPdf));
    }

    table.resize(distrib.size(), Element(-1, 0.0f));

    while (!greater.empty() && !lesser.empty()) {
        auto [l, pl] = lesser.front();
        lesser.pop();
        auto [g, pg] = greater.front();
        greater.pop();

        table[l] = Element(g, pl);

        pg += pl - sumDistrib;
        (pg < sumDistrib ? lesser : greater).push(Element(g, pg));
    }

    while (!greater.empty()) {
        auto [g, pg] = greater.front();
        greater.pop();
        table[g] = Element(g, pg);
    }

    while (!lesser.empty()) {
        auto [l, pl] = lesser.front();
        lesser.pop();
        table[l] = Element(l, pl);
    }
}

int Piecewise1D::sample(const Vec2f &u) const {
    int rx = u.x * table.size();
    float ry = u.y;
    return (ry <= table[rx].second / sumDistrib) ? rx : table[rx].first;
}

PiecewiseIndependent2D::PiecewiseIndependent2D(float *pdf, int width, int height) {
    std::vector<float> colDistrib(height);
    for (int i = 0; i < height; i++) {
        std::vector<float> table(pdf + i * width, pdf + (i + 1) * width);
        Piecewise1D rowDistrib(table);
        rowTables.push_back(rowDistrib);
        colDistrib[i] = rowDistrib.sum();
    }
    colTable = Piecewise1D(colDistrib);
}

std::pair<int, int> PiecewiseIndependent2D::sample(const Vec2f &u1, const Vec2f &u2) const {
    int row = colTable.sample(u1);
    return std::pair<int, int>(rowTables[row].sample(u2), row);
}