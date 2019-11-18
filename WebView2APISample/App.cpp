// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "App.h"

#include <map>
#include <shellscalingapi.h>
#include <shobjidl.h>
#include <string.h>
#include <vector>

#include "AppWindow.h"

HINSTANCE g_hInstance;
int g_nCmdShow;
bool g_autoTabHandle = true;

static std::map<DWORD, HANDLE> s_threads;

static int RunMessagePump();
static DWORD WINAPI ThreadProc(void* pvParam);
static void WaitForOtherThreads();

int APIENTRY wWinMain(HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      PWSTR lpCmdLine,
                      int nCmdShow)
{
    g_hInstance = hInstance;
    UNREFERENCED_PARAMETER(hPrevInstance);
    g_nCmdShow = nCmdShow;

    // Default DPI awareness to PerMonitorV2. The commandline parameters can
    // override this.
    DPI_AWARENESS_CONTEXT dpiAwarenessContext =
        DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2;
    // Same but for older OS versions that don't support per-monitor v2
    PROCESS_DPI_AWARENESS oldDpiAwareness = PROCESS_PER_MONITOR_DPI_AWARE;
    std::wstring appId(L"EBWebView.SampleApp");
    std::wstring initialUri(L"https://www.bing.com");

    if (lpCmdLine && lpCmdLine[0])
    {
        bool commandLineError = false;

        PWSTR nextParam = lpCmdLine;

        if (nextParam[0] == L'-')
        {
            ++nextParam;
            if (nextParam[0] == L'-')
            {
                ++nextParam;
            }
            if (_wcsnicmp(nextParam, L"dpiunaware", ARRAYSIZE(L"dpiunaware") - 1) ==
                0)
            {
                dpiAwarenessContext = DPI_AWARENESS_CONTEXT_UNAWARE;
                oldDpiAwareness = PROCESS_DPI_UNAWARE;
            }
            else if (_wcsnicmp(nextParam, L"dpisystemaware",
                ARRAYSIZE(L"dpisystemaware") - 1) == 0)
            {
                dpiAwarenessContext = DPI_AWARENESS_CONTEXT_SYSTEM_AWARE;
                oldDpiAwareness = PROCESS_SYSTEM_DPI_AWARE;
            }
            else if (_wcsnicmp(nextParam, L"dpipermonitorawarev2",
                ARRAYSIZE(L"dpipermonitorawarev2") - 1) == 0)
            {
                dpiAwarenessContext = DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2;
                oldDpiAwareness = PROCESS_PER_MONITOR_DPI_AWARE;
            }
            else if (_wcsnicmp(nextParam, L"dpipermonitoraware",
                ARRAYSIZE(L"dpipermonitoraware") - 1) == 0)
            {
                dpiAwarenessContext = DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE;
                oldDpiAwareness = PROCESS_PER_MONITOR_DPI_AWARE;
            }
            else if (_wcsnicmp(nextParam, L"noinitialnavigation",
                ARRAYSIZE(L"noinitialnavigation") - 1) == 0)
            {
                initialUri = L"";
            }
            else if (_wcsnicmp(nextParam, L"appid=",
                ARRAYSIZE(L"appid=") - 1) == 0)
            {
                PWSTR appidStart = nextParam + ARRAYSIZE(L"appid=");
                size_t len = 0;
                while (appidStart[len] && (appidStart[len] != ' '))
                    ++len;
                appId = std::wstring(appidStart, len);
            }
            else if (_wcsnicmp(nextParam, L"initialUri=",
                ARRAYSIZE(L"initialUri=") - 1) == 0)
            {
                PWSTR uriStart = nextParam + ARRAYSIZE(L"initialUri=") - 1;
                size_t len = 0;
                while (uriStart[len] && (uriStart[len] != ' '))
                    ++len;
                initialUri = std::wstring(uriStart, len);
            }
            else
            {
                // --edge-webview-switches is a supported switch to pass addtional
                // command line switches to WebView's browser process.
                // For example, adding
                //   --edge-webview-switches="--remote-debugging-port=9222"
                // enables remote debugging for webview.
                // And adding
                //   --edge-webview-switches="--auto-open-devtools-for-tabs"
                // causes dev tools to open automatically for the WebView.
                commandLineError =
                    (wcsncmp(nextParam, L"edge-webview-switches",
                        ARRAYSIZE(L"edge-webview-switches") - 1) != 0) &&
                        (wcsncmp(nextParam, L"restore",
                            ARRAYSIZE(L"restore") - 1) != 0);
            }
        }
        else
        {
            commandLineError = true;
        }

        if (commandLineError)
        {
            MessageBox(nullptr,
                       L"Valid command line "
                       L"parameters:\n\t-DPIUnaware\n\t-DPISystemAware\n\t-"
                       L"DPIPerMonitorAware\n\t-DPIPerMonitorAwareV2",
                       L"Command Line Parameters", MB_OK);
        }
    }

    SetCurrentProcessExplicitAppUserModelID(appId.c_str());

    // Call the latest DPI awareness function possible
    HMODULE user32 = LoadLibraryA("User32.dll");
    auto func = reinterpret_cast<decltype(&SetProcessDpiAwarenessContext)>(
        GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
    if (func)
    {
        // Windows 10 1703+: SetProcessDpiAwarenessContext
        func(dpiAwarenessContext);
    }
    else {
        HMODULE shcore = LoadLibraryA("Shcore.dll");
        auto func = reinterpret_cast<decltype(&SetProcessDpiAwareness)>(
            GetProcAddress(shcore, "SetProcessDpiAwareness"));
        if (func)
        {
            // Windows 8.1+: SetProcessDpiAwareness
            func(oldDpiAwareness);
        }
        else if (dpiAwarenessContext != DPI_AWARENESS_CONTEXT_UNAWARE)
        {
            // Windows 7+: SetProcessDPIAware
            SetProcessDPIAware();
        }
    }

    new AppWindow(initialUri);

    int retVal = RunMessagePump();

    WaitForOtherThreads();

    return retVal;
}

// Run the message pump for one thread.
static int RunMessagePump()
{
    HACCEL hAccelTable =
        LoadAccelerators(g_hInstance, MAKEINTRESOURCE(IDC_WEBVIEW2APISAMPLE));

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
void CreateNewThread()
{
    DWORD threadId;
    HANDLE thread = CreateThread(nullptr, 0, ThreadProc, nullptr,
                                 STACK_SIZE_PARAM_IS_A_RESERVATION, &threadId);
    s_threads.insert(std::pair<DWORD, HANDLE>(threadId, thread));
}

// This function is the starting point for new threads. It will open a new app window.
static DWORD WINAPI ThreadProc(void* pvParam)
{
    new AppWindow();
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
            static_cast<DWORD>(threadHandles.size()), threadHandles.data(), FALSE,
            INFINITE, QS_ALLEVENTS);

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
