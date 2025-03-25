// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "DiscardsComponent.h"

DiscardsComponent::DiscardsComponent(AppWindow* appWindow) : m_appWindow(appWindow)
{
    m_appWindowDiscardsView = new AppWindow(
        IDM_CREATION_MODE_WINDOWED, appWindow->GetWebViewOption(), L"edge://discards/graph",
        appWindow->GetUserDataFolder(), false /* isMainWindow */,
        nullptr /* webviewCreatedCallback */, true /* customWindowRect */, {100, 100, 900, 900},
        false /* shouldHaveToolbar */);

    m_appWindowDiscardsView->SetOnAppWindowClosing([&] { m_appWindow->DeleteComponent(this); });
}

DiscardsComponent::~DiscardsComponent()
{
    m_appWindowDiscardsView->SetOnAppWindowClosing(nullptr);
}
