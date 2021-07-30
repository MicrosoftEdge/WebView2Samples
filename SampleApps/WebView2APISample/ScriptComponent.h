// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBVIEW_WIN_API_SAMPLE_SCRIPT_COMPONENT_H
#define THIRD_PARTY_WEBVIEW_WIN_API_SAMPLE_SCRIPT_COMPONENT_H

#include "stdafx.h"

#include <map>
#include <set>
#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"

// This component handles commands from the Script menu.
class ScriptComponent : public ComponentBase
{
public:
    ScriptComponent(AppWindow* appWindow);

    bool HandleWindowMessage(
        HWND hWnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam,
        LRESULT* result) override;

    void InjectScript();
    void AddInitializeScript();
    void RemoveInitializeScript();
    void SendStringWebMessage();
    void SendJsonWebMessage();
    void SubscribeToCdpEvent();
    void CallCdpMethod();
    void AddComObject();
    void OpenTaskManagerWindow();
    ~ScriptComponent() override;

private:
    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2> m_webView;

    std::wstring m_lastInitializeScriptId;
    std::map<std::wstring, EventRegistrationToken> m_devToolsProtocolEventReceivedTokenMap;
};

#endif
