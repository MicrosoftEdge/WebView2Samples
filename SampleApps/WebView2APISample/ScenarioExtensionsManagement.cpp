// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "ScenarioExtensionsManagement.h"

#include "AppWindow.h"
#include "CheckFailure.h"
#include <string>

using namespace Microsoft::WRL;

static constexpr WCHAR c_samplePath[] = L"extensions/example-devtools-extension";
ScenarioExtensionsManagement::ScenarioExtensionsManagement(AppWindow* appWindow, bool offload)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    if (!offload)
    {
        InstallDefaultExtensions();
    }
    else
    {
        OffloadDefaultExtensionsIfExtraExtensionsInstalled();
    }
}

void ScenarioExtensionsManagement::InstallDefaultExtensions()
{
}
void ScenarioExtensionsManagement::OffloadDefaultExtensionsIfExtraExtensionsInstalled()
{
}