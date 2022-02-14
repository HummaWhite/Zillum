#include "../../../include/Core/Camera.h"

Vec2f PanoramaCamera::rasterPos(Ray ray)
{
    return {};
}

Ray PanoramaCamera::generateRay(SamplerPtr sampler)
{
    return generateRay(sampler->get2(), sampler);
}

Ray PanoramaCamera::generateRay(Vec2f uv, SamplerPtr sampler)
{
    auto dir = Transform::planeToSphere((uv + Vec2f(0.5f, 1.0f)) * 0.5f);
    dir.x = -dir.x;
    dir.z = -dir.z;
    return {mPos, dir};
}

std::optional<CameraIiSample> PanoramaCamera::sampleIi(Vec3f ref, Vec2f u)
{
    return std::nullopt;
}

CameraPdf PanoramaCamera::pdfIe(Ray ray)
{
    return {};
}

Spectrum PanoramaCamera::Ie(Ray ray)
{
    return {};
}