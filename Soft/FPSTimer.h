#pragma once

#include <iostream>
#include <ctime>

const int STANDARD_FPS = 60;

class FPSTimer
{
public:
    FPSTimer() { reset(); }
    void work()
    {
        if (clock() - lastT > CLOCKS_PER_SEC)
        {
            std::cout << "FPS: " << count << "  RT: " << 1000.0f / (float)count << " ms" << std::endl;
            fps = count;
            reset();
        }
        count++;
    }
    void reset()
    {
        lastT = clock();
        count = 0;
    }
    int FPS() { return fps; }
private:
    clock_t lastT;
    int count;
    int fps;
};