// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"
#include <shellapi.h>
#include <shlwapi.h>
#include <sstream>
#include <string>
#include <windows.h>

#include "ScenarioNotificationReceived.h"

#include "App.h"
#include "CheckFailure.h"
#include "Util.h"
#include "resource.h"

using namespace Microsoft::WRL;

static constexpr WCHAR c_samplePath[] = L"ScenarioNotificationReceived.html";

ScenarioNotificationReceived::ScenarioNotificationReceived(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    m_sampleUri = m_appWindow->GetLocalUri(c_samplePath);
    m_webView2_24 = m_webView.try_query<ICoreWebView2_24>();
    if (!m_webView2_24)
        return;
    //! [NotificationReceived]
    // Register a handler for the NotificationReceived event.
    CHECK_FAILURE(m_webView2_24->add_NotificationReceived(
        Callback<ICoreWebView2NotificationReceivedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2NotificationReceivedEventArgs* args)
                -> HRESULT
            {
                // Block notifications from specific URIs and set Handled to
                // true so the the default notification UI will not be
                // shown by WebView2 either.
                CHECK_FAILURE(args->put_Handled(TRUE));

                Microsoft::WRL::ComPtr<ICoreWebView2Deferral> deferral;
                CHECK_FAILURE(args->GetDeferral(&deferral));
                wil::unique_cotaskmem_string origin;
                CHECK_FAILURE(args->get_SenderOrigin(&origin));
                std::wstring originString = origin.get();
                Microsoft::WRL::ComPtr<ICoreWebView2Notification> notification;
                CHECK_FAILURE(args->get_Notification(&notification));

                notification->add_CloseRequested(
                    Callback<ICoreWebView2NotificationCloseRequestedEventHandler>(
                        [this, &sender](
                            ICoreWebView2Notification* notification, IUnknown* args) -> HRESULT
                        {
                            // Remove the notification from the list of active
                            // notifications.
                            RemoveNotification(notification);
                            return S_OK;
                        })
                        .Get(),
                    &m_notificationCloseRequestedToken);

                m_appWindow->RunAsync(
                    [this,
                     notificationCom =
                         wil::make_com_ptr<ICoreWebView2Notification>(notification.Get()),
                     deferral, originString]()
                    {
                        ShowNotification(notificationCom.get(), originString);
                        deferral->Complete();
                    });

                return S_OK;
            })
            .Get(),
        &m_notificationReceivedToken));
    //! [NotificationReceived]
}

bool ScenarioNotificationReceived::HandleWindowMessage(
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result)
{
    if (message == WM_COMMAND)
    {
        switch (LOWORD(wParam))
        {
        case IDM_SCENARIO_NOTIFICATION:
            NavigateToNotificationPage();
            return true;
        }
    }
    return false;
}

void ScenarioNotificationReceived::ShowNotification(
    ICoreWebView2Notification* notification, std::wstring origin)
{
    ICoreWebView2* webView = m_webView.get();
    wil::unique_cotaskmem_string title;
    CHECK_FAILURE(notification->get_Title(&title));
    wil::unique_cotaskmem_string body;
    CHECK_FAILURE(notification->get_Body(&body));
    wil::unique_cotaskmem_string language;
    CHECK_FAILURE(notification->get_Language(&language));
    wil::unique_cotaskmem_string tag;
    CHECK_FAILURE(notification->get_Tag(&tag));
    wil::unique_cotaskmem_string iconUri;
    CHECK_FAILURE(notification->get_IconUri(&iconUri));
    wil::unique_cotaskmem_string badgeUri;
    CHECK_FAILURE(notification->get_BadgeUri(&badgeUri));
    wil::unique_cotaskmem_string imageUri;
    CHECK_FAILURE(notification->get_BodyImageUri(&imageUri));
    double timestamp;
    CHECK_FAILURE(notification->get_Timestamp(&timestamp));
    BOOL requireInteraction;
    CHECK_FAILURE(notification->get_RequiresInteraction(&requireInteraction));
    BOOL silent;
    CHECK_FAILURE(notification->get_IsSilent(&silent));
    BOOL renotify;
    CHECK_FAILURE(notification->get_ShouldRenotify(&renotify));

    std::wstringstream message;
    message << L"WebView2 has received an Notification: " << L"\n\t" << L"Sender origin: "
            << origin << L"\n\t" << L"Title: " << title.get() << L"\n\t" << L"Body: "
            << body.get() << L"\n\t" << L"Language: " << language.get() << L"\n\t" << L"Tag: "
            << tag.get() << L"\n\t" << L"IconUri: " << iconUri.get() << L"\n\t" << L"BadgeUri: "
            << badgeUri.get() << L"\n\t" << L"ImageUri: " << imageUri.get() << L"\n\t"
            << L"Timestamp: " << Util::UnixEpochToDateTime(timestamp) << L"\n\t"
            << L"RequireInteraction: " << ((!!requireInteraction) ? L"true" : L"false")
            << L"\n\t" << L"Silent: " << ((!!silent) ? L"true" : L"false") << L"\n\t"
            << L"Renotify: " << ((!!renotify) ? L"true" : L"false");

    message << std::endl;

    int response = MessageBox(nullptr, message.str().c_str(), title.get(), MB_OKCANCEL);
    notification->ReportShown();
    (response == IDOK) ? notification->ReportClicked() : notification->ReportClosed();
}

void ScenarioNotificationReceived::RemoveNotification(ICoreWebView2Notification* notification)
{
    // Close custom notification.

    // Unsubscribe from notification event.
    CHECK_FAILURE(notification->remove_CloseRequested(m_notificationCloseRequestedToken));
}

void ScenarioNotificationReceived::NavigateToNotificationPage()
{
    CHECK_FAILURE(m_webView->Navigate(m_sampleUri.c_str()));
}

ScenarioNotificationReceived::~ScenarioNotificationReceived()
{
    if (m_webView2_24)
    {
        CHECK_FAILURE(m_webView2_24->remove_NotificationReceived(m_notificationReceivedToken));
    }
}
