// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include <DispatcherQueue.h>
#include <windows.ui.composition.interop.h>
#include <winrt/Windows.UI.Composition.Desktop.h>

#include "Appwindow.h"
#include "WebView2EnvironmentOptions.h"
#include "WebView2Experimental.h"

class CompositionHost
{
public:
    CompositionHost() = default;
    ~CompositionHost();
    void Initialize(AppWindow* appWindow);
    void OnMouseMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void SetBounds(RECT bounds);

private:
    void EnsureDispatcherQueue();
    void CreateDesktopWindowTarget(HWND window);
    void CreateCompositionRoot();
    void CreateWebViewVisual();
    void DestroyWinCompVisualTree();
    void AddElement();
    void UpdateVisual(POINT point, UINT message, WPARAM wParam);
    winrt::Windows::UI::Composition::ContainerVisual FindVisual(POINT point);
    void CreateVisuals();
    winrt::Windows::UI::Color RandomBlue();
    void SetWebViewVisualBounds();
    void ResizeAllVisuals();

    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2Controller> m_controller;
    wil::com_ptr<ICoreWebView2CompositionController> m_compositionController;

    winrt::Windows::UI::Composition::Compositor m_compositor{nullptr};
    winrt::Windows::System::DispatcherQueueController m_dispatcherQueueController{nullptr};
    winrt::Windows::UI::Composition::Desktop::DesktopWindowTarget m_target{nullptr};
    winrt::Windows::UI::Composition::ContainerVisual m_rootVisual{nullptr};
    winrt::Windows::UI::Composition::ContainerVisual m_webViewVisual{nullptr};
    RECT m_appBounds = {};
};
