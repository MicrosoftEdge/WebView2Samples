// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "CheckFailure.h"
#include "DpiUtil.h"

void DpiUtil::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT dpiAwarenessContext)
{
    // Call the latest DPI awareness function possible
    static auto SetProcessDpiAwarenessContextFunc = []() {
        return reinterpret_cast<decltype(&::SetProcessDpiAwarenessContext)>(
            ::GetProcAddress(GetUser32Module(), "SetProcessDpiAwarenessContext"));
    }();
    if (SetProcessDpiAwarenessContextFunc)
    {
        // Windows 10 1703+: SetProcessDpiAwarenessContext
        SetProcessDpiAwarenessContextFunc(dpiAwarenessContext);
    }
    else
    {
        static auto SetProcessDpiAwarenessFunc = []() {
            return reinterpret_cast<decltype(&::SetProcessDpiAwareness)>(
                ::GetProcAddress(GetShcoreModule(), "SetProcessDpiAwareness"));
        }();
        if (SetProcessDpiAwarenessFunc)
        {
            // Windows 8.1+: SetProcessDpiAwareness
            SetProcessDpiAwarenessFunc(
                ProcessDpiAwarenessFromDpiAwarenessContext(dpiAwarenessContext));
        }
        else if (dpiAwarenessContext != DPI_AWARENESS_CONTEXT_UNAWARE)
        {
            // Windows 7+: SetProcessDPIAware
            ::SetProcessDPIAware();
        }
    }
}

int DpiUtil::GetDpiForWindow(HWND window)
{
    static auto GetDpiForMonitorFunc = []() {
        return reinterpret_cast<decltype(&::GetDpiForMonitor)>(
            ::GetProcAddress(GetShcoreModule(), "GetDpiForMonitor"));
    }();
    if (GetDpiForMonitorFunc)
    {
        UINT dpi_x, dpi_y;
        HMONITOR monitor = ::MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
        CHECK_FAILURE(GetDpiForMonitorFunc(monitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y));
        return dpi_x;
    }
    else
    {
        return GetDeviceCaps(GetDC(nullptr), LOGPIXELSX);
    }
}

HMODULE DpiUtil::GetUser32Module()
{
    static HMODULE user32Module = nullptr;
    if (user32Module == nullptr)
    {
        user32Module = LoadLibraryA("User32.dll");
    }
    return user32Module;
}

HMODULE DpiUtil::GetShcoreModule()
{
    static HMODULE shcoreModule = nullptr;
    if (shcoreModule == nullptr)
    {
        shcoreModule = LoadLibraryA("Shcore.dll");
    }
    return shcoreModule;
}

PROCESS_DPI_AWARENESS DpiUtil::ProcessDpiAwarenessFromDpiAwarenessContext(
    DPI_AWARENESS_CONTEXT dpiAwarenessContext)
{
    if (dpiAwarenessContext == DPI_AWARENESS_CONTEXT_UNAWARE ||
        dpiAwarenessContext == DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED)
    {
        return PROCESS_DPI_UNAWARE;
    }
    if (dpiAwarenessContext == DPI_AWARENESS_CONTEXT_SYSTEM_AWARE)
    {
        return PROCESS_SYSTEM_DPI_AWARE;
    }
    if (dpiAwarenessContext == DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE ||
        dpiAwarenessContext == DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)
    {
        return PROCESS_PER_MONITOR_DPI_AWARE;
    }
    // All DPI awarenes contexts should be covered above.
    FAIL_FAST();
}
