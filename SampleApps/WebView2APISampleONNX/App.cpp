// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "App.h"

#include <shellapi.h>
#include <shellscalingapi.h>
#include <shobjidl.h>
#include <string.h>

#include "AppWindow.h"
#include "DpiUtil.h"

HINSTANCE g_hInstance;
int g_nCmdShow;

static int RunMessagePump();

int APIENTRY
wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    g_hInstance = hInstance;
    UNREFERENCED_PARAMETER(hPrevInstance);
    g_nCmdShow = nCmdShow;

    // Default DPI awareness to PerMonitorV2
    DPI_AWARENESS_CONTEXT dpiAwarenessContext = DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2;
    std::wstring userDataFolder(L"");

    SetCurrentProcessExplicitAppUserModelID(L"WebView2APISampleONNX");
    DpiUtil::SetProcessDpiAwarenessContext(dpiAwarenessContext);

    // Create the main app window
    new AppWindow(userDataFolder);

    int retVal = RunMessagePump();

    return retVal;
}

// Run the message pump for one thread.
static int RunMessagePump()
{
    MSG msg;

    // Main message loop
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!IsDialogMessage(GetAncestor(msg.hwnd, GA_ROOT), &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}
