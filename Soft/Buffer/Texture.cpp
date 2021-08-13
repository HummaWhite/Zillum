#include "Texture.h"

void Texture::loadRGB24(const char *filePath)
{
    std::cout << "Texture::loading RGB: " << filePath << std::endl;

    int bits;
    uint8_t *data = stbi_load(filePath, &width, &height, &bits, 3);

    if (data == nullptr)
    {
        std::cout << "Texture::error loading" << std::endl;
        exit(-1);
    }

    init(width, height);
    for (int i = 0; i < width; i++)
    {
        for (int j = 0; j < height; j++)
        {
            (*this)(i, j) = glm::vec3((*(RGB24 *)&data[(j * width + i) * 3]).toVec4());
        }
    }

    if (data != nullptr)
        stbi_image_free(data);
}

void Texture::loadFloat(const char *filePath)
{
    std::cout << "Texture::loading HDR: " << filePath << std::endl;

    int bits;
    float *data = stbi_loadf(filePath, &width, &height, &bits, 0);

    if (data == nullptr)
    {
        std::cout << "Texture::error loading" << std::endl;
        exit(-1);
    }

    init(width, height);
    memcpy(bufPtr(), data, width * height * sizeof(glm::vec3));

    if (data != nullptr)
        stbi_image_free(data);
}

glm::vec4 Texture::get(float u, float v)
{
    if (Math::isNan(u) || Math::isNan(v))
        return glm::vec4(0.0f);

    glm::vec4 res(1.0f);

    float x = (width - 1) * u;
    float y = (height - 1) * v;

    int u1 = (int)(x);
    int v1 = (int)(y);
    int u2 = (int)(x + 1.0f);
    int v2 = (int)(y + 1.0f);

    if (filterType == FilterType::NEAREST)
    {
        int pu = (u2 - x) > (x - u1) ? u2 : u1;
        int pv = (v2 - y) > (y - v1) ? v2 : v1;

        pu = (pu + width) % width;
        pv = (pv + height) % height;

        return glm::vec4((*this)(pu, pv), 1.0f);
    }
    else if (filterType == FilterType::LINEAR)
    {
        u1 = (u1 + width) % width;
        v1 = (v1 + height) % height;
        u2 = (u2 + width) % width;
        v2 = (v2 + height) % height;

        glm::vec3 c1 = (*this)(u1, v1);
        glm::vec3 c2 = (*this)(u2, v1);
        glm::vec3 c3 = (*this)(u1, v2);
        glm::vec3 c4 = (*this)(u2, v2);

        float lx = x - (int)x;
        float ly = y - (int)y;

        return glm::vec4(Math::lerp(Math::lerp(c1, c2, lx), Math::lerp(c3, c4, lx), ly), 1.0f);
    }
    else
        return glm::vec4(0.0f);
}

glm::vec4 Texture::getSpherical(const glm::vec3 &uv)
{
    if (Math::isNan(uv.x) || Math::isNan(uv.y) || Math::isNan(uv.z))
    {
        std::cout << "Texture::getSpherical: Invalid uv with NAN(s)\n";
        return get(0.0f, 0.0f);
    }
    glm::vec2 planeUV = Transform::sphereToPlane(glm::normalize(uv));
    return get(planeUV.x, planeUV.y);
}