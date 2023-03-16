#include "Core/BSDF.h"

Spectrum SlabBSDF::bsdf(const SurfaceIntr& intr, TransportMode mode) {
    if (!bottom) {
        return top->bsdf(intr, mode);
    }
    else if (!top) {
        return bottom->bsdf(intr, mode);
    }
}

float SlabBSDF::pdf(const SurfaceIntr& intr, TransportMode mode) {
    if (!bottom) {
        return top->pdf(intr, mode);
    }
    else if (!top) {
        return bottom->pdf(intr, mode);
    }
}

std::optional<BSDFSample> SlabBSDF::sample(const SurfaceIntr& intr, const Vec3f& u, TransportMode mode) {
    if (!bottom) {
        return top->sample(intr, u, mode);
    }
    else if (!top) {
        return bottom->sample(intr, u, mode);
    }
}

Spectrum LayeredBSDF::bsdf(const SurfaceIntr& intr, TransportMode mode) {
    if (nestedSlabs.empty()) {
        return Spectrum(0.f);
    }
    else {
        return nestedSlabs[0].bsdf(intr, mode);
    }
}

float LayeredBSDF::pdf(const SurfaceIntr& intr, TransportMode mode) {
    if (nestedSlabs.empty()) {
        return 0.f;
    }
    else {
        return nestedSlabs[0].pdf(intr, mode);
    }
}

std::optional<BSDFSample> LayeredBSDF::sample(const SurfaceIntr& intr, const Vec3f& u, TransportMode mode) {
    if (nestedSlabs.empty()) {
        return std::nullopt;
    }
    else {
        return nestedSlabs[0].sample(intr, u, mode);
    }
}

void LayeredBSDF::addBSDF(BSDF* bsdf) {
    if (nestedSlabs.empty()) {
        nestedSlabs.push_back({ bsdf, nullptr });
    }
    else {
        int endIdx = nestedSlabs.size() - 1;
        if (!nestedSlabs[endIdx].bottom) {
            nestedSlabs[endIdx].bottom = bsdf;
        }
        else {
            nestedSlabs.push_back({ bsdf, nullptr });
            nestedSlabs[endIdx].bottom = &nestedSlabs[endIdx + 1];
        }
    }
}