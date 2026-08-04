// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include "stdafx.h"

#include <string>
#include <vector>

#include "AppWindow.h"
#include "ComponentBase.h"

// Scenario that demonstrates the Diagnostic Monitor API.
// Supports creating multiple named monitors with per-category JSON filters,
// and clearing all monitors. Each monitor logs received events to a message
// box.
class ScenarioDiagnosticMonitor : public ComponentBase
{
public:
    ScenarioDiagnosticMonitor(AppWindow* appWindow);
    ~ScenarioDiagnosticMonitor() override;

    // Menu handlers called from AppWindow.
    void CreateMonitorWithDialog();
    void ClearAllMonitors();

private:
    struct MonitorEntry
    {
        std::wstring name;
        wil::com_ptr<ICoreWebView2ExperimentalDiagnosticMonitor> monitor;
        EventRegistrationToken token = {};
    };

    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2ExperimentalEnvironment16> m_environment16;

    std::vector<MonitorEntry> m_monitors;
    int m_nextMonitorId = 1;
    int m_totalEventCount = 0;
};
