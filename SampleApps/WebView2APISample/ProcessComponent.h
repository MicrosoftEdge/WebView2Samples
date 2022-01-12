// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include "AppWindow.h"
#include "ComponentBase.h"

// This component handles commands from the Process menu, as well as some miscellaneous
// functions for managing the browser process.
class ProcessComponent : public ComponentBase
{
public:
    ProcessComponent(AppWindow* appWindow);

    static bool IsAppContentUri(const std::wstring& source);

    bool HandleWindowMessage(
        HWND hWnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam,
        LRESULT* result) override;

    void ShowBrowserProcessInfo();
    std::wstring ProcessFailedKindToString(const COREWEBVIEW2_PROCESS_FAILED_KIND kind);
    std::wstring ProcessFailedReasonToString(const COREWEBVIEW2_PROCESS_FAILED_REASON reason);
    std::wstring ProcessKindToString(const COREWEBVIEW2_PROCESS_KIND kind);
    void CrashBrowserProcess();
    void CrashRenderProcess();
    void PerformanceInfo();

    ~ProcessComponent() override;

    // Wait for process to exit for timeoutMs, then force quit it if it hasn't.
    static void EnsureProcessIsClosed(UINT processId, int timeoutMs);

private:
    void ScheduleReinitIfSelectedByUser(
        const std::wstring& message, const std::wstring& caption);
    void ScheduleReloadIfSelectedByUser(
        const std::wstring& message, const std::wstring& caption);

    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2> m_webView;
    wil::com_ptr<ICoreWebView2Environment> m_webViewEnvironment;

    UINT m_browserProcessId = 0;
    wil::com_ptr<ICoreWebView2ProcessInfoCollection> m_processCollection;

    EventRegistrationToken m_processFailedToken = {};
    EventRegistrationToken m_processInfosChangedToken = {};
};
