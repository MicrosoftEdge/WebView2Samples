// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include <string>
#include "ComponentBase.h"

std::wstring WebErrorStatusToString(COREWEBVIEW2_WEB_ERROR_STATUS status);

// The event monitor examines events from the m_appWindowEventSource and
// m_webviewEventSource and displays the details of those events in
// m_appWindowEventView and m_webviewEventView.
class ScenarioWebViewEventMonitor : public ComponentBase
{
public:
    ScenarioWebViewEventMonitor(AppWindow* appWindowEventSource);
    ~ScenarioWebViewEventMonitor() override;

    void InitializeEventView(ICoreWebView2* webviewEventView);

private:
    void InitializeFrameEventView(wil::com_ptr<ICoreWebView2Frame> webviewFrame, int depth);
    // Because WebResourceRequested fires so much more often than
    // all other events, we default to it off and it is configurable.
    void EnableWebResourceRequestedEvent(bool enable);

    void EnableWebResourceResponseReceivedEvent(bool enable);
    // Send information about an event to the event view.
    void PostEventMessage(std::wstring messageAsJson);

    std::wstring InterruptReasonToString(const COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON interrupt_reason);

    // The event view displays the events and their details.
    AppWindow* m_appWindowEventView;
    wil::com_ptr<ICoreWebView2> m_webviewEventView;
    // The URI of the HTML document that displays the events.
    std::wstring m_sampleUri;

    // The event source objects fire the events.
    AppWindow* m_appWindowEventSource;
    wil::com_ptr<ICoreWebView2> m_webviewEventSource;
    wil::com_ptr<ICoreWebView2Controller> m_controllerEventSource;
    wil::com_ptr<ICoreWebView2_2> m_webviewEventSource2;
    wil::com_ptr<ICoreWebView2_4> m_webviewEventSource4;
    wil::com_ptr<ICoreWebView2_9> m_webViewEventSource9;

    // The events we register on the event sources
    EventRegistrationToken m_frameNavigationStartingToken = {};
    EventRegistrationToken m_frameNavigationCompletedToken = {};
    EventRegistrationToken m_navigationStartingToken = {};
    EventRegistrationToken m_sourceChangedToken = {};
    EventRegistrationToken m_contentLoadingToken = {};
    EventRegistrationToken m_historyChangedToken = {};
    EventRegistrationToken m_navigationCompletedToken = {};
    EventRegistrationToken m_DOMContentLoadedToken = {};
    EventRegistrationToken m_documentTitleChangedToken = {};
    EventRegistrationToken m_webMessageReceivedToken = {};
    EventRegistrationToken m_webResourceRequestedToken = {};
    EventRegistrationToken m_newWindowRequestedToken = {};
    EventRegistrationToken m_webResourceResponseReceivedToken = {};
    EventRegistrationToken m_downloadStartingToken = {};
    EventRegistrationToken m_stateChangedToken = {};
    EventRegistrationToken m_bytesReceivedChangedToken = {};
    EventRegistrationToken m_estimatedEndTimeChanged = {};
    EventRegistrationToken m_frameCreatedToken = {};
    EventRegistrationToken m_gotFocusToken = {};
    EventRegistrationToken m_lostFocusToken = {};
    EventRegistrationToken m_isDefaultDownloadDialogOpenChangedToken = {};
    EventRegistrationToken m_permissionRequestedToken = {};

    // This event is registered with the event viewer so they
    // can communicate back to us for toggling the WebResourceRequested
    // event.
    EventRegistrationToken m_eventViewWebMessageReceivedToken = {};
};
