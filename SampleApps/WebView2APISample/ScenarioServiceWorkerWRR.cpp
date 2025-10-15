// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "ScenarioServiceWorkerWRR.h"

#include "AppWindow.h"
#include "CheckFailure.h"

#include "shlwapi.h"

using namespace Microsoft::WRL;

static constexpr WCHAR c_samplePath[] =
    L"https://mdn.github.io/dom-examples/service-worker/simple-service-worker/";
static constexpr WCHAR c_wrrUrlPattern[] = L"*";

ScenarioServiceWorkerWRR::ScenarioServiceWorkerWRR(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView()), m_sampleUri(c_samplePath)
{
    wil::com_ptr<ICoreWebView2_22> webView_22 = m_webView.try_query<ICoreWebView2_22>();
    CHECK_FEATURE_RETURN_EMPTY(webView_22);

    webView_22->AddWebResourceRequestedFilterWithRequestSourceKinds(
        c_wrrUrlPattern, COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL,
        COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS_ALL);
    webView_22->add_WebResourceRequested(
        Callback<ICoreWebView2WebResourceRequestedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2WebResourceRequestedEventArgs* args)
                -> HRESULT
            {
                COREWEBVIEW2_WEB_RESOURCE_CONTEXT resourceContext;
                args->get_ResourceContext(&resourceContext);
                wil::com_ptr<ICoreWebView2WebResourceRequest> request;
                CHECK_FAILURE(args->get_Request(&request));
                wil::unique_cotaskmem_string uri;
                CHECK_FAILURE(request->get_Uri(&uri));
                if (wcscmp(uri.get(), c_samplePath) != 0)
                {
                    return S_OK;
                }

                wil::com_ptr<IStream> stream;
                // Create a stream with some text content
                std::string content = "<html><head><script>"
                                      "navigator.serviceWorker.register('sw.js')"
                                      "</script></head><body><h1>Response from WV2 "
                                      "interceptor!</h1></body></html>";

                stream.attach(SHCreateMemStream(
                    reinterpret_cast<const BYTE*>(content.c_str()),
                    static_cast<UINT>(content.size())));

                wil::com_ptr<ICoreWebView2WebResourceResponse> response;
                wil::com_ptr<ICoreWebView2Environment> environment;
                wil::com_ptr<ICoreWebView2_2> webview2;
                m_webView->QueryInterface(IID_PPV_ARGS(&webview2));
                webview2->get_Environment(&environment);
                environment->CreateWebResourceResponse(
                    stream.get(), 200, L"OK", L"Content-Type: text/html", &response);
                args->put_Response(response.get());
                return S_OK;
            })
            .Get(),
        nullptr);

    // Turn off this scenario if we navigate away from the sample page.
    CHECK_FAILURE(m_webView->add_ContentLoading(
        Callback<ICoreWebView2ContentLoadingEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2ContentLoadingEventArgs* args) -> HRESULT
            {
                wil::unique_cotaskmem_string uri;
                sender->get_Source(&uri);
                if (uri.get() != m_sampleUri)
                {
                    m_appWindow->DeleteComponent(this);
                }
                return S_OK;
            })
            .Get(),
        &m_contentLoadingToken));

    CHECK_FAILURE(m_webView->Navigate(m_sampleUri.c_str()));
}

ScenarioServiceWorkerWRR::~ScenarioServiceWorkerWRR()
{
    wil::com_ptr<ICoreWebView2_22> webView_22 = m_webView.try_query<ICoreWebView2_22>();
    if (webView_22)
    {
        CHECK_FAILURE(webView_22->RemoveWebResourceRequestedFilterWithRequestSourceKinds(
            c_wrrUrlPattern, COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL,
            COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS_ALL));
    }

    CHECK_FAILURE(m_webView->remove_WebResourceRequested(m_webResourceRequestedToken));
    m_webView->remove_ContentLoading(m_contentLoadingToken);
}
