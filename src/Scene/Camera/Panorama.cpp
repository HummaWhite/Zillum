#include "../../../include/Core/Camera.h"

Vec2f PanoramaCamera::getRasterPos(Ray ray)
{
    return {};
}

Ray PanoramaCamera::generateRay(SamplerPtr sampler)
{
    return generateRay(sampler->get2D(), sampler);
}

Ray PanoramaCamera::generateRay(Vec2f uv, SamplerPtr sampler)
{
    auto dir = Transform::planeToSphere((uv + Vec2f(0.5f, 1.0f)) * 0.5f);
    dir.x = -dir.x;
    dir.z = -dir.z;
    return {pos, dir};
}

CameraIiSample PanoramaCamera::sampleIi(Vec3f ref, Vec2f u)
{
    return {};
}

std::pair<float, float> PanoramaCamera::pdfIe(Ray ray)
{
    return {};
}

Vec3f PanoramaCamera::Ie(Ray ray)
{
    return {};
}