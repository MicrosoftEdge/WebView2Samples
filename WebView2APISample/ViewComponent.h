// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include "AppWindow.h"
#include "ComponentBase.h"
#include <dcomp.h>
#include <unordered_set>

// This component handles commands from the View menu, as well as the ZoomFactorChanged
// event, and any functionality related to sizing and visibility of the WebView.
// It also manages interaction with the compositor if running in windowless mode.
#include "WebView2APISample_WinCompHelper/WebView2APISample_WinCompHelper.h"

class ViewComponent : public ComponentBase
{
public:
    ViewComponent(
        AppWindow* appWindow,
        IDCompositionDevice* dcompDevice,
        IWinCompHelper* wincompHelper
    );

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

    enum class TransformType
    {
        kIdentity = 0,
        kScale2X,
        kRotate30Deg,
        kRotate60DegDiagonally
    };
    void SetTransform(TransformType transformType);

    ~ViewComponent() override;

private:
    void ResizeWebView();

    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2Controller> m_controller;
    wil::com_ptr<ICoreWebView2> m_webView;

    bool m_isVisible = true;
    float m_webViewRatio = 1.0f;
    float m_webViewZoomFactor = 1.0f;
    RECT m_webViewBounds = {};
    float m_webViewScale = 1.0f;

    EventRegistrationToken m_zoomFactorChangedToken = {};

    bool OnMouseMessage(UINT message, WPARAM wParam, LPARAM lParam);
    bool OnPointerMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void TrackMouseEvents(DWORD mouseTrackingFlags);

    wil::com_ptr<ICoreWebView2ExperimentalCompositionController> m_compositionController;
    bool m_isTrackingMouse = false;
    bool m_isCapturingMouse = false;
    std::unordered_set<UINT> m_pointerIdsStartingInWebView;
    D2D1_MATRIX_4X4_F m_webViewTransformMatrix = D2D1::Matrix4x4F();

    void BuildDCompTreeUsingVisual();
    void DestroyDCompVisualTree();

    wil::com_ptr<IDCompositionDevice> m_dcompDevice;
    wil::com_ptr<IDCompositionTarget> m_dcompHwndTarget;
    wil::com_ptr<IDCompositionVisual> m_dcompRootVisual;
    wil::com_ptr<IDCompositionVisual> m_dcompWebViewVisual;
    wil::com_ptr<IWinCompHelper> m_wincompHelper;
    wil::com_ptr<IUnknown> m_wincompVisual;

    EventRegistrationToken m_cursorChangedToken = {};
};
