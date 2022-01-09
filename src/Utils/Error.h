#pragma once

#include <iostream>
#include <sstream>

namespace Error
{
    static void log(const std::string &pri, const std::string &snd = "")
    {
        std::cerr << "[" << pri << "] " << snd << std::endl;
    }

    static void exit(const std::string &msg = "")
    {
        std::cerr << "[Error] " << msg << std::endl;
        std::abort();
    }

    static void impossiblePath()
    {
        exit("This path is impossible to be reached, check the program");
    }

    static void check(bool cond, const std::string &errMsg = "")
    {
        if (cond)
        {
            std::cerr << "[CheckFail] " << errMsg << std::endl;
            std::abort();
        }
    }
}