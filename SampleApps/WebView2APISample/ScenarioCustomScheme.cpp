// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "stdafx.h"

#include "ScenarioCustomScheme.h"

#include "AppWindow.h"
#include "CheckFailure.h"

#include <Shlwapi.h>

using namespace Microsoft::WRL;

ScenarioCustomScheme::ScenarioCustomScheme(AppWindow* appWindow) : m_appWindow(appWindow)
{
    CHECK_FAILURE(m_appWindow->GetWebView()->AddWebResourceRequestedFilter(
        L"custom-scheme*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL));
    CHECK_FAILURE(m_appWindow->GetWebView()->add_WebResourceRequested(
        Callback<ICoreWebView2WebResourceRequestedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2WebResourceRequestedEventArgs* args)
            {
                wil::com_ptr<ICoreWebView2WebResourceRequest> request;
                wil::com_ptr<ICoreWebView2WebResourceResponse> response;
                CHECK_FAILURE(args->get_Request(&request));
                wil::unique_cotaskmem_string uri;
                CHECK_FAILURE(request->get_Uri(&uri));
                if (wcsncmp(uri.get(), L"custom-scheme", ARRAYSIZE(L"custom-scheme") - 1) == 0)
                {
                    std::wstring assetsFilePath = L"assets/";
                    assetsFilePath += wcsstr(uri.get(), L":") + 1;
                    wil::com_ptr<IStream> stream;
                    SHCreateStreamOnFileEx(
                        assetsFilePath.c_str(), STGM_READ, FILE_ATTRIBUTE_NORMAL, FALSE,
                        nullptr, &stream);
                    if (stream)
                    {
                        CHECK_FAILURE(
                            m_appWindow->GetWebViewEnvironment()->CreateWebResourceResponse(
                                stream.get(), 200, L"OK",
                                L"Content-Type: application/json\nAccess-Control-Allow-Origin: "
                                L"*",
                                &response));
                        CHECK_FAILURE(args->put_Response(response.get()));
                    }
                    else
                    {
                        CHECK_FAILURE(
                            m_appWindow->GetWebViewEnvironment()->CreateWebResourceResponse(
                                nullptr, 404, L"Not Found", L"", &response));
                        CHECK_FAILURE(args->put_Response(response.get()));
                    }
                    return S_OK;
                }

                return S_OK;
            })
            .Get(),
        &m_webResourceRequestedToken));

    m_appWindow->GetWebView()->add_NavigationCompleted(
        Callback<ICoreWebView2NavigationCompletedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args)
            {
                // The following XHR will execute in the context of https://www.example.com page
                // and will succeed. WebResourceRequested event will be raised for this request
                // as *.example.com is in the allowed origin list of custom-scheme. Since the
                // response header provided in WebResourceRequested handler allows all origins
                // for CORS the XHR succeeds.
                CHECK_FAILURE(m_appWindow->GetWebView()->ExecuteScript(
                    L"function reqListener(e) { console.log(e.data) };"
                    L"function errListener(e) { console.log(e.error) };"
                    L"var oReq = new XMLHttpRequest();"
                    L"oReq.addEventListener(\"load\", reqListener);"
                    L"oReq.addEventListener(\"error\", errListener);"
                    L"oReq.open(\"GET\", \"custom-scheme:ScenarioCustomScheme.json\");"
                    L"oReq.send();",
                    Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                        [](HRESULT error, PCWSTR result) -> HRESULT { return S_OK; })
                        .Get()));
                // The following XHR will fail because *.example.com is not in the allowed
                // origin list of custom-scheme-not-in-allowed-origins. The WebResourceRequested
                // event will not be raised for this request.
                CHECK_FAILURE(m_appWindow->GetWebView()->ExecuteScript(
                    L"var oReq = new XMLHttpRequest();"
                    L"oReq.addEventListener(\"load\", reqListener);"
                    L"oReq.addEventListener(\"error\", errListener);"
                    L"oReq.open(\"GET\", "
                    L"\"custom-scheme-not-in-allowed-origins://"
                    L"ScenarioCustomScheme.json\");"
                    L"oReq.send();",
                    Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                        [](HRESULT error, PCWSTR result) -> HRESULT { return S_OK; })
                        .Get()));
                CHECK_FAILURE(m_appWindow->GetWebView()->remove_NavigationCompleted(
                    m_navigationCompletedToken));
                m_navigationCompletedToken = {0};
                return S_OK;
            })
            .Get(),
        &m_navigationCompletedToken);
    m_appWindow->GetWebView()->Navigate(L"https://www.example.com");
}

ScenarioCustomScheme::~ScenarioCustomScheme()
{
    CHECK_FAILURE(
        m_appWindow->GetWebView()->remove_WebResourceRequested(m_webResourceRequestedToken));
}
