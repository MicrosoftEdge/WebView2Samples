// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include "stdafx.h"

#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"

// This component demonstrates custom data partition management for child windows.
// It shows how to:
// 1. Create child windows and capture them via NewWindowRequested event
// 2. Apply custom data partitions to child windows only (not parent)
// 3. Create blob URLs from images in the parent window
// 4. Test blob URL accessibility across partition boundaries
//
// Key pattern: In NewWindowRequested handler:
//   1. Get deferral
//   2. Create a native window for the popup
//   3. Create a WebView controller for that window
//   4. In controller-created callback, provide args->put_NewWindow(childWebView)
//   5. Complete deferral
class ScenarioCustomDataPartition : public ComponentBase
{
public:
    ScenarioCustomDataPartition(AppWindow* appWindow);
    ~ScenarioCustomDataPartition() override;
    static LRESULT CALLBACK ChildWndProcStatic(
        HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2> m_webView;
    std::wstring m_sampleUri;
    EventRegistrationToken m_contentLoadingToken = {};
    EventRegistrationToken m_webMessageReceivedToken = {};
    EventRegistrationToken m_newWindowRequestedToken = {};
    EventRegistrationToken m_childWebMessageReceivedToken = {};

    // Track a single child popup window for partition management
    HWND m_childWindow = nullptr;
    wil::com_ptr<ICoreWebView2Controller> m_childController;
    wil::com_ptr<ICoreWebView2> m_childWebView;

    // Event handlers
    LRESULT ChildWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static bool EnsureChildWindowClassRegistered();
    void CloseChildWindow();
    void ResizeChildWebView();

    void RegisterEventHandlers();
    void HandleWebMessage(const std::wstring& message);
    void SetPartitionOnLastChild(const std::wstring& partitionId);
    void CheckCurrentPartition();
    void SendStatusToUI(const std::wstring& status);
    void SendStatusToChildUI(const std::wstring& status);
};
