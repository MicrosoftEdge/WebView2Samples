// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "ScenarioDOMContentLoaded.h"

#include "AppWindow.h"
#include "CheckFailure.h"

using namespace Microsoft::WRL;

static constexpr WCHAR c_samplePath[] = L"ScenarioDOMContentLoaded.html";
ScenarioDOMContentLoaded::ScenarioDOMContentLoaded(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    m_sampleUri = m_appWindow->GetLocalUri(c_samplePath);
    //! [DOMContentLoaded]
    // Register a handler for the DOMContentLoaded event.
    // Check whether the DOM content loaded
    m_webViewExperimental = m_webView.query<ICoreWebView2Experimental>();
    CHECK_FAILURE(m_webViewExperimental->add_DOMContentLoaded(
        Callback<ICoreWebView2ExperimentalDOMContentLoadedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2ExperimentalDOMContentLoadedEventArgs* args)
                -> HRESULT {
                m_webView->ExecuteScript(
                    L"let "
                    L"content=document.createElement(\"h2\");content.style.color='blue';"
                    L"content.textContent=\"This text was added by the host "
                    L"app\";document.body.appendChild(content);",
                    Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                        [](HRESULT error, PCWSTR result) -> HRESULT { return S_OK; })
                        .Get());
                return S_OK;
            })
            .Get(),
        &m_DOMContentLoadedToken));
    //! [DOMContentLoaded]

    // Turn off this scenario if we navigate away from the sample page
    CHECK_FAILURE(m_webView->add_ContentLoading(
        Callback<ICoreWebView2ContentLoadingEventHandler>(
            [this](
                ICoreWebView2* sender, ICoreWebView2ContentLoadingEventArgs* args) -> HRESULT {
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

ScenarioDOMContentLoaded::~ScenarioDOMContentLoaded()
{
    m_webViewExperimental->remove_DOMContentLoaded(m_DOMContentLoadedToken);
    m_webView->remove_ContentLoading(m_contentLoadingToken);
}
