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
            CP_UTF8, 0, postData.c_str(), postData.size(), nullptr,
            0,
            nullptr, nullptr);

        std::unique_ptr<char[]> postDataBytes = std::make_unique<char[]>(sizeNeededForMultiByte);
        WideCharToMultiByte(
            CP_UTF8, 0, postData.c_str(), postData.size(), postDataBytes.get(),
            sizeNeededForMultiByte, nullptr, nullptr);

        //! [NavigateWithWebResourceRequest]
        wil::com_ptr<ICoreWebView2Experimental> webviewExperimental;
        CHECK_FAILURE(appWindow->GetWebView()->QueryInterface(IID_PPV_ARGS(&webviewExperimental)));
        wil::com_ptr<ICoreWebView2ExperimentalEnvironment> webviewEnvironmentExperimental;
        CHECK_FAILURE(appWindow->GetWebViewEnvironment()->QueryInterface(
            IID_PPV_ARGS(&webviewEnvironmentExperimental)));
        wil::com_ptr<ICoreWebView2WebResourceRequest> webResourceRequest;
        wil::com_ptr<IStream> postDataStream = SHCreateMemStream(
            reinterpret_cast<const BYTE*>(postDataBytes.get()), sizeNeededForMultiByte);

        // This is acts as a form submit to https://www.w3schools.com/action_page.php
        CHECK_FAILURE(webviewEnvironmentExperimental->CreateWebResourceRequest(
            L"https://www.w3schools.com/action_page.php", L"POST", postDataStream.get(),
            L"Content-Type: application/x-www-form-urlencoded", &webResourceRequest));
        CHECK_FAILURE(webviewExperimental->NavigateWithWebResourceRequest(webResourceRequest.get()));
        //! [NavigateWithWebResourceRequest]
    }
}