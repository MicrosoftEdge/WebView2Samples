// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.Composition.Desktop.h>
#include "WebView2APISample_WinCompHelper.h"

class CompositionHost : public winrt::implements<CompositionHost, IWinCompHelper>
{
public:
    CompositionHost();
    ~CompositionHost();

    // IWinCompHelper methods
    STDMETHODIMP CreateCompositor();
    STDMETHODIMP BuildVisualTree(HWND hostWindow, IUnknown** visual);
    STDMETHODIMP UpdateSizeAndPosition(POINT webViewOffset, SIZE webViewSize);
    STDMETHODIMP ApplyMatrix(D2D1_MATRIX_4X4_F matrix);
    STDMETHODIMP DestroyVisualTree();

private:
    void EnsureDispatcherQueue();

    winrt::Windows::UI::Composition::Compositor m_compositor{ nullptr };
    winrt::Windows::UI::Composition::Desktop::DesktopWindowTarget m_hwndTarget{ nullptr };
    winrt::Windows::UI::Composition::ContainerVisual m_rootVisual{ nullptr };
    winrt::Windows::UI::Composition::ContainerVisual m_webViewVisual{ nullptr };
};

