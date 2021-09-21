// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "AudioComponent.h"
#include "CheckFailure.h"

using namespace Microsoft::WRL;

//! [IsDocumentPlayingAudioChanged] [IsDocumentPlayingAudio] [ToggleIsMuted]
AudioComponent::AudioComponent(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    auto webviewExperimental9 = m_webView.try_query<ICoreWebView2Experimental9>();
    if (webviewExperimental9)
    {
        // Register a handler for the IsDocumentPlayingAudioChanged event.
        CHECK_FAILURE(webviewExperimental9->add_IsDocumentPlayingAudioChanged(
            Callback<ICoreWebView2ExperimentalIsDocumentPlayingAudioChangedEventHandler>(
                [this, webviewExperimental9](ICoreWebView2* sender, IUnknown* args) -> HRESULT {
                    UpdateTitleWithMuteState(webviewExperimental9);
                    return S_OK;
                })
                .Get(),
            &m_isDocumentPlayingAudioChangedToken));

        // Register a handler for the IsMutedChanged event.
        CHECK_FAILURE(webviewExperimental9->add_IsMutedChanged(
            Callback<ICoreWebView2ExperimentalIsMutedChangedEventHandler>(
                [this, webviewExperimental9](ICoreWebView2* sender, IUnknown* args) -> HRESULT {
                    UpdateTitleWithMuteState(webviewExperimental9);
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
    auto webviewExperimental9 = m_webView.try_query<ICoreWebView2Experimental9>();
    if (webviewExperimental9)
     {
         BOOL isMuted;
         CHECK_FAILURE(webviewExperimental9->get_IsMuted(&isMuted));
         CHECK_FAILURE(webviewExperimental9->put_IsMuted(!isMuted));
     }
 }

 void AudioComponent::UpdateTitleWithMuteState(
     wil::com_ptr<ICoreWebView2Experimental9> webviewExperimental9)
 {
     BOOL isDocumentPlayingAudio;
     CHECK_FAILURE(webviewExperimental9->get_IsDocumentPlayingAudio(&isDocumentPlayingAudio));

     BOOL isMuted;
     CHECK_FAILURE(webviewExperimental9->get_IsMuted(&isMuted));

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
 //! [IsDocumentPlayingAudioChanged] [IsDocumentPlayingAudio] [ToggleIsMuted]

 AudioComponent::~AudioComponent()
 {
     auto webviewExperimental9 = m_webView.try_query<ICoreWebView2Experimental9>();
     if (webviewExperimental9)
     {
         CHECK_FAILURE(webviewExperimental9->remove_IsDocumentPlayingAudioChanged(
             m_isDocumentPlayingAudioChangedToken));
         CHECK_FAILURE(webviewExperimental9->remove_IsMutedChanged(m_isMutedChangedToken));
    }
}