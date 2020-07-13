// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// WebView2APISample_WinCompHelper.cpp : Defines the exported functions for the DLL.

#include "pch.h"
#include "WebView2APISample_WinCompHelper.h"
#include "CompositionHost.h"

HRESULT CreateWinCompHelper(IWinCompHelper** wincompHelper)
{
    auto wincompHelperInstance{ winrt::make<CompositionHost>() };
    wincompHelperInstance.as<IWinCompHelper>().copy_to(wincompHelper);
    return S_OK;
}

