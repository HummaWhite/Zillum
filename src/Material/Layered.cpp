#include "Core/BSDF.h"

Spectrum SlabBSDF::bsdf(const SurfaceIntr& intr, TransportMode mode) {
    if (!bottom) {
        return top->bsdf(intr, mode);
    }
    else if (!top) {
        return bottom->bsdf(intr, mode);
    }

    BSDF* inBSDF = intr.cosWi() > 0 ? top : bottom;
    BSDF* outBSDF = intr.cosWo() > 0 ? top : bottom;

    if (!inBSDF->type().hasType(BSDFType::Transmission)) {
        return inBSDF->bsdf(intr, mode);
    }

    if (inBSDF != outBSDF) {
        //inBSDF->sample(
    }
    else {
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
        return nestedSlabs[0].bsdf(intr.localized(), mode);
    }
}

float LayeredBSDF::pdf(const SurfaceIntr& intr, TransportMode mode) {
    if (nestedSlabs.empty()) {
        return 0.f;
    }
    else {
        return nestedSlabs[0].pdf(intr.localized(), mode);
    }
}

std::optional<BSDFSample> LayeredBSDF::sample(const SurfaceIntr& intr, const Vec3f& u, TransportMode mode) {
    if (nestedSlabs.empty()) {
        return std::nullopt;
    }
    else {
        auto sample = nestedSlabs[0].sample(intr.localized(), u, mode);
        if (!sample) {
            return std::nullopt;
        }
        auto [dir, bsdf, pdf, type, eta] = *sample;
        return BSDFSample(Transform::localToWorld(intr.n, dir), bsdf, pdf, type, eta);
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