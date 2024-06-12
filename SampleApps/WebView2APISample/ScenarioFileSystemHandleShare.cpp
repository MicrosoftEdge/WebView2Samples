// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "stdafx.h"

#include "ScenarioFileSystemHandleShare.h"

#include "AppWindow.h"
#include "CheckFailure.h"

#include <string>

using namespace Microsoft::WRL;

static constexpr WCHAR c_samplePath[] = L"ScenarioFileSystemHandleShare.html";

extern wil::unique_bstr GetDomainOfUri(PWSTR uri);

//! [PostWebMessageWithAdditionalObjects]
ScenarioFileSystemHandleShare::ScenarioFileSystemHandleShare(AppWindow* appWindow)
    : m_appWindow(appWindow)
{
    m_webView = m_appWindow->GetWebView();

    CHECK_FAILURE(m_webView->Navigate(m_appWindow->GetLocalUri(c_samplePath).c_str()));

    CHECK_FAILURE(m_webView->add_NavigationCompleted(
        Callback<ICoreWebView2NavigationCompletedEventHandler>(
            [this, appWindow](
                ICoreWebView2* sender,
                ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT
            {
                wil::com_ptr<ICoreWebView2_23> webview23 =
                    m_webView.try_query<ICoreWebView2_23>();
                CHECK_FEATURE_RETURN_HRESULT(webview23);
                wil::com_ptr<ICoreWebView2Environment> environment =
                    appWindow->GetWebViewEnvironment();
                wil::com_ptr<ICoreWebView2Environment14>
                    environment14 =
                        environment.try_query<ICoreWebView2Environment14>();
                CHECK_FEATURE_RETURN_HRESULT(environment14);
                wil::com_ptr<ICoreWebView2FileSystemHandle> rootHandle;
                CHECK_FAILURE(environment14->CreateWebFileSystemDirectoryHandle(
                    L"C:\\", COREWEBVIEW2_FILE_SYSTEM_HANDLE_PERMISSION_READ_ONLY,
                    &rootHandle));
                wil::com_ptr<ICoreWebView2ObjectCollection> webObjectCollection;
                IUnknown* webObjects[] = {rootHandle.get()};
                CHECK_FAILURE(environment14->CreateObjectCollection(
                    ARRAYSIZE(webObjects), webObjects, &webObjectCollection));
                wil::unique_cotaskmem_string source;
                CHECK_FAILURE(m_webView->get_Source(&source));

                static const wchar_t* expectedDomain = L"appassets.example";
                wil::unique_bstr sourceDomain = GetDomainOfUri(source.get());

                // Check the source to ensure the message is sent to the correct target content.
                if (std::wstring(expectedDomain) == sourceDomain.get())
                {
                    CHECK_FAILURE(webview23->PostWebMessageAsJsonWithAdditionalObjects(
                        L"{ \"messageType\" : \"RootDirectoryHandle\" }",
                        webObjectCollection.get()));
                }

                return S_OK;
            })
            .Get(),
        &m_navigationCompletedToken));
}
//! [PostWebMessageWithAdditionalObjects]

ScenarioFileSystemHandleShare::~ScenarioFileSystemHandleShare()
{
    CHECK_FAILURE(m_webView->remove_WebMessageReceived(m_navigationCompletedToken));
}