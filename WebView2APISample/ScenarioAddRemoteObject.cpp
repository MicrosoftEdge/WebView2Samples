// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "ScenarioAddRemoteObject.h"

#include <algorithm>

#include "AppWindow.h"
#include "CheckFailure.h"
#include "HostObjectSampleImpl.h"

using namespace Microsoft::WRL;

bool AreFileUrisEqual (std::wstring leftUri, std::wstring rightUri)
{
    // Have to to lower due to current bug
    std::transform(leftUri.begin(), leftUri.end(),
            leftUri.begin(), ::tolower);
    std::transform(rightUri.begin(), rightUri.end(),
            rightUri.begin(), ::tolower);

    return leftUri == rightUri;
}

ScenarioAddRemoteObject::ScenarioAddRemoteObject(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    std::wstring sampleUri = m_appWindow->GetLocalUri(L"ScenarioAddRemoteObject.html");

    m_hostObject = Microsoft::WRL::Make<HostObjectSample>(
        [appWindow = m_appWindow](std::function<void(void)> callback)
    {
        appWindow->RunAsync(callback);
    });

    CHECK_FAILURE(m_webView->add_NavigationStarting(
        Microsoft::WRL::Callback<ICoreWebView2NavigationStartingEventHandler>(
            [this, sampleUri](ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT
    {
        wil::unique_cotaskmem_string navigationTargetUri;
        CHECK_FAILURE(args->get_Uri(&navigationTargetUri));
        std::wstring uriTarget(navigationTargetUri.get());

        if (AreFileUrisEqual(sampleUri, uriTarget))
        {
            //! [AddHostObjectToScript]
            VARIANT remoteObjectAsVariant = {};
            m_hostObject.query_to<IDispatch>(&remoteObjectAsVariant.pdispVal);
            remoteObjectAsVariant.vt = VT_DISPATCH;

            // We can call AddHostObjectToScript multiple times in a row without
            // calling RemoveHostObject first. This will replace the previous object
            // with the new object. In our case this is the same object and everything
            // is fine.
            CHECK_FAILURE(
                m_webView->AddHostObjectToScript(L"sample", &remoteObjectAsVariant));
            remoteObjectAsVariant.pdispVal->Release();
            //! [AddHostObjectToScript]
        }
        else
        {
            // We can call RemoveHostObject multiple times in a row without
            // calling AddHostObjectToScript first. This will produce an error
            // result so we ignore the failure.
            m_webView->RemoveHostObjectFromScript(L"sample");

            // When we navigate elsewhere we're off of the sample
            // scenario page and so should remove the scenario.
            m_appWindow->DeleteComponent(this);
        }

        return S_OK;
    }).Get(), &m_navigationStartingToken));

    CHECK_FAILURE(m_webView->Navigate(sampleUri.c_str()));
}

ScenarioAddRemoteObject::~ScenarioAddRemoteObject()
{
    m_webView->RemoveHostObjectFromScript(L"sample");
    m_webView->remove_NavigationStarting(m_navigationStartingToken);
}
