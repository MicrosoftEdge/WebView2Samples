// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "ScenarioThrottlingControl.h"

#include "CheckFailure.h"
#include "ScriptComponent.h"

using namespace Microsoft::WRL;

ScenarioThrottlingControl::ScenarioThrottlingControl(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webview(appWindow->GetWebView())
{
#pragma region init_monitor
    m_appWindow->EnableHandlingNewWindowRequest(false);
    CHECK_FAILURE(m_webview->add_NewWindowRequested(
        Callback<ICoreWebView2NewWindowRequestedEventHandler>(
            [this,
             appWindow](ICoreWebView2* sender, ICoreWebView2NewWindowRequestedEventArgs* args)
            {
                wil::com_ptr<ICoreWebView2Deferral> deferral;
                CHECK_FAILURE(args->GetDeferral(&deferral));

                auto initCallback = [args, deferral, this]()
                {
                    m_monitorWebview = m_monitorAppWindow->GetWebView();
                    CHECK_FAILURE(args->put_NewWindow(m_monitorAppWindow->GetWebView()));
                    CHECK_FAILURE(args->put_Handled(TRUE));

                    CHECK_FAILURE(m_monitorWebview->add_WebMessageReceived(
                        Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                            [this](
                                ICoreWebView2* sender,
                                ICoreWebView2WebMessageReceivedEventArgs* args)
                            {
                                WebViewMessageReceived(args);
                                return S_OK;
                            })
                            .Get(),
                        &m_webMessageReceivedToken));

                    CHECK_FAILURE(deferral->Complete());
                };

                // passing "none" as uri as its a noinitialnavigation
                m_monitorAppWindow = new AppWindow(
                    IDM_CREATION_MODE_WINDOWED, appWindow->GetWebViewOption(), L"none",
                    appWindow->GetUserDataFolder(), false /* isMainWindow */,
                    initCallback /* webviewCreatedCallback */, true /* customWindowRect */,
                    {100, 100, 1720, 1360}, false /* shouldHaveToolbar */, true /* isPopup */);

                m_monitorAppWindow->SetOnAppWindowClosing(
                    [&] { m_appWindow->DeleteComponent(this); });

                return S_OK;
            })
            .Get(),
        nullptr));
#pragma endregion init_monitor

#pragma region init_target
    // Turn off this scenario if we navigate away from the sample page
    CHECK_FAILURE(m_webview->add_ContentLoading(
        Callback<ICoreWebView2ContentLoadingEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2ContentLoadingEventArgs* args) -> HRESULT
            {
                wil::unique_cotaskmem_string uri;
                sender->get_Source(&uri);
                if (uri.get() != m_targetUri)
                {
                    m_appWindow->DeleteComponent(this);
                }
                return S_OK;
            })
            .Get(),
        &m_contentLoadingToken));

    // While actual WebView creation in this sample app is outside the scope of
    // this component, calling these functions here is equivalent to calling
    // them upon WebView creation for the purposes of this feature. In a real
    // application, you would usually call these after WebView is created and
    // before the first navigation.
    OnWebViewCreated();
    SetupIsolatedFramesHandler();

    m_targetUri = m_appWindow->GetLocalUri(std::wstring(L"ScenarioThrottlingControl.html"));
    m_webview->Navigate(m_targetUri.c_str());
#pragma endregion init_target
}

// App is responsible for calling this function. An app would usually call this
// function from within the callback passed to
// CreateCoreWebView2Controller(WithOptions).
void ScenarioThrottlingControl::OnWebViewCreated()
{
    wil::com_ptr<ICoreWebView2Settings> settings;
    m_webview->get_Settings(&settings);
    auto settings9 = settings.try_query<ICoreWebView2ExperimentalSettings9>();

    // Store the default values from the WebView2 Runtime so we can restore them
    // later.
    CHECK_FAILURE(settings9->get_PreferredForegroundTimerWakeIntervalInMilliseconds(
        &m_defaultIntervalForeground));
    CHECK_FAILURE(settings9->get_PreferredBackgroundTimerWakeIntervalInMilliseconds(
        &m_defaultIntervalBackground));
    CHECK_FAILURE(settings9->get_PreferredIntensiveTimerWakeIntervalInMilliseconds(
        &m_defaultIntervalIntensive));
    CHECK_FAILURE(settings9->get_PreferredOverrideTimerWakeIntervalInMilliseconds(
        &m_defaultIntervalOverride));
}

// The primary use-case here is an app embedding 3rd party content and wanting
// to be able to independently limit the performance impact of it. Generally,
// that's something like "low battery, throttle more" or "giving the frame N
// seconds to run some logic, throttle less".
void ScenarioThrottlingControl::SetupIsolatedFramesHandler()
{
    auto webview4 = m_webview.try_query<ICoreWebView2_4>();
    CHECK_FEATURE_RETURN_EMPTY(webview4);

    // You can use the frame properties to determine whether it should be
    // marked to be throttled separately from main frame.
    CHECK_FAILURE(webview4->add_FrameCreated(
        Callback<ICoreWebView2FrameCreatedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2FrameCreatedEventArgs* args) -> HRESULT
            {
                wil::com_ptr<ICoreWebView2Frame> webviewFrame;
                CHECK_FAILURE(args->get_Frame(&webviewFrame));

                auto webviewExperimentalFrame7 =
                    webviewFrame.try_query<ICoreWebView2ExperimentalFrame7>();
                CHECK_FEATURE_RETURN_HRESULT(webviewExperimentalFrame7);

                wil::unique_cotaskmem_string name;
                CHECK_FAILURE(webviewFrame->get_Name(&name));
                if (wcscmp(name.get(), L"untrusted") == 0)
                {
                    CHECK_FAILURE(
                        webviewExperimentalFrame7->put_UseOverrideTimerWakeInterval(TRUE));
                }

                return S_OK;
            })
            .Get(),
        &m_frameCreatedToken));

    wil::com_ptr<ICoreWebView2Settings> settings;
    m_webview->get_Settings(&settings);
    auto settings9 = settings.try_query<ICoreWebView2ExperimentalSettings9>();

    // Restrict frames selected by the above callback to always match the default
    // timer interval for background frames.
    CHECK_FAILURE(settings9->put_PreferredOverrideTimerWakeIntervalInMilliseconds(
        m_defaultIntervalBackground));
}

void ScenarioThrottlingControl::WebViewMessageReceived(
    ICoreWebView2WebMessageReceivedEventArgs* args)
{
    // received command from monitor, handle
    wil::unique_cotaskmem_string json;
    CHECK_FAILURE(args->get_WebMessageAsJson(&json));

    auto command = GetJSONStringField(json.get(), L"command");
    if (command.compare(L"set-interval") == 0)
    {
        auto category = GetJSONStringField(json.get(), L"priority");
        auto interval = GetJSONStringField(json.get(), L"intervalMs");

        wil::com_ptr<ICoreWebView2Settings> settings;
        m_webview->get_Settings(&settings);
        auto settings9 = settings.try_query<ICoreWebView2ExperimentalSettings9>();
        CHECK_FEATURE_RETURN_EMPTY(settings9);

        if (category.compare(L"foreground") == 0)
        {
            CHECK_FAILURE(settings9->put_PreferredForegroundTimerWakeIntervalInMilliseconds(
                std::stoul(interval)));
        }
        else if (category.compare(L"background") == 0)
        {
            CHECK_FAILURE(settings9->put_PreferredBackgroundTimerWakeIntervalInMilliseconds(
                std::stoul(interval)));
        }
        else if (category.compare(L"untrusted") == 0)
        {
            CHECK_FAILURE(settings9->put_PreferredOverrideTimerWakeIntervalInMilliseconds(
                std::stoul(interval)));
        }
    }
    else if (command.compare(L"toggle-visibility") == 0)
    {
        BOOL visible;
        m_appWindow->GetWebViewController()->get_IsVisible(&visible);
        m_appWindow->GetWebViewController()->put_IsVisible(!visible);
    }
    else if (command.compare(L"scenario") == 0)
    {
        auto label = GetJSONStringField(json.get(), L"label");
        if (label.compare(L"interaction-throttle") == 0)
        {
            OnNoUserInteraction();
        }
        else if (label.compare(L"interaction-reset") == 0)
        {
            OnUserInteraction();
        }
        else if (label.compare(L"hidden-unthrottle") == 0)
        {
            HideWebView();
        }
        else if (label.compare(L"hidden-reset") == 0)
        {
            ShowWebView();
        }
    }
}

// This sample app calls this method when receiving a simulated event from its
// control monitor, but your app can decide how and when to go into this state.
void ScenarioThrottlingControl::OnNoUserInteraction()
{
    wil::com_ptr<ICoreWebView2Settings> settings;
    m_webview->get_Settings(&settings);
    auto settings9 = settings.try_query<ICoreWebView2ExperimentalSettings9>();
    CHECK_FEATURE_RETURN_EMPTY(settings9);

    // User is not interactive, keep webview visible but throttle foreground
    // timers to 500ms.
    CHECK_FAILURE(settings9->put_PreferredForegroundTimerWakeIntervalInMilliseconds(500));
}

void ScenarioThrottlingControl::OnUserInteraction()
{
    wil::com_ptr<ICoreWebView2Settings> settings;
    m_webview->get_Settings(&settings);
    auto settings9 = settings.try_query<ICoreWebView2ExperimentalSettings9>();
    CHECK_FEATURE_RETURN_EMPTY(settings9);

    // User is interactive again, set foreground timer interval back to its
    // default value.
    CHECK_FAILURE(settings9->put_PreferredForegroundTimerWakeIntervalInMilliseconds(
        m_defaultIntervalForeground));
}

// Prepares the WebView to go into hidden mode with no background timer
// throttling.
void ScenarioThrottlingControl::HideWebView()
{
    wil::com_ptr<ICoreWebView2Settings> settings;
    m_webview->get_Settings(&settings);
    auto settings9 = settings.try_query<ICoreWebView2ExperimentalSettings9>();
    CHECK_FEATURE_RETURN_EMPTY(settings9);

    // This WebView2 will remain hidden but needs to keep running timers.
    // Unthrottle background timers.
    CHECK_FAILURE(settings9->put_PreferredBackgroundTimerWakeIntervalInMilliseconds(0));
    // Effectively disable intensive throttling by overriding its timer interval.
    CHECK_FAILURE(settings9->put_PreferredIntensiveTimerWakeIntervalInMilliseconds(0));

    CHECK_FAILURE(m_appWindow->GetWebViewController()->put_IsVisible(FALSE));
}

// Shows the WebView and restores default throttling behavior.
void ScenarioThrottlingControl::ShowWebView()
{
    CHECK_FAILURE(m_appWindow->GetWebViewController()->put_IsVisible(TRUE));

    wil::com_ptr<ICoreWebView2Settings> settings;
    m_webview->get_Settings(&settings);
    auto settings9 = settings.try_query<ICoreWebView2ExperimentalSettings9>();
    CHECK_FEATURE_RETURN_EMPTY(settings9);

    CHECK_FAILURE(settings9->put_PreferredBackgroundTimerWakeIntervalInMilliseconds(
        m_defaultIntervalBackground));
    CHECK_FAILURE(settings9->put_PreferredIntensiveTimerWakeIntervalInMilliseconds(
        m_defaultIntervalIntensive));
}

ScenarioThrottlingControl::~ScenarioThrottlingControl()
{
    if (m_monitorAppWindow)
    {
        m_monitorAppWindow->SetOnAppWindowClosing(nullptr);
        ::PostMessage(m_monitorAppWindow->GetMainWindow(), WM_CLOSE, NULL, NULL);
        m_monitorAppWindow = nullptr;
    }
}
