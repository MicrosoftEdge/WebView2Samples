// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "ViewComponent.h"

#include <d2d1helper.h>
#include <sstream>
#include <windowsx.h>

#include "CheckFailure.h"

using namespace Microsoft::WRL;

ViewComponent::ViewComponent(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    //! [ZoomFactorChanged]
    // Register a handler for the ZoomFactorChanged event.
    // This handler just announces the new level of zoom on the window's title bar.
    CHECK_FAILURE(m_webView->add_ZoomFactorChanged(
        Callback<IWebView2ZoomFactorChangedEventHandler>(
            [this](IWebView2WebView* sender, IUnknown* args) -> HRESULT {
                double zoomFactor;
                CHECK_FAILURE(sender->get_ZoomFactor(&zoomFactor));

                std::wstring message = L"WebView2APISample (Zoom: " +
                                       std::to_wstring(int(zoomFactor * 100)) + L"%)";
                SetWindowText(m_appWindow->GetMainWindow(), message.c_str());
                return S_OK;
            })
            .Get(),
        &m_zoomFactorChangedToken));
    //! [ZoomFactorChanged]

    ResizeWebView();
}

bool ViewComponent::HandleWindowMessage(
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result)
{
    if (message == WM_COMMAND)
    {
        switch (LOWORD(wParam))
        {
        case IDM_TOGGLE_VISIBILITY:
            ToggleVisibility();
            return true;
        case IDM_ZOOM_05:
            SetZoomFactor(0.5f);
            return true;
        case IDM_ZOOM_10:
            SetZoomFactor(1.0f);
            return true;
        case IDM_ZOOM_20:
            SetZoomFactor(2.0f);
            return true;
        case IDM_SIZE_25:
            SetSizeRatio(0.5f);
            return true;
        case IDM_SIZE_50:
            SetSizeRatio(0.7071f);
            return true;
        case IDM_SIZE_75:
            SetSizeRatio(0.866f);
            return true;
        case IDM_SIZE_100:
            SetSizeRatio(1.0f);
            return true;
        case IDM_GET_WEBVIEW_BOUNDS:
            ShowWebViewBounds();
            return true;
        }
    }
    //! [ToggleIsVisibleOnMinimize]
    if (message == WM_SYSCOMMAND)
    {
        if (wParam == SC_MINIMIZE)
        {
            // Hide the webview when the app window is minimized.
            m_webView->put_IsVisible(FALSE);
        }
        else if (wParam == SC_RESTORE)
        {
            // When the app window is restored, show the webview
            // (unless the user has toggle visibility off).
            if (m_isVisible)
            {
                m_webView->put_IsVisible(TRUE);
            }
        }
    }
    //! [ToggleIsVisibleOnMinimize]
    return false;
}
//! [ToggleIsVisible]
void ViewComponent::ToggleVisibility()
{
    BOOL visible;
    m_webView->get_IsVisible(&visible);
    m_isVisible = !visible;
    m_webView->put_IsVisible(m_isVisible);
}
//! [ToggleIsVisible]

void ViewComponent::SetSizeRatio(float ratio)
{
    m_webViewRatio = ratio;
    ResizeWebView();
}

void ViewComponent::SetZoomFactor(float zoom)
{
    m_webViewZoomFactor = zoom;
    m_webView->put_ZoomFactor(zoom);
}

void ViewComponent::SetBounds(RECT bounds)
{
    m_webViewBounds = bounds;
    ResizeWebView();
}

//! [ResizeWebView]
// Update the bounds of the WebView window to fit available space.
void ViewComponent::ResizeWebView()
{
    RECT desiredBounds = m_webViewBounds;
    desiredBounds.bottom = LONG(
        (m_webViewBounds.bottom - m_webViewBounds.top) * m_webViewRatio + m_webViewBounds.top);
    desiredBounds.right = LONG(
        (m_webViewBounds.right - m_webViewBounds.left) * m_webViewRatio + m_webViewBounds.left);

    m_webView->put_Bounds(desiredBounds);
}
//! [ResizeWebView]

// Show the current bounds of the WebView.
void ViewComponent::ShowWebViewBounds()
{
    RECT bounds;
    HRESULT result = m_webView->get_Bounds(&bounds);
    if (SUCCEEDED(result))
    {
        std::wstringstream message;
        message << L"Left:\t" << bounds.left << L"\n"
                << L"Top:\t" << bounds.top << L"\n"
                << L"Right:\t" << bounds.right << L"\n"
                << L"Bottom:\t" << bounds.bottom << std::endl;
        MessageBox(nullptr, message.str().c_str(), L"WebView Bounds", MB_OK);
    }
}

ViewComponent::~ViewComponent()
{
    m_webView->remove_ZoomFactorChanged(m_zoomFactorChangedToken);
}
