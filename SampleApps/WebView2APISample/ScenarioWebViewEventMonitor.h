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
    // Because WebResourceRequested fires so much more often than
    // all other events, we default to it off and it is configurable.
    void EnableWebResourceRequestedEvent(bool enable);
    // Send information about an event to the event view.
    void PostEventMessage(std::wstring messageAsJson);

    // The event view displays the events and their details.
    AppWindow* m_appWindowEventView;
    wil::com_ptr<ICoreWebView2> m_webviewEventView;
    // The URI of the HTML document that displays the events.
    std::wstring m_sampleUri;

    // The event source objects fire the events.
    AppWindow* m_appWindowEventSource;
    wil::com_ptr<ICoreWebView2> m_webviewEventSource;

    // The events we register on the event source
    EventRegistrationToken m_frameNavigationStartingToken = {};
    EventRegistrationToken m_navigationStartingToken = {};
    EventRegistrationToken m_sourceChangedToken = {};
    EventRegistrationToken m_contentLoadingToken = {};
    EventRegistrationToken m_historyChangedToken = {};
    EventRegistrationToken m_navigationCompletedToken = {};
    EventRegistrationToken m_documentTitleChangedToken = {};
    EventRegistrationToken m_webMessageReceivedToken = {};
    EventRegistrationToken m_webResourceRequestedToken = {};
    EventRegistrationToken m_newWindowRequestedToken = {};

    // This event is registered with the event viewer so they
    // can communicate back to us for toggling the WebResourceRequested
    // event.
    EventRegistrationToken m_eventViewWebMessageReceivedToken = {};
};
