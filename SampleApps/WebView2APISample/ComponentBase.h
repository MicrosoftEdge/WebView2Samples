// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

// A component is meant to encapsulate all details required for a specific
// capability of the AppWindow, typically demonstrating usage of a WebView2 API.
//
// Component instances are owned by an AppWindow, which will give each of its
// components a chance to handle any messages it gets. AppWindow deletes all its
// components when WebView is closed.
//
// Components are meant to be created and registered by AppWindow itself,
// through `AppWindow::NewComponent<TComponent>(...)`. For example, the
// AppWindow might create a new component upon selection of a menu item by the
// user. Components typically take and keep a pointer to their owning AppWindow
// so they can control the WebView.
class ComponentBase
{
public:
    // *result defaults to 0
    virtual bool HandleWindowMessage(
        HWND hWnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam,
        LRESULT* result)
    {
        return false;
    }
    virtual ~ComponentBase() { }
};
