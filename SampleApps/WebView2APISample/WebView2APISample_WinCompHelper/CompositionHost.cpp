// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pch.h"

#include "CompositionHost.h"
#include <DispatcherQueue.h>
#include <windows.foundation.numerics.h>
#include <windows.ui.composition.interop.h>
#include <winrt/Windows.UI.Composition.Desktop.h>

using namespace winrt;
using namespace Windows::System;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Composition::Desktop;
using namespace Windows::Foundation::Numerics;

CompositionHost::CompositionHost() = default;
CompositionHost::~CompositionHost() = default;

HRESULT CompositionHost::CreateCompositor()
{
    if (m_compositor == nullptr)
    {
        EnsureDispatcherQueue();
        m_compositor = Compositor();
    }
    return S_OK;
}

HRESULT CompositionHost::BuildVisualTree(HWND hostWindow, IUnknown** visual)
{
    namespace abiComp = ABI::Windows::UI::Composition;
    namespace winrtComp = winrt::Windows::UI::Composition;

    if (m_compositor == nullptr)
    {
        return E_UNEXPECTED;
    }

    if (m_webViewVisual == nullptr)
    {
        auto interop = m_compositor.as<abiComp::Desktop::ICompositorDesktopInterop>();
        check_hresult(interop->CreateDesktopWindowTarget(hostWindow, false,
            reinterpret_cast<abiComp::Desktop::IDesktopWindowTarget**>(put_abi(m_hwndTarget))));

        m_rootVisual = m_compositor.CreateContainerVisual();
        m_hwndTarget.Root(m_rootVisual);

        m_webViewVisual = m_compositor.CreateContainerVisual();
        m_rootVisual.Children().InsertAtTop(m_webViewVisual);
    }

    m_webViewVisual.as<IUnknown>().copy_to(visual);

    return S_OK;
}

HRESULT CompositionHost::UpdateSizeAndPosition(POINT webViewOffset, SIZE webViewSize)
{
    if (m_rootVisual != nullptr)
    {
        namespace winrtComp = winrt::Windows::UI::Composition;
        namespace numerics = winrt::Windows::Foundation::Numerics;

        numerics::float2 size = {
            static_cast<float>(webViewSize.cx),
            static_cast<float>(webViewSize.cy) };
        m_rootVisual.Size(size);

        numerics::float3 offset = {
            static_cast<float>(webViewOffset.x),
            static_cast<float>(webViewOffset.y), 0.0f };
        m_rootVisual.Offset(offset);

        winrtComp::IInsetClip insetClip = m_compositor.CreateInsetClip();
        m_rootVisual.Clip(insetClip.as<winrtComp::CompositionClip>());
    }

    return S_OK;
}

HRESULT CompositionHost::ApplyMatrix(D2D1_MATRIX_4X4_F matrix)
{
    if (m_webViewVisual != nullptr)
    {
        m_webViewVisual.TransformMatrix(*reinterpret_cast<Windows::Foundation::Numerics::float4x4 *>(&matrix));
    }

    return S_OK;
}

HRESULT CompositionHost::DestroyVisualTree()
{
    if (m_webViewVisual != nullptr)
    {
        m_webViewVisual.Children().RemoveAll();
        m_webViewVisual = nullptr;

        m_rootVisual.Children().RemoveAll();
        m_rootVisual = nullptr;

        m_hwndTarget.Root(nullptr);
        m_hwndTarget = nullptr;
    }

    return S_OK;
}

void CompositionHost::EnsureDispatcherQueue()
{
    namespace abi = ABI::Windows::System;
    thread_local winrt::Windows::System::DispatcherQueueController dispatcherQueueController{ nullptr };

    if (dispatcherQueueController == nullptr)
    {
        DispatcherQueueOptions options
        {
            sizeof(DispatcherQueueOptions),
            DQTYPE_THREAD_CURRENT,
            DQTAT_COM_STA
        };

        Windows::System::DispatcherQueueController controller{ nullptr };
        check_hresult(CreateDispatcherQueueController(options, reinterpret_cast<abi::IDispatcherQueueController**>(put_abi(controller))));
        dispatcherQueueController = controller;
    }
}
