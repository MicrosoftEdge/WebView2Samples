// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "stdafx.h"

#include "ScenarioNavigateWithWebResourceRequest.h"
#include "AppWindow.h"
#include "CheckFailure.h"
#include "TextInputDialog.h"

#include <Shlwapi.h>

#include <memory>

using namespace Microsoft::WRL;

ScenarioNavigateWithWebResourceRequest::ScenarioNavigateWithWebResourceRequest(
    AppWindow* appWindow)
    : m_appWindow(appWindow)
{
    // Prepare post data as UTF-8 byte array and convert it to stream
    // as required by the application/x-www-form-urlencoded Content-Type
    TextInputDialog dialog(
        appWindow->GetMainWindow(), L"Post data", L"Post data:",
        L"Specify post data to submit to https://www.w3schools.com/action_page.php",
        L"");
    if (dialog.confirmed)
    {
        std::wstring postData = std::wstring(L"input=") + dialog.input;
        int sizeNeededForMultiByte = WideCharToMultiByte(
            CP_UTF8, 0, postData.c_str(), int(postData.size()), nullptr,
            0,
            nullptr, nullptr);

        std::unique_ptr<char[]> postDataBytes = std::make_unique<char[]>(sizeNeededForMultiByte);
        WideCharToMultiByte(
            CP_UTF8, 0, postData.c_str(), int(postData.size()), postDataBytes.get(),
            sizeNeededForMultiByte, nullptr, nullptr);

        //! [NavigateWithWebResourceRequest]
        wil::com_ptr<ICoreWebView2Environment2> webviewEnvironment2;
        CHECK_FAILURE(appWindow->GetWebViewEnvironment()->QueryInterface(
            IID_PPV_ARGS(&webviewEnvironment2)));
        wil::com_ptr<ICoreWebView2WebResourceRequest> webResourceRequest;
        wil::com_ptr<IStream> postDataStream = SHCreateMemStream(
            reinterpret_cast<const BYTE*>(postDataBytes.get()), sizeNeededForMultiByte);

        // This is acts as a form submit to https://www.w3schools.com/action_page.php
        CHECK_FAILURE(webviewEnvironment2->CreateWebResourceRequest(
            L"https://www.w3schools.com/action_page.php", L"POST", postDataStream.get(),
            L"Content-Type: application/x-www-form-urlencoded", &webResourceRequest));
        wil::com_ptr<ICoreWebView2_2> webview2;
        CHECK_FAILURE(m_appWindow->GetWebView()->QueryInterface(IID_PPV_ARGS(&webview2)));
        CHECK_FAILURE(webview2->NavigateWithWebResourceRequest(webResourceRequest.get()));
        //! [NavigateWithWebResourceRequest]
    }
}