// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

struct Toolbar
{
    HWND backWindow = nullptr;
    HWND forwardWindow = nullptr;
    HWND reloadWindow = nullptr;
    HWND cancelWindow = nullptr;
    HWND addressBarWindow = nullptr;
    HWND goWindow = nullptr;

    void Initialize(HWND mainWindow);
    void SetEnabled(bool enabled);
    // Returns remaining available area for the WebView
    RECT Resize(RECT availableBounds);
};
