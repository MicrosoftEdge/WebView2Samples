// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include "stdafx.h"

#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"

class ScenarioIFrameDevicePermission : public ComponentBase
{
public:
    ScenarioIFrameDevicePermission(AppWindow* appWindow);
    ~ScenarioIFrameDevicePermission() override;

private:
    HRESULT OnPermissionRequested(
        ICoreWebView2Frame* sender, ICoreWebView2PermissionRequestedEventArgs2* args);
    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2> m_webView;
    wil::com_ptr<ICoreWebView2_4> m_webView4;
    std::map<std::tuple<std::wstring, COREWEBVIEW2_PERMISSION_KIND, BOOL>, bool>
        m_cached_permissions;
    wil::com_ptr<ICoreWebView2Frame3> m_frame3;
    std::wstring m_sampleUri;
    EventRegistrationToken m_FrameCreatedToken = {};
    EventRegistrationToken m_ContentLoadingToken = {};
    EventRegistrationToken m_PermissionRequestedToken = {};
};
