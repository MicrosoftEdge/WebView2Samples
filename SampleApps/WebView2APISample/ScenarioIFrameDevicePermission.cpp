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

    //! [PermissionRequested]
    m_webView4 = m_webView.try_query<ICoreWebView2_4>();
    if (m_webView4)
    {
        CHECK_FAILURE(m_webView4->add_FrameCreated(
            Callback<ICoreWebView2FrameCreatedEventHandler>(
                [this](ICoreWebView2* sender, ICoreWebView2FrameCreatedEventArgs* args) -> HRESULT {
                    wil::com_ptr<ICoreWebView2Frame> webviewFrame;
                    CHECK_FAILURE(args->get_Frame(&webviewFrame));

                    m_experimentalFrame3 = webviewFrame.try_query<ICoreWebView2ExperimentalFrame3>();
                    if (m_experimentalFrame3)
                    {
                        CHECK_FAILURE(m_experimentalFrame3->add_PermissionRequested(
                            Callback<ICoreWebView2ExperimentalFramePermissionRequestedEventHandler>(
                                [this](ICoreWebView2Frame* sender,
                                   ICoreWebView2ExperimentalPermissionRequestedEventArgs* args)
                                      -> HRESULT {
                                        // If we set Handled to true, then we will not fire the PermissionRequested
                                        // event off of the CoreWebView2.
                                        args->put_Handled(true);

                                        auto showDialog = [this, args]
                                        {
                                          COREWEBVIEW2_PERMISSION_KIND kind =
                                              COREWEBVIEW2_PERMISSION_KIND_UNKNOWN_PERMISSION;
                                          BOOL userInitiated = FALSE;
                                          wil::unique_cotaskmem_string uri;

                                          CHECK_FAILURE(args->get_PermissionKind(&kind));
                                          CHECK_FAILURE(args->get_IsUserInitiated(&userInitiated));
                                          CHECK_FAILURE(args->get_Uri(&uri));

                                          auto cached_key = std::make_tuple(
                                              std::wstring(uri.get()), kind, userInitiated);

                                          auto cached_permission =
                                              m_cached_permissions.find(cached_key);
                                          if (cached_permission != m_cached_permissions.end())
                                          {
                                              bool allow = cached_permission->second;
                                              if (allow)
                                              {
                                                  CHECK_FAILURE(args->put_State(
                                                      COREWEBVIEW2_PERMISSION_STATE_ALLOW));
                                              }
                                              else
                                              {
                                                  CHECK_FAILURE(args->put_State(
                                                      COREWEBVIEW2_PERMISSION_STATE_DENY));
                                              }
                                              return S_OK;
                                          }

                                          std::wstring message =
                                            L"An iframe has requested device permission for ";
                                          message += SettingsComponent::NameOfPermissionKind(kind);
                                          message += L" to the website at ";
                                          message += uri.get();
                                          message += L"?\n\n";
                                          message += L"Do you want to grant permission?\n";
                                          message +=
                                              (userInitiated
                                                  ? L"This request came from a user gesture."
                                                  : L"This request did not come from a user "
                                                    L"gesture.");

                                          int response = MessageBox(
                                              nullptr, message.c_str(), L"Permission Request",
                                              MB_YESNOCANCEL | MB_ICONWARNING);

                                          if (response == IDYES)
                                          {
                                              m_cached_permissions[cached_key] = true;
                                          }

                                          if (response == IDNO)
                                          {
                                              m_cached_permissions[cached_key] = false;
                                          }

                                          COREWEBVIEW2_PERMISSION_STATE state =
                                              response == IDYES
                                                  ? COREWEBVIEW2_PERMISSION_STATE_ALLOW
                                                  : response == IDNO ? COREWEBVIEW2_PERMISSION_STATE_DENY
                                                                  : COREWEBVIEW2_PERMISSION_STATE_DEFAULT;

                                          CHECK_FAILURE(args->put_State(state));
                                          return S_OK;
                                        };

                                        // Obtain a deferral for the event so that the CoreWebView2
                                        // doesn't examine the properties we set on the event args until
                                        // after we call the Complete method asynchronously later.
                                        wil::com_ptr<ICoreWebView2Deferral> deferral;
                                        CHECK_FAILURE(args->GetDeferral(&deferral));

                                        m_appWindow->RunAsync([deferral, showDialog]() {
                                            showDialog();
                                            CHECK_FAILURE(deferral->Complete());
                                        });

                                        return S_OK;
                            }).Get(),
                            &m_PermissionRequestedToken));
                    }

                    return S_OK;
            }).Get(),
            &m_FrameCreatedToken));
    }
    //! [PermissionRequested]

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

ScenarioIFrameDevicePermission::~ScenarioIFrameDevicePermission()
{
    if (m_experimentalFrame3)
    {
        CHECK_FAILURE(m_experimentalFrame3->remove_PermissionRequested(m_PermissionRequestedToken));
    }
    if (m_webView4)
    {
        CHECK_FAILURE(m_webView4->remove_FrameCreated(m_FrameCreatedToken));
    }
    m_cached_permissions.clear();
    CHECK_FAILURE(m_webView->remove_ContentLoading(m_ContentLoadingToken));
}