#pragma once

#include "Integrator.h"

class LightPathIntegrator :
    public Integrator
{
public:
    LightPathIntegrator(ScenePtr scene, int photonsOnePass) :
        photonsOnePass(photonsOnePass), Integrator(scene, IntegratorType::LightPath) {}
    void renderOnePass();

private:
    void trace();

public:
    int maxDepth = 5;

private:
    int photonsOnePass;
};