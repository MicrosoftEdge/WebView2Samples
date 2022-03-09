// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "stdafx.h"

#include "ScenarioAuthentication.h"

#include "AppWindow.h"
#include "CheckFailure.h"

using namespace Microsoft::WRL;

ScenarioAuthentication::ScenarioAuthentication(AppWindow* appWindow) :
    m_appWindow(appWindow)
{
    m_webView = wil::com_ptr<ICoreWebView2>(m_appWindow->GetWebView()).query<ICoreWebView2_2>();

    //! [WebResourceResponseReceived]
    CHECK_FAILURE(m_webView->add_WebResourceResponseReceived(
        Callback<ICoreWebView2WebResourceResponseReceivedEventHandler>(
            [this](
                ICoreWebView2* sender,
                ICoreWebView2WebResourceResponseReceivedEventArgs* args) {
                wil::com_ptr<ICoreWebView2WebResourceRequest> request;
                CHECK_FAILURE(args->get_Request(&request));
                wil::unique_cotaskmem_string uri;
                CHECK_FAILURE(request->get_Uri(&uri));
                if (wcscmp(uri.get(), L"https://authenticationtest.com/HTTPAuth/") == 0)
                {
                    wil::com_ptr<ICoreWebView2HttpRequestHeaders> requestHeaders;
                    CHECK_FAILURE(request->get_Headers(&requestHeaders));

                    wil::unique_cotaskmem_string authHeaderValue;
                    if (requestHeaders->GetHeader(L"Authorization", &authHeaderValue) == S_OK)
                    {
                        m_appWindow->AsyncMessageBox(
                            std::wstring(L"Authorization: ") + authHeaderValue.get(),
                            L"Authentication result");
                        m_appWindow->DeleteComponent(this);
                    }
                }

                return S_OK;
            })
            .Get(),
        &m_webResourceResponseReceivedToken));
    //! [WebResourceResponseReceived]

    //! [BasicAuthenticationRequested]
    if (auto webView10 = m_webView.try_query<ICoreWebView2_10>())
    {
        CHECK_FAILURE(webView10->add_BasicAuthenticationRequested(
            Callback<ICoreWebView2BasicAuthenticationRequestedEventHandler>(
                [this](
                    ICoreWebView2* sender,
                    ICoreWebView2BasicAuthenticationRequestedEventArgs* args) {
                    wil::com_ptr<ICoreWebView2BasicAuthenticationResponse> basicAuthenticationResponse;
                    CHECK_FAILURE(args->get_Response(&basicAuthenticationResponse));
                    CHECK_FAILURE(basicAuthenticationResponse->put_UserName(L"user"));
                    CHECK_FAILURE(basicAuthenticationResponse->put_Password(L"pass"));

                    return S_OK;
                })
                .Get(),
            &m_basicAuthenticationRequestedToken));
    }
    else {
        FeatureNotAvailable();
    }
    //! [BasicAuthenticationRequested]
    CHECK_FAILURE(m_webView->Navigate(L"https://authenticationtest.com/HTTPAuth/"));
}

ScenarioAuthentication::~ScenarioAuthentication() {
    CHECK_FAILURE(
        m_webView->remove_WebResourceResponseReceived(m_webResourceResponseReceivedToken));
    if (auto webView10 = m_webView.try_query<ICoreWebView2_10>())
    {
        CHECK_FAILURE(webView10->remove_BasicAuthenticationRequested(
            m_basicAuthenticationRequestedToken));
    }
}
