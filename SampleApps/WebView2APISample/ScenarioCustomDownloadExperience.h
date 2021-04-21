// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include "stdafx.h"

#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"

class ScenarioCustomDownloadExperience : public ComponentBase
{
public:
    ScenarioCustomDownloadExperience(AppWindow* appWindow);
    bool HandleWindowMessage(
        HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result) override;
    void StartDeferredDownload();
    void CompleteDownloadDeferral();
    void UpdateProgress(ICoreWebView2ExperimentalDownloadOperation* download);
    void CompleteDownload(ICoreWebView2ExperimentalDownloadOperation* download);
    ~ScenarioCustomDownloadExperience() override;

private:
    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2> m_webView;
    wil::com_ptr<ICoreWebView2Experimental2> m_webViewExperimental2;
    std::wstring m_demoUri;
    EventRegistrationToken m_downloadStartingToken = {};
    EventRegistrationToken m_contentLoadingToken = {};
    EventRegistrationToken m_bytesReceivedChangedToken = {};
    EventRegistrationToken m_stateChangedToken = {};
    std::function<void()> m_completeDeferredDownloadEvent;
};