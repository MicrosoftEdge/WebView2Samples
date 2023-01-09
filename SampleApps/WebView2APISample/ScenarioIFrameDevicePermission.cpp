// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "ScenarioIFrameDevicePermission.h"

#include "AppWindow.h"
#include "CheckFailure.h"
#include "SettingsComponent.h"

using namespace Microsoft::WRL;

static constexpr WCHAR c_samplePath[] = L"ScenarioIFrameDevicePermission.html";

ScenarioIFrameDevicePermission::ScenarioIFrameDevicePermission(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    m_sampleUri = m_appWindow->GetLocalUri(c_samplePath);

    //! [PermissionRequested0]
    m_webView4 = m_webView.try_query<ICoreWebView2_4>();
    if (m_webView4)
    {
        CHECK_FAILURE(m_webView4->add_FrameCreated(
            Callback<ICoreWebView2FrameCreatedEventHandler>(
                [this](ICoreWebView2* sender, ICoreWebView2FrameCreatedEventArgs* args) -> HRESULT {
                    wil::com_ptr<ICoreWebView2Frame> webviewFrame;
                    CHECK_FAILURE(args->get_Frame(&webviewFrame));

                    m_frame3 = webviewFrame.try_query<ICoreWebView2Frame3>();
                    if (m_frame3)
                    {
                        CHECK_FAILURE(m_frame3->add_PermissionRequested(
                            Callback<ICoreWebView2FramePermissionRequestedEventHandler>(
                                this, &ScenarioIFrameDevicePermission::OnPermissionRequested
                            ).Get(),
                            &m_PermissionRequestedToken));
                    }
                    else {
                        m_appWindow->RunAsync([]{ FeatureNotAvailable(); });
                    }
                    m_webView4->remove_FrameCreated(m_FrameCreatedToken);
                    return S_OK;
            }).Get(),
            &m_FrameCreatedToken));
    }
    else
    {
        FeatureNotAvailable();
    }
    //! [PermissionRequested0]

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
        &m_ContentLoadingToken));

    CHECK_FAILURE(m_webView->Navigate(m_sampleUri.c_str()));
}

//! [PermissionRequested1]
HRESULT ScenarioIFrameDevicePermission::OnPermissionRequested(
    ICoreWebView2Frame* sender, ICoreWebView2PermissionRequestedEventArgs2* args)
{
    // If we set Handled to true, then we will not fire the PermissionRequested
    // event off of the CoreWebView2.
    args->put_Handled(true);

    // Obtain a deferral for the event so that the CoreWebView2
    // doesn't examine the properties we set on the event args until
    // after we call the Complete method asynchronously later.
    wil::com_ptr<ICoreWebView2Deferral> deferral;
    CHECK_FAILURE(args->GetDeferral(&deferral));

    // Do the rest asynchronously, to avoid calling MessageBox in an event handler.
    m_appWindow->RunAsync([this, deferral, args]
    {
        COREWEBVIEW2_PERMISSION_KIND kind = COREWEBVIEW2_PERMISSION_KIND_UNKNOWN_PERMISSION;
        BOOL userInitiated = FALSE;
        wil::unique_cotaskmem_string uri;
        CHECK_FAILURE(args->get_PermissionKind(&kind));
        CHECK_FAILURE(args->get_IsUserInitiated(&userInitiated));
        CHECK_FAILURE(args->get_Uri(&uri));

        COREWEBVIEW2_PERMISSION_STATE state;

        auto cached_key = std::make_tuple(std::wstring(uri.get()), kind, userInitiated);
        auto cached_permission = m_cached_permissions.find(cached_key);
        if (cached_permission != m_cached_permissions.end())
        {
            state = (cached_permission->second
                ? COREWEBVIEW2_PERMISSION_STATE_ALLOW
                : COREWEBVIEW2_PERMISSION_STATE_DENY);
        }
        else
        {
            std::wstring message = L"An iframe has requested device permission for ";
            message += SettingsComponent::NameOfPermissionKind(kind);
            message += L" to the website at ";
            message += uri.get();
            message += L"?\n\n";
            message += L"Do you want to grant permission?\n";
            message += (userInitiated
                ? L"This request came from a user gesture."
                : L"This request did not come from a user gesture.");

            int response = MessageBox(
                nullptr, message.c_str(), L"Permission Request",
                MB_YESNOCANCEL | MB_ICONWARNING);
            switch (response) {
                case IDYES:
                    m_cached_permissions[cached_key] = true;
                    state = COREWEBVIEW2_PERMISSION_STATE_ALLOW;
                    break;
                case IDNO:
                    m_cached_permissions[cached_key] = false;
                    state = COREWEBVIEW2_PERMISSION_STATE_DENY;
                    break;
                default:
                    state = COREWEBVIEW2_PERMISSION_STATE_DEFAULT;
                    break;
            }
        }

        CHECK_FAILURE(args->put_State(state));
        CHECK_FAILURE(deferral->Complete());
    });
    return S_OK;
}
//! [PermissionRequested1]

ScenarioIFrameDevicePermission::~ScenarioIFrameDevicePermission()
{
    if (m_frame3)
    {
        CHECK_FAILURE(m_frame3->remove_PermissionRequested(m_PermissionRequestedToken));
    }
    if (m_webView4)
    {
        CHECK_FAILURE(m_webView4->remove_FrameCreated(m_FrameCreatedToken));
    }
    m_cached_permissions.clear();
    CHECK_FAILURE(m_webView->remove_ContentLoading(m_ContentLoadingToken));
}
