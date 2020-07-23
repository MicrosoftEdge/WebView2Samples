// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include <vector>

class AppWindow;

class Toolbar
{
public:
    enum Item
    {
        Item_BackButton,
        Item_ForwardButton,
        Item_ReloadButton,
        Item_CancelButton,
        Item_AddressBar,
        Item_GoButton,
        Item_LAST,
    };
    Toolbar();
    ~Toolbar();
    void Initialize(AppWindow* appWindow);
    void SetItemEnabled(Item item, bool enabled);
    void DisableAllItems();
    HWND GetItem(Item item) const;
    const std::vector<HWND>& GetItems() const;
    // Returns remaining available area for the WebView
    RECT Resize(RECT availableBounds);
    void UpdateDpiAndTextScale();

private:
    int GetItemLogicalWidth(Item item, int clientLogicalWidth) const;
    void UpdateFont();

    AppWindow* m_appWindow = nullptr;
    HFONT m_font = nullptr;

    std::vector<HWND> m_items;
};
