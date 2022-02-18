#include "../include/Application.h"
#include "../include/Core/ToneMapping.h"

void Application::init(const std::string &name, HINSTANCE instance, const char *cmdParam)
{
    std::string integType;
    std::string samplerType;
    int width, height;
    int spp;
    std::stringstream param(cmdParam);

    param >> integType >> samplerType >> width >> height >> spp;

    this->mInstance = instance;
    this->mWindowWidth = width;
    this->mWindowHeight = height;

    mLastCursorX = width / 2;
    mLastCursorY = height / 2;

    mWindow = CreateWindowA(
        name.c_str(), name.c_str(),
        WS_OVERLAPPEDWINDOW & ~WS_MINIMIZEBOX & ~WS_SIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        nullptr,
        nullptr,
        0,
        nullptr);

    SetWindowPos(mWindow, HWND_TOP, 0, 0, width, height, SWP_NOMOVE);
    ShowWindow(mWindow, 1);
    UpdateWindow(mWindow);
    SetWindowPos(mWindow, HWND_TOP, 0, 0, width + 5, height + 25, SWP_NOMOVE);

    mColorBuffer.init(width, height);
    initScene();

    bool scramble;
    if (integType == "-path2")
    {
        int pathsOnePass;
        param >> pathsOnePass;
        auto integ = std::make_shared<PathIntegrator2>(mScene, spp, pathsOnePass);
        integ->mParam.maxDepth = 5;
        integ->mParam.MIS = true;
        mIntegrator = integ;
        scramble = false;
    }
    else if (integType == "-path")
    {
        auto integ = std::make_shared<PathIntegrator>(mScene, spp);
        integ->mLimitSpp = (spp != 0);
        integ->mParam.maxDepth = 5;
        integ->mParam.MIS = true;
        mIntegrator = integ;
        scramble = true;
    }
    else if (integType == "-lpath")
    {
        auto integ = std::make_shared<LightPathIntegrator>(mScene, spp);
        integ->mParam.maxDepth = 5;
        mIntegrator = integ;
        scramble = false;
    }
    else if (integType == "-bdpt")
    {
        auto integ = std::make_shared<BDPTIntegrator>(mScene, spp);
        integ->mParam.resampleDirect = true;
        integ->mLightSampler = std::make_shared<IndependentSampler>();
    }
    else if (integType == "-ao")
    {
        auto integ = std::make_shared<AOIntegrator>(mScene, spp);
        param >> integ->mParam.radius;
        mIntegrator = integ;
    }
    else if (integType == "-ao2")
    {
        int pathsOnePass;
        param >> pathsOnePass;
        auto integ = std::make_shared<AOIntegrator2>(mScene, spp, pathsOnePass);
        param >> integ->mParam.radius;
        mIntegrator = integ;
    }
    
    if (samplerType == "-rng")
        mIntegrator->mSampler = std::make_shared<IndependentSampler>();
    else
        mIntegrator->mSampler = std::make_shared<SimpleSobolSampler>(mWindowWidth, mWindowHeight, scramble);
}

LRESULT Application::process(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        auto metricX = GetSystemMetrics(SM_CXSCREEN);
        auto metricY = GetSystemMetrics(SM_CYSCREEN);

        HDC hdc = GetDC(hWnd);
        mBitmap = CreateCompatibleBitmap(hdc, metricX, metricY);

        mMemdc = CreateCompatibleDC(hdc);
        SelectObject(mMemdc, mBitmap);

        BitBlt(mMemdc, 0, 0, metricX, metricY, mMemdc, 0, 0, WHITENESS);

        DeleteDC(mMemdc);
        ReleaseDC(hWnd, hdc);

        break;
    }

    case WM_PAINT:
    {
        HDC hdc;
        PAINTSTRUCT ps;
        RECT rect;

        hdc = BeginPaint(hWnd, &ps);
        mMemdc = CreateCompatibleDC(hdc);
        SelectObject(mMemdc, mBitmap);
        GetClientRect(hWnd, &rect);

        BitBlt(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top, mMemdc, 0, 0, SRCCOPY);

        DeleteDC(mMemdc);
        mMemdc = 0;
        EndPaint(hWnd, &ps);

        break;
    }

    case WM_KEYDOWN:
    {
        mKeyPressing[(int)wParam] = true;
        if ((int)wParam == VK_F1)
        {
            mCursorDisabled ^= 1;
            mFirstCursorMove = true;
        }
        else if ((int)wParam == 'T')
            mToneMappingMethod = (mToneMappingMethod + 1) % 4;
        else if ((int)wParam == 'O')
            saveImage();
        break;
    }

    case WM_KEYUP:
        mKeyPressing[(int)wParam] = false;
        break;

    case WM_MOUSEMOVE:
    {
        if (mCursorDisabled)
            break;
        mIntegrator->reset();

        if (mFirstCursorMove)
        {
            mLastCursorX = (int)LOWORD(lParam);
            mLastCursorY = (int)HIWORD(lParam);
            mFirstCursorMove = false;
        }

        float offsetX = ((int)LOWORD(lParam) - mLastCursorX) * CameraRotateSensitivity;
        float offsetY = ((int)HIWORD(lParam) - mLastCursorY) * CameraRotateSensitivity;

        Vec3f offset(-offsetX, -offsetY, 0.0f);
        mScene->mCamera->rotate(offset);

        mLastCursorX = (int)LOWORD(lParam);
        mLastCursorY = (int)HIWORD(lParam);
        break;
    }

        /*
		case WM_SIZE:
			windowWidth = LOWORD(lParam);
			windowHeight = HIWORD(lParam);
			colorBuffer.resize(windowWidth, windowHeight);
			if (integrator != nullptr) integrator->resizeBuffer(windowWidth, windowHeight);
			break;
		*/

    case WM_DESTROY:
        DeleteObject(mBitmap);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void Application::render()
{
    processKey();

    if (!mIntegrator->isFinished())
    {
        mIntegrator->renderOnePass();
        writeBuffer();
    }
    flushScreen();

    if (mIntegrator->isFinished())
        return;
    mColorBuffer.swap();
}

void Application::initScene()
{
    srand(time(nullptr));

    auto scene = std::make_shared<Scene>();

    // const float level = -1.0f;
    // scene->addHittable(
    //     std::make_shared<Object>(
    //     std::make_shared<Quad>(
    //         Vec3f(-200.0f, -10.0f, level),
    //         Vec3f(200.0f, -10.0f, level),
    //         Vec3f(-200.0f, 100.0f, level)),
    //     std::make_shared<MetalWorkflow>(Spectrum(1.0f), 0.0f, 1.0f)));

    // scene->addHittable(
    //     std::make_shared<Object>(
    //         std::make_shared<Sphere>(Vec3f(0.0f), 1.0f, true),
    //         std::make_shared<Dielectric>(Spectrum(1.0f), 0.2f, 1.5f)
    //         //std::make_shared<Clearcoat>(0.01f, 1.0f)
    //         //std::make_shared<MetalWorkflow>(Spectrum(1.0f, 0.5f, 0.2f), 0.0f, 1.0f)
    //         //std::make_shared<Lambertian>(Spectrum(1.0f, 0.5f, 0.2f))
    //         //std::make_shared<DisneyDiffuse>(Spectrum(1.0f, 0.5f, 0.2f), 1.0f, 1.0f)
    //     ));

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
            //std::make_shared<MetalWorkflow>(Spectrum(1.0f), 0.0f, 1.0f)
            std::make_shared<Lambertian>(Spectrum(1.0f))
            ));

    // scene->addHittable(
    //     std::make_shared<Object>(
    //         std::make_shared<Sphere>(Spectrum(0.0f, 3.0f, 0.0f), 1.0f, true),
    //         std::make_shared<Dielectric>(Spectrum(1.0f), 0.0f, 1.5f)
    //     ));

    // scene->addHittable(
    //     std::make_shared<Object>(
    //         std::make_shared<Quad>(
    //             Vec3f(1.0f, 2.0f, 2.1f),
    //             Vec3f(1.0f, 4.0f, 2.1f),
    //             Vec3f(-1.0f, 2.0f, 2.8f)),
    //         std::make_shared<MetalWorkflow>(Spectrum(1.0f), 1.0f, 0.014f)
    //         ));

    glm::mat4 model(1.0f);
    model = glm::translate(model, Vec3f(1.0f, 2.0f, -2.1f));
    model = glm::rotate(model, glm::radians(-17.5f), Vec3f(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, Vec3f(1.8f));
    std::shared_ptr<Transform> trBoxSmall = std::make_shared<Transform>(model);

    scene->addObjectMesh("res/model/cube.obj", trBoxSmall,
                         //std::make_shared<MetalWorkflow>(Spectrum(1.0f, 0.8f, 0.6f), 1.0f, 0.6f)
                         std::make_shared<Lambertian>(Spectrum(1.0f))
                         //std::make_shared<Dielectric>(Spectrum(1.0f), 0.0f, 1.5f)
    );

    model = glm::mat4(1.0f);
    model = glm::translate(model, Vec3f(-1.0f, 4.0f, -1.2f));
    model = glm::rotate(model, glm::radians(17.5f), Vec3f(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, Vec3f(1.8f, 1.8f, 3.6f));
    std::shared_ptr<Transform> trBoxLarge = std::make_shared<Transform>(model);

    scene->addObjectMesh("res/model/cube.obj", trBoxLarge,
                         //std::make_shared<MetalWorkflow>(Spectrum(1.0f), 0.0f, 1.0f)
                         std::make_shared<Lambertian>(Spectrum(1.0f))
    );

    // glm::mat4 model(1.0f);
    // model = glm::translate(model, Vec3f(0.0f, 3.0f, -1.0f));
    // model = glm::rotate(model, glm::radians(90.0f), Vec3f(1.0f, 0.0f, 0.0f));
    // model = glm::scale(model, Vec3f(1.0f));
    // std::shared_ptr<Transform> transform = std::make_shared<Transform>(model);
    // scene->addObjectMesh("res/model/bunny.obj", transform,
    //                      std::make_shared<Dielectric>(Spectrum(1.0f), 0.0f, 1.5f));

    scene->addLight(
        std::make_shared<Light>(
            std::make_shared<Quad>(
                Vec3f(-0.75f, 3.75f, 2.999f),
                Vec3f(0.75f, 3.75f, 2.999f),
                Vec3f(-0.75f, 2.25f, 2.999f)),
                // Vec3f(-0.025f, 3.025f, 2.999f),
                // Vec3f(0.025f, 3.025f, 2.999f),
                // Vec3f(-0.025f, 2.975f, 2.999f)),
            Spectrum(100.0f), false));

    // auto transform = std::make_shared<Transform>(glm::rotate(Mat4f(1.0f), glm::radians(90.0f), Vec3f(1.0f, 0.0f, 0.0f)));
    // scene->addObjectMesh("res/model/bidir/diffuse.obj", transform,
    //     std::make_shared<Lambertian>(Spectrum(0.5f)));
    // scene->addObjectMesh("res/model/bidir/glass.obj", transform,
    //     std::make_shared<Dielectric>(Spectrum(1.0f), 0.0f, 1.5f));
    // scene->addObjectMesh("res/model/bidir/lamp.obj", transform,
    //     std::make_shared<MetalWorkflow>(Spectrum(0.8f, 0.356f, 0.135f), 1.0f, 0.25f));
    // scene->addObjectMesh("res/model/bidir/wood.obj", transform,
    //     std::make_shared<Lambertian>(Spectrum(0.33f, 0.26f, 0.15f)));
    // scene->addLightMesh("res/model/bidir/light1.obj", transform, Spectrum(100.0f));
    // scene->addLightMesh("res/model/bidir/light2.obj", transform, Spectrum(200.0f));

    auto camera = std::make_shared<ThinLensCamera>(40.0f);
    camera->initFilm(mWindowWidth, mWindowHeight);
    // camera->setPos({0.0f, -12.0f, 4.0f});
    // camera->lookAt(Vec3f(0.0f, 0.0f, 4.0f));
    // camera->setPos({0.0f, -12.0f, 6.0f});
    // camera->lookAt(Vec3f(0.0f, 0.0f, 2.5f));
    camera->setPos({0.0f, -8.0f, 0.0f});
    camera->lookAt(Vec3f(0.0f));

    //scene->mEnv = std::make_shared<EnvSphereMapHDR>("res/texture/090.hdr");
    scene->mEnv = std::make_shared<EnvSingleColor>(Vec3f(0.0f));
    scene->mLightAndEnvStrategy = LightSampleStrategy::ByPower;
    scene->mLightAndEnvStrategy = LightSampleStrategy::ByPower;
    scene->mCamera = camera;
    scene->buildScene();
    mScene = scene;
}

void Application::writeBuffer()
{
    auto resultBuffer = mIntegrator->result();
    using namespace ToneMapping;

    Vec3f (*toneMapping[4])(const Vec3f &) = {reinhard, CE, filmic, ACES};

    for (int i = 0; i < mWindowWidth; i++)
    {
        for (int j = 0; j < mWindowHeight; j++)
        {
            auto result = resultBuffer(i, j) * mIntegrator->mResultScale;
            result = glm::clamp(result, Vec3f(0.0f), Vec3f(1e8f));
            result = toneMapping[mToneMappingMethod](result);
            result = glm::pow(result, Vec3f(1.0f / 2.2f));
            mColorBuffer(i, j) = RGB24::swapRB(RGB24(result));
        }
    }
}

void Application::flushScreen()
{
    BITMAPINFO bInfo;
    ZeroMemory(&bInfo, sizeof(BITMAPINFO));

    auto &header = bInfo.bmiHeader;
    header.biBitCount = 24;
    header.biCompression = BI_RGB;
    header.biWidth = mWindowWidth;
    header.biHeight = -mWindowHeight;
    header.biPlanes = 1;
    header.biSizeImage = 0;
    header.biSize = sizeof(BITMAPINFOHEADER);

    HDC dc = GetDC(mWindow);
    HDC compatibleDC = CreateCompatibleDC(dc);
    HBITMAP compatibleBitmap = CreateCompatibleBitmap(dc, mWindowWidth, mWindowHeight);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(compatibleDC, compatibleBitmap);

    SetDIBits(
        dc,
        compatibleBitmap,
        0,
        mWindowHeight,
        (BYTE *)mColorBuffer.getCurrentBuffer().bufPtr(),
        &bInfo,
        DIB_RGB_COLORS);
    BitBlt(dc, -1, -1, mWindowWidth, mWindowHeight, compatibleDC, 0, 0, SRCCOPY);

    DeleteDC(compatibleDC);
    DeleteObject(oldBitmap);
    DeleteObject(compatibleBitmap);
    UpdateWindow(mWindow);
}

void Application::processKey()
{
    uint8_t keyList[] = {'W', 'S', 'A', 'D', 'Q', 'E', 'R', VK_SHIFT, VK_SPACE};

    for (int i = 0; i < 9; i++)
    {
        if (mKeyPressing[keyList[i]])
        {
            mScene->mCamera->move(keyList[i]);
            mIntegrator->reset();
        }
    }
}

void Application::saveImage()
{
    int w = mColorBuffer.width(), h = mColorBuffer.height();
    int size = w * h;
    RGB24 *data = new RGB24[size];
    for (int i = 0; i < size; i++)
        data[i] = RGB24::swapRB(mColorBuffer[i]);
    std::string file = "screenshot/saves/save" + std::to_string((int)time(0)) + ".png";
    stbi_write_png(file.c_str(), w, h, 3, data, w * 3);
    delete[] data;
    std::cout << "[Image captured]\n";
}