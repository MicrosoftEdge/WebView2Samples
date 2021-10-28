// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include <ShellScalingApi.h>

class DpiUtil
{
public:
    static void SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT dpiAwarenessContext);
    static int GetDpiForWindow(HWND window);

private:
    static HMODULE GetUser32Module();
    static HMODULE GetShcoreModule();
    static PROCESS_DPI_AWARENESS ProcessDpiAwarenessFromDpiAwarenessContext(
        DPI_AWARENESS_CONTEXT dpiAwarenessContext);
};

