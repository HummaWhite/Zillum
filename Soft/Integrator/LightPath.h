#pragma once

#include <atomic>

#include "Integrator.h"

class LightPathIntegrator :
    public Integrator
{
public:
    LightPathIntegrator(ScenePtr scene, int pathsOnePass);
    void renderOnePass();
    void reset();

private:
    void trace();
    void addToFilm(Vec2f uv, Vec3f val);

public:
    int maxDepth = 5;

private:
    int pathsOnePass;
    Buffer2D<uint64_t> pixelCount;
    uint64_t pathCount = 0;
};