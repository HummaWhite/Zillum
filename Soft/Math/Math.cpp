#include "Math.h"

namespace Math
{
    namespace FloatBitMask
    {
        const uint32_t Sign = 0x80000000;
        const uint32_t Exp = 0xff << 23;
        const uint32_t Val = 0x7fffff;
    }

    void printBits32(void *bits, std::string info)
    {
        uint32_t v = *(uint32_t*)bits;
        for (uint32_t i = 0x80000000; i; i >>= 1)
            std::cout << (i & v ? 1 : 0);
        std::cout << info;
    }

    void printVec3(const Vec3f &v, std::string info)
    {
        std::cout << std::setprecision(6) << info << "  " << v.x << "  " << v.y << "  " << v.z << "\n";
    }

    std::string vec3ToString(const Vec3f &v)
    {
        std::stringstream ss;
        ss << "[Vec3 " << v.x << " " << v.y << " " << v.z << "]";
        return ss.str();
    }

    bool isNan(float v)
    {
        uint32_t u = *(int *)&v;
        return ((u & FloatBitMask::Exp) == FloatBitMask::Exp && (u & FloatBitMask::Val) != 0);
    }

    bool hasNan(const Vec3f &v)
    {
        return isNan(v.x) || isNan(v.y) || isNan(v.z);
    }

    bool isInf(float v)
    {
        uint32_t u = *(uint32_t *)&v;
        return ((u & FloatBitMask::Exp) == FloatBitMask::Exp && (u & FloatBitMask::Val) == 0);
    }

    glm::mat3 TBNMatrix(const Vec3f &N)
    {
        Vec3f T = (glm::abs(N.z) > 0.99f) ? Vec3f(0.0f, 1.0f, 0.0f) : Vec3f(0.0f, 0.0f, 1.0f);
        Vec3f B = glm::normalize(glm::cross(N, T));
        T = glm::normalize(glm::cross(B, N));
        return glm::mat3(T, B, N);
    }

    uint32_t inverseBits(uint32_t bits)
    {
        bits = (bits << 16u) | (bits >> 16u);
        bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
        bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
        bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
        bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
        return bits;
    }

    float radicalInverse(uint32_t bits)
    {
        return float(inverseBits(bits)) * 2.3283064365386963e-10;
    }

    float satDot(const Vec3f &a, const Vec3f &b)
    {
        return glm::max(glm::dot(a, b), 0.0f);
    }

    float absDot(const Vec3f &a, const Vec3f &b)
    {
        return glm::abs(glm::dot(a, b));
    }

    bool coin(float u)
    {
        return u < 0.5f;
    }

    bool sameHemisphere(const Vec3f &N, const Vec3f &A, const Vec3f &B)
    {
        return glm::dot(N, A) * glm::dot(N, B) > 0.0f;
    }

    float maxComponent(const Vec3f &v)
    {
        return glm::max(glm::max(v.x, v.y), v.z);
    }

    float minComponent(const Vec3f &v)
    {
        return glm::min(glm::min(v.x, v.y), v.z);
    }

    int maxExtent(const Vec3f &v)
    {
        if (v.x > v.y)
            return v.x > v.z ? 0 : 2;
        return v.y > v.z ? 1 : 2;
    }

    int cubeMapFace(const Vec3f &dir)
    {
        int maxDim = maxExtent(glm::abs(dir));
        return maxDim * 2 + (vecElement(dir, maxDim) < 0);
    }

    float qpow(float x, int n)
    {
        float ret = 1.0f;
        while (n)
        {
            if (n & 1)
                ret *= x;
            x *= x, n >>= 1;
        }
        return ret;
    }

    float heuristic(int nf, float pf, int ng, float pg, int pow)
    {
        float f = qpow(nf * pf, pow);
        float g = qpow(ng * pg, pow);
        return f / (f + g);
    }

    float biHeuristic(float pf, float pg)
    {
        return heuristic(1, pf, 1, pg, 2);
    }

    float rgbBrightness(const Vec3f &c)
    {
        return glm::dot(BRIGHTNESS, c);
    }

    float diskArea(float radius)
    {
        return radius * radius * Math::Pi;
    }
}