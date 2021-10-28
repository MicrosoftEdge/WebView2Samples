// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "CustomStatusBar.h"
#include "AppWindow.h"


CustomStatusBar::CustomStatusBar()
{
}

CustomStatusBar::~CustomStatusBar()
{
}

void CustomStatusBar::Initialize(AppWindow* appWindow)
{
    m_appWindow = appWindow;
    HWND mainWindow = m_appWindow->GetMainWindow();

    int width = 600;
    int height = 70;
    int x = 50;
    int y = 50;


    m_statusBarWindow = CreateWindow(
        L"Edit", L"", WS_CHILD | WS_BORDER | ES_READONLY, x, y, width, height, mainWindow,
        nullptr, nullptr, 0);

    BringWindowToTop(m_statusBarWindow);
}

void CustomStatusBar::Show(std::wstring value)
{
    SetWindowTextW(m_statusBarWindow, value.c_str());
    ShowWindow(m_statusBarWindow, SW_NORMAL);
}

void CustomStatusBar::Hide()
{
    SetWindowTextW(m_statusBarWindow, L"");
    ShowWindow(m_statusBarWindow, SW_HIDE);
}