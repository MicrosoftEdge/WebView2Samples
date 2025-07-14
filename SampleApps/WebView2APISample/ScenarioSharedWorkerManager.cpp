// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"
#include <sstream>

#include "CheckFailure.h"

#include "ScenarioSharedWorkerManager.h"

using namespace Microsoft::WRL;

ScenarioSharedWorkerManager::ScenarioSharedWorkerManager(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    GetSharedWorkerManager();
    SetupEventsOnWebview();
}

void ScenarioSharedWorkerManager::GetSharedWorkerManager()
{
    //! [SharedWorkerManager]
    auto webView2_13 = m_webView.try_query<ICoreWebView2_13>();
    CHECK_FEATURE_RETURN_EMPTY(webView2_13);

    wil::com_ptr<ICoreWebView2Profile> webView2Profile;
    CHECK_FAILURE(webView2_13->get_Profile(&webView2Profile));
    auto webViewExperimentalProfile13 =
        webView2Profile.try_query<ICoreWebView2ExperimentalProfile13>();
    CHECK_FEATURE_RETURN_EMPTY(webViewExperimentalProfile13);
    CHECK_FAILURE(
        webViewExperimentalProfile13->get_SharedWorkerManager(&m_sharedWorkerManager));
    //! [SharedWorkerManager]
}

void ScenarioSharedWorkerManager::SetupEventsOnWebview()
{
    if (!m_sharedWorkerManager)
    {
        return;
    }

    //! [SharedWorkerCreated]
    CHECK_FAILURE(m_sharedWorkerManager->add_SharedWorkerCreated(
        Callback<ICoreWebView2ExperimentalSharedWorkerCreatedEventHandler>(
            [this](
                ICoreWebView2ExperimentalSharedWorkerManager* sender,
                ICoreWebView2ExperimentalSharedWorkerCreatedEventArgs* args)
            {
                wil::com_ptr<ICoreWebView2ExperimentalSharedWorker> sharedWorker;
                CHECK_FAILURE(args->get_Worker(&sharedWorker));

                wil::unique_cotaskmem_string scriptUri;
                CHECK_FAILURE(sharedWorker->get_ScriptUri(&scriptUri));

                std::wstring scriptUriStr(scriptUri.get());
                m_appWindow->AsyncMessageBox(scriptUriStr, L"Shared worker is created");

                // Subscribe to worker destroying event
                sharedWorker->add_Destroying(
                    Callback<ICoreWebView2ExperimentalSharedWorkerDestroyingEventHandler>(
                        [this, scriptUriStr](
                            ICoreWebView2ExperimentalSharedWorker* sender,
                            IUnknown* args) -> HRESULT
                        {
                            /*Cleanup on worker destruction*/
                            m_appWindow->AsyncMessageBox(
                                scriptUriStr, L"Shared worker is destroyed");
                            return S_OK;
                        })
                        .Get(),
                    nullptr);

                return S_OK;
            })
            .Get(),
        &m_sharedWorkerCreatedToken));
    //! [SharedWorkerCreated]
}

void ScenarioSharedWorkerManager::GetAllSharedWorkers()
{
    CHECK_FEATURE_RETURN_EMPTY(m_sharedWorkerManager);
    CHECK_FAILURE(m_sharedWorkerManager->GetSharedWorkers(
        Callback<ICoreWebView2ExperimentalGetSharedWorkersCompletedHandler>(
            [this](
                HRESULT error,
                ICoreWebView2ExperimentalSharedWorkerCollectionView* workersCollection)
                -> HRESULT
            {
                UINT32 workersCount = 0;
                CHECK_FAILURE(workersCollection->get_Count(&workersCount));

                std::wstringstream message{};
                message << L"Number of shared workers created: " << workersCount << std::endl;

                for (UINT32 i = 0; i < workersCount; i++)
                {
                    ComPtr<ICoreWebView2ExperimentalSharedWorker> sharedWorker;
                    CHECK_FAILURE(workersCollection->GetValueAtIndex(i, &sharedWorker));

                    wil::unique_cotaskmem_string scriptUri;
                    CHECK_FAILURE(sharedWorker->get_ScriptUri(&scriptUri));

                    wil::unique_cotaskmem_string origin;
                    CHECK_FAILURE(sharedWorker->get_Origin(&origin));

                    wil::unique_cotaskmem_string topLevelOrigin;
                    CHECK_FAILURE(sharedWorker->get_TopLevelOrigin(&topLevelOrigin));

                    message << L"ScriptUri: " << scriptUri.get();
                    message << L" Origin: " << origin.get();
                    message << L" TopLevelOrigin: " << topLevelOrigin.get();
                    message << std::endl;
                }

                m_appWindow->AsyncMessageBox(
                    std::move(message.str()), L"Get all shared workers");

                return S_OK;
            })
            .Get()));
}

ScenarioSharedWorkerManager::~ScenarioSharedWorkerManager()
{
    if (m_sharedWorkerManager)
    {
        m_sharedWorkerManager->remove_SharedWorkerCreated(m_sharedWorkerCreatedToken);
    }
}
