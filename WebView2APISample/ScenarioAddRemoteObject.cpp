// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "ScenarioAddRemoteObject.h"

#include "AppWindow.h"
#include "CheckFailure.h"
#include "RemoteObjectSampleImpl.h"

using namespace Microsoft::WRL;

ScenarioAddRemoteObject::ScenarioAddRemoteObject(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    std::wstring sampleUri = m_appWindow->GetLocalUri(L"ScenarioAddRemoteObject.html");

    m_remoteObject = Microsoft::WRL::Make<RemoteObjectSample>(
        [appWindow = m_appWindow](std::function<void(void)> callback)
    {
        appWindow->RunAsync(callback);
    });

    CHECK_FAILURE(m_webView->add_NavigationStarting(
        Microsoft::WRL::Callback<IWebView2NavigationStartingEventHandler>(
            [this, sampleUri](IWebView2WebView* sender, IWebView2NavigationStartingEventArgs* args) -> HRESULT
    {
        wil::unique_cotaskmem_string navigationTargetUri;
        CHECK_FAILURE(args->get_Uri(&navigationTargetUri));

        if (sampleUri == navigationTargetUri.get())
        {
            //! [AddRemoteObject]
            VARIANT remoteObjectAsVariant = {};
            m_remoteObject.query_to<IDispatch>(&remoteObjectAsVariant.pdispVal);
            remoteObjectAsVariant.vt = VT_DISPATCH;

            // We can call AddRemoteObject multiple times in a row without
            // calling RemoveRemoteObject first. This will replace the previous object
            // with the new object. In our case this is the same object and everything
            // is fine.
            CHECK_FAILURE(m_webView->AddRemoteObject(L"sample", &remoteObjectAsVariant));
            remoteObjectAsVariant.pdispVal->Release();
            //! [AddRemoteObject]
        }
        else
        {
            // We can call RemoveRemoteObject multiple times in a row without
            // calling AddRemoteObject first. This will produce an error result
            // so we ignore the failure.
            m_webView->RemoveRemoteObject(L"sample");

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
    m_webView->RemoveRemoteObject(L"sample");
    m_webView->remove_NavigationStarting(m_navigationStartingToken);
}
