#include "Integrator.h"

void Integrator::setModified()
{
    modified = true;
    finished = false;
}

PixelIndependentIntegrator::PixelIndependentIntegrator(ScenePtr scene, int maxSpp, IntegratorType type) :
    maxSpp(maxSpp), Integrator(scene, type)
{
    auto film = scene->camera->getFilm();
    width = film.width;
    height = film.height;
}

void PixelIndependentIntegrator::renderOnePass()
{
    if (modified)
    {
        scene->camera->getFilm().fill(Vec3f(0.0f));
        curSpp = 0;
        modified = false;
    }
    if (limitSpp && curSpp >= maxSpp)
    {
        finished = true;
        return;
    }

    std::thread threads[MaxThreads];
    for (int i = 0; i < MaxThreads; i++)
    {
        int start = (width / MaxThreads) * i;
        int end = std::min(width, (width / MaxThreads) * (i + 1));
        if (i == MaxThreads - 1)
            end = width;

        auto threadSampler = (i == 0) ? mSampler : mSampler->copy();
        threads[i] = std::thread(doTracing, this, start, end, threadSampler);
    }
    mSampler->nextSample();

    for (auto &t : threads)
        t.join();

    curSpp++;
    std::cout << "\r" << std::setw(4) << curSpp << "/" << maxSpp << " spp  ";

    float perc = (float)curSpp / (float)maxSpp * 100.0f;
    std::cout << "  " << std::fixed << std::setprecision(2) << perc << "%";
}

void PixelIndependentIntegrator::doTracing(int start, int end, SamplerPtr sampler)
{
    float invW = 1.0f / width;
    float invH = 1.0f / height;
    for (int x = start; x < end; x++)
    {
        for (int y = 0; y < height; y++)
        {
            sampler->setPixel(x, y);
            float sx = 2.0f * (x + 0.5f) * invW - 1.0f;
            float sy = 1.0f - 2.0f * (y + 0.5f) * invH;

            Ray ray = scene->camera->generateRay({sx, sy}, sampler);
            Vec3f result = tracePixel(ray, sampler);

            if (Math::isNan(result.x) || Math::isNan(result.y) || Math::isNan(result.z))
            {
                Error::log("[Computational Error] NAN value occurred");
                result = Vec3f(0.0f);
            }

            result = glm::clamp(result, Vec3f(0.0f), Vec3f(1e8f));
            auto resultBuffer = scene->camera->getFilm();
            resultBuffer(x, y) = resultBuffer(x, y) * ((float)(curSpp) / (float)(curSpp + 1)) + result / (float)(curSpp + 1);
        }
    }
}