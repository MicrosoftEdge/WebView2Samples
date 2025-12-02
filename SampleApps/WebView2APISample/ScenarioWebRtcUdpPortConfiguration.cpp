// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "CheckFailure.h"
#include "TextInputDialog.h"

#include "ScenarioWebRtcUdpPortConfiguration.h"

using namespace Microsoft::WRL;

static constexpr WCHAR c_samplePath[] = L"ScenarioWebRtcUdpPortConfiguration.html";

ScenarioWebRtcUdpPortConfiguration::ScenarioWebRtcUdpPortConfiguration(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    // Demonstrates how to configure a custom WebRTC UDP port range using the
    // new ICoreWebView2WebRtcPortConfiguration exposed via
    // ICoreWebView2ExperimentalEnvironmentOptions.

    // Navigate to a demo page that will trigger WebRTC usage.
    m_sampleUri = m_appWindow->GetLocalUri(c_samplePath);
    CHECK_FAILURE(m_webView->Navigate(m_sampleUri.c_str()));

    // If we navigate away from the demo page, turn off this scenario.
    CHECK_FAILURE(m_webView->add_ContentLoading(
        Callback<ICoreWebView2ContentLoadingEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2ContentLoadingEventArgs* /*args*/)
                -> HRESULT
            {
                wil::unique_cotaskmem_string uri;
                CHECK_FAILURE(sender->get_Source(&uri));
                if (uri.get() != m_sampleUri)
                {
                    m_appWindow->DeleteComponent(this);
                }
                return S_OK;
            })
            .Get(),
        &m_contentLoadingToken));
}

ScenarioWebRtcUdpPortConfiguration::~ScenarioWebRtcUdpPortConfiguration()
{
    CHECK_FAILURE(m_webView->remove_ContentLoading(m_contentLoadingToken));
}