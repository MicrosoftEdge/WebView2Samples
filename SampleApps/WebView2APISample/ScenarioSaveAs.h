// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"

class ScenarioSaveAs : public ComponentBase
{
public:
    ScenarioSaveAs(AppWindow* appWindow);
    bool ProgrammaticSaveAs();
    bool ToggleSilent();
    bool HandleWindowMessage(
        HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result) override;

private:
    ~ScenarioSaveAs() override;
    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2> m_webView;
    wil::com_ptr<ICoreWebView2_25> m_webView2_25;
    EventRegistrationToken m_saveAsUIShowingToken = {};
    bool m_silentSaveAs = false;
};

struct SaveAsDialog
{
    SaveAsDialog(HWND parent, std::initializer_list<COREWEBVIEW2_SAVE_AS_KIND> kinds);

    std::initializer_list<COREWEBVIEW2_SAVE_AS_KIND> kinds;
    std::wstring path;
    bool allowReplace = false;
    COREWEBVIEW2_SAVE_AS_KIND selectedKind = COREWEBVIEW2_SAVE_AS_KIND_DEFAULT;
    bool confirmed = false;
};