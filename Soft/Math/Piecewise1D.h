#pragma once

#include "Math.h"

#include <array>
#include <queue>

class Piecewise1D
{
public:
    Piecewise1D() {}

    Piecewise1D(const std::vector<float> &distrib)
    {
        std::queue<Element> greater, lesser;

        sumDistrib = 0.0f;
        for (auto i : distrib) sumDistrib += i;

        for (int i = 0; i < distrib.size(); i++)
        {
            float scaledPdf = distrib[i] * distrib.size();
            (scaledPdf >= sumDistrib ? greater : lesser).push(Element(i, scaledPdf));
        }

        table.resize(distrib.size(), Element(-1, 0.0f));

        while (!greater.empty() && !lesser.empty())
        {
            auto [l, pl] = lesser.front();
            lesser.pop();
            auto [g, pg] = greater.front();
            greater.pop();

            table[l] = Element(g, pl);

            pg += pl - sumDistrib;
            (pg < sumDistrib ? lesser : greater).push(Element(g, pg));
        }

        while (!greater.empty())
        {
            auto [g, pg] = greater.front();
            greater.pop();
            table[g] = Element(g, pg);
        }

        while (!lesser.empty())
        {
            auto [l, pl] = lesser.front();
            lesser.pop();
            table[l] = Element(l, pl);
        }
    }

    int sample(const glm::vec2 &u)
    {
        int rx = static_cast<int>(table.size() * u.x);
        float ry = u.y;

        return (ry <= table[rx].second) ? rx : table[rx].first;
    }

    float sum() const { return sumDistrib; }

    std::vector<std::pair<int, float>> getTable() const { return table; }

private:
    typedef std::pair<int, float> Element;

private:
    std::vector<Element> table;
    float sumDistrib;
};
