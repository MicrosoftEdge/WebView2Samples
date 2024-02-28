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
                wil::com_ptr<ICoreWebView2Experimental24> webview24 =
                    m_webView.try_query<ICoreWebView2Experimental24>();
                CHECK_FEATURE_RETURN_HRESULT(webview24);
                wil::com_ptr<ICoreWebView2Environment> environment =
                    appWindow->GetWebViewEnvironment();
                wil::com_ptr<ICoreWebView2ExperimentalEnvironment14> environment_staging14 =
                    environment.try_query<ICoreWebView2ExperimentalEnvironment14>();
                CHECK_FEATURE_RETURN_HRESULT(environment_staging14);
                wil::com_ptr<ICoreWebView2ExperimentalFileSystemHandle> rootHandle;
                CHECK_FAILURE(environment_staging14->CreateWebFileSystemDirectoryHandle(
                    L"C:\\", COREWEBVIEW2_FILE_SYSTEM_HANDLE_PERMISSION_READ_ONLY,
                    &rootHandle));
                wil::com_ptr<ICoreWebView2ExperimentalObjectCollection> webObjectCollection;
                IUnknown* webObjects[] = {rootHandle.get()};
                CHECK_FAILURE(environment_staging14->CreateObjectCollection(
                    ARRAYSIZE(webObjects), webObjects, &webObjectCollection));
                wil::com_ptr<ICoreWebView2ObjectCollectionView> webObjectCollectionView =
                    webObjectCollection.try_query<ICoreWebView2ObjectCollectionView>();
                wil::unique_cotaskmem_string source;
                CHECK_FAILURE(m_webView->get_Source(&source));

                static const wchar_t* expectedDomain = L"appassets.example";
                wil::unique_bstr sourceDomain = GetDomainOfUri(source.get());

                // Check the source to ensure the message is sent to the correct target content.
                if (std::wstring(expectedDomain) == sourceDomain.get())
                {
                    CHECK_FAILURE(webview24->PostWebMessageAsJsonWithAdditionalObjects(
                        L"{ \"messageType\" : \"RootDirectoryHandle\" }",
                        webObjectCollectionView.get()));
                }

                return S_OK;
            })
            .Get(),
        &m_navigationCompletedToken));
}

ScenarioFileSystemHandleShare::~ScenarioFileSystemHandleShare()
{
    CHECK_FAILURE(m_webView->remove_WebMessageReceived(m_navigationCompletedToken));
}