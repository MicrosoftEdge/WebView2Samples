// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "Toolbar.h"

#include "resource.h"

static const int s_addressBarHeight = 32;
static const int s_addressBarGoWidth = 64;
static const int s_cancelWidth = 64;
static const int s_backWidth = 64;
static const int s_forwardWidth = 64;
static const int s_reloadWidth = 64;

void Toolbar::Initialize(HWND mainWindow)
{
    RECT availableBounds = {0};
    GetClientRect(mainWindow, &availableBounds);

    backWindow = CreateWindow(
        L"button", L"Back", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP, 0, 0, s_backWidth,
        s_addressBarHeight, mainWindow, (HMENU)IDE_BACK, nullptr, 0);
    forwardWindow = CreateWindow(
        L"button", L"Forward", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP, s_backWidth, 0,
        s_forwardWidth, s_addressBarHeight, mainWindow, (HMENU)IDE_FORWARD, nullptr, 0);
    reloadWindow = CreateWindow(
        L"button", L"Reload", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP,
        s_backWidth + s_forwardWidth, 0, s_reloadWidth, s_addressBarHeight, mainWindow,
        (HMENU)IDE_ADDRESSBAR_RELOAD, nullptr, 0);
    cancelWindow = CreateWindow(
        L"button", L"Cancel", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP,
        s_backWidth + s_forwardWidth + s_reloadWidth, 0, s_cancelWidth, s_addressBarHeight,
        mainWindow, (HMENU)IDE_CANCEL, nullptr, 0);
    addressBarWindow = CreateWindow(
        L"edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP,
        s_backWidth + s_forwardWidth + s_reloadWidth + s_cancelWidth, 0,
        (availableBounds.right - availableBounds.left) -
            (s_addressBarGoWidth + s_backWidth + s_forwardWidth + s_reloadWidth +
             s_cancelWidth),
        s_addressBarHeight, mainWindow, (HMENU)IDE_ADDRESSBAR, nullptr, 0);
    goWindow = CreateWindow(
        L"button", L"Go", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | BS_DEFPUSHBUTTON,
        (availableBounds.right - availableBounds.left) - s_addressBarGoWidth, 0,
        s_addressBarGoWidth, s_addressBarHeight, mainWindow, (HMENU)IDE_ADDRESSBAR_GO, nullptr,
        0);
    SetEnabled(false);
}

void Toolbar::SetEnabled(bool enabled)
{
    EnableWindow(backWindow, enabled);
    EnableWindow(forwardWindow, enabled);
    EnableWindow(reloadWindow, enabled);
    EnableWindow(cancelWindow, enabled);
    EnableWindow(addressBarWindow, enabled);
    EnableWindow(goWindow, enabled);
}

RECT Toolbar::Resize(RECT availableBounds)
{
    const int clientWidth = availableBounds.right - availableBounds.left;
    const int clientHeight = availableBounds.bottom - availableBounds.top;

    SetWindowPos(backWindow, nullptr, 0, 0, s_backWidth, s_addressBarHeight, SWP_NOZORDER);
    SetWindowPos(
        forwardWindow, nullptr, s_backWidth, 0, s_forwardWidth, s_addressBarHeight,
        SWP_NOZORDER);
    SetWindowPos(
        reloadWindow, nullptr, s_backWidth + s_forwardWidth, 0, s_reloadWidth,
        s_addressBarHeight, SWP_NOZORDER);
    SetWindowPos(
        cancelWindow, nullptr, s_backWidth + s_forwardWidth + s_reloadWidth, 0, s_cancelWidth,
        s_addressBarHeight, SWP_NOZORDER);
    SetWindowPos(
        addressBarWindow, nullptr, s_backWidth + s_forwardWidth + s_reloadWidth + s_cancelWidth,
        0,
        (availableBounds.right - availableBounds.left) -
            (s_addressBarGoWidth + s_backWidth + s_forwardWidth + s_reloadWidth +
             s_cancelWidth),
        s_addressBarHeight, SWP_NOZORDER);
    SetWindowPos(
        goWindow, nullptr, clientWidth - s_addressBarGoWidth, 0, s_addressBarGoWidth,
        s_addressBarHeight, SWP_NOZORDER);
    return {0, s_addressBarHeight, clientWidth, clientHeight};
}
