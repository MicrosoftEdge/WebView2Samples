// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "App.h"

#include <map>
#include <shellapi.h>
#include <shellscalingapi.h>
#include <shobjidl.h>
#include <string.h>
#include <vector>

#include "AppWindow.h"
#include "DpiUtil.h"

HINSTANCE g_hInstance;
int g_nCmdShow;
bool g_autoTabHandle = true;
static std::map<DWORD, HANDLE> s_threads;

static int RunMessagePump();
static DWORD WINAPI ThreadProc(void* pvParam);
static void WaitForOtherThreads();

#define NEXT_PARAM_CONTAINS(command)                                                           \
    _wcsnicmp(nextParam.c_str(), command, ARRAYSIZE(command) - 1) == 0

int APIENTRY
wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    g_hInstance = hInstance;
    UNREFERENCED_PARAMETER(hPrevInstance);
    g_nCmdShow = nCmdShow;

    // Default DPI awareness to PerMonitorV2. The commandline parameters can
    // override this.
    DPI_AWARENESS_CONTEXT dpiAwarenessContext = DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2;
    std::wstring appId(L"EBWebView.SampleApp");
    std::wstring userDataFolder(L"");
    std::wstring initialUri;
    DWORD creationModeId = IDM_CREATION_MODE_WINDOWED;
    WebViewCreateOption opt;

    if (lpCmdLine && lpCmdLine[0])
    {
        int paramCount = 0;
        LPWSTR* params = CommandLineToArgvW(GetCommandLineW(), &paramCount);
        for (int i = 0; i < paramCount; ++i)
        {
            std::wstring nextParam;
            if (params[i][0] == L'-')
            {
                if (params[i][1] == L'-')
                {
                    nextParam.assign(params[i] + 2);
                }
                else
                {
                    nextParam.assign(params[i] + 1);
                }
            }
            if (NEXT_PARAM_CONTAINS(L"dpiunaware"))
            {
                dpiAwarenessContext = DPI_AWARENESS_CONTEXT_UNAWARE;
            }
            else if (NEXT_PARAM_CONTAINS(L"dpisystemaware"))
            {
                dpiAwarenessContext = DPI_AWARENESS_CONTEXT_SYSTEM_AWARE;
            }
            else if (NEXT_PARAM_CONTAINS(L"dpipermonitorawarev2"))
            {
                dpiAwarenessContext = DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2;
            }
            else if (NEXT_PARAM_CONTAINS(L"dpipermonitoraware"))
            {
                dpiAwarenessContext = DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE;
            }
            else if (NEXT_PARAM_CONTAINS(L"noinitialnavigation"))
            {
                initialUri = L"none";
            }
            else if (NEXT_PARAM_CONTAINS(L"appid="))
            {
                appId = nextParam.substr(nextParam.find(L'=') + 1);
            }
            else if (NEXT_PARAM_CONTAINS(L"initialUri="))
            {
                initialUri = nextParam.substr(nextParam.find(L'=') + 1);
            }
            else if (NEXT_PARAM_CONTAINS(L"userdatafolder="))
            {
                userDataFolder = nextParam.substr(nextParam.find(L'=') + 1);
            }
            else if (NEXT_PARAM_CONTAINS(L"creationmode="))
            {
                nextParam = nextParam.substr(nextParam.find(L'=') + 1);
                if (NEXT_PARAM_CONTAINS(L"windowed"))
                {
                    creationModeId = IDM_CREATION_MODE_WINDOWED;
                }
                else if (NEXT_PARAM_CONTAINS(L"allowhostinput"))
                {
                    creationModeId = IDM_CREATION_MODE_HOST_INPUT_PROCESSING;
                }
                else if (NEXT_PARAM_CONTAINS(L"visualdcomp"))
                {
                    creationModeId = IDM_CREATION_MODE_VISUAL_DCOMP;
                }
                else if (NEXT_PARAM_CONTAINS(L"targetdcomp"))
                {
                    creationModeId = IDM_CREATION_MODE_TARGET_DCOMP;
                }
                else if (NEXT_PARAM_CONTAINS(L"visualwincomp"))
                {
                    creationModeId = IDM_CREATION_MODE_VISUAL_WINCOMP;
                }
            }
        }
        LocalFree(params);
    }
    SetCurrentProcessExplicitAppUserModelID(appId.c_str());

    DpiUtil::SetProcessDpiAwarenessContext(dpiAwarenessContext);

    new AppWindow(creationModeId, opt, initialUri, userDataFolder, true);

    int retVal = RunMessagePump();

    WaitForOtherThreads();

    return retVal;
}

// Run the message pump for one thread.
static int RunMessagePump()
{
    HACCEL hAccelTable = LoadAccelerators(g_hInstance, MAKEINTRESOURCE(IDC_WEBVIEW2APISAMPLE));

    MSG msg;

    // Main message loop:
    //! [MoveFocus0]
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            // Calling IsDialogMessage handles Tab traversal automatically. If the
            // app wants the platform to auto handle tab, then call IsDialogMessage
            // before calling TranslateMessage/DispatchMessage. If the app wants to
            // handle tabbing itself, then skip calling IsDialogMessage and call
            // TranslateMessage/DispatchMessage directly.
            if (!g_autoTabHandle || !IsDialogMessage(GetAncestor(msg.hwnd, GA_ROOT), &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
    //! [MoveFocus0]

    DWORD threadId = GetCurrentThreadId();
    auto it = s_threads.find(threadId);
    if (it != s_threads.end())
    {
        CloseHandle(it->second);
        s_threads.erase(threadId);
    }

    return (int)msg.wParam;
}

// Make a new thread.
void CreateNewThread(AppWindow* app)
{
    DWORD threadId;
    app->AddRef();
    HANDLE thread = CreateThread(
        nullptr, 0, ThreadProc, reinterpret_cast<LPVOID>(app),
        STACK_SIZE_PARAM_IS_A_RESERVATION, &threadId);
    s_threads.insert(std::pair<DWORD, HANDLE>(threadId, thread));
}

// This function is the starting point for new threads. It will open a new app window.
static DWORD WINAPI ThreadProc(void* pvParam)
{
    AppWindow* app = static_cast<AppWindow*>(pvParam);
    new AppWindow(app->GetCreationModeId(), app->GetWebViewOption());
    app->Release();
    return RunMessagePump();
}

// Called on the main thread.  Wait for all other threads to complete before exiting.
static void WaitForOtherThreads()
{
    while (!s_threads.empty())
    {
        std::vector<HANDLE> threadHandles;
        for (auto it = s_threads.begin(); it != s_threads.end(); ++it)
        {
            threadHandles.push_back(it->second);
        }

        HANDLE* handleArray = threadHandles.data();
        DWORD dwIndex = MsgWaitForMultipleObjects(
            static_cast<DWORD>(threadHandles.size()), threadHandles.data(), FALSE, INFINITE,
            QS_ALLEVENTS);

        if (dwIndex == WAIT_OBJECT_0 + threadHandles.size())
        {
            MSG msg;
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
}
