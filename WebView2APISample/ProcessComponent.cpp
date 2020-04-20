// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "ProcessComponent.h"

#include "CheckFailure.h"

using namespace Microsoft::WRL;

ProcessComponent::ProcessComponent(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    //! [ProcessFailed]
    // Register a handler for the ProcessFailed event.
    // This handler checks if the browser process failed, and asks the user if
    // they want to recreate the webview.
    CHECK_FAILURE(m_webView->add_ProcessFailed(
        Callback<ICoreWebView2ProcessFailedEventHandler>(
            [this](ICoreWebView2* sender,
                ICoreWebView2ProcessFailedEventArgs* args) -> HRESULT
    {
        COREWEBVIEW2_PROCESS_FAILED_KIND failureType;
        CHECK_FAILURE(args->get_ProcessFailedKind(&failureType));
        if (failureType == COREWEBVIEW2_PROCESS_FAILED_KIND_BROWSER_PROCESS_EXITED)
        {
            int button = MessageBox(
                m_appWindow->GetMainWindow(),
                L"Browser process exited unexpectedly.  Recreate webview?",
                L"Browser process exited",
                MB_YESNO);
            if (button == IDYES)
            {
                m_appWindow->ReinitializeWebView();
            }
        }
        else if (failureType == COREWEBVIEW2_PROCESS_FAILED_KIND_RENDER_PROCESS_UNRESPONSIVE)
        {
            int button = MessageBox(
                m_appWindow->GetMainWindow(),
                L"Browser render process has stopped responding.  Recreate webview?",
                L"Web page unresponsive", MB_YESNO);
            if (button == IDYES)
            {
                m_appWindow->ReinitializeWebView();
            }
        }
        else if (failureType == COREWEBVIEW2_PROCESS_FAILED_KIND_RENDER_PROCESS_EXITED)
        {
            int button = MessageBox(
                m_appWindow->GetMainWindow(),
                L"Browser render process exited unexpectedly. Reload page?",
                L"Web page unresponsive", MB_YESNO);
            if (button == IDYES)
            {
                CHECK_FAILURE(m_webView->Reload());
            }
        }
        return S_OK;
    }).Get(), &m_processFailedToken));
    //! [ProcessFailed]
}

bool ProcessComponent::HandleWindowMessage(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT* result)
{
    if (message == WM_COMMAND)
    {
        switch (LOWORD(wParam))
        {
        case IDM_PROCESS_INFO:
            ShowBrowserProcessInfo();
            return true;
        case IDM_CRASH_PROCESS:
            CrashBrowserProcess();
            return true;
        case IDM_CRASH_RENDER_PROCESS:
            CrashRenderProcess();
            return true;
        }
    }
    return false;
}

// Show the WebView's PID to the user.
void ProcessComponent::ShowBrowserProcessInfo() {
    UINT32 processId;
    m_webView->get_BrowserProcessId(&processId);

    WCHAR buffer[4096] = L"";
    StringCchPrintf(buffer, ARRAYSIZE(buffer), L"Process ID: %u\n", processId);
    MessageBox(m_appWindow->GetMainWindow(), buffer, L"Process Info", MB_OK);
}

// Crash the browser's process on command, to test crash handlers.
void ProcessComponent::CrashBrowserProcess()
{
    m_webView->Navigate(L"edge://inducebrowsercrashforrealz");
}

// Crash the browser's render process on command, to test crash handlers.
void ProcessComponent::CrashRenderProcess()
{
    m_webView->Navigate(L"edge://kill");
}

/*static*/ void ProcessComponent::EnsureProcessIsClosed(UINT processId, int timeoutMs)
{
    UINT exitCode = 1;
    if (processId != 0)
    {
        HANDLE hBrowserProcess = ::OpenProcess(PROCESS_TERMINATE, false, processId);
        // Wait for the process to exit by itself
        DWORD waitResult = ::WaitForSingleObject(hBrowserProcess, timeoutMs);
        if (waitResult != WAIT_OBJECT_0)
        {
            // Force kill the process if it doesn't exit by itself
            BOOL result = ::TerminateProcess(hBrowserProcess, exitCode);
            ::CloseHandle(hBrowserProcess);
        }
    }
}


ProcessComponent::~ProcessComponent()
{
    m_webView->remove_ProcessFailed(m_processFailedToken);
}
