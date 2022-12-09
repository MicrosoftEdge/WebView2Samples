// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "stdafx.h"

#include "ScenarioCustomSchemeNavigate.h"

#include "AppWindow.h"
#include "CheckFailure.h"

#include <Shlwapi.h>

using namespace Microsoft::WRL;

ScenarioCustomSchemeNavigate::ScenarioCustomSchemeNavigate(AppWindow* appWindow)
    : m_appWindow(appWindow)
{
    CHECK_FAILURE(m_appWindow->GetWebView()->AddWebResourceRequestedFilter(
        L"wv2rocks*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL));
    CHECK_FAILURE(m_appWindow->GetWebView()->add_WebResourceRequested(
        Callback<ICoreWebView2WebResourceRequestedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2WebResourceRequestedEventArgs* args)
            {
                wil::com_ptr<ICoreWebView2WebResourceRequest> request;
                wil::com_ptr<ICoreWebView2WebResourceResponse> response;
                CHECK_FAILURE(args->get_Request(&request));
                wil::com_ptr<IStream> content;
                request->get_Content(&content);
                wil::unique_cotaskmem_string uri;
                CHECK_FAILURE(request->get_Uri(&uri));
                if (wcsncmp(
                        uri.get(), L"wv2rocks://domain/",
                        ARRAYSIZE(L"wv2rocks://domain/") - 1) == 0)
                {
                    std::wstring assetsFilePath = L"assets/";
                    assetsFilePath +=
                        wcsstr(uri.get(), L"://domain/") + ARRAYSIZE(L"://domain/") - 1;
                    wil::com_ptr<IStream> stream;
                    SHCreateStreamOnFileEx(
                        assetsFilePath.c_str(), STGM_READ, FILE_ATTRIBUTE_NORMAL, FALSE,
                        nullptr, &stream);
                    if (stream)
                    {
                        std::wstring headers;
                        if (assetsFilePath.substr(assetsFilePath.find_last_of(L".") + 1) ==
                            L"html")
                        {
                            headers = L"Content-Type: text/html";
                        }
                        else if (
                            assetsFilePath.substr(assetsFilePath.find_last_of(L".") + 1) ==
                            L"jpg")
                        {
                            headers = L"Content-Type: image/jpeg";
                        }
                        else if (
                            assetsFilePath.substr(assetsFilePath.find_last_of(L".") + 1) ==
                            L"png")
                        {
                            headers = L"Content-Type: image/png";
                        }
                        else if (
                            assetsFilePath.substr(assetsFilePath.find_last_of(L".") + 1) ==
                            L"css")
                        {
                            headers = L"Content-Type: text/css";
                        }
                        else if (
                            assetsFilePath.substr(assetsFilePath.find_last_of(L".") + 1) ==
                            L"js")
                        {
                            headers = L"Content-Type: application/javascript";
                        }

                        CHECK_FAILURE(
                            m_appWindow->GetWebViewEnvironment()->CreateWebResourceResponse(
                                stream.get(), 200, L"OK", headers.c_str(), &response));
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

    m_appWindow->GetWebView()->Navigate(L"wv2rocks://domain/ScenarioCustomScheme.html");
}

ScenarioCustomSchemeNavigate::~ScenarioCustomSchemeNavigate()
{
    CHECK_FAILURE(
        m_appWindow->GetWebView()->remove_WebResourceRequested(m_webResourceRequestedToken));
}
