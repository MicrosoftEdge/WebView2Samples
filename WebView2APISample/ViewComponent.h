// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include "AppWindow.h"
#include "ComponentBase.h"

// This component handles commands from the View menu, as well as the ZoomFactorChanged
// event, and any functionality related to sizing and visibility of the WebView.
class ViewComponent : public ComponentBase
{
public:
    ViewComponent(AppWindow* appWindow);

    bool HandleWindowMessage(
        HWND hWnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam,
        LRESULT* result) override;

    void ToggleVisibility();
    void SetSizeRatio(float ratio);
    void SetZoomFactor(float zoom);
    void SetBounds(RECT bounds);
    void SetScale(float scale);
    void ShowWebViewBounds();
    void ShowWebViewZoom();

    ~ViewComponent() override;

private:
    void ResizeWebView();

    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2Host> m_host;
    wil::com_ptr<ICoreWebView2> m_webView;

    bool m_isVisible = true;
    float m_webViewRatio = 1.0f;
    float m_webViewZoomFactor = 1.0f;
    RECT m_webViewBounds = {};
    float m_webViewScale = 1.0f;

    EventRegistrationToken m_zoomFactorChangedToken = {};
};

