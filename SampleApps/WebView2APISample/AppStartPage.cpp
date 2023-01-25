// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include <pathcch.h>
#include <psapi.h>

#include "AppStartPage.h"
#include "AppWindow.h"
#include "CheckFailure.h"

using namespace Microsoft::WRL;

namespace AppStartPage
{

bool AreFileUrisEqual(std::wstring leftUri, std::wstring rightUri)
{
    // Have to to lower due to current bug
    std::transform(leftUri.begin(), leftUri.end(),
            leftUri.begin(), ::tolower);
    std::transform(rightUri.begin(), rightUri.end(),
            rightUri.begin(), ::tolower);

    return leftUri == rightUri;
}

std::wstring ResolvePathAndTrimFile(std::wstring path)
{
    wchar_t resultPath[MAX_PATH];
    PathCchCanonicalize(resultPath, ARRAYSIZE(resultPath), path.c_str());
    PathCchRemoveFileSpec(resultPath, ARRAYSIZE(resultPath));
    return resultPath;
}

std::wstring GetSdkBuild()
{
    auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
    wil::unique_cotaskmem_string targetVersion;
    CHECK_FAILURE(options->get_TargetCompatibleBrowserVersion(&targetVersion));

    // The full version string A.B.C.D
    const wchar_t* targetVersionMajorAndRest = targetVersion.get();
    // Should now be .B.C.D
    const wchar_t* targetVersionMinorAndRest = wcschr(targetVersionMajorAndRest, L'.');
    CHECK_FAILURE((targetVersionMinorAndRest != nullptr && *targetVersionMinorAndRest == L'.') ? S_OK : E_UNEXPECTED);

    // Should now be .C.D
    const wchar_t* targetVersionBuildAndRest = wcschr(targetVersionMinorAndRest + 1, L'.');
    CHECK_FAILURE((targetVersionBuildAndRest != nullptr && *targetVersionBuildAndRest == L'.') ? S_OK : E_UNEXPECTED);

    // Return + 1 to skip the first . so just C.D
    return targetVersionBuildAndRest + 1;
}

std::wstring GetRuntimeVersion(AppWindow* appWindow)
{
    wil::com_ptr<ICoreWebView2Environment> environment = appWindow->GetWebViewEnvironment();
    wil::unique_cotaskmem_string runtimeVersion;
    CHECK_FAILURE(environment->get_BrowserVersionString(&runtimeVersion));

    return runtimeVersion.get();
}

std::wstring GetAppPath()
{
    wchar_t appPath[MAX_PATH];
    GetModuleFileName(nullptr, appPath, ARRAYSIZE(appPath));
    return ResolvePathAndTrimFile(appPath);
}

std::wstring GetRuntimePath(AppWindow* appWindow)
{
    wil::com_ptr<ICoreWebView2> webview = appWindow->GetWebView();
    UINT32 browserProcessId = 0;
    wchar_t runtimePath[MAX_PATH];
    CHECK_FAILURE(webview->get_BrowserProcessId(&browserProcessId));

    HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, browserProcessId);
    CHECK_FAILURE(processHandle == nullptr ? E_FAIL : S_OK);
    GetModuleFileNameEx(processHandle, nullptr, runtimePath, ARRAYSIZE(runtimePath));
    CloseHandle(processHandle);

    return ResolvePathAndTrimFile(runtimePath);
}

std::wstring GetUri(AppWindow* appWindow)
{
    std::wstring uri = appWindow->GetLocalUri(L"AppStartPage.html", true);

    uri += L"?sdkBuild=";
    uri += GetSdkBuild();

    uri += L"&runtimeVersion=";
    uri += GetRuntimeVersion(appWindow);

    uri += L"&appPath=";
    uri += GetAppPath();

    uri += L"&runtimePath=";
    uri += GetRuntimePath(appWindow);

    return uri;
}

}; // namespace AppStartPage