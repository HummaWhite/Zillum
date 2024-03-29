#include "SceneLoader.h"

ScenePtr bidirScene() {
    auto scene = std::make_shared<Scene>();
    Transform transform(glm::rotate(Mat4f(1.0f), glm::radians(90.0f), Vec3f(1.0f, 0.0f, 0.0f)));
    scene->addObjectMesh("res/model/bidir/diffuse.obj", transform,
        std::make_shared<LambertBSDF>(ColorMap(Spectrum(0.5f))));
    scene->addObjectMesh("res/model/bidir/glass.obj", transform,
        std::make_shared<DielectricBSDF>(Spectrum(1.0f), 0.0f, 1.5f));
    scene->addObjectMesh("res/model/bidir/lamp.obj", transform,
        std::make_shared<MetallicWorkflowBSDF>(ColorMap(Spectrum(0.8f, 0.356f, 0.135f)), 1.0f, 0.25f));
    scene->addObjectMesh("res/model/bidir/wood.obj", transform,
        std::make_shared<LambertBSDF>(ColorMap(Spectrum(0.33f, 0.26f, 0.15f))));
    scene->addObjectMesh("res/model/bidir/shell.obj", transform,
        //std::make_shared<MetallicWorkflowBSDF>(ColorMap(Spectrum(1.0f), 1.0f, 0.014f)
        std::make_shared<LambertBSDF>(ColorMap(Spectrum(1.0f))));
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

ScenePtr boxScene() {
    auto scene = std::make_shared<Scene>();
    #define DIR "res/model/"
    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(-3.0f, 0.0f, -3.0f),
                Vec3f(3.0f, 0.0f, -3.0f),
                Vec3f(-3.0f, 6.0f, -3.0f)),
            //std::make_shared<MetallicWorkflowBSDF>(ColorMap(Spectrum(1.0f), 0.0f, 1.0f)
            std::make_shared<LambertBSDF>(ColorMap(Spectrum(1.0f)))
            //std::make_shared<DielectricBSDF>(Spectrum(1.0f), 0.0f, 0.1f)
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(-3.0f, 0.0f, -3.0f),
                Vec3f(-3.0f, 6.0f, -3.0f),
                Vec3f(-3.0f, 0.0f, 3.0f)),
            //std::make_shared<MetallicWorkflowBSDF>(ColorMap(Spectrum(1.0f, 0.25f, 0.25f), 0.0f, 1.0f)
            std::make_shared<LambertBSDF>(ColorMap(Spectrum(1.0f, 0.25f, 0.25f)))
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(3.0f, 6.0f, -3.0f),
                Vec3f(3.0f, 0.0f, -3.0f),
                Vec3f(3.0f, 6.0f, 3.0f)),
            //std::make_shared<MetallicWorkflowBSDF>(ColorMap(Spectrum(0.25f, 0.25f, 1.0f), 0.0f, 1.0f)
            std::make_shared<LambertBSDF>(ColorMap(Spectrum(0.25f, 0.25f, 1.0f)))
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(3.0f, 6.0f, 3.0f),
                Vec3f(3.0f, 0.0f, 3.0f),
                Vec3f(-3.0f, 6.0f, 3.0f)),
            //std::make_shared<MetallicWorkflowBSDF>(ColorMap(Spectrum(1.0f), 0.0f, 1.0f)
            std::make_shared<LambertBSDF>(ColorMap(Spectrum(1.0f)))
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(-3.0f, 6.0f, -3.0f),
                Vec3f(3.0f, 6.0f, -3.0f),
                Vec3f(-3.0f, 6.0f, 3.0f)),
            //std::make_shared<MetallicWorkflowBSDF>(ColorMap(Spectrum(1.0f), 1.0f, 0.014f)
            //std::make_shared<DielectricBSDF>(Vec3f(1.0f), 0.0f, 0.1f)
            //std::make_shared<LambertBSDF>(ColorMap(Spectrum(1.0f)))
            std::make_shared<LambertBSDF>(TextureLoader::fromU8x3("res/checker.png", true))
            ));

    // scene->addHittable(
    //     std::make_shared<Object>(
    //         std::make_shared<Sphere>(Spectrum(-1.5f, 3.0f, -1.0f), 1.0f, true),
    //         std::make_shared<DielectricBSDF>(Spectrum(1.0f), 0.0f, 1.5f)
    //     ));

    {
        auto model = glm::translate(Mat4f(1.0f), Vec3f(1.0f, 2.0f, -2.1f));
        model = glm::rotate(model, glm::radians(-17.5f), Vec3f(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, Vec3f(1.8f));

        // scene->addObjectMesh("res/model/cube.obj", std::make_shared<Transform>(model),
        //     std::make_shared<MetallicWorkflowBSDF>(ColorMap(Spectrum(1.0f, 0.8f, 0.6f), 1.0f, 0.14f)
        //     //std::make_shared<LambertBSDF>(Spectrum(1.0f))
        //     //std::make_shared<DielectricBSDF>(Spectrum(1.0f), 0.0f, 1.5f)
        // );
    }
    {
        auto model = glm::translate(Mat4f(1.0f), Vec3f(-1.0f, 4.0f, -1.2f));
        model = glm::rotate(model, glm::radians(17.5f), Vec3f(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, Vec3f(1.8f, 1.8f, 3.6f));
        std::shared_ptr<Transform> trBoxLarge = std::make_shared<Transform>(model);

        // scene->addObjectMesh("res/model/cube.obj", trBoxLarge,
        //     //std::make_shared<MetallicWorkflowBSDF>(ColorMap(Spectrum(1.0f), 1.0f, 0.14f)
        //     //std::make_shared<DielectricBSDF>(Vec3f(1.0f), 0.0f, 0.1f)
        //     std::make_shared<LambertBSDF>(Spectrum(1.0f))
        // );
    }
    {
        // auto model = glm::translate(Mat4f(1.0f), Vec3f(-0.5f, 3.0f, -1.0f));
        // model = glm::rotate(model, glm::radians(90.0f), Vec3f(1.0f, 0.0f, 0.0f));
        // model = glm::scale(model, Vec3f(1.0f));
        // scene->addObjectMesh("res/model/bunny.obj", std::make_shared<Transform>(model),
        //     std::make_shared<DielectricBSDF>(Spectrum(1.0f), 0.0f, 1.5f));

        // auto model = glm::translate(Mat4f(1.0f), Vec3f(-0.75f, 4.0f, -1.0f));
        // model = glm::rotate(model, glm::radians(95.0f), Vec3f(0.0f, 0.0f, 1.0f));
        // model = glm::rotate(model, glm::radians(-20.0f), Vec3f(0.0f, 1.0f, 0.0f));
        // model = glm::rotate(model, glm::radians(110.0f), Vec3f(1.0f, 0.0f, 0.0f));
        // model = glm::scale(model, Vec3f(0.5f));
        // scene->addObjectMesh("res/model/Mesh003.obj", std::make_shared<Transform>(model),
        //     std::make_shared<DielectricBSDF>(Spectrum(1.0f), 0.0f, 1.5f));
        // scene->addObjectMesh("res/model/Mesh004.obj", std::make_shared<Transform>(model),
        //     std::make_shared<DielectricBSDF>(Spectrum(1.0f), 0.0f, 1.5f));

        auto model = glm::translate(Mat4f(1.0f), Vec3f(1.5f, 4.0f, -2.5f));
        model = glm::rotate(model, glm::radians(90.0f), Vec3f(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, Vec3f(1.0f));
        scene->addObjectMesh(DIR "bunny.obj", { model },
            std::make_shared<MetallicWorkflowBSDF>(ColorMap(Spectrum(1.0f, 0.7f, 0.4f)), 1.0f, 0.014f)
            //std::make_shared<DielectricBSDF>(Spectrum(1.0f), 0.0f, 1.5f)
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
        Transform transform(model);
        scene->addLightMesh(DIR "caustic/light_s.obj", transform, Spectrum(200.0f));
        scene->addObjectMesh(DIR "caustic/mirror.obj", transform,
            std::make_shared<MetallicWorkflowBSDF>(ColorMap(Spectrum(1.0f)), 1.0f, 0.014f));
    }

    {
        auto model = glm::translate(Mat4f(1.0f), Vec3f(2.0f, 0.0f, -2.75f));
        model = glm::rotate(model, glm::radians(45.0f), Vec3f(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, glm::radians(15.0f), Vec3f(1.0f, 0.0f, 0.0f));
        Transform transform(model);
        scene->addLightMesh(DIR "caustic/light_s.obj", transform, Spectrum(100.0f));
        scene->addObjectMesh(DIR "caustic/mirror.obj", transform,
            std::make_shared<MetallicWorkflowBSDF>(ColorMap(Spectrum(1.0f)), 1.0f, 0.014f));
    }

    {
        auto model = glm::translate(Mat4f(1.0f), Vec3f(-1.5f, 3.7f, -2.0f));
        model = glm::rotate(model, glm::radians(-45.0f), Vec3f(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, glm::radians(90.0f), Vec3f(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, Vec3f(1.0f));
        scene->addObjectMesh(DIR "cube.obj", { model },
            std::make_shared<DielectricBSDF>(Spectrum(1.0f), 0.0f, 1.5f)
            //std::make_shared<LambertBSDF>(TextureLoader::fromU8x3("res/checker.png"))
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
    //         std::make_shared<MetallicWorkflowBSDF>(Vec3f(1.0f), 1.0f, 0.5f));
    //     scene->addObjectMesh("res/model/penn/penn_red.obj", transform,
    //         std::make_shared<MetallicWorkflowBSDF>(RGB24(153, 0, 0).toVec3(), 1.0f, 0.5f));
    //     scene->addObjectMesh("res/model/penn/penn_blue.obj", transform,
    //         std::make_shared<MetallicWorkflowBSDF>(RGB24(1, 31, 91).toVec3(), 1.0f, 0.5f));
    //     scene->addObjectMesh("res/model/penn/alp_pos.obj", transform,
    //         std::make_shared<MetallicWorkflowBSDF>(RGB24(1, 31, 91).toVec3(), 1.0f, 0.5f));
    // }

    // scene->addHittable(
    //     std::make_shared<Object>(
    //         std::make_shared<Sphere>(Spectrum(-1.2f, 3.0f, -0.5f), 0.7f, true),
    //         std::make_shared<DielectricBSDF>(Spectrum(1.0f), 0.0f, 1.5f)
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

BSDFPtr makeLayeredBSDF() {
//#define baseColor Spectrum(.9f, .7f, .4f)
#define baseColor Spectrum(.9f, .5f, .9f)
//#define baseColor Spectrum(1.f)
    auto normalMap = TextureLoader::fromU8x3("res/texture/alien-metal_normal.png", false);

    auto diffuse = new LambertBSDF(ColorMap(baseColor));
    auto met = new MetalBSDF(ColorMap(baseColor), .2f, 0.3f, .5f);
    auto met2 = new MetalBSDF(ColorMap(baseColor), 0.f, 0.3f, .5f);
    auto met3 = new MetalBSDF(ColorMap(baseColor), .4f, 0.3f, .5f);
    auto met4 = new MetallicWorkflowBSDF(ColorMap(baseColor), 1.f, .2f);
    auto dielec = new DielectricBSDF(Spectrum(1.f), 0.1f, 1.5f);
    auto dielec2 = new DielectricBSDF(Spectrum(1.f), 0.f, 1.5f);
    auto dielec3 = new DielectricBSDF(Spectrum(1.f), 0.f, 1.f / 1.5f);
    auto tdielec = new ThinDielectricBSDF(Spectrum(1.f), 1.5f);
    auto tdielec2 = new ThinDielectricBSDF(Spectrum(1.f), 4.f);
    auto fake = new FakeBSDF;

    auto bdiffuse = new LambertBSDF(ColorMap(Spectrum(0.1f)));
    auto bmet = new MetallicWorkflowBSDF(ColorMap(Spectrum(.3f, .1f, .05f)), 1.f, 0.5);
    auto bmet2 = new MetalBSDF(ColorMap(Spectrum(.3f, .1f, .05f)), .5f, .3f, 2.f);
    auto bmet3 = new MetalBSDF(ColorMap(Spectrum(.3f, .1f, .05f)), .1f, .3f, 2.f);

    //ColorMap albedo(baseColor * .75f);
    //ColorMap albedo(Spectrum(1.f));
    //ColorMap albedo(Spectrum(0.f));
    ColorMap albedo(Spectrum(.8f, .3f, .1f));
    //return std::make_shared<MetallicWorkflowBSDF>(ColorMap(baseColor), 1.f, 0.1f);
    //return std::make_shared<MetalBSDF>(ColorMap(baseColor), 0.1f, .2f, .2f);
    auto layeredBSDF = std::make_shared<LayeredBSDF>(0.5f, 0.4f, albedo, 1, dielec2, diffuse);
    //layeredBSDF->topNormal = normalMap;
    //layeredBSDF->bottomNormal = normalMap;
    layeredBSDF->maxDepth = 32;
    return layeredBSDF;

    auto layeredBSDF2 = std::make_shared<LayeredBSDF2>();
    //layeredBSDF->addBSDF(fake);
    layeredBSDF2->addBSDF(dielec, nullptr);
    layeredBSDF2->addBSDF(bmet, normalMap);
    //layeredBSDF->addBSDF(diffuse, nullptr);
    //layeredBSDF->addBSDF(bmet, nullptr);
    return layeredBSDF2;
#undef baseColor
}

ScenePtr materialTest() {
    auto scene = std::make_shared<Scene>();

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Sphere>(Vec3f(0.f, 4.0f, 0.f), 1.5f, true),
            //std::make_shared<Quad>(Vec3f(1.f, 3.f, -1.f), Vec3f(-1.f, 3.f, -1.f), Vec3f(1.f, 3.f, 1.f)),
            makeLayeredBSDF()
            ));

    scene->mCamera = std::make_shared<ThinLensCamera>(20.0f, .1f, 11.f);
    //scene->mCamera = std::make_shared<ThinLensCamera>(20.0f, .0f, 11.f);
    scene->mCamera->setPos({ 0.0f, -8.0f, 0.0f });
    scene->mCamera->lookAt(Vec3f(0.0f, 10.0f, 0.0f));

    //scene->mEnv = std::make_shared<EnvSingleColor>(Vec3f(0.0f));
    scene->mEnv = std::make_shared<EnvSphereMapHDR>("res/texture/090.hdr");
    scene->mLightAndEnvStrategy = LightSampleStrategy::ByPower;
    scene->mLightSampleStrategy = LightSampleStrategy::ByPower;

    return scene;
}

ScenePtr cornellBox() {
    auto scene = std::make_shared<Scene>();
    
    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(-3.0f, 0.0f, -3.0f),
                Vec3f(3.0f, 0.0f, -3.0f),
                Vec3f(-3.0f, 6.0f, -3.0f)),
            //std::make_shared<MetallicWorkflowBSDF>(ColorMap(Spectrum(1.0f), 0.0f, 1.0f)
            std::make_shared<LambertBSDF>(ColorMap(Spectrum(1.0f)))
            //std::make_shared<DielectricBSDF>(Spectrum(1.0f), 0.0f, 0.1f)
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(-3.0f, 0.0f, -3.0f),
                Vec3f(-3.0f, 6.0f, -3.0f),
                Vec3f(-3.0f, 0.0f, 3.0f)),
            //std::make_shared<MetallicWorkflowBSDF>(ColorMap(Spectrum(1.0f, 0.25f, 0.25f), 0.0f, 1.0f)
            std::make_shared<LambertBSDF>(ColorMap(Spectrum(1.0f, 0.25f, 0.25f)))
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(3.0f, 6.0f, -3.0f),
                Vec3f(3.0f, 0.0f, -3.0f),
                Vec3f(3.0f, 6.0f, 3.0f)),
            //std::make_shared<MetallicWorkflowBSDF>(ColorMap(Spectrum(0.25f, 0.25f, 1.0f), 0.0f, 1.0f)
            std::make_shared<LambertBSDF>(ColorMap(Spectrum(0.25f, 0.25f, 1.0f)))
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(3.0f, 6.0f, 3.0f),
                Vec3f(3.0f, 0.0f, 3.0f),
                Vec3f(-3.0f, 6.0f, 3.0f)),
            //std::make_shared<MetallicWorkflowBSDF>(ColorMap(Spectrum(1.0f), 0.0f, 1.0f)
            std::make_shared<LambertBSDF>(ColorMap(Spectrum(1.0f)))
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(-3.0f, 6.0f, -3.0f),
                Vec3f(3.0f, 6.0f, -3.0f),
                Vec3f(-3.0f, 6.0f, 3.0f)),
            std::make_shared<LambertBSDF>(TextureLoader::fromU8x3("res/checker.png", true))
            ));

    scene->addHittable(
         std::make_shared<Object>(
             std::make_shared<Sphere>(Vec3f(0.f, 4.0f, -0.5f), 1.5f, true),
             //std::make_shared<DielectricBSDF>(Spectrum(1.0f), 0.0f, 1.5f)
             //std::make_shared<MetallicWorkflowBSDF>(Spectrum(1.f, .8f, .5f), 1.f, .2f)
             makeLayeredBSDF()
         ));

    /*
    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(-1.0f, 4.0f, -1.0f),
                Vec3f(1.0f, 4.0f, -1.0f),
                Vec3f(-1.0f, 5.0f, 1.0f)),
            //std::make_shared<DielectricBSDF>(Spectrum(1.0f), 0.0f, 1.5f)
            //std::make_shared<MetallicWorkflowBSDF>(Spectrum(1.f, .8f, .5f), 1.f, .2f)
            //layeredBSDF
            std::make_shared<FakeBSDF>()
            ));
            */

    /*
    {
        auto model = glm::translate(Mat4f(1.0f), Vec3f(-1.0f, 4.0f, -1.2f));
        model = glm::rotate(model, glm::radians(17.5f), Vec3f(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, Vec3f(1.8f, 1.8f, 3.6f));
        Transform trBoxLarge(model);

        scene->addObjectMesh("res/model/cube.obj", trBoxLarge,
            //std::make_shared<LambertBSDF>(ColorMap(Spectrum(1.0f)))
            std::make_shared<MirrorBSDF>(ColorMap(Spectrum(1.0f)))
            //std::make_shared<DielectricBSDF>(Vec3f(1.f), 0.f, 1.5f)
            //std::make_shared<ThinDielectricBSDF>(Vec3f(1.f), 1.5f)
        );
    }

    {
        auto model = glm::translate(Mat4f(1.0f), Vec3f(1.0f, 2.0f, -2.1f));
        model = glm::rotate(model, glm::radians(-17.5f), Vec3f(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, Vec3f(1.8f));

        scene->addObjectMesh("res/model/cube.obj", { model },
            std::make_shared<LambertBSDF>(ColorMap(Spectrum(1.0f)))
        );
    }
    */

    scene->addLight(
        std::make_shared<Light>(
            std::make_shared<Quad>(
                Vec3f(-0.75f, 3.75f, 2.999f),
                Vec3f(0.75f, 3.75f, 2.999f),
                Vec3f(-0.75f, 2.25f, 2.999f)),
            /*
                Vec3f(-0.025f, 3.025f, 2.999f),
                Vec3f(0.025f, 3.025f, 2.999f),
                Vec3f(-0.025f, 2.975f, 2.999f)),
                */
            Spectrum(200.0f), false));

    scene->mCamera = std::make_shared<ThinLensCamera>(40.0f);
    scene->mCamera->setPos({ 0.0f, -8.0f, 0.0f });
    scene->mCamera->lookAt(Vec3f(0.0f, 10.0f, 0.0f));

    scene->mEnv = std::make_shared<EnvSingleColor>(Vec3f(0.0f));
    scene->mLightAndEnvStrategy = LightSampleStrategy::ByPower;
    scene->mLightSampleStrategy = LightSampleStrategy::ByPower;

    return scene;
}

ScenePtr originalBox() {
    auto scene = std::make_shared<Scene>();
    float roughness = 0.05f;

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(-3.0f, 0.0f, -3.0f),
                Vec3f(3.0f, 0.0f, -3.0f),
                Vec3f(-3.0f, 6.0f, -3.0f)),
            std::make_shared<MetallicWorkflowBSDF>(ColorMap(Spectrum(1.0f)), 0.0f, 1.0f)
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(-3.0f, 0.0f, -3.0f),
                Vec3f(-3.0f, 6.0f, -3.0f),
                Vec3f(-3.0f, 0.0f, 3.0f)),
            std::make_shared<MetallicWorkflowBSDF>(ColorMap(Spectrum(1.0f, 0.0f, 0.0f)), 1.0f, roughness)
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(3.0f, 6.0f, -3.0f),
                Vec3f(3.0f, 0.0f, -3.0f),
                Vec3f(3.0f, 6.0f, 3.0f)),
            std::make_shared<MetallicWorkflowBSDF>(ColorMap(Spectrum(0.0f, 1.0f, 0.0f)), 1.0f, roughness)
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(3.0f, 6.0f, 3.0f),
                Vec3f(3.0f, 0.0f, 3.0f),
                Vec3f(-3.0f, 6.0f, 3.0f)),
            std::make_shared<MetallicWorkflowBSDF>(ColorMap(Spectrum(1.0f)), 0.0f, 1.0f)
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(-3.0f, 3.0f, -3.0f),
                Vec3f(3.0f, 3.0f, -3.0f),
                Vec3f(-3.0f, 3.0f, 3.0f)),
            std::make_shared<MetallicWorkflowBSDF>(ColorMap(Spectrum(1.0f)), 1.0f, roughness)
            ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Sphere>(Vec3f(1.0f, 1.5f, -2.0f), 1.2f, false),
            std::make_shared<MetallicWorkflowBSDF>(ColorMap(Spectrum(1.0f)), 1.0f, 0.015f)
        ));

    scene->addHittable(
        std::make_shared<Object>(
            std::make_shared<Sphere>(Vec3f(-1.2f, 1.8f, -1.0f), 1.0f, false),
            std::make_shared<MetallicWorkflowBSDF>(ColorMap(Spectrum(0.2f, 0.4f, 1.0f)), 0.0f, 0.12f)
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

    //scene->mEnv = std::make_shared<EnvSphereMapHDR>("res/texture/076.hdr");
    scene->mEnv = std::make_shared<EnvSingleColor>(Vec3f(0.0f));
    scene->mLightAndEnvStrategy = LightSampleStrategy::Uniform;
    scene->mLightSampleStrategy = LightSampleStrategy::ByPower;

    return scene;
}

ScenePtr fireplace(bool thin, bool lamp) {
    #define DIR "res/model/fireplace/"
    #define TEX DIR "textures/"
    auto scene = std::make_shared<Scene>();
    Transform transform(glm::rotate(Mat4f(1.0f), glm::radians(90.0f), Vec3f(1.0f, 0.0f, 0.0f)));

    scene->addObjectMesh(DIR "bottle_cap.obj", transform, std::make_shared<LambertBSDF>(ColorMap(Vec3f(0.46f, 0.0f, 0.0f))));
    scene->addObjectMesh(DIR "floor.obj", transform, std::make_shared<MetallicWorkflowBSDF>(TextureLoader::fromU8x3(TEX "wood.png", true), 0.0f, 0.3f));
    //scene->addObjectMesh(DIR "floor.obj", transform, std::make_shared<LambertBSDF>(TextureLoader::fromU8x3(TEX "wood.png", true)));
    scene->addObjectMesh(DIR "frame.obj", transform, std::make_shared<LambertBSDF>(ColorMap(Vec3f(0.259f, 0.251f, 0.141f))));
    scene->addObjectMesh(DIR "glass.obj", transform, std::make_shared<DielectricBSDF>(Spectrum(1.0f), 0.0f, 1.5f));
    scene->addObjectMesh(DIR "leaves.obj", transform, std::make_shared<LambertBSDF>(ColorMap(Vec3f(1.0f))));
    scene->addObjectMesh(DIR "metal.obj", transform, std::make_shared<MetallicWorkflowBSDF>(ColorMap(Spectrum(1.0f, 0.7f, 0.4f)), 1.0f, 0.25f));
    scene->addObjectMesh(DIR "mirror.obj", transform, std::make_shared<MetallicWorkflowBSDF>(ColorMap(Spectrum(1.0f)), 1.0f, 0.014f));
    scene->addObjectMesh(DIR "photo.obj", transform, std::make_shared<LambertBSDF>(TextureLoader::fromU8x3(TEX "picture8.png", true)));
    scene->addObjectMesh(DIR "leaves.obj", transform, std::make_shared<LambertBSDF>(TextureLoader::fromU8x3(TEX "leaf.png", true)));
    scene->addObjectMesh(DIR "plant_base.obj", transform, std::make_shared<LambertBSDF>(ColorMap(Vec3f(0.6f))));
    scene->addObjectMesh(DIR "sofa.obj", transform, std::make_shared<LambertBSDF>(ColorMap(Vec3f(0.9f, 0.9f, 0.87f))));
    scene->addObjectMesh(DIR "sofa_legs.obj", transform, std::make_shared<LambertBSDF>(ColorMap(Vec3f(0.1f))));
    scene->addObjectMesh(DIR "soil.obj", transform, std::make_shared<LambertBSDF>(ColorMap(Vec3f(0.1f))));
    scene->addObjectMesh(DIR "leaves.obj", transform, std::make_shared<LambertBSDF>(ColorMap(Vec3f(1.0f))));
    scene->addObjectMesh(DIR "stem.obj", transform, std::make_shared<LambertBSDF>(ColorMap(Vec3f(0.16, 0.078f, 0.02f))));
    scene->addObjectMesh(DIR "stove_base.obj", transform, std::make_shared<LambertBSDF>(ColorMap(Vec3f(0.05f))));
    scene->addObjectMesh(DIR "stove_ceiling.obj", transform, std::make_shared<LambertBSDF>(ColorMap(Vec3f(1.0f))));
    scene->addObjectMesh(DIR "unknown.obj", transform, std::make_shared<LambertBSDF>(ColorMap(Vec3f(1.0f))));
    scene->addObjectMesh(DIR "wall.obj", transform, std::make_shared<LambertBSDF>(ColorMap(Vec3f(0.169f, 0.133f, 0.102f))));
    scene->addObjectMesh(DIR "window_base.obj", transform, std::make_shared<LambertBSDF>(ColorMap(Vec3f(0.9f))));
    scene->addObjectMesh(DIR "window_frame.obj", transform, std::make_shared<LambertBSDF>(ColorMap(Vec3f(1.0f))));
    scene->addObjectMesh(DIR "wood.obj", transform, std::make_shared<MetallicWorkflowBSDF>(TextureLoader::fromU8x3(TEX "wood5.png", true), 0.0f, 0.1f));
    //scene->addObjectMesh(DIR "wood.obj", transform, std::make_shared<LambertBSDF>(TextureLoader::fromU8x3(TEX "wood5.png", true)));
    scene->addLightMesh(DIR "light.obj", transform, Spectrum(lamp ? 100.0f : 400.0f));
    if (lamp)
        scene->addLightMesh(DIR "light2.obj", transform, Spectrum(400.0f));
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
        // scene->mCamera = std::make_shared<PanoramaCamera>();
        // scene->mCamera->setPos({ 1.26f, 1.0f, 1.4f });
    }

    scene->mEnv = std::make_shared<EnvSingleColor>(Vec3f(0.0f));
    scene->mLightAndEnvStrategy = LightSampleStrategy::ByPower;
    scene->mLightSampleStrategy = LightSampleStrategy::ByPower;

    return scene;
}

ScenePtr staircase2()
{
    #define DIR "res/model/staircase2/"
    #define MOD DIR "models/"
    #define TEX DIR "textures/"
    auto scene = std::make_shared<Scene>();
    Transform transform(glm::rotate(Mat4f(1.0f), glm::radians(90.0f), Vec3f(1.0f, 0.0f, 0.0f)));

    scene->addObjectMesh(MOD "Mesh000.obj", transform, std::make_shared<LambertBSDF>(ColorMap(Vec3f(1.0f))));
    scene->addObjectMesh(MOD "Mesh001.obj", transform, std::make_shared<MetallicWorkflowBSDF>(TextureLoader::fromU8x3(TEX "wood5.jpg", true), 0.0f, 0.3f));
    scene->addObjectMesh(MOD "Mesh004.obj", transform, std::make_shared<LambertBSDF>(ColorMap(Vec3f(1.0f))));
    scene->addObjectMesh(MOD "Mesh005.obj", transform, std::make_shared<LambertBSDF>(ColorMap(Vec3f(1.0f))));
    scene->addObjectMesh(MOD "Mesh007.obj", transform, std::make_shared<LambertBSDF>(ColorMap(Vec3f(1.0f))));
    scene->addObjectMesh(MOD "Mesh009.obj", transform, std::make_shared<LambertBSDF>(ColorMap(Vec3f(1.0f))));
    scene->addObjectMesh(MOD "Mesh011.obj", transform, std::make_shared<MetallicWorkflowBSDF>(TextureLoader::fromU8x3(TEX "Tiles.jpg", true), 0.0f, 0.1f));
    scene->addObjectMesh(MOD "Mesh012.obj", transform, std::make_shared<LambertBSDF>(ColorMap(Vec3f(1.0f))));
    scene->addObjectMesh(MOD "Mesh014.obj", transform, std::make_shared<DielectricBSDF>(Spectrum(1.0f), 0.0f, 1.4f));
    scene->addObjectMesh(MOD "Mesh016.obj", transform, std::make_shared<LambertBSDF>(TextureLoader::fromU8x3(TEX "wood5.jpg", true)));
    scene->addObjectMesh(MOD "Mesh017.obj", transform, std::make_shared<LambertBSDF>(TextureLoader::fromU8x3(TEX "wood5.jpg", true)));
    scene->addObjectMesh(MOD "Mesh018.obj", transform, std::make_shared<LambertBSDF>(TextureLoader::fromU8x3(TEX "Wallpaper.jpg", true)));
    scene->addLightMesh(MOD "light.obj", transform, Spectrum(16000.0f, 14000.0f, 10000.0f));
    #undef DIR
    #undef TEX
    #undef MOD

    scene->mCamera = std::make_shared<ThinLensCamera>(66.34f);
    scene->mCamera->setPos({ 6.988f, -1.815f, 2.1f });
    scene->mCamera->setAngle({ -202.81f, 0.f, 0.f });

    scene->mEnv = std::make_shared<EnvSingleColor>(Vec3f(0.0f));
    scene->mLightAndEnvStrategy = LightSampleStrategy::ByPower;
    scene->mLightSampleStrategy = LightSampleStrategy::ByPower;

    return scene;
}

ScenePtr setupScene(int windowWidth, int windowHeight)
{
    //auto scene = fireplace(false, true);
    //auto scene = boxScene();
    //auto scene = staircase2();
    //auto scene = originalBox();
    //auto scene = bidirScene();
    //auto scene = cornellBox();
    auto scene = materialTest();
    scene->mCamera->initFilm(windowWidth, windowHeight);
    return scene;
}