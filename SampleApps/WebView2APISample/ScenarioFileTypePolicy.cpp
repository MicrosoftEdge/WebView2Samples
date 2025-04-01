// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "AppWindow.h"
#include "CheckFailure.h"
#include "ScenarioFileTypePolicy.h"

using namespace Microsoft::WRL;

static constexpr WCHAR c_samplePath[] = L"SecnarioFileTypePolicy.html";

ScenarioFileTypePolicy::ScenarioFileTypePolicy(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView2(appWindow->GetWebView())
{
    if (m_webView2)
    {
        m_webView2_2 = m_webView2.try_query<ICoreWebView2_2>();

        m_sampleUri = m_appWindow->GetLocalUri(c_samplePath);
        CHECK_FAILURE(m_webView2->Navigate(m_sampleUri.c_str()));
        SuppressPolicyForExtension();
        ListenToWebMessages();
        // Turn off this scenario if we navigate away from the demo page.
        CHECK_FAILURE(m_webView2_2->add_DOMContentLoaded(
            Callback<ICoreWebView2DOMContentLoadedEventHandler>(
                [this](ICoreWebView2* sender, ICoreWebView2DOMContentLoadedEventArgs* args)
                    -> HRESULT
                {
                    wil::unique_cotaskmem_string uri;
                    sender->get_Source(&uri);
                    if (uri.get() != m_sampleUri)
                        m_appWindow->DeleteComponent(this);
                    return S_OK;
                })
                .Get(),
            &m_DOMcontentLoadedToken));
    }
}

//! [SuppressPolicyForExtension]
// This example will register the event with two custom rules.
// 1. Suppressing file type policy, security dialog, and allows saving ".eml" files
// directly; when the URI is trusted.
// 2. Showing customized warning UI when saving ".iso" files. It allows to block
// the saving directly.
bool ScenarioFileTypePolicy::SuppressPolicyForExtension()
{
    m_webView2_26 = m_webView2.try_query<ICoreWebView2_26>();
    if (!m_webView2_26)
        return false;
    m_webView2_26->add_SaveFileSecurityCheckStarting(
        Callback<ICoreWebView2SaveFileSecurityCheckStartingEventHandler>(
            [this](
                ICoreWebView2* sender,
                ICoreWebView2SaveFileSecurityCheckStartingEventArgs* args) -> HRESULT
            {
                // Get the file extension for file to be saved.
                // And convert the extension to lower case for a
                // case-insensitive comparasion.
                wil::unique_cotaskmem_string extension;
                CHECK_FAILURE(args->get_FileExtension(&extension));
                std::wstring extension_lower = extension.get();
                std::transform(
                    extension_lower.begin(), extension_lower.end(), extension_lower.begin(),
                    ::towlower);

                // Suppress default policy for ".eml" file.
                if (wcscmp(extension_lower.c_str(), L".eml") == 0)
                {
                    CHECK_FAILURE(args->put_SuppressDefaultPolicy(TRUE));
                }

                // Cancel save/download for ".iso" file.
                if (wcscmp(extension_lower.c_str(), L".iso") == 0)
                {
                    wil::com_ptr<ICoreWebView2Deferral> deferral;
                    CHECK_FAILURE(args->GetDeferral(&deferral));

                    m_appWindow->RunAsync(
                        [this, args = wil::make_com_ptr(args), deferral]()
                        {
                            // With the deferral, the cancel decision and
                            // message box can be replaced with a customized UI.
                            CHECK_FAILURE(args->put_CancelSave(TRUE));
                            MessageBox(
                                m_appWindow->GetMainWindow(), L"The saving has been blocked",
                                L"Info", MB_OK);
                            CHECK_FAILURE(deferral->Complete());
                        });
                }
                if (wcscmp(extension_lower.c_str(), L".exe") == 0)
                {
                    if (is_exe_blocked.has_value())
                    {
                        if (is_exe_blocked.value())
                        {
                            args->put_CancelSave(true);
                        }
                        else
                        {
                            args->put_SuppressDefaultPolicy(true);
                        }
                    }
                }
                if (wcscmp(extension_lower.c_str(), L".emlx") == 0)
                {
                    wil::com_ptr<ICoreWebView2Deferral> deferral;
                    CHECK_FAILURE(args->GetDeferral(&deferral));
                    m_appWindow->RunAsync(
                        [this, args = wil::make_com_ptr(args), deferral]()
                        {
                            // With the deferral, the cancel decision and
                            // message box can be replaced with a customized UI.
                            auto selection = MessageBox(
                                m_appWindow->GetMainWindow(), L"Block the download?",
                                L"Info", MB_OKCANCEL);
                            if (selection == IDOK)
                            {
                                CHECK_FAILURE(args->put_CancelSave(TRUE));
                            }
                            else if (selection == IDCANCEL)
                            {
                                CHECK_FAILURE(args->put_SuppressDefaultPolicy(TRUE));

                            }
                            CHECK_FAILURE(deferral->Complete());
                        });
                }
                return S_OK;
            })
            .Get(),
        &m_saveFileSecurityCheckStartingToken);

    MessageBox(
        m_appWindow->GetMainWindow(),
        (L"Example rules of Dangerous File Security Policy has been applied in this demo page"),
        L"Info", MB_OK);
    return true;
}
//! [SuppressPolicyForExtension]

void ScenarioFileTypePolicy::ListenToWebMessages()
{
    CHECK_FAILURE(m_webView2->add_WebMessageReceived(
        Callback<ICoreWebView2WebMessageReceivedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args)
                -> HRESULT
            {
                LPWSTR message;
                args->TryGetWebMessageAsString(&message);
                ICoreWebView2Settings* settings;
                sender->get_Settings(&settings);
                ICoreWebView2Settings8* settings8;
                settings->QueryInterface(IID_PPV_ARGS(&settings8));
                if (wcscmp(message, L"enable_smartscreen") == 0)
                {

                    settings8->put_IsReputationCheckingRequired(true);
                    MessageBox(
                        m_appWindow->GetMainWindow(), (L"Enabled Smartscreen"), L"Info", MB_OK);
                }
                else if (wcscmp(L"disable_smartscreen", message) == 0)
                {
                    settings8->put_IsReputationCheckingRequired(false);
                    MessageBox(
                        m_appWindow->GetMainWindow(), (L"Disabled Smartscreen"), L"Info",
                        MB_OK);
                }
                else if (wcscmp(L"block_exe", message) == 0)
                {
                    is_exe_blocked = true;
                }
                else if (wcscmp(L"allow_exe", message) == 0)
                {
                    is_exe_blocked = false;
                }
                else if (wcscmp(L"clear_exe_policy", message) == 0)
                {
                    is_exe_blocked = std::nullopt;
                }
                return S_OK;
            })
            .Get(),
        &m_webMessageReceivedToken));
}

ScenarioFileTypePolicy::~ScenarioFileTypePolicy()
{
    if (m_webView2_26)
    {
        CHECK_FAILURE(m_webView2_26->remove_SaveFileSecurityCheckStarting(
            m_saveFileSecurityCheckStartingToken));
    }
    CHECK_FAILURE(m_webView2_2->remove_WebResourceResponseReceived(m_webMessageReceivedToken));
    CHECK_FAILURE(m_webView2_2->remove_DOMContentLoaded(m_DOMcontentLoadedToken));
}