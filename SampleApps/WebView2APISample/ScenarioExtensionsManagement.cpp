// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "ScenarioExtensionsManagement.h"

#include "AppWindow.h"
#include "CheckFailure.h"
#include <string>

using namespace Microsoft::WRL;

static constexpr WCHAR c_samplePath[] = L"extensions/example-devtools-extension";
ScenarioExtensionsManagement::ScenarioExtensionsManagement(AppWindow* appWindow, bool offload)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    if (!offload)
    {
        InstallDefaultExtensions();
    }
    else
    {
        OffloadDefaultExtensionsIfExtraExtensionsInstalled();
    }
}

void ScenarioExtensionsManagement::InstallDefaultExtensions()
{
    auto webView2_13 = m_webView.try_query<ICoreWebView2_13>();
    CHECK_FEATURE_RETURN_EMPTY(webView2_13);
    wil::com_ptr<ICoreWebView2Profile> webView2Profile;
    CHECK_FAILURE(webView2_13->get_Profile(&webView2Profile));
    auto profile7 = webView2Profile.try_query<ICoreWebView2Profile7>();
    CHECK_FEATURE_RETURN_EMPTY(profile7);

    std::wstring extension_path_file = m_appWindow->GetLocalUri(c_samplePath, false);
    // Remove "file:///" from the beginning of extension_path_file
    std::wstring extension_path = extension_path_file.substr(8);

    profile7->GetBrowserExtensions(
        Callback<ICoreWebView2ProfileGetBrowserExtensionsCompletedHandler>(
            [this, profile7, extension_path](
                HRESULT error, ICoreWebView2BrowserExtensionList* extensions) -> HRESULT
            {
                std::wstring extensionIdString;
                bool extensionInstalled = false;
                UINT extensionsCount = 0;
                extensions->get_Count(&extensionsCount);

                for (UINT index = 0; index < extensionsCount; ++index)
                {
                    wil::com_ptr<ICoreWebView2BrowserExtension> extension;
                    extensions->GetValueAtIndex(index, &extension);

                    wil::unique_cotaskmem_string id;
                    wil::unique_cotaskmem_string name;
                    BOOL enabled = false;
                    std::wstring message;

                    extension->get_IsEnabled(&enabled);
                    extension->get_Id(&id);
                    extension->get_Name(&name);
                    extensionIdString = id.get();

                    if (extensionIdString.compare(m_extensionId) == 0)
                    {
                        extensionInstalled = true;
                        message += L"Extension already installed";
                        if (enabled)
                        {
                            message += L" and enabled.";
                        }
                        else
                        {
                            message += L" but was disabled.";
                            extension->Enable(
                                !enabled,
                                Callback<ICoreWebView2BrowserExtensionEnableCompletedHandler>(
                                    [](HRESULT error) -> HRESULT
                                    {
                                        if (error != S_OK)
                                        {
                                            ShowFailure(error, L"Enable Extension failed");
                                        }
                                        return S_OK;
                                    })
                                    .Get());
                            message += L" Extension has now been enabled.";
                        }
                        MessageBox(nullptr, message.c_str(), name.get(), MB_OK);
                        break;
                    }
                }

                if (!extensionInstalled)
                {
                    CHECK_FAILURE(profile7->AddBrowserExtension(
                        extension_path.c_str(),
                        Callback<ICoreWebView2ProfileAddBrowserExtensionCompletedHandler>(
                            [](HRESULT error,
                               ICoreWebView2BrowserExtension* extension) -> HRESULT
                            {
                                if (error != S_OK)
                                {
                                    ShowFailure(error, L"Fail to add browser extension");
                                    return S_OK;
                                }

                                wil::unique_cotaskmem_string name;
                                extension->get_Name(&name);

                                MessageBox(
                                    nullptr,
                                    L"Extension was not installed, has now been installed and "
                                    L"enabled.",
                                    name.get(), MB_OK);
                                return S_OK;
                            })
                            .Get()));
                }
                return S_OK;
            })
            .Get());
}

void ScenarioExtensionsManagement::OffloadDefaultExtensionsIfExtraExtensionsInstalled()
{
    auto webView2_13 = m_webView.try_query<ICoreWebView2_13>();
    CHECK_FEATURE_RETURN_EMPTY(webView2_13);
    wil::com_ptr<ICoreWebView2Profile> webView2Profile;
    CHECK_FAILURE(webView2_13->get_Profile(&webView2Profile));
    auto profile7 = webView2Profile.try_query<ICoreWebView2Profile7>();
    CHECK_FEATURE_RETURN_EMPTY(profile7);

    profile7->GetBrowserExtensions(
        Callback<ICoreWebView2ProfileGetBrowserExtensionsCompletedHandler>(
            [this](HRESULT error, ICoreWebView2BrowserExtensionList* extensions) -> HRESULT
            {
                std::wstring extensionIdString;
                UINT extensionsCount = 0;
                extensions->get_Count(&extensionsCount);

                if (extensionsCount > m_maxInstalledExtensions)
                {
                    for (UINT index = 0; index < extensionsCount; ++index)
                    {
                        wil::com_ptr<ICoreWebView2BrowserExtension> extension;
                        extensions->GetValueAtIndex(index, &extension);

                        wil::unique_cotaskmem_string id;
                        extension->get_Id(&id);
                        extensionIdString = id.get();

                        if (extensionIdString.compare(m_extensionId) == 0)
                        {
                            extension->Remove(
                                Callback<ICoreWebView2BrowserExtensionRemoveCompletedHandler>(
                                    [extension](HRESULT error) -> HRESULT
                                    {
                                        if (error != S_OK)
                                        {
                                            ShowFailure(error, L"Remove Extension failed");
                                        }
                                        wil::unique_cotaskmem_string name;
                                        extension->get_Name(&name);
                                        MessageBox(
                                            nullptr,
                                            L"Extension was installed, but has now been "
                                            L"removed.",
                                            name.get(), MB_OK);
                                        return S_OK;
                                    })
                                    .Get());
                        }
                    }
                }
                else
                {
                    MessageBox(nullptr, L"No extra extensions to offload.", L"OK", MB_OK);
                }
                return S_OK;
            })
            .Get());
}