#include "../include/Application.h"

void Application::init(const std::string &name, HINSTANCE instance, int width, int height, int spp)
{
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
    initScene(spp);
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
        mScene->camera->rotate(offset);

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

void Application::initScene(int spp)
{
    srand(time(nullptr));

    auto sc = std::make_shared<Scene>();

    // auto sphere = std::make_shared<Object>(
    // 	std::make_shared<Sphere>(Vec3f(0.0f, 0.0f, 1.0f), 1.0f, true),
    // 	//std::make_shared<MetalWorkflow>(Vec3f(1.0f), 1.0f, 0.2f)
    // 	//std::make_shared<Clearcoat>(0.01f, 1.0f)
    // 	std::make_shared<Dielectric>(Vec3f(1.0f), 0.0f, 1.4f)
    // 	// std::make_shared<MixedMaterial>(
    // 	// 	std::make_shared<MetalWorkflow>(Vec3f(1.0, 0.2f, 0.2f), 0.0f, 0.1f),
    // 	// 	std::make_shared<Clearcoat>(0.01f, 1.0f),
    // 	// 	std::make_shared<Dielectric>(Vec3f(1.0f), 0.1f, 1.4f),
    // 	// 	0.9)
    // );
    // //sphere->setTransform(std::make_shared<Transform>(glm::scale(glm::mat4(1.0f), Vec3f(1.0f, 1.0f, 2.0f))));
    // scene->addHittable(sphere);

    // const float level = -2.0f;
    // sc->addHittable(
    //     std::make_shared<Object>(
    //     std::make_shared<Quad>(
    //         Vec3f(-200.0f, -10.0f, level),
    //         Vec3f(200.0f, -10.0f, level),
    //         Vec3f(-200.0f, 100.0f, level)),
    //     std::make_shared<MetalWorkflow>(Vec3f(1.0f), 0.0f, 1.0f)));

    // sc->addHittable(
    //     std::make_shared<Object>(
    //         std::make_shared<Quad>(
    //             Vec3f(-30.0f, -30.0f, -0.5f),
    //             Vec3f(30.0f, -30.0f, -0.5f),
    //             Vec3f(-30.0f, 30.0f, -0.5f)),
    //         std::make_shared<MetalWorkflow>(Vec3f(1.0f), 1.0f, 0.2f)
    //         //std::make_shared<Lambertian>(Vec3f(1.0f))
    //         ));

    sc->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(-3.0f, 0.0f, -3.0f),
                Vec3f(3.0f, 0.0f, -3.0f),
                Vec3f(-3.0f, 6.0f, -3.0f)),
            std::make_shared<MetalWorkflow>(Vec3f(1.0f), 0.0f, 1.0f)
            //std::make_shared<Lambertian>(Vec3f(1.0f))
            ));

    sc->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(-3.0f, 0.0f, -3.0f),
                Vec3f(-3.0f, 6.0f, -3.0f),
                Vec3f(-3.0f, 0.0f, 3.0f)),
            std::make_shared<MetalWorkflow>(Vec3f(1.0f, 0.25f, 0.25f), 0.0f, 1.0f)
            //std::make_shared<Lambertian>(Vec3f(1.0f, 0.25f, 0.25f))
            ));

    sc->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(3.0f, 6.0f, -3.0f),
                Vec3f(3.0f, 0.0f, -3.0f),
                Vec3f(3.0f, 6.0f, 3.0f)),
            std::make_shared<MetalWorkflow>(Vec3f(0.25f, 0.25f, 1.0f), 0.0f, 1.0f)
            //std::make_shared<Lambertian>(Vec3f(0.25f, 0.25f, 1.0f))
            ));

    sc->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(3.0f, 6.0f, 3.0f),
                Vec3f(3.0f, 0.0f, 3.0f),
                Vec3f(-3.0f, 6.0f, 3.0f)),
            std::make_shared<MetalWorkflow>(Vec3f(1.0f), 0.0f, 1.0f)
            //std::make_shared<Lambertian>(Vec3f(1.0f))
            ));

    sc->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                Vec3f(-3.0f, 6.0f, -3.0f),
                Vec3f(3.0f, 6.0f, -3.0f),
                Vec3f(-3.0f, 6.0f, 3.0f)),
            std::make_shared<MetalWorkflow>(Vec3f(1.0f), 0.0f, 1.0f)
            //std::make_shared<Lambertian>(Vec3f(1.0f))
            ));

    sc->addHittable(
        std::make_shared<Object>(
            std::make_shared<Sphere>(Vec3f(1.0f, 3.0f, 0.0f), 1.0f, true),
            std::make_shared<Dielectric>(Vec3f(1.0f), 0.0f, 1.5f)
            //std::make_shared<Clearcoat>(0.01f, 1.0f)
            //std::make_shared<MetalWorkflow>(Vec3f(1.0f, 0.5f, 0.2f), 0.0f, 1.0f)
            //std::make_shared<Lambertian>(Vec3f(1.0f, 0.5f, 0.2f))
            //std::make_shared<DisneyDiffuse>(Vec3f(1.0f, 0.5f, 0.2f), 1.0f, 1.0f)
        ));

    glm::mat4 model(1.0f);
    model = glm::translate(model, Vec3f(1.0f, 2.0f, -2.1f));
    model = glm::rotate(model, glm::radians(-17.5f), Vec3f(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, Vec3f(1.8f));
    std::shared_ptr<Transform> trBoxSmall = std::make_shared<Transform>(model);

    sc->addObjectMesh("res/model/cube.obj", trBoxSmall,
                         std::make_shared<MetalWorkflow>(Vec3f(1.0f, 0.8f, 0.6f), 0.0f, 1.0f)
                         //std::make_shared<Lambertian>(Vec3f(1.0f))
    );

    model = glm::mat4(1.0f);
    model = glm::translate(model, Vec3f(-1.0f, 4.0f, -1.2f));
    model = glm::rotate(model, glm::radians(17.5f), Vec3f(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, Vec3f(1.8f, 1.8f, 3.6f));
    std::shared_ptr<Transform> trBoxLarge = std::make_shared<Transform>(model);

    sc->addObjectMesh("res/model/cube.obj", trBoxLarge,
                         std::make_shared<MetalWorkflow>(Vec3f(1.0f), 0.0f, 1.0f)
                         //std::make_shared<Lambertian>(Vec3f(1.0f))
    );

    sc->addLight(
        std::make_shared<Light>(
            std::make_shared<Quad>(
                Vec3f(-0.75f, 3.75f, 2.999f),
                Vec3f(0.75f, 3.75f, 2.999f),
                Vec3f(-0.75f, 2.25f, 2.999f)),
            Vec3f(100.0f), false));

    auto camera = std::make_shared<ThinLensCamera>(40.0f);
    camera->initFilm(mWindowWidth, mWindowHeight);
    camera->setPos({0.0f, -8.0f, 0.0f});
    camera->lookAt(Vec3f(0.0f));

    //sc->env = std::make_shared<EnvSphereMapHDR>("res/texture/076.hdr");
    sc->env = std::make_shared<EnvSingleColor>(Vec3f(0.0f));
    sc->lightAndEnvStrategy = LightSampleStrategy::ByPower;
    sc->lightAndEnvStrategy = LightSampleStrategy::ByPower;
    sc->camera = camera;
    sc->buildScene();
    mScene = sc;
    
    // auto integ = std::make_shared<PathIntegrator>(sc, spp);
    // integ->limitSpp = (spp != 0);
    // integ->tracingDepth = 5;
    // integ->sampleDirectLight = true;
    // integ->enableMIS = true;
    auto integ = std::make_shared<LightPathIntegrator>(sc, spp);
    integ->maxDepth = 5;
    integ->resultScale = &mResultScale;
    mIntegrator = integ;
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
            auto result = resultBuffer(i, j) * mResultScale;
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
            mScene->camera->move(keyList[i]);
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