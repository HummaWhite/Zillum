#include "../include/SceneLoader.h"

ScenePtr bidirScene()
{
    auto scene = std::make_shared<Scene>();
    auto transform = std::make_shared<Transform>(glm::rotate(Mat4f(1.0f), glm::radians(90.0f), Vec3f(1.0f, 0.0f, 0.0f)));
    scene->addObjectMesh("res/model/bidir/diffuse.obj", transform,
        std::make_shared<Lambertian>(std::make_shared<SingleColorSpec>(Spectrum(0.5f))));
    scene->addObjectMesh("res/model/bidir/glass.obj", transform,
        std::make_shared<Dielectric>(Spectrum(1.0f), 0.0f, 1.5f));
    scene->addObjectMesh("res/model/bidir/lamp.obj", transform,
        std::make_shared<MetalWorkflow>(Spectrum(0.8f, 0.356f, 0.135f), 1.0f, 0.25f));
    scene->addObjectMesh("res/model/bidir/wood.obj", transform,
        std::make_shared<Lambertian>(std::make_shared<SingleColorSpec>(Spectrum(0.33f, 0.26f, 0.15f))));
    scene->addObjectMesh("res/model/bidir/shell.obj", transform,
        //std::make_shared<MetalWorkflow>(Spectrum(1.0f), 1.0f, 0.014f)
        std::make_shared<Lambertian>(std::make_shared<SingleColorSpec>(Spectrum(1.0f))));
    scene->addLightMesh("res/model/bidir/light1.obj", transform, Spectrum(200.0f));
    scene->addLightMesh("res/model/bidir/light2.obj", transform, Spectrum(400.0f));

    scene->mCamera = std::make_shared<ThinLensCamera>(40.0f);
    scene->mCamera->setPos({0.0f, -12.0f, 4.0f});
    scene->mCamera->lookAt(Vec3f(0.0f, 0.0f, 4.0f));

    scene->mEnv = std::make_shared<EnvSingleColor>(Vec3f(0.0f));
    scene->mLightAndEnvStrategy = LightSampleStrategy::ByPower;
    scene->mLightSampleStrategy = LightSampleStrategy::ByPower;

    return scene;
}

ScenePtr boxScene()
{
    auto scene = std::make_shared<Scene>();
    #define DIR "res/model/"
    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(-3.0f, 0.0f, -3.0f),
                Vec3f(3.0f, 0.0f, -3.0f),
                Vec3f(-3.0f, 6.0f, -3.0f)),
            //std::make_shared<MetalWorkflow>(Spectrum(1.0f), 0.0f, 1.0f)
            std::make_shared<Lambertian>(std::make_shared<SingleColorSpec>(Spectrum(1.0f)))
            //std::make_shared<Dielectric>(Spectrum(1.0f), 0.0f, 0.1f)
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(-3.0f, 0.0f, -3.0f),
                Vec3f(-3.0f, 6.0f, -3.0f),
                Vec3f(-3.0f, 0.0f, 3.0f)),
            //std::make_shared<MetalWorkflow>(Spectrum(1.0f, 0.25f, 0.25f), 0.0f, 1.0f)
            std::make_shared<Lambertian>(std::make_shared<SingleColorSpec>(Spectrum(1.0f, 0.25f, 0.25f)))
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(3.0f, 6.0f, -3.0f),
                Vec3f(3.0f, 0.0f, -3.0f),
                Vec3f(3.0f, 6.0f, 3.0f)),
            //std::make_shared<MetalWorkflow>(Spectrum(0.25f, 0.25f, 1.0f), 0.0f, 1.0f)
            std::make_shared<Lambertian>(std::make_shared<SingleColorSpec>(Spectrum(0.25f, 0.25f, 1.0f)))
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(3.0f, 6.0f, 3.0f),
                Vec3f(3.0f, 0.0f, 3.0f),
                Vec3f(-3.0f, 6.0f, 3.0f)),
            //std::make_shared<MetalWorkflow>(Spectrum(1.0f), 0.0f, 1.0f)
            std::make_shared<Lambertian>(std::make_shared<SingleColorSpec>(Spectrum(1.0f)))
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(-3.0f, 6.0f, -3.0f),
                Vec3f(3.0f, 6.0f, -3.0f),
                Vec3f(-3.0f, 6.0f, 3.0f)),
            //std::make_shared<MetalWorkflow>(Spectrum(1.0f), 1.0f, 0.014f)
            //std::make_shared<Dielectric>(Vec3f(1.0f), 0.0f, 0.1f)
            //std::make_shared<Lambertian>(std::make_shared<SingleColorSpec>(Spectrum(1.0f)))
            std::make_shared<Lambertian>(TextureLoader::fromU8x3("res/checker.png", true))
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
        scene->addObjectMesh(DIR "bunny.obj", std::make_shared<Transform>(model),
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
        scene->addLightMesh(DIR "caustic/light_s.obj", transform, Spectrum(200.0f));
        scene->addObjectMesh(DIR "caustic/mirror.obj", transform,
            std::make_shared<MetalWorkflow>(Spectrum(1.0f), 1.0f, 0.014f));
    }

    {
        auto model = glm::translate(Mat4f(1.0f), Vec3f(2.0f, 0.0f, -2.75f));
        model = glm::rotate(model, glm::radians(45.0f), Vec3f(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, glm::radians(15.0f), Vec3f(1.0f, 0.0f, 0.0f));
        auto transform = std::make_shared<Transform>(model);
        scene->addLightMesh(DIR "caustic/light_s.obj", transform, Spectrum(100.0f));
        scene->addObjectMesh(DIR "caustic/mirror.obj", transform,
            std::make_shared<MetalWorkflow>(Spectrum(1.0f), 1.0f, 0.014f));
    }

    {
        auto model = glm::translate(Mat4f(1.0f), Vec3f(-1.5f, 3.7f, -2.0f));
        model = glm::rotate(model, glm::radians(-45.0f), Vec3f(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, glm::radians(90.0f), Vec3f(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, Vec3f(1.0f));
        scene->addObjectMesh(DIR "cube.obj", std::make_shared<Transform>(model),
            std::make_shared<Dielectric>(Spectrum(1.0f), 0.0f, 1.5f)
            //std::make_shared<Lambertian>(TextureLoader::fromU8x3("res/checker.png"))
        );
    }

    #undef DIR

    scene->addLight(
        std::make_shared<Light>(
            std::make_shared<Quad>(
                Vec3f(-0.75f, 3.75f, 2.999f),
                Vec3f(0.75f, 3.75f, 2.999f),
                Vec3f(-0.75f, 2.25f, 2.999f)),
            Spectrum(150.0f), false));

    // {
    //     auto model = glm::translate(Mat4f(1.0f), Vec3f(-1.5f, 6.0f, 1.0f));
    //     model = glm::rotate(model, glm::radians(180.0f), Vec3f(1.0f, 0.0f, 0.0f));
    //     model = glm::scale(model, Vec3f(1.0f));
    //     auto transform = std::make_shared<Transform>(model);
    //     scene->addObjectMesh("res/model/penn/penn_white.obj", transform,
    //         std::make_shared<MetalWorkflow>(Vec3f(1.0f), 1.0f, 0.5f));
    //     scene->addObjectMesh("res/model/penn/penn_red.obj", transform,
    //         std::make_shared<MetalWorkflow>(RGB24(153, 0, 0).toVec3(), 1.0f, 0.5f));
    //     scene->addObjectMesh("res/model/penn/penn_blue.obj", transform,
    //         std::make_shared<MetalWorkflow>(RGB24(1, 31, 91).toVec3(), 1.0f, 0.5f));
    //     scene->addObjectMesh("res/model/penn/alp_pos.obj", transform,
    //         std::make_shared<MetalWorkflow>(RGB24(1, 31, 91).toVec3(), 1.0f, 0.5f));
    // }

    // scene->addHittable(
    //     std::make_shared<Object>(
    //         std::make_shared<Sphere>(Spectrum(-1.2f, 3.0f, -0.5f), 0.7f, true),
    //         std::make_shared<Dielectric>(Spectrum(1.0f), 0.0f, 1.5f)
    //     ));

    scene->mCamera = std::make_shared<ThinLensCamera>(40.0f);
    scene->mCamera->setPos({ 0.0f, -8.0f, 0.0f });
    scene->mCamera->lookAt(Vec3f(0.0f, 10.0f, 0.0f));

    //scene->mEnv = std::make_shared<EnvSphereMapHDR>("res/texture/090.hdr");
    scene->mEnv = std::make_shared<EnvSingleColor>(Vec3f(0.0f));
    scene->mLightAndEnvStrategy = LightSampleStrategy::ByPower;
    scene->mLightSampleStrategy = LightSampleStrategy::ByPower;

    return scene;
}

ScenePtr originalBox()
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
    scene->mCamera->setPos({ 0.0f, -3.0f, 0.0f });
    scene->mCamera->lookAt(Vec3f(0.0f, 10.0f, 0.0f));

    scene->mEnv = std::make_shared<EnvSphereMapHDR>("res/texture/076.hdr");
    scene->mLightAndEnvStrategy = LightSampleStrategy::Uniform;
    scene->mLightSampleStrategy = LightSampleStrategy::ByPower;

    return scene;
}

ScenePtr fireplace(bool thin, bool lamp)
{
    #define DIR "res/model/fireplace/"
    #define TEX DIR "textures/"
    auto scene = std::make_shared<Scene>();
    auto transform = std::make_shared<Transform>(glm::rotate(Mat4f(1.0f), glm::radians(90.0f), Vec3f(1.0f, 0.0f, 0.0f)));

    scene->addObjectMesh(DIR "bottle_cap.obj", transform, std::make_shared<Lambertian>(std::make_shared<SingleColorSpec>(Vec3f(0.46f, 0.0f, 0.0f))));
    scene->addObjectMesh(DIR "floor.obj", transform, std::make_shared<Lambertian>(TextureLoader::fromU8x3(TEX "wood.png", true)));
    scene->addObjectMesh(DIR "frame.obj", transform, std::make_shared<Lambertian>(std::make_shared<SingleColorSpec>(Vec3f(0.259f, 0.251f, 0.141f))));
    scene->addObjectMesh(DIR "glass.obj", transform, std::make_shared<Dielectric>(Spectrum(1.0f), 0.0f, 1.5f));
    scene->addObjectMesh(DIR "leaves.obj", transform, std::make_shared<Lambertian>(std::make_shared<SingleColorSpec>(Vec3f(1.0f))));
    scene->addObjectMesh(DIR "metal.obj", transform, std::make_shared<MetalWorkflow>(Spectrum(1.0f, 0.7f, 0.4f), 1.0f, 0.25f));
    scene->addObjectMesh(DIR "mirror.obj", transform, std::make_shared<MetalWorkflow>(Spectrum(1.0f), 1.0f, 0.014f));
    scene->addObjectMesh(DIR "photo.obj", transform, std::make_shared<Lambertian>(TextureLoader::fromU8x3(TEX "picture8.png", true)));
    scene->addObjectMesh(DIR "leaves.obj", transform, std::make_shared<Lambertian>(TextureLoader::fromU8x3(TEX "leaf.png", true)));
    scene->addObjectMesh(DIR "plant_base.obj", transform, std::make_shared<Lambertian>(std::make_shared<SingleColorSpec>(Vec3f(0.6f))));
    scene->addObjectMesh(DIR "sofa.obj", transform, std::make_shared<Lambertian>(std::make_shared<SingleColorSpec>(Vec3f(0.9f, 0.9f, 0.87f))));
    scene->addObjectMesh(DIR "sofa_legs.obj", transform, std::make_shared<Lambertian>(std::make_shared<SingleColorSpec>(Vec3f(0.1f))));
    scene->addObjectMesh(DIR "soil.obj", transform, std::make_shared<Lambertian>(std::make_shared<SingleColorSpec>(Vec3f(0.1f))));
    scene->addObjectMesh(DIR "leaves.obj", transform, std::make_shared<Lambertian>(std::make_shared<SingleColorSpec>(Vec3f(1.0f))));
    scene->addObjectMesh(DIR "stem.obj", transform, std::make_shared<Lambertian>(std::make_shared<SingleColorSpec>(Vec3f(0.16, 0.078f, 0.02f))));
    scene->addObjectMesh(DIR "stove_base.obj", transform, std::make_shared<Lambertian>(std::make_shared<SingleColorSpec>(Vec3f(0.05f))));
    scene->addObjectMesh(DIR "stove_ceiling.obj", transform, std::make_shared<Lambertian>(std::make_shared<SingleColorSpec>(Vec3f(1.0f))));
    scene->addObjectMesh(DIR "unknown.obj", transform, std::make_shared<Lambertian>(std::make_shared<SingleColorSpec>(Vec3f(1.0f))));
    scene->addObjectMesh(DIR "wall.obj", transform, std::make_shared<Lambertian>(std::make_shared<SingleColorSpec>(Vec3f(0.169f, 0.133f, 0.102f))));
    scene->addObjectMesh(DIR "window_base.obj", transform, std::make_shared<Lambertian>(std::make_shared<SingleColorSpec>(Vec3f(0.9f))));
    scene->addObjectMesh(DIR "window_frame.obj", transform, std::make_shared<Lambertian>(std::make_shared<SingleColorSpec>(Vec3f(1.0f))));
    scene->addObjectMesh(DIR "wood.obj", transform, std::make_shared<Lambertian>(TextureLoader::fromU8x3(TEX "wood5.png", true)));
    scene->addLightMesh(DIR "light.obj", transform, Spectrum(lamp ? 10.0f : 40.0f));
    if (lamp)
        scene->addLightMesh(DIR "light2.obj", transform, Spectrum(40.0f));
    #undef DIR
    #undef TEX

    if (thin)
    {
        scene->mCamera = std::make_shared<ThinLensCamera>(43.6f);
        scene->mCamera->setPos({ 4.054f, 3.096f, 1.4f });
        scene->mCamera->setAngle({ -147.75f, 0.0f, 0.0f });
    }
    else
    {
        scene->mCamera = std::make_shared<ThinLensCamera>(76.652f);
        scene->mCamera->setPos({ 4.054f, 3.096f, 1.3f });
        scene->mCamera->setAngle({ -147.75f, 1.87f, 0.0f });
    }

    scene->mEnv = std::make_shared<EnvSingleColor>(Vec3f(0.0f));
    scene->mLightAndEnvStrategy = LightSampleStrategy::ByPower;
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
    auto scene = fireplace(false, false);
    scene->mCamera->initFilm(windowWidth, windowHeight);
    return scene;
}