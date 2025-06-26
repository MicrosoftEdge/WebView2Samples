// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"
#include <sstream>

#include "CheckFailure.h"

#include "ScenarioServiceWorkerPostMessage.h"

using namespace Microsoft::WRL;

static constexpr WCHAR c_samplePath[] = L"scenario_sw_post_msg_scope/index.html";

ScenarioServiceWorkerPostMessage::ScenarioServiceWorkerPostMessage(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    CreateServiceWorkerManager();
    SetupEventsOnWebview();
}

void ScenarioServiceWorkerPostMessage::CreateServiceWorkerManager()
{
    auto webView2_13 = m_webView.try_query<ICoreWebView2_13>();
    CHECK_FEATURE_RETURN_EMPTY(webView2_13);

    wil::com_ptr<ICoreWebView2Profile> webView2Profile;
    CHECK_FAILURE(webView2_13->get_Profile(&webView2Profile));
    auto webViewExperimentalProfile13 =
        webView2Profile.try_query<ICoreWebView2ExperimentalProfile13>();
    CHECK_FEATURE_RETURN_EMPTY(webViewExperimentalProfile13);
    CHECK_FAILURE(
        webViewExperimentalProfile13->get_ServiceWorkerManager(&m_serviceWorkerManager));
}

void ScenarioServiceWorkerPostMessage::SetupEventsOnWebview()
{
    if (!m_serviceWorkerManager)
    {
        return;
    }

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
                    wil::unique_cotaskmem_string scopeUri;
                    CHECK_FAILURE(serviceWorkerRegistration->get_ScopeUri(&scopeUri));
                    std::wstring scopeUriStr(scopeUri.get());

                    wil::com_ptr<ICoreWebView2ExperimentalServiceWorker> serviceWorker;
                    CHECK_FAILURE(
                        serviceWorkerRegistration->get_ActiveServiceWorker(&serviceWorker));

                    if (serviceWorker)
                    {
                        SetupEventsOnServiceWorker(serviceWorker);
                        AddToCache(L"img.jpg", serviceWorker);
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
                                    AddToCache(L"img.jpg", serviceWorker);

                                    return S_OK;
                                })
                                .Get(),
                            nullptr));
                    }

                    m_appWindow->AsyncMessageBox(scopeUriStr, L"Service worker is registered");
                }

                return S_OK;
            })
            .Get(),
        &m_serviceWorkerRegisteredToken));

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

    m_sampleUri = m_appWindow->GetLocalUri(c_samplePath);
    CHECK_FAILURE(m_webView->Navigate(m_sampleUri.c_str()));
}

void ScenarioServiceWorkerPostMessage::SetupEventsOnServiceWorker(
    wil::com_ptr<ICoreWebView2ExperimentalServiceWorker> serviceWorker)
{
    //! [WebMessageReceived]
    serviceWorker->add_WebMessageReceived(
        Callback<ICoreWebView2ExperimentalServiceWorkerWebMessageReceivedEventHandler>(
            [this](
                ICoreWebView2ExperimentalServiceWorker* sender,
                ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT
            {
                wil::unique_cotaskmem_string scriptUri;
                CHECK_FAILURE(args->get_Source(&scriptUri));

                wil::unique_cotaskmem_string messageRaw;
                CHECK_FAILURE(args->TryGetWebMessageAsString(&messageRaw));
                std::wstring messageFromWorker = messageRaw.get();

                std::wstringstream message{};
                message << L"Service Worker: " << std::endl << scriptUri.get() << std::endl;
                message << std::endl;
                message << L"Message: " << std::endl << messageFromWorker << std::endl;
                m_appWindow->AsyncMessageBox(message.str(), L"Message from Service Worker");

                return S_OK;
            })
            .Get(),
        nullptr);
    //! [WebMessageReceived]
}

void ScenarioServiceWorkerPostMessage::AddToCache(
    std::wstring url, wil::com_ptr<ICoreWebView2ExperimentalServiceWorker> serviceWorker)
{
    //! [PostWebMessageAsJson]
    std::wstring msg = L"{\"command\":\"ADD_TO_CACHE\",\"url\":\"" + url + L"\"}";
    serviceWorker->PostWebMessageAsJson(msg.c_str());
    //! [PostWebMessageAsJson]
}

ScenarioServiceWorkerPostMessage::~ScenarioServiceWorkerPostMessage()
{
    if (m_serviceWorkerManager)
    {
        m_serviceWorkerManager->remove_ServiceWorkerRegistered(m_serviceWorkerRegisteredToken);
    }

    m_webView->remove_ContentLoading(m_contentLoadingToken);
}
