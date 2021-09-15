// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include "AppWindow.h"
#include "ComponentBase.h"
#include <commdlg.h>

// This component handles commands from the File menu, except for Exit.
// It also handles the DocumentTitleChanged event.
class FileComponent : public ComponentBase
{
public:
    FileComponent(AppWindow* appWindow);

    bool HandleWindowMessage(
        HWND hWnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam,
        LRESULT* result) override;

    void SaveScreenshot();
    void PrintToPdf(bool enableLandscape);
    bool IsPrintToPdfInProgress();

    ~FileComponent() override;

private:
    OPENFILENAME CreateOpenFileName(LPWSTR defaultName, LPCWSTR filter);

    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2> m_webView;
    bool m_printToPdfInProgress = false;
    bool m_enableLandscape = false;

    EventRegistrationToken m_documentTitleChangedToken = {};
};

