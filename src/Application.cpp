#include "../include/Application.h"
#include "../include/Core/ToneMapping.h"

void Application::init(const std::string &name, HINSTANCE instance, const char *cmdParam)
{
    Error::bracketLine<0>(cmdParam);

    mCorrectGamma = true;
    mToneMapping = false;
    std::string integType;
    std::string samplerType;
    int width, height;
    int maxDepth;
    int spp;
    std::stringstream param(cmdParam);

    param >> integType >> samplerType >> width >> height >> spp >> maxDepth;

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
        integ->mParam.russianRoulette = maxDepth == 0;
        integ->mParam.maxDepth = maxDepth;
        integ->mParam.MIS = true;
        mIntegrator = integ;
        scramble = false;
    }
    else if (integType == "-path")
    {
        auto integ = std::make_shared<PathIntegrator>(mScene, spp);
        integ->mParam.russianRoulette = maxDepth == 0;
        integ->mParam.maxDepth = maxDepth;
        integ->mParam.MIS = true;
        mIntegrator = integ;
        scramble = true;
    }
    else if (integType == "-lpath")
    {
        auto integ = std::make_shared<LightPathIntegrator>(mScene, spp);
        integ->mParam.russianRoulette = maxDepth == 0;
        integ->mParam.maxDepth = maxDepth;
        mIntegrator = integ;
        scramble = false;
    }
    else if (integType == "-bdpt")
    {
        auto integ = std::make_shared<BDPTIntegrator>(mScene, spp);
        param >> integ->mParam.debugStrategy.x >> integ->mParam.debugStrategy.y;
        integ->mParam.debug = (integ->mParam.debugStrategy.y != 0);
        integ->mParam.rrCameraPath = true;
        integ->mParam.maxCameraDepth = maxDepth;
        integ->mParam.rrLightPath = true;
        integ->mParam.maxLightDepth = maxDepth;
        integ->mParam.maxConnectDepth = maxDepth;
        //integ->mLightSampler = std::make_shared<SimpleSobolSampler>(UniformUint(), false);
        integ->mLightSampler = std::make_shared<IndependentSampler>();
        mIntegrator = integ;
        scramble = true;
    }
    else if (integType == "-bdpt2")
    {
        int pathsOnePass;
        param >> pathsOnePass;
        auto integ = std::make_shared<BDPTIntegrator2>(mScene, spp, pathsOnePass);
        param >> integ->mParam.debugStrategy.x >> integ->mParam.debugStrategy.y;
        integ->mParam.debug = (integ->mParam.debugStrategy.y != 0);
        integ->mParam.rrCameraPath = true;
        integ->mParam.maxCameraDepth = maxDepth;
        integ->mParam.rrLightPath = true;
        integ->mParam.maxLightDepth = maxDepth;
        integ->mParam.maxConnectDepth = maxDepth;
        integ->mLightSampler = std::make_shared<SimpleSobolSampler>(0x12345678, true);
        mIntegrator = integ;
        scramble = false;
    }
    else if (integType == "-tpath")
    {
        int pathsOnePass;
        param >> pathsOnePass;
        auto integ = std::make_shared<TriPathIntegrator>(mScene, spp, pathsOnePass);
        mIntegrator = integ;
        scramble = false;
    }
    else if (integType == "-ao")
    {
        auto integ = std::make_shared<AOIntegrator>(mScene, spp);
        param >> integ->mParam.radius;
        mIntegrator = integ;
        scramble = true;
    }
    else if (integType == "-ao2")
    {
        int pathsOnePass;
        param >> pathsOnePass;
        auto integ = std::make_shared<AOIntegrator2>(mScene, spp, pathsOnePass);
        param >> integ->mParam.radius;
        mIntegrator = integ;
        scramble = false;
    }
    
    if (samplerType == "-rng")
        mIntegrator->mSampler = std::make_shared<IndependentSampler>();
    else
        mIntegrator->mSampler = std::make_shared<SimpleSobolSampler>(0, scramble);
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
            mToneMapping = !mToneMapping;
        else if ((int)wParam == 'G')
            mCorrectGamma = !mCorrectGamma;
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

bool Application::render()
{
    processKey();

    if (!mIntegrator->isFinished())
    {
        mIntegrator->renderOnePass();
        writeBuffer();
    }
    flushScreen();

    if (mIntegrator->isFinished())
    {
        saveImage();
        return false;
    }

    mColorBuffer.swap();
    return true;
}

void Application::initScene()
{
    srand(time(nullptr));
    auto scene = setupScene(mWindowWidth, mWindowHeight);
    scene->buildScene();
    mScene = scene;
}

void Application::writeBuffer()
{
    auto resultBuffer = mIntegrator->result();
    for (int i = 0; i < mWindowWidth; i++)
    {
        for (int j = 0; j < mWindowHeight; j++)
        {
            auto result = resultBuffer(i, j) * mIntegrator->mResultScale;
            result = glm::clamp(result, Vec3f(0.0f), Vec3f(1e8f));
            if (mToneMapping)
                result = ToneMapping::filmic(result);
            if (mCorrectGamma)
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