// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "ScenarioPermissionManagement.h"

#include "App.h"
#include "CheckFailure.h"
#include "PermissionDialog.h"
#include "resource.h"

using namespace Microsoft::WRL;

std::wstring PermissionStateToString(COREWEBVIEW2_PERMISSION_STATE state)
{
    switch (state)
    {
    case COREWEBVIEW2_PERMISSION_STATE_ALLOW:
        return L"allow";
    case COREWEBVIEW2_PERMISSION_STATE_DENY:
        return L"deny";
    default:
        return L"default";
    }
}
std::wstring PermissionKindToString(COREWEBVIEW2_PERMISSION_KIND type)
{
    switch (type)
    {
    case COREWEBVIEW2_PERMISSION_KIND_MULTIPLE_AUTOMATIC_DOWNLOADS:
        return L"auto downloads";
    case COREWEBVIEW2_PERMISSION_KIND_AUTOPLAY:
        return L"autoplay";
    case COREWEBVIEW2_PERMISSION_KIND_CAMERA:
        return L"camera";
    case COREWEBVIEW2_PERMISSION_KIND_CLIPBOARD_READ:
        return L"clipboard read";
    case COREWEBVIEW2_PERMISSION_KIND_FILE_READ_WRITE:
        return L"file read and write";
    case COREWEBVIEW2_PERMISSION_KIND_GEOLOCATION:
        return L"geolocation";
    case COREWEBVIEW2_PERMISSION_KIND_LOCAL_FONTS:
        return L"local fonts";
    case COREWEBVIEW2_PERMISSION_KIND_MICROPHONE:
        return L"mic";
    case COREWEBVIEW2_PERMISSION_KIND_NOTIFICATIONS:
        return L"notifications";
    case COREWEBVIEW2_PERMISSION_KIND_OTHER_SENSORS:
        return L"other sensors";
    case COREWEBVIEW2_PERMISSION_KIND_MIDI_SYSTEM_EXCLUSIVE_MESSAGES:
        return L"midi sysex";
    case COREWEBVIEW2_PERMISSION_KIND_WINDOW_MANAGEMENT:
        return L"window management";
    default:
        return L"unknown";
    }
}

static constexpr WCHAR c_samplePath[] = L"ScenarioPermissionManagement.html";

std::vector<COREWEBVIEW2_PERMISSION_STATE> permissionStates{
    COREWEBVIEW2_PERMISSION_STATE_ALLOW, COREWEBVIEW2_PERMISSION_STATE_DENY,
    COREWEBVIEW2_PERMISSION_STATE_DEFAULT};

std::vector<COREWEBVIEW2_PERMISSION_KIND> permissionKinds{
    COREWEBVIEW2_PERMISSION_KIND_MULTIPLE_AUTOMATIC_DOWNLOADS,
    COREWEBVIEW2_PERMISSION_KIND_AUTOPLAY,
    COREWEBVIEW2_PERMISSION_KIND_CAMERA,
    COREWEBVIEW2_PERMISSION_KIND_CLIPBOARD_READ,
    COREWEBVIEW2_PERMISSION_KIND_FILE_READ_WRITE,
    COREWEBVIEW2_PERMISSION_KIND_GEOLOCATION,
    COREWEBVIEW2_PERMISSION_KIND_LOCAL_FONTS,
    COREWEBVIEW2_PERMISSION_KIND_MICROPHONE,
    COREWEBVIEW2_PERMISSION_KIND_NOTIFICATIONS,
    COREWEBVIEW2_PERMISSION_KIND_OTHER_SENSORS,
    COREWEBVIEW2_PERMISSION_KIND_MIDI_SYSTEM_EXCLUSIVE_MESSAGES,
    COREWEBVIEW2_PERMISSION_KIND_WINDOW_MANAGEMENT};

ScenarioPermissionManagement::ScenarioPermissionManagement(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    m_sampleUri = m_appWindow->GetLocalUri(c_samplePath);
    auto webView2_13 = m_webView.try_query<ICoreWebView2_13>();
    wil::com_ptr<ICoreWebView2Profile> profile;
    if (!webView2_13)
        return;
    CHECK_FAILURE(webView2_13->get_Profile(&profile));
    m_webViewProfile4 = profile.try_query<ICoreWebView2Profile4>();
    if (!m_webViewProfile4)
        return;

    //! [GetNonDefaultPermissionSettings]
    CHECK_FAILURE(webView2_13->add_DOMContentLoaded(
        Callback<ICoreWebView2DOMContentLoadedEventHandler>(
            [this](
                ICoreWebView2* sender, ICoreWebView2DOMContentLoadedEventArgs* args) -> HRESULT
            {
                wil::unique_cotaskmem_string source;
                CHECK_FAILURE(sender->get_Source(&source));
                // Permission management APIs are only used on the app's
                // permission management page.
                if (source.get() != m_sampleUri)
                {
                    return S_OK;
                }
                // Get all the nondefault permissions and post them to the
                // app's permission management page. The permission management
                // page can present a list of custom permissions set for this
                // profile and let the end user modify them.
                CHECK_FAILURE(m_webViewProfile4->GetNonDefaultPermissionSettings(
                    Callback<ICoreWebView2GetNonDefaultPermissionSettingsCompletedHandler>(
                        [this, sender](
                            HRESULT code,
                            ICoreWebView2PermissionSettingCollectionView* collectionView)
                            -> HRESULT
                        {
                            UINT32 count;
                            collectionView->get_Count(&count);
                            for (UINT32 i = 0; i < count; i++)
                            {
                                wil::com_ptr<ICoreWebView2PermissionSetting> setting;
                                CHECK_FAILURE(collectionView->GetValueAtIndex(i, &setting));
                                COREWEBVIEW2_PERMISSION_KIND kind;
                                CHECK_FAILURE(setting->get_PermissionKind(&kind));
                                std::wstring kind_string = PermissionKindToString(kind);
                                COREWEBVIEW2_PERMISSION_STATE state;
                                CHECK_FAILURE(setting->get_PermissionState(&state));
                                wil::unique_cotaskmem_string origin;
                                CHECK_FAILURE(setting->get_PermissionOrigin(&origin));
                                std::wstring state_string = PermissionStateToString(state);
                                std::wstring reply = L"{\"PermissionSetting\": \"" +
                                                     kind_string + L", " + origin.get() +
                                                     L", " + state_string + L"\"}";
                                CHECK_FAILURE(sender->PostWebMessageAsJson(reply.c_str()));
                            }
                            return S_OK;
                        })
                        .Get()));
                return S_OK;
            })
            .Get(),
        &m_DOMContentLoadedToken));
    //! [GetNonDefaultPermissionSettings]

    // Called when the user wants to change permission state from the custom
    // permission management page.
    CHECK_FAILURE(m_webView->add_WebMessageReceived(
        Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args)
                -> HRESULT
            {
                wil::unique_cotaskmem_string source;
                CHECK_FAILURE(args->get_Source(&source));
                if (source.get() != m_sampleUri)
                {
                    return S_OK;
                }
                wil::unique_cotaskmem_string message;
                CHECK_FAILURE(args->TryGetWebMessageAsString(&message));
                if (wcscmp(message.get(), L"SetPermission") == 0)
                {
                    m_appWindow->RunAsync([this] { ShowSetPermissionDialog(); });
                }
                return S_OK;
            })
            .Get(),
        &m_webMessageReceivedToken));
}

//! [SetPermissionState]
// The app's permission management page can provide a way for the end user to
// change permissions. For example, a button on the page can trigger a dialog,
// where the user specifies the desired permission kind, origin, and state.
void ScenarioPermissionManagement::ShowSetPermissionDialog()
{
    PermissionDialog dialog(m_appWindow->GetMainWindow(), permissionKinds, permissionStates);
    if (dialog.confirmed && m_webViewProfile4)
    {
        // Example: m_webViewProfile4->SetPermissionState(
        //    COREWEBVIEW2_PERMISSION_KIND_GEOLOCATION,
        //    L"https://example.com",
        //    COREWEBVIEW2_PERMISSION_STATE_DENY
        //    SetPermissionStateCallback);
        CHECK_FAILURE(m_webViewProfile4->SetPermissionState(
            dialog.kind, dialog.origin.c_str(), dialog.state,
            Callback<ICoreWebView2SetPermissionStateCompletedHandler>(
                [this](HRESULT error) -> HRESULT
                {
                    m_webView->Reload();
                    return S_OK;
                })
                .Get()));
    }
}
//! [SetPermissionState]

bool ScenarioPermissionManagement::HandleWindowMessage(
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result)
{
    if (message == WM_COMMAND)
    {
        switch (LOWORD(wParam))
        {
        case IDM_PERMISSION_MANAGEMENT:
            NavigateToPermissionManager();
            return true;
        }
    }
    return false;
}

void ScenarioPermissionManagement::NavigateToPermissionManager()
{
    CHECK_FAILURE(m_webView->Navigate(m_sampleUri.c_str()));
}

ScenarioPermissionManagement::~ScenarioPermissionManagement()
{
    if (!m_webViewProfile4)
        return;
    if (m_webView2)
    {
        CHECK_FAILURE(m_webView2->remove_DOMContentLoaded(m_DOMContentLoadedToken));
    }
    CHECK_FAILURE(m_webView->remove_WebMessageReceived(m_webMessageReceivedToken));
}
