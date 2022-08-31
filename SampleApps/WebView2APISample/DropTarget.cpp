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
    ICoreWebView2CompositionController3* webViewCompositionController3)
{
    m_webViewCompositionController3 = webViewCompositionController3;
    m_viewComponent = viewComponent;
    m_window = window;
    ::RegisterDragDrop(m_window, this);
}

//! [DragEnter]
HRESULT DropTarget::DragEnter(
    IDataObject* dataObject, DWORD keyState, POINTL cursorPosition, DWORD* effect)
{
    POINT point = {cursorPosition.x, cursorPosition.y};
    // Convert the screen point to client coordinates add the WebView's offset.
    m_viewComponent->OffsetPointToWebView(&point);
    return m_webViewCompositionController3->DragEnter(dataObject, keyState, point, effect);
}
//! [DragEnter]

//! [DragOver]
HRESULT DropTarget::DragOver(DWORD keyState, POINTL cursorPosition, DWORD* effect)
{
    POINT point = {cursorPosition.x, cursorPosition.y};
    // Convert the screen point to client coordinates add the WebView's offset.
    // This returns whether the resultant point is over the WebView visual.
    m_viewComponent->OffsetPointToWebView(&point);
    return m_webViewCompositionController3->DragOver(keyState, point, effect);
}
//! [DragOver]

//! [DragLeave]
HRESULT DropTarget::DragLeave()
{
    return m_webViewCompositionController3->DragLeave();
}
//! [DragLeave]

//! [Drop]
HRESULT DropTarget::Drop(
    IDataObject* dataObject, DWORD keyState, POINTL cursorPosition, DWORD* effect)
{
    POINT point = {cursorPosition.x, cursorPosition.y};
    // Convert the screen point to client coordinates add the WebView's offset.
    // This returns whether the resultant point is over the WebView visual.
    m_viewComponent->OffsetPointToWebView(&point);
    return m_webViewCompositionController3->Drop(dataObject, keyState, point, effect);
}
//! [Drop]