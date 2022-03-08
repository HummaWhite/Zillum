#pragma once

#include <iostream>
#include <sstream>
#include <mutex>

#include "NamespaceDecl.h"

static std::mutex DebugPrintMutex;
#define DEBUG_PRINT_BEGIN DebugPrintMutex.lock();
#define DEBUG_PRINT_END DebugPrintMutex.unlock();
#define DEBUG_PRINT(msg) { DebugPrintMutex.lock(); std::cout << msg; DebugPrintMutex.unlock(); }
#define DEBUG_LOG(msg) DEBUG_PRINT(msg << "\n")
#define REPORT_IF(cond, msg) if (cond) DEBUG_LOG(msg)
#define REPORT_RETURN_IF(cond, ret, msg) if (cond) { DEBUG_LOG(msg) return ret; }

NAMESPACE_BEGIN(Error)

template<int NTabs>
void bracketLine(const std::string& msg)
{
    static_assert(NTabs >= 0);
    for (int i = 0; i < NTabs; i++)
        std::cerr << "\t";
    std::cerr << "[" << msg << "]" << std::endl;
}

static void line(const std::string& msg)
{
    std::cerr << msg << std::endl;
}

static void exit(const std::string& msg = "")
{
    std::cerr << "[Error exit " << msg << "]" << std::endl;
    std::abort();
}

static void impossiblePath()
{
    exit("[Impossible path: this path is impossible to be reached, check the program]");
}

static void check(bool cond, const std::string& errMsg = "")
{
    if (!cond)
    {
        std::cerr << "[Check failed " << errMsg << "]" << std::endl;
        std::abort();
    }
}

NAMESPACE_END(Error)