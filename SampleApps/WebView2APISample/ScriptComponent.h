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

private:
    bool HandleWindowMessage(
        HWND hWnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam,
        LRESULT* result) override;

    void InjectScript();
    void InjectScriptInIFrame();
    void AddInitializeScript();
    void RemoveInitializeScript();
    void SendStringWebMessage();
    void SendJsonWebMessage();
    void SubscribeToCdpEvent();
    void CallCdpMethod();
    void HandleCDPTargets();
    void CallCdpMethodForSession();
    HRESULT CDPMethodCallback(HRESULT error, PCWSTR resultJson);
    void CollectHeapUsageViaCdp();
    void HandleHeapUsageResult(std::wstring targetInfo, PCWSTR resultJson);
    void AddComObject();
    void OpenTaskManagerWindow();
    void SendStringWebMessageIFrame();
    void SendJsonWebMessageIFrame();

    void AddSiteEmbeddingIFrame();
    void ExecuteScriptWithResult();
    ~ScriptComponent() override;
    void HandleIFrames();
    std::wstring IFramesToString();
    std::vector<wil::com_ptr<ICoreWebView2Frame>> m_frames;

    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2> m_webView;
    int m_siteEmbeddingIFrameCount = 0;

    std::wstring m_lastInitializeScriptId;
    std::map<std::wstring, EventRegistrationToken> m_devToolsProtocolEventReceivedTokenMap;
    EventRegistrationToken m_targetAttachedToken;
    EventRegistrationToken m_targetDetachedToken;
    EventRegistrationToken m_targetCreatedToken;
    EventRegistrationToken m_targetInfoChangedToken;
    EventRegistrationToken m_consoleAPICalledToken;
    // SessionId to TargetId map
    std::map<std::wstring, std::wstring> m_devToolsSessionMap;
    // TargetId to description label map, where label is "<target type>,<target url>".
    std::map<std::wstring, std::wstring> m_devToolsTargetLabelMap;
    int m_pendingHeapUsageCollectionCount = 0;
    std::wstringstream m_heapUsageResult;
};

#endif
