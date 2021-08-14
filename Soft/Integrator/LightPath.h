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
    int photonsOnePass;
};