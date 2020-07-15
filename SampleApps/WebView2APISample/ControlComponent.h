// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include <vector>

#include "AppWindow.h"
#include "ComponentBase.h"

// This component handles commands from the buttons and address bar, as well as keyboard
// input, tabbing, focus changing, and related events.
class ControlComponent : public ComponentBase
{
public:
    ControlComponent(AppWindow* appWindow, Toolbar* toolbar);

    bool HandleWindowMessage(
        HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result) override;

    void NavigateToAddressBar();

    void TabForwards(int currentIndex);
    void TabBackwards(int currentIndex);

    static LRESULT CALLBACK
    ChildWndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    bool HandleChildWindowMessage(
        HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result);

    ~ControlComponent() override;

private:
    AppWindow* m_appWindow;
    wil::com_ptr<ICoreWebView2Controller> m_controller;
    wil::com_ptr<ICoreWebView2> m_webView;
    Toolbar* m_toolbar;

    std::vector<std::pair<HWND, WNDPROC>> m_tabbableWindows;

    EventRegistrationToken m_navigationStartingToken = {};
    EventRegistrationToken m_sourceChangedToken = {};
    EventRegistrationToken m_historyChangedToken = {};
    EventRegistrationToken m_navigationCompletedToken = {};
    EventRegistrationToken m_moveFocusRequestedToken = {};
    EventRegistrationToken m_acceleratorKeyPressedToken = {};
    EventRegistrationToken m_frameNavigationCompletedToken = {};
};
