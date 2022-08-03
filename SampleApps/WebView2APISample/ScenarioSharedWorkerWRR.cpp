// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "stdafx.h"

#include "ScenarioSharedWorkerWRR.h"

#include "AppWindow.h"
#include "CheckFailure.h"

#include <Shlwapi.h>

using namespace Microsoft::WRL;

ScenarioSharedWorkerWRR::ScenarioSharedWorkerWRR(AppWindow* appWindow)
    : m_webView(appWindow->GetWebView())
{
    //! [WebResourceRequested2]
    wil::com_ptr<ICoreWebView2Experimental16> webView =
        m_webView.try_query<ICoreWebView2Experimental16>();
    if (webView)
    {
        // Filter must be added for application to receive any WebResourceRequested event
        CHECK_FAILURE(webView->AddWebResourceRequestedFilterWithRequestSourceKinds(
            L"*worker.js", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL,
            COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS_ALL));
        CHECK_FAILURE(m_webView->add_WebResourceRequested(
            Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                [this](ICoreWebView2* sender, ICoreWebView2WebResourceRequestedEventArgs* args)
                {
                    wil::com_ptr<ICoreWebView2ExperimentalWebResourceRequestedEventArgs>
                        webResourceRequestArgs;
                    if (SUCCEEDED(args->QueryInterface(IID_PPV_ARGS(&webResourceRequestArgs))))
                    {
                        COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS requestSourceKind =
                            COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS_ALL;
                        CHECK_FAILURE(webResourceRequestArgs->get_RequestedSourceKind(
                            &requestSourceKind));
                        // Ensure that script is from shared worker source
                        if (requestSourceKind ==
                            COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS_SHARED_WORKER)
                        {
                            Microsoft::WRL::ComPtr<IStream> response_stream;
                            CHECK_FAILURE(SHCreateStreamOnFileEx(
                                L"assets/DemoWorker.js", STGM_READ, FILE_ATTRIBUTE_NORMAL,
                                FALSE, nullptr, &response_stream));

                            Microsoft::WRL::ComPtr<ICoreWebView2WebResourceResponse> response;
                            // Get the default webview environment
                            Microsoft::WRL::ComPtr<ICoreWebView2_2> webview2;
                            CHECK_FAILURE(sender->QueryInterface(IID_PPV_ARGS(&webview2)));

                            Microsoft::WRL::ComPtr<ICoreWebView2Environment> environment;
                            CHECK_FAILURE(webview2->get_Environment(&environment));
                            CHECK_FAILURE(environment->CreateWebResourceResponse(
                                response_stream.Get(), 200, L"OK", L"", &response));

                            CHECK_FAILURE(args->put_Response(response.Get()));
                        }
                    }
                    return S_OK;
                })
                .Get(),
            &m_webResourceRequestedToken));
    }
    //! [WebResourceRequested2]

    CHECK_FAILURE(
        m_webView->Navigate(L"https://mdn.github.io/simple-shared-worker/index2.html"));
}

ScenarioSharedWorkerWRR::~ScenarioSharedWorkerWRR()
{
    wil::com_ptr<ICoreWebView2Experimental16> webView =
        m_webView.try_query<ICoreWebView2Experimental16>();
    if (webView)
    {
        CHECK_FAILURE(webView->RemoveWebResourceRequestedFilterWithRequestSourceKinds(
            L"*worker.js", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL,
            COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS_ALL));
    }
    CHECK_FAILURE(m_webView->remove_WebResourceRequested(m_webResourceRequestedToken));
}
