// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "ScenarioSLM.h"

#include <algorithm>

#include "AppWindow.h"
#include "CheckFailure.h"
#include "SLMHostObjectImpl.h"

using namespace Microsoft::WRL;

bool ScenarioSLM::AreFileUrisEqual(const std::wstring& leftUri, const std::wstring& rightUri)
{
    std::wstring left = leftUri;
    std::wstring right = rightUri;
    
    // Convert to lowercase for comparison
    std::transform(left.begin(), left.end(), left.begin(), ::tolower);
    std::transform(right.begin(), right.end(), right.begin(), ::tolower);

    return left == right;
}

bool ScenarioSLM::CanLaunch()
{
    // ONNX Runtime version can always launch
    // Model will be downloaded on first use if needed
    return true;
}

ScenarioSLM::ScenarioSLM(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    std::wstring sampleUri = m_appWindow->GetLocalUri(L"ScenarioSLM.html");

    // Create the SLM host object
    m_hostObject = Microsoft::WRL::Make<SLMHostObjectImpl>(
        [appWindow = m_appWindow](std::function<void(void)> callback)
        { 
            appWindow->RunAsync(callback); 
        });

    // Register navigation event to add/remove host object
    CHECK_FAILURE(m_webView->add_NavigationStarting(
        Microsoft::WRL::Callback<ICoreWebView2NavigationStartingEventHandler>(
            [this, sampleUri](
                ICoreWebView2* sender,
                ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT
            {
                wil::unique_cotaskmem_string navigationTargetUri;
                CHECK_FAILURE(args->get_Uri(&navigationTargetUri));
                std::wstring uriTarget(navigationTargetUri.get());

                if (AreFileUrisEqual(sampleUri, uriTarget))
                {
                    // Add the SLM host object to the page
                    VARIANT remoteObjectAsVariant = {};
                    Microsoft::WRL::ComPtr<IDispatch> dispatch;
                    m_hostObject.As(&dispatch);
                    remoteObjectAsVariant.pdispVal = dispatch.Detach();
                    remoteObjectAsVariant.vt = VT_DISPATCH;

                    CHECK_FAILURE(
                        m_webView->AddHostObjectToScript(L"slm", &remoteObjectAsVariant));
                    remoteObjectAsVariant.pdispVal->Release();
                }
                else
                {
                    // Remove host object when navigating away
                    m_webView->RemoveHostObjectFromScript(L"slm");

                    // Clean up this scenario when navigating away
                    m_appWindow->DeleteComponent(this);
                }

                return S_OK;
            })
            .Get(),
        &m_navigationStartingToken));

    // Navigate to the SLM scenario page
    CHECK_FAILURE(m_webView->Navigate(sampleUri.c_str()));
}

ScenarioSLM::~ScenarioSLM()
{
    // Clean up
    m_webView->RemoveHostObjectFromScript(L"slm");
    m_webView->remove_NavigationStarting(m_navigationStartingToken);
}
