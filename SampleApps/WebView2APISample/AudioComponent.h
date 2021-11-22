// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "AppWindow.h"
#include "ComponentBase.h"

// This component handles commands from the Audio menu.
class AudioComponent : public ComponentBase
{
public:
    AudioComponent(AppWindow* appWindow);

    bool HandleWindowMessage(
        HWND hWnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam,
        LRESULT* result) override;

    void ToggleMuteState();
    void UpdateTitleWithMuteState(wil::com_ptr<ICoreWebView2_8> webview2_8);

    ~AudioComponent() override;

private:
    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2> m_webView;

    EventRegistrationToken m_isDocumentPlayingAudioChangedToken = {};
    EventRegistrationToken m_isMutedChangedToken = {};
};


