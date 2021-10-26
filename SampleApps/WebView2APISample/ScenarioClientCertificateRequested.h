// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include <string>
#include "AppWindow.h"
#include "ClientCertificateSelectionDialog.h"

class ScenarioClientCertificateRequested : public ComponentBase
{
public:
    ScenarioClientCertificateRequested(AppWindow* appWindow);
    ~ScenarioClientCertificateRequested() override;

private:
    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2> m_webView;
    wil::com_ptr<ICoreWebView2_5> m_webView2_5;
    EventRegistrationToken m_ClientCertificateRequestedToken = {};
    std::vector<ClientCertificate> clientCertificates_;
};