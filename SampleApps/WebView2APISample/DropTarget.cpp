// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "DropTarget.h"
#include "ViewComponent.h"
#include <ShlGuid.h>
#include <Shobjidl.h>

DropTarget::DropTarget() : m_window(nullptr) {}

DropTarget::~DropTarget()
{
}

void DropTarget::Init(
    HWND window, ViewComponent* viewComponent,
    ICoreWebView2ExperimentalCompositionController3* webViewExperimentalCompositionController3)
{
    m_webViewExperimentalCompositionController3 = webViewExperimentalCompositionController3;
    m_viewComponent = viewComponent;
    m_window = window;
    ::RegisterDragDrop(m_window, this);
}

wil::com_ptr<IDropTargetHelper> DropTarget::DropHelper() {
    if (!m_dropTargetHelper)
    {
        CoCreateInstance(
            CLSID_DragDropHelper, 0, CLSCTX_INPROC_SERVER, IID_IDropTargetHelper,
            reinterpret_cast<void**>(&m_dropTargetHelper));
    }
    return m_dropTargetHelper;
}

//! [DragEnter]
HRESULT DropTarget::DragEnter(
    IDataObject* dataObject, DWORD keyState, POINTL cursorPosition, DWORD* effect)
{
    POINT point = { cursorPosition.x, cursorPosition.y};
    // Tell the helper that we entered so it can update the drag image and get
    // the correct effect.
    wil::com_ptr<IDropTargetHelper> dropHelper = DropHelper();
    if (dropHelper)
    {
        dropHelper->DragEnter(GetHWND(), dataObject, &point, *effect);
    }

    // Convert the screen point to client coordinates add the WebView's offset.
    m_viewComponent->OffsetPointToWebView(&point);
    return m_webViewExperimentalCompositionController3->DragEnter(
        dataObject, keyState, point, effect);
}
//! [DragEnter]

//! [DragOver]
HRESULT DropTarget::DragOver(DWORD keyState, POINTL cursorPosition, DWORD* effect)
{
    POINT point = { cursorPosition.x, cursorPosition.y};
    // Tell the helper that we moved over it so it can update the drag image
    // and get the correct effect.
    wil::com_ptr<IDropTargetHelper> dropHelper = DropHelper();
    if (dropHelper)
    {
        dropHelper->DragOver(&point, *effect);
    }

    // Convert the screen point to client coordinates add the WebView's offset.
    // This returns whether the resultant point is over the WebView visual.
    m_viewComponent->OffsetPointToWebView(&point);
    return m_webViewExperimentalCompositionController3->DragOver(
        keyState, point, effect);
}
//! [DragOver]

//! [DragLeave]
HRESULT DropTarget::DragLeave()
{
    // Tell the helper that we moved out of it so it can update the drag image.
    wil::com_ptr<IDropTargetHelper> dropHelper = DropHelper();
    if (dropHelper)
    {
        dropHelper->DragLeave();
    }

    return m_webViewExperimentalCompositionController3->DragLeave();
}
//! [DragLeave]

//! [Drop]
HRESULT DropTarget::Drop(
    IDataObject* dataObject, DWORD keyState, POINTL cursorPosition, DWORD* effect)
{
    POINT point = { cursorPosition.x, cursorPosition.y};
    // Tell the helper that we dropped onto it so it can update the drag image and
    // get the correct effect.
    wil::com_ptr<IDropTargetHelper> dropHelper = DropHelper();
    if (dropHelper)
    {
        dropHelper->Drop(dataObject, &point, *effect);
    }

    // Convert the screen point to client coordinates add the WebView's offset.
    // This returns whether the resultant point is over the WebView visual.
    m_viewComponent->OffsetPointToWebView(&point);
    return m_webViewExperimentalCompositionController3->Drop(dataObject, keyState, point, effect);
}
//! [Drop]