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
    MessageBox(
        nullptr,
        L"Authentication scenario:\n Click HTML/NTLM Auth to get Authentication headers",
        nullptr, MB_OK);
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
                        std::wstring message(L"Authorization: ");
                        message += authHeaderValue.get();
                        MessageBox(nullptr, message.c_str(), nullptr, MB_OK);
                        m_appWindow->DeleteComponent(this);
                    }
                }

                return S_OK;
            })
            .Get(),
        &m_webResourceResponseReceivedToken));
    //! [WebResourceResponseReceived]
    CHECK_FAILURE(m_webView->Navigate(L"https://authenticationtest.com"));
}

ScenarioAuthentication::~ScenarioAuthentication() {
    CHECK_FAILURE(
        m_webView->remove_WebResourceResponseReceived(m_webResourceResponseReceivedToken));
}
