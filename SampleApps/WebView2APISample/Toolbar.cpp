// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "Toolbar.h"

#include "AppWindow.h"
#include "resource.h"

Toolbar::Toolbar() : m_items{ Item_LAST } {}

Toolbar::~Toolbar()
{
    if (m_font)
    {
        DeleteObject(m_font);
    }
}

void Toolbar::Initialize(AppWindow* appWindow)
{
    m_appWindow = appWindow;
    HWND mainWindow = m_appWindow->GetMainWindow();

    m_items[Item_BackButton] = CreateWindow(
        L"button", L"Back", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP, 0, 0, 0, 0,
        mainWindow, (HMENU)IDE_BACK, nullptr, 0);
    m_items[Item_ForwardButton] = CreateWindow(
        L"button", L"Forward", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP, 0, 0, 0, 0,
        mainWindow, (HMENU)IDE_FORWARD, nullptr, 0);
    m_items[Item_ReloadButton] = CreateWindow(
        L"button", L"Reload", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP, 0, 0, 0, 0,
        mainWindow, (HMENU)IDE_ADDRESSBAR_RELOAD, nullptr, 0);
    m_items[Item_CancelButton] = CreateWindow(
        L"button", L"Cancel", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP, 0, 0, 0, 0,
        mainWindow, (HMENU)IDE_CANCEL, nullptr, 0);
    m_items[Item_AddressBar] = CreateWindow(
        L"edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP, 0, 0, 0, 0,
        mainWindow, (HMENU)IDE_ADDRESSBAR, nullptr, 0);
    m_items[Item_GoButton] = CreateWindow(
        L"button", L"Go", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | BS_DEFPUSHBUTTON,
        0, 0, 0, 0, mainWindow, (HMENU)IDE_ADDRESSBAR_GO, nullptr, 0);

    UpdateDpiAndTextScale();
    RECT availableBounds = { 0 };
    GetClientRect(mainWindow, &availableBounds);
    Resize(availableBounds);

    DisableAllItems();
}

void Toolbar::SetItemEnabled(Item item, bool enabled)
{
    EnableWindow(m_items[item], enabled);
}

void Toolbar::DisableAllItems()
{
    for (HWND hwnd : m_items)
    {
        EnableWindow(hwnd, FALSE);
    }
}

HWND Toolbar::GetItem(Item item) const
{
    return m_items[item];
}

const std::vector<HWND>& Toolbar::GetItems() const
{
    return m_items;
}

RECT Toolbar::Resize(RECT availableBounds)
{
    const int clientWidth = availableBounds.right - availableBounds.left;
    const int clientHeight = availableBounds.bottom - availableBounds.top;
    const float dpiScale = float(m_appWindow->GetDpiScale());
    const int clientLogicalWidth = int(float(clientWidth) / dpiScale);
    const int itemHeight = int(32 * dpiScale);

    int nextOffsetX = 0;

    for (Item item = Item_BackButton; item < Item_LAST; item = Item(item + 1))
    {
        int itemWidth = int(GetItemLogicalWidth(item, clientLogicalWidth) * dpiScale);
        SetWindowPos(m_items[item], nullptr, nextOffsetX, 0, itemWidth, itemHeight,
            SWP_NOZORDER | SWP_NOACTIVATE);
        nextOffsetX += itemWidth;
    }

    return { 0, itemHeight, clientWidth, clientHeight };
}

void Toolbar::UpdateDpiAndTextScale()
{
    UpdateFont();
    for (Item item = Item_BackButton; item < Item_LAST; item = Item(item + 1))
    {
        SendMessage(m_items[item], WM_SETFONT, (WPARAM)m_font, FALSE);
    }
}

int Toolbar::GetItemLogicalWidth(Item item, int clientLogicalWidth) const
{
    static const int s_itemButtonLogicalWidth = 64;

    int itemLogicalWidth = 0;
    switch (item)
    {
    case Item_BackButton:
    case Item_ForwardButton:
    case Item_ReloadButton:
    case Item_CancelButton:
    case Item_GoButton:
        itemLogicalWidth = s_itemButtonLogicalWidth;
        break;
    case Item_AddressBar:
        itemLogicalWidth = clientLogicalWidth - s_itemButtonLogicalWidth * (Item_LAST - 1);
        break;
    default:
        FAIL_FAST();
    }
    return itemLogicalWidth;
}

void Toolbar::UpdateFont()
{
    static const WCHAR s_fontName[] = L"WebView2APISample Font";
    if (m_font)
    {
        DeleteObject(m_font);
    }
    LOGFONT logFont;
    GetObject(GetStockObject(SYSTEM_FONT), sizeof(LOGFONT), &logFont);
    double dpiScale = m_appWindow->GetDpiScale();
    double textScale = m_appWindow->GetTextScale();
    logFont.lfHeight *= LONG(dpiScale * textScale);
    logFont.lfWidth *= LONG(dpiScale * textScale);
    StringCchCopy(logFont.lfFaceName, ARRAYSIZE(logFont.lfFaceName), s_fontName);
    m_font = CreateFontIndirect(&logFont);
}
