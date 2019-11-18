// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include <map>
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

    ~ScriptComponent() override;

private:
    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<IWebView2WebView4> m_webView;

    std::wstring m_lastInitializeScriptId;
    std::map<std::wstring, EventRegistrationToken> m_devToolsProtocolEventReceivedTokenMap;
};

