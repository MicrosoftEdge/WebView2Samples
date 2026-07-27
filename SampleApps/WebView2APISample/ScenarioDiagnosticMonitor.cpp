// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "ScenarioDiagnosticMonitor.h"

#include <sstream>
#include <string>

#include "App.h"
#include "CheckFailure.h"
#include "TextInputDialog.h"
#include "Util.h"
#include "resource.h"

using namespace Microsoft::WRL;

ScenarioDiagnosticMonitor::ScenarioDiagnosticMonitor(AppWindow* appWindow)
    : m_appWindow(appWindow)
{
    wil::com_ptr<ICoreWebView2ExperimentalEnvironment16> env16;
    HRESULT hr = m_appWindow->GetWebViewEnvironment()->QueryInterface(IID_PPV_ARGS(&env16));
    if (FAILED(hr) || !env16)
    {
        MessageBox(
            m_appWindow->GetMainWindow(),
            L"ICoreWebView2ExperimentalEnvironment16 not available. "
            L"The Diagnostic API requires a newer WebView2 Runtime.",
            L"Diagnostic Monitor", MB_OK);
        return;
    }
    m_environment16 = env16;
}

ScenarioDiagnosticMonitor::~ScenarioDiagnosticMonitor()
{
    ClearAllMonitors();
}

void ScenarioDiagnosticMonitor::CreateMonitorWithDialog()
{
    if (!m_environment16)
    {
        return;
    }

    // Default monitor name.
    std::wstringstream defaultName;
    defaultName << L"Diagnostic Monitor " << m_nextMonitorId;

    // The filter dialog label is used both as a UI prompt and as the
    // lookup key in `dialog.results`, so keep it in a single constant.
    // The example matches the JSON schema documented for the
    // NETWORK_REQUEST category in environment.idl.
    static constexpr wchar_t kFilterLabel[] =
        L"Example filter:\r\n"
        L"{\r\n"
        L"  \"errorCode\":  [-105, -7],\r\n"
        L"  \"statusCode\": [404, 500],\r\n"
        L"  \"uriPattern\": [\"https://*.contoso.com/*\"],\r\n"
        L"  \"httpMethod\": [\"GET\", \"POST\"]\r\n"
        L"}\r\n"
        L"\r\n"
        L"Match any value within a field (OR).\r\n"
        L"All specified fields must match (AND).\r\n"
        L"Leave empty to receive all events.";

    // Show a single dialog with monitor name, category dropdown, and JSON filter.
    TextInputDialog dialog = TextInputDialog::Builder(
                                 m_appWindow->GetMainWindow(), L"Create Diagnostic Monitor",
                                 L"Configure the monitor and its filter")
                                 .AddTextArea(L"Monitor Name", defaultName.str(), false, 20, 24)
                                 .AddDropDown(L"Category", {L"NETWORK_REQUEST"}, 0)
                                 .AddTextArea(kFilterLabel, L"", false, 170, 80)
                                 .Build();

    if (!dialog.confirmed)
    {
        return;
    }

    // Read user inputs from the dialog results.
    std::wstring monitorName = defaultName.str();
    std::wstring jsonFilter;

    auto nameIt = dialog.results.find(L"Monitor Name");
    if (nameIt != dialog.results.end())
    {
        monitorName = std::get<TextArea>(nameIt->second).input;
    }

    auto filterIt = dialog.results.find(kFilterLabel);
    if (filterIt != dialog.results.end())
    {
        jsonFilter = std::get<TextArea>(filterIt->second).input;
    }

    //! [CreateDiagnosticMonitor]
    // Create the monitor.
    wil::com_ptr<ICoreWebView2ExperimentalDiagnosticMonitor> monitor;
    HRESULT hr = m_environment16->CreateDiagnosticMonitor(&monitor);
    if (FAILED(hr))
    {
        std::wstringstream ss;
        ss << L"CreateDiagnosticMonitor failed: 0x" << std::hex << hr;
        MessageBox(
            m_appWindow->GetMainWindow(), ss.str().c_str(), L"Diagnostic Monitor",
            MB_OK | MB_ICONERROR);
        return;
    }

    // Set filter for NETWORK_REQUEST category.
    hr = monitor->SetDiagnosticFilter(
        COREWEBVIEW2_DIAGNOSTIC_CATEGORY_NETWORK_REQUEST,
        jsonFilter.empty() ? L"" : jsonFilter.c_str());
    if (FAILED(hr))
    {
        std::wstringstream ss;
        ss << L"SetDiagnosticFilter failed: 0x" << std::hex << hr;
        MessageBox(
            m_appWindow->GetMainWindow(), ss.str().c_str(), L"Diagnostic Monitor",
            MB_OK | MB_ICONERROR);
        return;
    }

    // Register the event handler. Capture the monitor name for logging.
    std::wstring capturedName = monitorName;
    EventRegistrationToken token;
    CHECK_FAILURE(monitor->add_DiagnosticReceived(
        Callback<ICoreWebView2ExperimentalDiagnosticReceivedEventHandler>(
            [this, capturedName](
                ICoreWebView2ExperimentalDiagnosticMonitor* sender,
                ICoreWebView2ExperimentalDiagnosticReceivedEventArgs* args) -> HRESULT
            {
                COREWEBVIEW2_DIAGNOSTIC_CATEGORY category;
                CHECK_FAILURE(args->get_Category(&category));

                COREWEBVIEW2_DIAGNOSTIC_SCOPE scope;
                CHECK_FAILURE(args->get_Scope(&scope));

                INT64 timestamp = 0;
                CHECK_FAILURE(args->get_Timestamp(&timestamp));

                wil::unique_cotaskmem_string detailsJson;
                CHECK_FAILURE(args->get_DetailsAsJson(&detailsJson));

                m_totalEventCount++;

                // Pretty-print the JSON details for readability.
                std::wstring json = detailsJson.get();
                std::wstringstream pretty;
                int indent = 0;
                bool inString = false;
                wchar_t prevCh = 0;
                for (wchar_t ch : json)
                {
                    if (ch == L'"' && prevCh != L'\\')
                        inString = !inString;
                    if (inString)
                    {
                        pretty << ch;
                        prevCh = ch;
                        continue;
                    }
                    if (ch == L'{' || ch == L'[')
                    {
                        pretty << ch << L"\r\n";
                        indent += 2;
                        for (int i = 0; i < indent; ++i)
                            pretty << L' ';
                    }
                    else if (ch == L'}' || ch == L']')
                    {
                        pretty << L"\r\n";
                        indent -= 2;
                        for (int i = 0; i < indent; ++i)
                            pretty << L' ';
                        pretty << ch;
                    }
                    else if (ch == L',')
                    {
                        pretty << ch << L"\r\n";
                        for (int i = 0; i < indent; ++i)
                            pretty << L' ';
                    }
                    else
                    {
                        pretty << ch;
                    }
                    prevCh = ch;
                }

                std::wstringstream ss;
                ss << L"Event #" << m_totalEventCount << L"\r\n"
                   << L"Category: " << static_cast<int>(category) << L"  Scope: "
                   << static_cast<int>(scope) << L"  Timestamp (ms since UNIX epoch): "
                   << timestamp << L"\r\n\r\n"
                   << L"Details:\r\n"
                   << pretty.str();

                MessageBox(
                    m_appWindow->GetMainWindow(), ss.str().c_str(), capturedName.c_str(),
                    MB_OK | MB_ICONINFORMATION);

                return S_OK;
            })
            .Get(),
        &token));
    //! [CreateDiagnosticMonitor]

    m_monitors.push_back({monitorName, monitor, token});
    m_nextMonitorId++;

    std::wstringstream msg;
    msg << L"Created \"" << monitorName << L"\" with filter: "
        << (jsonFilter.empty() ? L"(accept all)" : jsonFilter);
    MessageBox(
        m_appWindow->GetMainWindow(), msg.str().c_str(), L"Diagnostic Monitor",
        MB_OK | MB_ICONINFORMATION);
}

void ScenarioDiagnosticMonitor::ClearAllMonitors()
{
    for (auto& entry : m_monitors)
    {
        if (entry.monitor)
        {
            CHECK_FAILURE(entry.monitor->remove_DiagnosticReceived(entry.token));
            CHECK_FAILURE(entry.monitor->Close());
        }
    }
    size_t count = m_monitors.size();
    m_monitors.clear();
    m_nextMonitorId = 1;
    m_totalEventCount = 0;

    if (count > 0)
    {
        std::wstringstream ss;
        ss << L"Cleared " << count << L" monitor(s).";
        MessageBox(
            m_appWindow->GetMainWindow(), ss.str().c_str(), L"Diagnostic Monitor",
            MB_OK | MB_ICONINFORMATION);
    }
}
