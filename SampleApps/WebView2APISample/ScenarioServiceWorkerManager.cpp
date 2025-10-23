// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"
#include <sstream>

#include "CheckFailure.h"
#include "TextInputDialog.h"

#include "ScenarioServiceWorkerManager.h"

using namespace Microsoft::WRL;

ScenarioServiceWorkerManager::ScenarioServiceWorkerManager(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    SetupEventsOnWebview();
}


void ScenarioServiceWorkerManager::SetupEventsOnWebview()
{
    //! [ServiceWorkerManager]
    auto webView2_13 = m_webView.try_query<ICoreWebView2_13>();
    CHECK_FEATURE_RETURN_EMPTY(webView2_13);

    wil::com_ptr<ICoreWebView2Profile> webView2Profile;
    CHECK_FAILURE(webView2_13->get_Profile(&webView2Profile));
    auto webViewExperimentalProfile13 =
        webView2Profile.try_query<ICoreWebView2ExperimentalProfile13>();
    CHECK_FEATURE_RETURN_EMPTY(webViewExperimentalProfile13);
    wil::com_ptr<ICoreWebView2ExperimentalServiceWorkerManager> serviceWorkerManager;
    CHECK_FAILURE(
        webViewExperimentalProfile13->get_ServiceWorkerManager(&serviceWorkerManager));
    //! [ServiceWorkerManager]

    //! [ServiceWorkerRegistered]
    CHECK_FAILURE(serviceWorkerManager->add_ServiceWorkerRegistered(
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

                    wil::unique_cotaskmem_string origin;
                    CHECK_FAILURE(serviceWorkerRegistration->get_Origin(&origin));

                    wil::unique_cotaskmem_string topLevelOrigin;
                    CHECK_FAILURE(
                        serviceWorkerRegistration->get_TopLevelOrigin(&topLevelOrigin));

                    // Subscribe to worker registration unregistering event
                    serviceWorkerRegistration->add_Unregistering(
                        Callback<
                            ICoreWebView2ExperimentalServiceWorkerRegistrationUnregisteringEventHandler>(
                            [this, scopeUriStr](
                                ICoreWebView2ExperimentalServiceWorkerRegistration* sender,
                                IUnknown* args) -> HRESULT
                            {
                                /*Cleanup on worker registration destruction*/
                                m_appWindow->AsyncMessageBox(
                                    scopeUriStr, L"Service worker is unregistered");
                                return S_OK;
                            })
                            .Get(),
                        nullptr);

                    wil::com_ptr<ICoreWebView2ExperimentalServiceWorker> serviceWorker;
                    CHECK_FAILURE(
                        serviceWorkerRegistration->get_ActiveServiceWorker(&serviceWorker));

                    if (serviceWorker)
                    {
                        wil::unique_cotaskmem_string scriptUri;
                        CHECK_FAILURE(serviceWorker->get_ScriptUri(&scriptUri));
                        std::wstring scriptUriStr(scriptUri.get());

                        // Subscribe to worker destroying event
                        serviceWorker->add_Destroying(
                            Callback<
                                ICoreWebView2ExperimentalServiceWorkerDestroyingEventHandler>(
                                [this, scriptUriStr](
                                    ICoreWebView2ExperimentalServiceWorker* sender,
                                    IUnknown* args) -> HRESULT
                                {
                                    /*Cleanup on worker destruction*/
                                    m_appWindow->AsyncMessageBox(
                                        scriptUriStr, L"Service worker is destroyed");
                                    return S_OK;
                                })
                                .Get(),
                            nullptr);
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
                                    wil::unique_cotaskmem_string scriptUri;
                                    CHECK_FAILURE(serviceWorker->get_ScriptUri(&scriptUri));
                                    std::wstring scriptUriStr(scriptUri.get());

                                    // Subscribe to worker destroying event
                                    serviceWorker->add_Destroying(
                                        Callback<
                                            ICoreWebView2ExperimentalServiceWorkerDestroyingEventHandler>(
                                            [this, scriptUriStr](
                                                ICoreWebView2ExperimentalServiceWorker* sender,
                                                IUnknown* args) -> HRESULT
                                            {
                                                /*Cleanup on worker destruction*/
                                                m_appWindow->AsyncMessageBox(
                                                    scriptUriStr,
                                                    L"Service worker is destroyed");
                                                return S_OK;
                                            })
                                            .Get(),
                                        nullptr);

                                    m_appWindow->AsyncMessageBox(
                                        L"Service worker is activated", L"Service worker");

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
    //! [ServiceWorkerRegistered]
}

void ScenarioServiceWorkerManager::GetAllServiceWorkerRegistrations()
{
    // CHECK_FEATURE_RETURN_EMPTY(serviceWorkerManager);
    // CHECK_FAILURE(serviceWorkerManager->GetServiceWorkerRegistrations(
    //     Callback<ICoreWebView2ExperimentalGetServiceWorkerRegistrationsCompletedHandler>(
    //         [this](
    //             HRESULT error, ICoreWebView2ExperimentalServiceWorkerRegistrationCollectionView*
    //                                workerRegistrationCollection) -> HRESULT
    //         {
    //             CHECK_FAILURE(error);
    //             UINT32 workersCount = 0;
    //             CHECK_FAILURE(workerRegistrationCollection->get_Count(&workersCount));

    //             std::wstringstream message{};
    //             message << L"Number of service workers registered: " << workersCount
    //                     << std::endl;

    //             for (UINT32 i = 0; i < workersCount; i++)
    //             {
    //                 ComPtr<ICoreWebView2ExperimentalServiceWorkerRegistration>
    //                     serviceWorkerRegistration;
    //                 CHECK_FAILURE(workerRegistrationCollection->GetValueAtIndex(
    //                     i, &serviceWorkerRegistration));

    //                 wil::unique_cotaskmem_string scopeUri;
    //                 CHECK_FAILURE(serviceWorkerRegistration->get_ScopeUri(&scopeUri));

    //                 wil::unique_cotaskmem_string origin;
    //                 CHECK_FAILURE(serviceWorkerRegistration->get_Origin(&origin));

    //                 wil::unique_cotaskmem_string topLevelOrigin;
    //                 CHECK_FAILURE(
    //                     serviceWorkerRegistration->get_TopLevelOrigin(&topLevelOrigin));

    //                 message << L"ScopeUri: " << scopeUri.get() << std::endl;
    //                 message << L"Origin: " << origin.get() << std::endl;
    //                 message << L"TopLevelOrigin: " << topLevelOrigin.get() << std::endl;
    //             }

    //             m_appWindow->AsyncMessageBox(
    //                 std::move(message.str()), L"Registered service workers");

    //             return S_OK;
    //         })
    //         .Get()));
}

void ScenarioServiceWorkerManager::GetServiceWorkerRegisteredForScope()
{
    // CHECK_FEATURE_RETURN_EMPTY(serviceWorkerManager);
    // TextInputDialog dialog(
    //     m_appWindow->GetMainWindow(), L"Service Worker", L"Scope:",
    //     L"Specify a scope to get the service worker", L"");

    // if (dialog.confirmed)
    // {
    //     std::wstring scope = dialog.input.c_str();
    //     CHECK_FAILURE(serviceWorkerManager->GetServiceWorkerRegistrationsForScope(
    //         scope.c_str(),
    //         Callback<ICoreWebView2ExperimentalGetServiceWorkerRegistrationsCompletedHandler>(
    //             [this, scope](
    //                 HRESULT error,
    //                 ICoreWebView2ExperimentalServiceWorkerRegistrationCollectionView*
    //                     workerRegistrationCollection) -> HRESULT
    //             {
    //                 CHECK_FAILURE(error);
    //                 UINT32 workersCount = 0;
    //                 CHECK_FAILURE(workerRegistrationCollection->get_Count(&workersCount));

    //                 std::wstringstream message{};
    //                 message << L"Number of service workers registered for the scope ("
    //                         << scope.c_str() << ") : " << workersCount << std::endl;

    //                 for (UINT32 i = 0; i < workersCount; i++)
    //                 {
    //                     ComPtr<ICoreWebView2ExperimentalServiceWorkerRegistration>
    //                         serviceWorkerRegistration;
    //                     CHECK_FAILURE(workerRegistrationCollection->GetValueAtIndex(
    //                         i, &serviceWorkerRegistration));

    //                     wil::unique_cotaskmem_string scopeUri;
    //                     CHECK_FAILURE(serviceWorkerRegistration->get_ScopeUri(&scopeUri));

    //                     wil::unique_cotaskmem_string origin;
    //                     CHECK_FAILURE(serviceWorkerRegistration->get_Origin(&origin));

    //                     wil::unique_cotaskmem_string topLevelOrigin;
    //                     CHECK_FAILURE(
    //                         serviceWorkerRegistration->get_TopLevelOrigin(&topLevelOrigin));

    //                     message << L"ScopeUri: " << scopeUri.get() << std::endl;
    //                     message << L"Origin: " << origin.get() << std::endl;
    //                     message << L"TopLevelOrigin: " << topLevelOrigin.get() << std::endl;
    //                 }

    //                 m_appWindow->AsyncMessageBox(
    //                     std::move(message.str()), L"Registered service workers for scope");

    //                 return S_OK;
    //             })
    //             .Get()));
    // }
}

ScenarioServiceWorkerManager::~ScenarioServiceWorkerManager()
{
    // if (serviceWorkerManager)
    // {
    //     serviceWorkerManager->remove_ServiceWorkerRegistered(m_serviceWorkerRegisteredToken);
    // }
}
