// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "stdafx.h"
#include <sstream>

#include "CheckFailure.h"

#include "ScenarioServiceWorkerPostMessageSetting.h"

using namespace Microsoft::WRL;

static constexpr WCHAR c_samplePath[] = L"scenario_sw_post_msg_scope/index2.html";

ScenarioServiceWorkerPostMessageSetting::ScenarioServiceWorkerPostMessageSetting(
    AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    SetupEventsOnWebview();
}

ScenarioServiceWorkerPostMessageSetting::~ScenarioServiceWorkerPostMessageSetting()
{
    UnregisterAllServiceWorkers();
    m_webView->remove_ContentLoading(m_contentLoadingToken);
    m_webView->remove_WebMessageReceived(m_webMessageReceivedToken);
    if (m_serviceWorkerManager)
    {
        m_serviceWorkerManager->remove_ServiceWorkerRegistered(m_serviceWorkerRegisteredToken);
    }
}

void ScenarioServiceWorkerPostMessageSetting::UnregisterAllServiceWorkers()
{
    m_webView->ExecuteScript(
        L"navigator.serviceWorker.getRegistrations().then(function(registrations) {"
        L"  for(let registration of registrations) {"
        L"    registration.unregister();"
        L"  }"
        L"});",
        nullptr);
}

void ScenarioServiceWorkerPostMessageSetting::ToggleServiceWorkerJsApiSetting()
{
    // Unregister all the existing service workers before toggling the setting.
    // So that the setting can be applied to new service workers.
    UnregisterAllServiceWorkers();
    auto webView2_13 = m_webView.try_query<ICoreWebView2_13>();
    CHECK_FEATURE_RETURN_EMPTY(webView2_13);

    wil::com_ptr<ICoreWebView2Profile> webView2Profile;
    CHECK_FAILURE(webView2_13->get_Profile(&webView2Profile));
    auto webViewExperimentalProfile15 =
        webView2Profile.try_query<ICoreWebView2ExperimentalProfile15>();

    if (webViewExperimentalProfile15)
    {
        BOOL isEnabled;
        //! [AreWebViewScriptApisEnabledForServiceWorkers]
        CHECK_FAILURE(
            webViewExperimentalProfile15->get_AreWebViewScriptApisEnabledForServiceWorkers(
                &isEnabled));
        CHECK_FAILURE(
            webViewExperimentalProfile15->put_AreWebViewScriptApisEnabledForServiceWorkers(
                !isEnabled));
        //! [AreWebViewScriptApisEnabledForServiceWorkers]

        MessageBox(
            nullptr,
            (std::wstring(L"Service Worker JS API setting has been ") +
             (!isEnabled ? L"enabled." : L"disabled."))
                .c_str(),
            L"Service Worker JS API Setting", MB_OK);
    }

    // Setup events on webview2 to listen message from service worker
    // main thread.
    m_sampleUri = m_appWindow->GetLocalUri(c_samplePath);
    CHECK_FAILURE(m_webView->Navigate(m_sampleUri.c_str()));
}

void ScenarioServiceWorkerPostMessageSetting::SetupEventsOnWebview()
{

    // Turn off this scenario if we navigate away from the sample page.
    CHECK_FAILURE(m_webView->add_ContentLoading(
        Callback<ICoreWebView2ContentLoadingEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2ContentLoadingEventArgs* args) -> HRESULT
            {
                wil::unique_cotaskmem_string uri;
                sender->get_Source(&uri);
                if (uri.get() != m_sampleUri)
                {
                    m_appWindow->DeleteComponent(this);
                }
                return S_OK;
            })
            .Get(),
        &m_contentLoadingToken));

    // Setup WebMessageReceived event to receive message from main thread.
    m_webView->add_WebMessageReceived(
        Callback<ICoreWebView2WebMessageReceivedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args)
                -> HRESULT
            {
                wil::unique_cotaskmem_string message;
                CHECK_FAILURE(args->TryGetWebMessageAsString(&message));

                std::wstring msgStr = message.get();
                if (msgStr == L"MessageFromMainThread")
                {
                    std::wstringstream message{};
                    message << L"Message: " << std::endl
                            << L"Service Worker Message from Main thread. Service worker "
                               L"direct messaging disabled."
                            << std::endl;
                    m_appWindow->AsyncMessageBox(message.str(), L"Message from Service Worker");
                }
                return S_OK;
            })
            .Get(),
        &m_webMessageReceivedToken);

    // Get ServiceWorkerManager from profile and setup events to listen to service worker post
    // messages.
    auto webView2_13 = m_webView.try_query<ICoreWebView2_13>();
    CHECK_FEATURE_RETURN_EMPTY(webView2_13);

    wil::com_ptr<ICoreWebView2Profile> webView2Profile;
    CHECK_FAILURE(webView2_13->get_Profile(&webView2Profile));
    auto webViewExperimentalProfile13 =
        webView2Profile.try_query<ICoreWebView2ExperimentalProfile13>();
    CHECK_FEATURE_RETURN_EMPTY(webViewExperimentalProfile13);
    CHECK_FAILURE(
        webViewExperimentalProfile13->get_ServiceWorkerManager(&m_serviceWorkerManager));

    CHECK_FAILURE(m_serviceWorkerManager->add_ServiceWorkerRegistered(
        Callback<ICoreWebView2ExperimentalServiceWorkerRegisteredEventHandler>(
            [this](
                ICoreWebView2ExperimentalServiceWorkerManager* sender,
                ICoreWebView2ExperimentalServiceWorkerRegisteredEventArgs* args)
            {
                wil::com_ptr<ICoreWebView2ExperimentalServiceWorkerRegistration>
                    serviceWorkerRegistration;
                CHECK_FAILURE(args->get_ServiceWorkerRegistration(&serviceWorkerRegistration));

                if (serviceWorkerRegistration)
                {
                    wil::com_ptr<ICoreWebView2ExperimentalServiceWorker> serviceWorker;
                    CHECK_FAILURE(
                        serviceWorkerRegistration->get_ActiveServiceWorker(&serviceWorker));

                    if (serviceWorker)
                    {
                        SetupEventsOnServiceWorker(serviceWorker);
                    }
                    else
                    {
                        CHECK_FAILURE(serviceWorkerRegistration->add_ServiceWorkerActivated(
                            Callback<
                                ICoreWebView2ExperimentalServiceWorkerActivatedEventHandler>(
                                [this](
                                    ICoreWebView2ExperimentalServiceWorkerRegistration* sender,
                                    ICoreWebView2ExperimentalServiceWorkerActivatedEventArgs*
                                        args) -> HRESULT
                                {
                                    wil::com_ptr<ICoreWebView2ExperimentalServiceWorker>
                                        serviceWorker;
                                    CHECK_FAILURE(
                                        args->get_ActiveServiceWorker(&serviceWorker));
                                    SetupEventsOnServiceWorker(serviceWorker);

                                    return S_OK;
                                })
                                .Get(),
                            nullptr));
                    }
                }

                return S_OK;
            })
            .Get(),
        &m_serviceWorkerRegisteredToken));
}

void ScenarioServiceWorkerPostMessageSetting::SetupEventsOnServiceWorker(
    wil::com_ptr<ICoreWebView2ExperimentalServiceWorker> serviceWorker)
{
    serviceWorker->add_WebMessageReceived(
        Callback<ICoreWebView2ExperimentalServiceWorkerWebMessageReceivedEventHandler>(
            [this](
                ICoreWebView2ExperimentalServiceWorker* sender,
                ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT
            {
                wil::unique_cotaskmem_string messageRaw;
                CHECK_FAILURE(args->TryGetWebMessageAsString(&messageRaw));
                std::wstring messageFromWorker = messageRaw.get();

                std::wstringstream message{};
                message << L"Message: " << std::endl << messageFromWorker << std::endl;
                m_appWindow->AsyncMessageBox(message.str(), L"Message from Service Worker");

                return S_OK;
            })
            .Get(),
        nullptr);
}
