#include "Application.h"

void Application::init(const std::string &name, HINSTANCE instance, int width, int height, int spp)
{
    this->instance = instance;
    this->windowWidth = width;
    this->windowHeight = height;

    lastCursorX = width / 2;
    lastCursorY = height / 2;

    window = CreateWindowA(
        name.c_str(), name.c_str(),
        WS_OVERLAPPEDWINDOW & ~WS_MINIMIZEBOX & ~WS_SIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        nullptr,
        nullptr,
        0,
        nullptr);

    SetWindowPos(window, HWND_TOP, 0, 0, width, height, SWP_NOMOVE);
    ShowWindow(window, 1);
    UpdateWindow(window);
    SetWindowPos(window, HWND_TOP, 0, 0, width, height, SWP_NOMOVE);

    colorBuffer.init(width, height);
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
        bitmap = CreateCompatibleBitmap(hdc, metricX, metricY);

        memdc = CreateCompatibleDC(hdc);
        SelectObject(memdc, bitmap);

        BitBlt(memdc, 0, 0, metricX, metricY, memdc, 0, 0, WHITENESS);

        DeleteDC(memdc);
        ReleaseDC(hWnd, hdc);

        break;
    }

    case WM_PAINT:
    {
        HDC hdc;
        PAINTSTRUCT ps;
        RECT rect;

        hdc = BeginPaint(hWnd, &ps);
        memdc = CreateCompatibleDC(hdc);
        SelectObject(memdc, bitmap);
        GetClientRect(hWnd, &rect);

        BitBlt(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top, memdc, 0, 0, SRCCOPY);

        DeleteDC(memdc);
        memdc = 0;
        EndPaint(hWnd, &ps);

        break;
    }

    case WM_KEYDOWN:
    {
        keyPressing[(int)wParam] = true;
        if ((int)wParam == VK_F1)
        {
            cursorDisabled ^= 1;
            firstCursorMove = true;
        }
        else if ((int)wParam == 'T')
            toneMappingMethod = (toneMappingMethod + 1) % 4;
        else if ((int)wParam == 'O')
            saveImage();
        break;
    }

    case WM_KEYUP:
        keyPressing[(int)wParam] = false;
        break;

    case WM_MOUSEMOVE:
    {
        if (cursorDisabled)
            break;
        integrator->reset();

        if (firstCursorMove)
        {
            lastCursorX = (int)LOWORD(lParam);
            lastCursorY = (int)HIWORD(lParam);
            firstCursorMove = false;
        }

        float offsetX = ((int)LOWORD(lParam) - lastCursorX) * CAMERA_ROTATE_SENSITIVITY;
        float offsetY = ((int)HIWORD(lParam) - lastCursorY) * CAMERA_ROTATE_SENSITIVITY;

        glm::vec3 offset(-offsetX, -offsetY, 0.0f);
        scene->camera->rotate(offset);

        lastCursorX = (int)LOWORD(lParam);
        lastCursorY = (int)HIWORD(lParam);
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
        DeleteObject(bitmap);
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

    if (!integrator->isFinished())
    {
        integrator->renderOnePass();
        writeBuffer();
    }
    flushScreen();

    if (integrator->isFinished())
        return;
    colorBuffer.swap();
}

void Application::initScene(int spp)
{
    srand(time(nullptr));

    auto sc = std::make_shared<Scene>();

    /*
		const float level = -2.0f;
		hittableList.push_back
		(
			std::make_shared<Quad>
			(
			  	glm::vec3(-200.0f, -10.0f, level),
			  	glm::vec3(200.0f, -10.0f, level),
			  	glm::vec3(-200.0f, 100.0f, level),
			  	std::make_shared<MetalWorkflow>(glm::vec3(1.0f), 0.0f, 1.0f)
			)
		);
		*/

    /*
		const float step = 1.0f, level = -2.0f;
		for (int i = -10; i < 10; i++)
		{
			for (int j = -10; j < 10; j++)
			{
				hittableList.push_back
				(
				 	std::make_shared<Quad>
				 	(
					 	glm::vec3(i * step, j * step, level),
						glm::vec3((i + 1) * step, j * step, level),
						glm::vec3(i * step, (j + 1) * step, level),
						std::make_shared<MetalWorkflow>(((i ^ j) & 1) ? glm::vec3(1.0f) : glm::vec3(0.1f), 0.0f, 1.0f)
					)
				);
			}
		}
		*/
    // glm::mat4 model(1.0f);
    // model = glm::translate(model, glm::vec3(0.0f, 0.0f, -2.0f));
    // model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    // model = glm::scale(model, glm::vec3(0.3f));
    // std::shared_ptr<Transform> trTeapot = std::make_shared<Transform>(model);
    // scene->addObjectMesh("res/model/dragon2.obj", trTeapot, std::make_shared<Dielectric>(glm::vec3(1.0f), 0.0f, 1.4f));
    // scene->addObjectMesh("res/model/dragon2_armor.obj", trTeapot, std::make_shared<Dielectric>(glm::vec3(1.0f), 0.0f, 1.5f));

    // auto sphere = std::make_shared<Object>(
    // 	std::make_shared<Sphere>(glm::vec3(0.0f, 0.0f, 1.0f), 1.0f, true),
    // 	//std::make_shared<MetalWorkflow>(glm::vec3(1.0f), 1.0f, 0.2f)
    // 	//std::make_shared<Clearcoat>(0.01f, 1.0f)
    // 	std::make_shared<Dielectric>(glm::vec3(1.0f), 0.0f, 1.4f)
    // 	// std::make_shared<MixedMaterial>(
    // 	// 	std::make_shared<MetalWorkflow>(glm::vec3(1.0, 0.2f, 0.2f), 0.0f, 0.1f),
    // 	// 	std::make_shared<Clearcoat>(0.01f, 1.0f),
    // 	// 	std::make_shared<Dielectric>(glm::vec3(1.0f), 0.1f, 1.4f),
    // 	// 	0.9)
    // );
    // //sphere->setTransform(std::make_shared<Transform>(glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 2.0f))));
    // scene->addHittable(sphere);

    // sc->addHittable(
    //     std::make_shared<Object>(
    //         std::make_shared<Quad>(
    //             glm::vec3(-30.0f, -30.0f, -0.5f),
    //             glm::vec3(30.0f, -30.0f, -0.5f),
    //             glm::vec3(-30.0f, 30.0f, -0.5f)),
    //         std::make_shared<MetalWorkflow>(glm::vec3(1.0f), 1.0f, 0.2f)
    //         //std::make_shared<Lambertian>(glm::vec3(1.0f))
    //         ));

    sc->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                glm::vec3(-3.0f, 0.0f, -3.0f),
                glm::vec3(3.0f, 0.0f, -3.0f),
                glm::vec3(-3.0f, 6.0f, -3.0f)),
            std::make_shared<MetalWorkflow>(glm::vec3(1.0f), 0.0f, 1.0f)
            //std::make_shared<Lambertian>(glm::vec3(1.0f))
            ));

    sc->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                glm::vec3(-3.0f, 0.0f, -3.0f),
                glm::vec3(-3.0f, 6.0f, -3.0f),
                glm::vec3(-3.0f, 0.0f, 3.0f)),
            std::make_shared<MetalWorkflow>(glm::vec3(1.0f, 0.25f, 0.25f), 0.0f, 1.0f)
            //std::make_shared<Lambertian>(glm::vec3(1.0f, 0.25f, 0.25f))
            ));

    sc->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                glm::vec3(3.0f, 6.0f, -3.0f),
                glm::vec3(3.0f, 0.0f, -3.0f),
                glm::vec3(3.0f, 6.0f, 3.0f)),
            std::make_shared<MetalWorkflow>(glm::vec3(0.25f, 0.25f, 1.0f), 0.0f, 1.0f)
            //std::make_shared<Lambertian>(glm::vec3(0.25f, 0.25f, 1.0f))
            ));

    sc->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                glm::vec3(3.0f, 6.0f, 3.0f),
                glm::vec3(3.0f, 0.0f, 3.0f),
                glm::vec3(-3.0f, 6.0f, 3.0f)),
            std::make_shared<MetalWorkflow>(glm::vec3(1.0f), 0.0f, 1.0f)
            //std::make_shared<Lambertian>(glm::vec3(1.0f))
            ));

    sc->addHittable(
        std::make_shared<Object>(
            std::make_shared<Quad>(
                glm::vec3(-3.0f, 6.0f, -3.0f),
                glm::vec3(3.0f, 6.0f, -3.0f),
                glm::vec3(-3.0f, 6.0f, 3.0f)),
            std::make_shared<MetalWorkflow>(glm::vec3(1.0f), 0.0f, 1.0f)
            //std::make_shared<Lambertian>(glm::vec3(1.0f))
            ));

    // sc->addHittable(
    //     std::make_shared<Object>(
    //         std::make_shared<Sphere>(glm::vec3(1.0f, 3.0f, 0.0f), 1.0f, true),
    //         std::make_shared<Dielectric>(glm::vec3(1.0f), 0.0f, 1.5f)
    //         //std::make_shared<MetalWorkflow>(glm::vec3(1.0f, 0.79f, 0.51f), 1.0f, 0.25f)
    //         ));

    // glm::mat4 model(1.0f);
    // model = glm::translate(model, glm::vec3(1.0f, 2.0f, -2.1f));
    // model = glm::rotate(model, glm::radians(-17.5f), glm::vec3(0.0f, 0.0f, 1.0f));
    // model = glm::scale(model, glm::vec3(1.8f));
    // std::shared_ptr<Transform> trBoxSmall = std::make_shared<Transform>(model);

    // sc->addObjectMesh("res/model/cube.obj", trBoxSmall,
    //                      std::make_shared<MetalWorkflow>(glm::vec3(1.0f, 0.8f, 0.6f), 0.0f, 1.0f)
    //                      //std::make_shared<Lambertian>(glm::vec3(1.0f))
    // );

    // model = glm::mat4(1.0f);
    // model = glm::translate(model, glm::vec3(-1.0f, 4.0f, -1.2f));
    // model = glm::rotate(model, glm::radians(17.5f), glm::vec3(0.0f, 0.0f, 1.0f));
    // model = glm::scale(model, glm::vec3(1.8f, 1.8f, 3.6f));
    // std::shared_ptr<Transform> trBoxLarge = std::make_shared<Transform>(model);

    // sc->addObjectMesh("res/model/cube.obj", trBoxLarge,
    //                      std::make_shared<MetalWorkflow>(glm::vec3(1.0f), 0.0f, 1.0f)
    //                      //std::make_shared<Lambertian>(glm::vec3(1.0f))
    // );

    sc->addLight(
        std::make_shared<Light>(
            std::make_shared<Quad>(
                glm::vec3(-0.75f, 3.75f, 2.999f),
                glm::vec3(0.75f, 3.75f, 2.999f),
                glm::vec3(-0.75f, 2.25f, 2.999f)),
            glm::vec3(100.0f), false));

    //camera->setPos({ 2.4f, -3.0f, 1.75f });
    //camera->setFOV(80.0f);
    //camera->lookAt(glm::vec3(0.4f, 0.0f, 1.0f));

    //camera->setPos({ 2.4f, -3.6f, /*2.75f*/ 3.75f });
    auto camera = std::make_shared<ThinLensCamera>(40.0f);
    //auto camera = std::make_shared<PanoramaCamera>();
    camera->initFilm(windowWidth, windowHeight);
    camera->setPos({0.0f, -8.0f, 0.0f});
    camera->lookAt(glm::vec3(0.0f));
    //camera->lookAt(glm::vec3(0.4f, 0.0f, 0.5f));

    //sc->env = std::make_shared<EnvSphereMapHDR>("res/texture/076.hdr");
    sc->lightAndEnvStrategy = LightSampleStrategy::ByPower;
    //sc->lightAndEnvStrategy = LightSampleStrategy::Uniform;
    sc->camera = camera;
    sc->buildScene();
    scene = sc;
    
    // auto integ = std::make_shared<PathIntegrator>(sc, spp);
    // integ->limitSpp = (spp != 0);
    // integ->tracingDepth = 1;
    // integ->sampleDirectLight = true;
    auto integ = std::make_shared<LightPathIntegrator>(sc, 20000);
    integ->maxDepth = 1;
    //integ->mSampler = std::make_shared<IndependentSampler>();
    //integ->mSampler = std::make_shared<SimpleSobolSampler>(windowWidth, windowHeight);
    integrator = integ;
}

void Application::writeBuffer()
{
    auto resultBuffer = integrator->result();
    using namespace ToneMapping;

    glm::vec3 (*toneMapping[4])(const glm::vec3 &) = {reinhard, CE, filmic, ACES};

    for (int i = 0; i < windowWidth; i++)
    {
        for (int j = 0; j < windowHeight; j++)
        {
            auto result = resultBuffer(i, j);
            result = glm::clamp(result, glm::vec3(0.0f), glm::vec3(1e8f));
            result = toneMapping[toneMappingMethod](result);
            result = glm::pow(result, glm::vec3(1.0f / 2.2f));
            colorBuffer(i, j) = RGB24::swapRB(RGB24(result));
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
    header.biWidth = windowWidth;
    header.biHeight = -windowHeight;
    header.biPlanes = 1;
    header.biSizeImage = 0;
    header.biSize = sizeof(BITMAPINFOHEADER);

    HDC dc = GetDC(window);
    HDC compatibleDC = CreateCompatibleDC(dc);
    HBITMAP compatibleBitmap = CreateCompatibleBitmap(dc, windowWidth, windowHeight);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(compatibleDC, compatibleBitmap);

    SetDIBits(
        dc,
        compatibleBitmap,
        0,
        windowHeight,
        (BYTE *)colorBuffer.getCurrentBuffer().bufPtr(),
        &bInfo,
        DIB_RGB_COLORS);
    BitBlt(dc, -1, -1, windowWidth, windowHeight, compatibleDC, 0, 0, SRCCOPY);

    DeleteDC(compatibleDC);
    DeleteObject(oldBitmap);
    DeleteObject(compatibleBitmap);
    UpdateWindow(window);
}

void Application::processKey()
{
    uint8_t keyList[] = {'W', 'S', 'A', 'D', 'Q', 'E', 'R', VK_SHIFT, VK_SPACE};

    for (int i = 0; i < 9; i++)
    {
        if (keyPressing[keyList[i]])
        {
            scene->camera->move(keyList[i]);
            integrator->reset();
        }
    }
}

void Application::saveImage()
{
    int w = colorBuffer.width(), h = colorBuffer.height();
    int size = w * h;
    RGB24 *data = new RGB24[size];
    for (int i = 0; i < size; i++)
        data[i] = RGB24::swapRB(colorBuffer[i]);
    std::string file = "screenshot/saves/save" + std::to_string((int)time(0)) + ".png";
    stbi_write_png(file.c_str(), w, h, 3, data, w * 3);
    delete[] data;
    std::cout << "[Image captured]\n";
}