#include "Integrator.h"

#include <optional>

class AdjointPathIntegrator :
    public PixelIndependentIntegrator
{
public:
    AdjointPathIntegrator(ScenePtr scene, int maxSpp) :
        PixelIndependentIntegrator(scene, maxSpp, IntegratorType::AdjointPath) {}
    Vec3f tracePixel(Ray ray, SamplerPtr sampler);

    struct Vertex
    {
        Vec3f P;
        Vec3f N;
        Vec3f Wo;
        MaterialPtr mat;
        Vec3f beta;
    };

private:
    std::optional<Vertex> findNonSpecularHit(Ray ray, SurfaceInfo sInfo, SamplerPtr sampler);
    Vec3f trace(Vertex v, SamplerPtr sampler);

public:
    int maxCameraDepth = 5;
    int maxLightDepth = 5;
};