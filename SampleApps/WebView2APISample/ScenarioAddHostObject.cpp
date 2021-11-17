// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "ScenarioAddHostObject.h"

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

ScenarioAddHostObject::ScenarioAddHostObject(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    std::wstring sampleUri = m_appWindow->GetLocalUri(L"ScenarioAddHostObject.html");

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

    wil::com_ptr<ICoreWebView2_4> webview2_4 = m_webView.try_query<ICoreWebView2_4>();
    if (webview2_4)
    {
        // Register a handler for the FrameCreated event.
        // This handler can be used to add host objects to the created iframe.
        CHECK_FAILURE(webview2_4->add_FrameCreated(
            Callback<ICoreWebView2FrameCreatedEventHandler>(
                [this](
                    ICoreWebView2* sender,
                    ICoreWebView2FrameCreatedEventArgs* args) -> HRESULT
        {
            wil::com_ptr<ICoreWebView2Frame> webviewFrame;
            CHECK_FAILURE(args->get_Frame(&webviewFrame));

            wil::unique_cotaskmem_string name;
            CHECK_FAILURE(webviewFrame->get_Name(&name));
            if (std::wcscmp(name.get(), L"iframe_name") == 0)
            {
                //! [AddHostObjectToScriptWithOrigins]
                wil::unique_variant remoteObjectAsVariant;
                // It will throw if m_hostObject fails the QI, but because it is our object
                // it should always succeed.
                m_hostObject.query_to<IDispatch>(&remoteObjectAsVariant.pdispVal);
                remoteObjectAsVariant.vt = VT_DISPATCH;

                // Create list of origins which will be checked.
                // iframe will have access to host object only if its origin belongs
                // to this list.
                LPCWSTR origin = L"https://appassets.example/";

                CHECK_FAILURE(webviewFrame->AddHostObjectToScriptWithOrigins(
                    L"sample", &remoteObjectAsVariant, 1, &origin));
                //! [AddHostObjectToScriptWithOrigins]
            }

            // Subscribe to frame name changed event
            webviewFrame->add_NameChanged(
                Callback<ICoreWebView2FrameNameChangedEventHandler>(
                    [this](ICoreWebView2Frame* sender, IUnknown* args) -> HRESULT {
                        wil::unique_cotaskmem_string newName;
                        CHECK_FAILURE(sender->get_Name(&newName));
                        // Handle name changed event
                        return S_OK;
                    }).Get(), NULL);

            // Subscribe to frame destroyed event
            webviewFrame->add_Destroyed(
                Callback<ICoreWebView2FrameDestroyedEventHandler>(
                    [this](ICoreWebView2Frame* sender, IUnknown* args) -> HRESULT {
                        /*Cleanup on frame destruction*/
                        return S_OK;
                    })
                    .Get(),
                NULL);
            return S_OK;
        }).Get(), &m_frameCreatedToken));
    }

    CHECK_FAILURE(m_webView->Navigate(sampleUri.c_str()));
}

ScenarioAddHostObject::~ScenarioAddHostObject()
{
    m_webView->RemoveHostObjectFromScript(L"sample");
    m_webView->remove_NavigationStarting(m_navigationStartingToken);
    wil::com_ptr<ICoreWebView2_4> webview2_4 = m_webView.try_query<ICoreWebView2_4>();
    if (webview2_4)
    {
        webview2_4->remove_FrameCreated(m_frameCreatedToken);
    }
}
