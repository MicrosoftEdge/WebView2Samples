// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

class AppWindow;

class CustomStatusBar
{
public:
    CustomStatusBar();
    ~CustomStatusBar();
    void Initialize(AppWindow* appWindow);
    void Show(std::wstring value);
    void Hide();

private:
    AppWindow* m_appWindow = nullptr;
    HWND m_statusBarWindow;
};
