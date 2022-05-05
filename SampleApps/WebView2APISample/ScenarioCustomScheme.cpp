// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "stdafx.h"

#include "ScenarioCustomScheme.h"

#include "AppWindow.h"
#include "CheckFailure.h"

#include <Shlwapi.h>

using namespace Microsoft::WRL;

ScenarioCustomScheme::ScenarioCustomScheme(AppWindow* appWindow) : m_appWindow(appWindow)
{
}
ScenarioCustomScheme::~ScenarioCustomScheme()
{
    CHECK_FAILURE(
        m_appWindow->GetWebView()->remove_WebResourceRequested(m_webResourceRequestedToken));
}
