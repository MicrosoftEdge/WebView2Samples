// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "stdafx.h"

#include "AppWindow.h"
#include "CheckFailure.h"
#include "ScenarioNonClientRegionSupport.h"

using namespace Microsoft::WRL;

static constexpr WCHAR c_samplePath[] = L"ScenarioNonClientRegionSupport.html";

ScenarioNonClientRegionSupport::ScenarioNonClientRegionSupport(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    m_sampleUri = m_appWindow->GetLocalUri(c_samplePath);

    wil::com_ptr<ICoreWebView2Controller> controller = appWindow->GetWebViewController();
    wil::com_ptr<ICoreWebView2CompositionController> compController =
        controller.try_query<ICoreWebView2CompositionController>();
    if (compController)
    {
        m_compController5 =
            compController.try_query<ICoreWebView2ExperimentalCompositionController5>();
    }

    CHECK_FAILURE(m_webView->get_Settings(&m_settings));

    m_experimentalSettings = m_settings.try_query<ICoreWebView2ExperimentalSettings8>();

    CHECK_FAILURE(m_webView->add_NavigationStarting(
        Callback<ICoreWebView2NavigationStartingEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args)
                -> HRESULT
            {
                wil::unique_cotaskmem_string uri;
                CHECK_FAILURE(args->get_Uri(&uri));
                CHECK_FEATURE_RETURN(m_experimentalSettings);

                BOOL enabled = 0;
                CHECK_FAILURE(
                    m_experimentalSettings->get_IsNonClientRegionSupportEnabled(&enabled));

                if (uri.get() == m_sampleUri && !enabled)
                {
                    CHECK_FAILURE(
                        m_experimentalSettings->put_IsNonClientRegionSupportEnabled(TRUE));
                    AddChangeListener();
                }
                else if (uri.get() != m_sampleUri && enabled)
                {
                    CHECK_FAILURE(
                        m_experimentalSettings->put_IsNonClientRegionSupportEnabled(FALSE));
                }
                return S_OK;
            })
            .Get(),
        &m_navigationStartingToken));

    // Turn off this scenario if we navigate away from the sample page
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
        &m_ContentLoadingToken));

    CHECK_FAILURE(m_webView->Navigate(m_sampleUri.c_str()));
}
//! [AddChangeListener]
void ScenarioNonClientRegionSupport::AddChangeListener()
{
    if (m_compController5)
    {
        CHECK_FAILURE(m_compController5->add_NonClientRegionChanged(
            Callback<ICoreWebView2ExperimentalNonClientRegionChangedEventHandler>(
                [this](
                    ICoreWebView2CompositionController* sender,
                    ICoreWebView2ExperimentalNonClientRegionChangedEventArgs* args) -> HRESULT
                {
                    COREWEBVIEW2_NON_CLIENT_REGION_KIND region =
                        COREWEBVIEW2_NON_CLIENT_REGION_KIND_NOWHERE;
                    args->get_RegionKind(&region);
                    wil::com_ptr<ICoreWebView2ExperimentalRegionRectCollectionView>
                        regionsCollection;
                    m_compController5->QueryNonClientRegion(region, &regionsCollection);
                    UINT32 count = 0;
                    regionsCollection->get_Count(&count);
                    RECT rect;
                    regionsCollection->GetValueAtIndex(0, &rect);
                    return S_OK;
                })
                .Get(),
            &m_nonClientRegionChanged));
    }
}
//! [AddChangeListener]
ScenarioNonClientRegionSupport::~ScenarioNonClientRegionSupport()
{
    CHECK_FAILURE(m_webView->remove_NavigationStarting(m_navigationStartingToken));
    CHECK_FAILURE(m_webView->remove_ContentLoading(m_ContentLoadingToken));
    if (m_compController5)
    {
        CHECK_FAILURE(
            m_compController5->remove_NonClientRegionChanged(m_nonClientRegionChanged));
    }
}