#include "../../include/Core/Integrator.h"

#include <array>

void Integrator::setModified()
{
    mModified = true;
    mFinished = false;
}

void Integrator::addToFilmLocked(const Vec2f &uv, const Spectrum &val)
{
    if (!Camera::inFilmBound(uv))
        return;
    auto &film = mScene->mCamera->film();
    auto &filmLocker = mScene->mCamera->filmLocker()(uv.x, uv.y);
    filmLocker.lock();
    film(uv.x, uv.y) += val;
    filmLocker.unlock();
}

PixelIndependentIntegrator::PixelIndependentIntegrator(ScenePtr scene, int maxSpp, IntegratorType type) :
    mMaxSpp(maxSpp), mLimitSpp(maxSpp != 0), Integrator(scene, type)
{
    auto film = scene->mCamera->film();
    mWidth = film.width;
    mHeight = film.height;
}

void PixelIndependentIntegrator::renderOnePass()
{
    if (mModified)
    {
        mScene->mCamera->film().fill(Vec3f(0.0f));
        mCurspp = 0;
        mModified = false;
    }
    if (mLimitSpp && mCurspp >= mMaxSpp)
    {
        mFinished = true;
        return;
    }

    std::thread *threads = new std::thread[MaxThreads];
    for (int i = 0; i < MaxThreads; i++)
    {
        int start = (mWidth / MaxThreads) * i;
        int end = std::min(mWidth, (mWidth / MaxThreads) * (i + 1));
        if (i == MaxThreads - 1)
            end = mWidth;

        auto threadSampler = (i == 0) ? mSampler : mSampler->copy();
        threads[i] = std::thread(doTracing, this, start, end, threadSampler);
    }
    mSampler->nextSample();

    for (int i = 0; i < MaxThreads; i++)
        threads[i].join();
    delete[] threads;

    mCurspp++;
    std::cout << "\r" << std::setw(4) << mCurspp << "/" << mMaxSpp << " spp  ";

    float perc = (float)mCurspp / (float)mMaxSpp * 100.0f;
    std::cout << "  " << std::fixed << std::setprecision(2) << perc << "%";
    scaleResult();
}

void PixelIndependentIntegrator::doTracing(int start, int end, SamplerPtr sampler)
{
    float invW = 1.0f / mWidth;
    float invH = 1.0f / mHeight;
    for (int x = start; x < end; x++)
    {
        for (int y = 0; y < mHeight; y++)
        {
            sampler->setPixel(x, y);
            float sx = 2.0f * (x + 0.5f) * invW - 1.0f;
            float sy = 1.0f - 2.0f * (y + 0.5f) * invH;

            Ray ray = mScene->mCamera->generateRay({ sx, sy }, sampler);
            Spectrum result = tracePixel(ray, sampler);

            if (Math::hasNan(result))
            {
                Error::bracketLine<0>("nan discovered");
                result = Spectrum(0.0f);
            }
            result = glm::clamp(result, Spectrum(0.0f), Spectrum(1e8f));
            auto resultBuffer = mScene->mCamera->film();
            resultBuffer(x, y) += result;
        }
    }
}

void PixelIndependentIntegrator::scaleResult()
{
    mResultScale = 1.0f / mCurspp;
}