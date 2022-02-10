// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "AudioComponent.h"
#include "CheckFailure.h"

using namespace Microsoft::WRL;

//! [IsDocumentPlayingAudioChanged] [IsDocumentPlayingAudio] [IsMutedChanged] [ToggleIsMuted]
AudioComponent::AudioComponent(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    auto webview2_8 = m_webView.try_query<ICoreWebView2_8>();
    if (webview2_8)
    {
        // Register a handler for the IsDocumentPlayingAudioChanged event.
        CHECK_FAILURE(webview2_8->add_IsDocumentPlayingAudioChanged(
            Callback<ICoreWebView2IsDocumentPlayingAudioChangedEventHandler>(
                [this, webview2_8](ICoreWebView2* sender, IUnknown* args) -> HRESULT
                {
                    UpdateTitleWithMuteState(webview2_8);
                    return S_OK;
                })
                .Get(),
            &m_isDocumentPlayingAudioChangedToken));

        // Register a handler for the IsMutedChanged event.
        CHECK_FAILURE(webview2_8->add_IsMutedChanged(
            Callback<ICoreWebView2IsMutedChangedEventHandler>(
                [this, webview2_8](ICoreWebView2* sender, IUnknown* args) -> HRESULT
                {
                    UpdateTitleWithMuteState(webview2_8);
                    return S_OK;
                })
                .Get(),
            &m_isMutedChangedToken));
    }
}

bool AudioComponent::HandleWindowMessage(
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result)
{
    if (message == WM_COMMAND)
    {
        switch (LOWORD(wParam))
        {
        case IDM_TOGGLE_MUTE_STATE:
            ToggleMuteState();
            return true;
        }
    }
    return false;
}

// Toggle the mute state of the current window and show a mute or unmute icon on the title bar
void AudioComponent::ToggleMuteState()
{
    auto webview2_8 = m_webView.try_query<ICoreWebView2_8>();
    if (webview2_8)
     {
         BOOL isMuted;
         CHECK_FAILURE(webview2_8->get_IsMuted(&isMuted));
         CHECK_FAILURE(webview2_8->put_IsMuted(!isMuted));
         std::wstring result = !isMuted ? L"WebView is Now Muted" : L"WebView is Now Unmuted";
         MessageBox(nullptr, result.c_str(), L"Mute State Changed", MB_OK);
     }
 }

 void AudioComponent::UpdateTitleWithMuteState(wil::com_ptr<ICoreWebView2_8> webview2_8)
 {
     BOOL isDocumentPlayingAudio;
     CHECK_FAILURE(webview2_8->get_IsDocumentPlayingAudio(&isDocumentPlayingAudio));

     BOOL isMuted;
     CHECK_FAILURE(webview2_8->get_IsMuted(&isMuted));

     wil::unique_cotaskmem_string title;
     CHECK_FAILURE(m_webView->get_DocumentTitle(&title));
     std::wstring result = L"";

     if (isDocumentPlayingAudio)
     {
         if (isMuted)
         {
             result = L"🔇 " + std::wstring(title.get());
         }
         else
         {
             result = L"🔊 " + std::wstring(title.get());
         }
     }
     else
     {
         result = std::wstring(title.get());
     }

     m_appWindow->SetDocumentTitle(result.c_str());
 }
 //! [IsDocumentPlayingAudioChanged] [IsDocumentPlayingAudio] [IsMutedChanged] [ToggleIsMuted]

 AudioComponent::~AudioComponent()
 {
     auto webview2_8 = m_webView.try_query<ICoreWebView2_8>();
     if (webview2_8)
     {
         CHECK_FAILURE(webview2_8->remove_IsDocumentPlayingAudioChanged(
             m_isDocumentPlayingAudioChangedToken));
         CHECK_FAILURE(webview2_8->remove_IsMutedChanged(m_isMutedChangedToken));
    }
}