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
    void UpdateProgress(ICoreWebView2DownloadOperation* download);
    void CompleteDownload(ICoreWebView2DownloadOperation* download);
    ~ScenarioCustomDownloadExperience() override;

private:
    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2> m_webView;
    wil::com_ptr<ICoreWebView2_4> m_webView2_4;
    std::wstring m_demoUri;
    EventRegistrationToken m_downloadStartingToken = {};
    EventRegistrationToken m_contentLoadingToken = {};
    EventRegistrationToken m_bytesReceivedChangedToken = {};
    EventRegistrationToken m_stateChangedToken = {};
};