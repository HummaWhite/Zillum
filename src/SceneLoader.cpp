#include "../include/SceneLoader.h"

ScenePtr bidirScene(int width, int height)
{
    auto scene = std::make_shared<Scene>();
    auto transform = std::make_shared<Transform>(glm::rotate(Mat4f(1.0f), glm::radians(90.0f), Vec3f(1.0f, 0.0f, 0.0f)));
    scene->addObjectMesh("res/model/bidir/diffuse.obj", transform,
        std::make_shared<Lambertian>(Spectrum(0.5f)));
    scene->addObjectMesh("res/model/bidir/glass.obj", transform,
        std::make_shared<Dielectric>(Spectrum(1.0f), 0.0f, 1.5f));
    scene->addObjectMesh("res/model/bidir/lamp.obj", transform,
        std::make_shared<MetalWorkflow>(Spectrum(0.8f, 0.356f, 0.135f), 1.0f, 0.25f));
    scene->addObjectMesh("res/model/bidir/wood.obj", transform,
        std::make_shared<Lambertian>(Spectrum(0.33f, 0.26f, 0.15f)));
    scene->addObjectMesh("res/model/bidir/shell.obj", transform,
        //std::make_shared<MetalWorkflow>(Spectrum(1.0f), 1.0f, 0.014f)
        std::make_shared<Lambertian>(Spectrum(1.0f)));
    scene->addLightMesh("res/model/bidir/light1.obj", transform, Spectrum(200.0f));
    scene->addLightMesh("res/model/bidir/light2.obj", transform, Spectrum(400.0f));

    scene->mCamera = std::make_shared<ThinLensCamera>(40.0f);
    scene->mCamera->initFilm(width, height);
    scene->mCamera->setPos({0.0f, -12.0f, 4.0f});
    scene->mCamera->lookAt(Vec3f(0.0f, 0.0f, 4.0f));

    scene->mEnv = std::make_shared<EnvSingleColor>(Vec3f(0.0f));
    scene->mLightAndEnvStrategy = LightSampleStrategy::ByPower;
    scene->mLightSampleStrategy = LightSampleStrategy::ByPower;

    return scene;
}

ScenePtr boxScene(int width, int height)
{
    auto scene = std::make_shared<Scene>();

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(-3.0f, 0.0f, -3.0f),
                Vec3f(3.0f, 0.0f, -3.0f),
                Vec3f(-3.0f, 6.0f, -3.0f)),
            //std::make_shared<MetalWorkflow>(Spectrum(1.0f), 0.0f, 1.0f)
            std::make_shared<Lambertian>(Spectrum(1.0f))
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(-3.0f, 0.0f, -3.0f),
                Vec3f(-3.0f, 6.0f, -3.0f),
                Vec3f(-3.0f, 0.0f, 3.0f)),
            //std::make_shared<MetalWorkflow>(Spectrum(1.0f, 0.25f, 0.25f), 0.0f, 1.0f)
            std::make_shared<Lambertian>(Spectrum(1.0f, 0.25f, 0.25f))
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(3.0f, 6.0f, -3.0f),
                Vec3f(3.0f, 0.0f, -3.0f),
                Vec3f(3.0f, 6.0f, 3.0f)),
            //std::make_shared<MetalWorkflow>(Spectrum(0.25f, 0.25f, 1.0f), 0.0f, 1.0f)
            std::make_shared<Lambertian>(Spectrum(0.25f, 0.25f, 1.0f))
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(3.0f, 6.0f, 3.0f),
                Vec3f(3.0f, 0.0f, 3.0f),
                Vec3f(-3.0f, 6.0f, 3.0f)),
            //std::make_shared<MetalWorkflow>(Spectrum(1.0f), 0.0f, 1.0f)
            std::make_shared<Lambertian>(Spectrum(1.0f))
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(-3.0f, 6.0f, -3.0f),
                Vec3f(3.0f, 6.0f, -3.0f),
                Vec3f(-3.0f, 6.0f, 3.0f)),
            //std::make_shared<MetalWorkflow>(Spectrum(1.0f), 1.0f, 0.014f)
            //std::make_shared<Dielectric>(Vec3f(1.0f), 0.0f, 0.1f)
            std::make_shared<Lambertian>(Spectrum(1.0f))
            ));

    // scene->addHittable(
    //     std::make_shared<Object>(
    //         std::make_shared<Sphere>(Spectrum(-1.5f, 3.0f, -1.0f), 1.0f, true),
    //         std::make_shared<Dielectric>(Spectrum(1.0f), 0.0f, 1.5f)
    //     ));

    {
        auto model = glm::translate(Mat4f(1.0f), Vec3f(1.0f, 2.0f, -2.1f));
        model = glm::rotate(model, glm::radians(-17.5f), Vec3f(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, Vec3f(1.8f));

        // scene->addObjectMesh("res/model/cube.obj", std::make_shared<Transform>(model),
        //     std::make_shared<MetalWorkflow>(Spectrum(1.0f, 0.8f, 0.6f), 1.0f, 0.14f)
        //     //std::make_shared<Lambertian>(Spectrum(1.0f))
        //     //std::make_shared<Dielectric>(Spectrum(1.0f), 0.0f, 1.5f)
        // );
    }
    {
        auto model = glm::translate(Mat4f(1.0f), Vec3f(-1.0f, 4.0f, -1.2f));
        model = glm::rotate(model, glm::radians(17.5f), Vec3f(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, Vec3f(1.8f, 1.8f, 3.6f));
        std::shared_ptr<Transform> trBoxLarge = std::make_shared<Transform>(model);

        // scene->addObjectMesh("res/model/cube.obj", trBoxLarge,
        //     //std::make_shared<MetalWorkflow>(Spectrum(1.0f), 1.0f, 0.14f)
        //     //std::make_shared<Dielectric>(Vec3f(1.0f), 0.0f, 0.1f)
        //     std::make_shared<Lambertian>(Spectrum(1.0f))
        // );
    }
    {
        // auto model = glm::translate(Mat4f(1.0f), Vec3f(-0.5f, 3.0f, -1.0f));
        // model = glm::rotate(model, glm::radians(90.0f), Vec3f(1.0f, 0.0f, 0.0f));
        // model = glm::scale(model, Vec3f(1.0f));
        // scene->addObjectMesh("res/model/bunny.obj", std::make_shared<Transform>(model),
        //     std::make_shared<Dielectric>(Spectrum(1.0f), 0.0f, 1.5f));

        // auto model = glm::translate(Mat4f(1.0f), Vec3f(-0.75f, 4.0f, -1.0f));
        // model = glm::rotate(model, glm::radians(95.0f), Vec3f(0.0f, 0.0f, 1.0f));
        // model = glm::rotate(model, glm::radians(-20.0f), Vec3f(0.0f, 1.0f, 0.0f));
        // model = glm::rotate(model, glm::radians(110.0f), Vec3f(1.0f, 0.0f, 0.0f));
        // model = glm::scale(model, Vec3f(0.5f));
        // scene->addObjectMesh("res/model/Mesh003.obj", std::make_shared<Transform>(model),
        //     std::make_shared<Dielectric>(Spectrum(1.0f), 0.0f, 1.5f));
        // scene->addObjectMesh("res/model/Mesh004.obj", std::make_shared<Transform>(model),
        //     std::make_shared<Dielectric>(Spectrum(1.0f), 0.0f, 1.5f));

        auto model = glm::translate(Mat4f(1.0f), Vec3f(1.5f, 4.0f, -2.5f));
        model = glm::rotate(model, glm::radians(90.0f), Vec3f(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, Vec3f(1.0f));
        scene->addObjectMesh("res/model/bunny.obj", std::make_shared<Transform>(model),
            std::make_shared<MetalWorkflow>(Spectrum(1.0f, 0.7f, 0.4f), 1.0f, 0.014f)
            //std::make_shared<Dielectric>(Spectrum(1.0f), 0.0f, 1.5f)
        );
    }

    // scene->addLight(
    //     std::make_shared<Light>(
    //         std::make_shared<Quad>(
    //             Vec3f(-0.75f, 3.75f, 2.999f),
    //             Vec3f(0.75f, 3.75f, 2.999f),
    //             Vec3f(-0.75f, 2.25f, 2.999f)),
    //             // Vec3f(-0.01f, 3.01f, 2.999f),
    //             // Vec3f(0.01f, 3.01f, 2.999f),
    //             // Vec3f(-0.01f, 2.99f, 2.999f)),
    //         Spectrum(200.0f), false));
    {
        auto model = glm::translate(Mat4f(1.0f), Vec3f(-2.0f, 0.0f, -2.75f));
        model = glm::rotate(model, glm::radians(-45.0f), Vec3f(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, glm::radians(15.0f), Vec3f(1.0f, 0.0f, 0.0f));
        auto transform = std::make_shared<Transform>(model);
        scene->addLightMesh("res/model/caustic/light_s.obj", transform, Spectrum(200.0f));
        scene->addObjectMesh("res/model/caustic/mirror.obj", transform,
            std::make_shared<MetalWorkflow>(Spectrum(1.0f), 1.0f, 0.014f));
    }

    {
        auto model = glm::translate(Mat4f(1.0f), Vec3f(2.0f, 0.0f, -2.75f));
        model = glm::rotate(model, glm::radians(45.0f), Vec3f(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, glm::radians(15.0f), Vec3f(1.0f, 0.0f, 0.0f));
        auto transform = std::make_shared<Transform>(model);
        scene->addLightMesh("res/model/caustic/light_s.obj", transform, Spectrum(100.0f));
        scene->addObjectMesh("res/model/caustic/mirror.obj", transform,
            std::make_shared<MetalWorkflow>(Spectrum(1.0f), 1.0f, 0.014f));
    }

    {
        auto model = glm::translate(Mat4f(1.0f), Vec3f(-1.5f, 3.7f, -2.0f));
        model = glm::rotate(model, glm::radians(-45.0f), Vec3f(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, glm::radians(90.0f), Vec3f(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, Vec3f(1.0f));
        scene->addObjectMesh("res/model/cube.obj", std::make_shared<Transform>(model),
            std::make_shared<Dielectric>(Spectrum(1.0f), 0.0f, 1.5f)
        );
    }

    // scene->addHittable(
    //     std::make_shared<Object>(
    //         std::make_shared<Sphere>(Spectrum(-1.2f, 3.0f, -0.5f), 0.7f, true),
    //         std::make_shared<Dielectric>(Spectrum(1.0f), 0.0f, 1.5f)
    //     ));

    scene->addLight(
        std::make_shared<Light>(
            std::make_shared<Quad>(
                Vec3f(-0.75f, 3.75f, 2.999f),
                Vec3f(0.75f, 3.75f, 2.999f),
                Vec3f(-0.75f, 2.25f, 2.999f)),
            Spectrum(150.0f), false));

    scene->mCamera = std::make_shared<ThinLensCamera>(40.0f);
    scene->mCamera->initFilm(width, height);
    scene->mCamera->setPos({ 0.0f, -8.0f, 0.0f });
    scene->mCamera->lookAt(Vec3f(0.0f, 10.0f, 0.0f));

    //scene->mEnv = std::make_shared<EnvSphereMapHDR>("res/texture/090.hdr");
    scene->mEnv = std::make_shared<EnvSingleColor>(Vec3f(0.0f));
    scene->mLightAndEnvStrategy = LightSampleStrategy::ByPower;
    scene->mLightSampleStrategy = LightSampleStrategy::ByPower;

    return scene;
}

ScenePtr originalBox(int width, int height)
{
    auto scene = std::make_shared<Scene>();
    float roughness = 0.05f;

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(-3.0f, 0.0f, -3.0f),
                Vec3f(3.0f, 0.0f, -3.0f),
                Vec3f(-3.0f, 6.0f, -3.0f)),
            std::make_shared<MetalWorkflow>(Spectrum(1.0f), 0.0f, 1.0f)
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(-3.0f, 0.0f, -3.0f),
                Vec3f(-3.0f, 6.0f, -3.0f),
                Vec3f(-3.0f, 0.0f, 3.0f)),
            std::make_shared<MetalWorkflow>(Spectrum(1.0f, 0.0f, 0.0f), 1.0f, roughness)
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(3.0f, 6.0f, -3.0f),
                Vec3f(3.0f, 0.0f, -3.0f),
                Vec3f(3.0f, 6.0f, 3.0f)),
            std::make_shared<MetalWorkflow>(Spectrum(0.0f, 1.0f, 0.0f), 1.0f, roughness)
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(3.0f, 6.0f, 3.0f),
                Vec3f(3.0f, 0.0f, 3.0f),
                Vec3f(-3.0f, 6.0f, 3.0f)),
            std::make_shared<MetalWorkflow>(Spectrum(1.0f), 0.0f, 1.0f)
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(-3.0f, 3.0f, -3.0f),
                Vec3f(3.0f, 3.0f, -3.0f),
                Vec3f(-3.0f, 3.0f, 3.0f)),
            std::make_shared<MetalWorkflow>(Spectrum(1.0f), 1.0f, roughness)
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Sphere>(Vec3f(1.0f, 1.5f, -2.0f), 1.2f, false),
            std::make_shared<MetalWorkflow>(Spectrum(1.0f), 1.0f, 0.015f)
        ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Sphere>(Vec3f(-1.2f, 1.8f, -1.0f), 1.0f, false),
            std::make_shared<MetalWorkflow>(Spectrum(0.2f, 0.4f, 1.0f), 0.0f, 0.12f)
        ));

    scene->addLight(
        std::make_shared<Light>(
            std::make_shared<Quad>(
                Vec3f(-2.0f, 2.5f, 2.99f),
			    Vec3f(2.0f, 2.5f, 2.99f),
			    Vec3f(-2.0f, 0.5f, 2.99f)),
            Spectrum(160.0f), false));

    scene->mCamera = std::make_shared<ThinLensCamera>(90.0f);
    scene->mCamera->initFilm(width, height);
    scene->mCamera->setPos({ 0.0f, -3.0f, 0.0f });
    scene->mCamera->lookAt(Vec3f(0.0f, 10.0f, 0.0f));

    scene->mEnv = std::make_shared<EnvSphereMapHDR>("res/texture/076.hdr");
    scene->mLightAndEnvStrategy = LightSampleStrategy::Uniform;
    scene->mLightSampleStrategy = LightSampleStrategy::ByPower;

    return scene;
}

ScenePtr setupScene(int windowWidth, int windowHeight)
{
    // const float level = -1.0f;
    // const float size = 5.0f;
    // scene->addHittable(
    //     std::make_shared<Object>(
    //     std::make_shared<Quad>(
    //         Vec3f(-size * 2.0f, -size, level),
    //         Vec3f(size * 2.0f, -size, level),
    //         Vec3f(-size * 1.0f, size, level)),
    //     std::make_shared<MetalWorkflow>(Spectrum(1.0f), 0.0f, 1.0f)));

    // scene->addHittable(
    //     std::make_shared<Object>(
    //         std::make_shared<Sphere>(Vec3f(0.0f), 1.0f, true),
    //         //std::make_shared<Dielectric>(Spectrum(1.0f), 0.0f, 1.5f)
    //         //std::make_shared<Clearcoat>(0.01f, 1.0f)
    //         //std::make_shared<MetalWorkflow>(Spectrum(1.0f, 0.5f, 0.2f), 0.0f, 1.0f)
    //         std::make_shared<Lambertian>(Spectrum(1.0f, 0.5f, 0.2f))
    //         //std::make_shared<DisneyDiffuse>(Spectrum(1.0f, 0.5f, 0.2f), 1.0f, 1.0f)
    //     ));
    return boxScene(windowWidth, windowHeight);
}