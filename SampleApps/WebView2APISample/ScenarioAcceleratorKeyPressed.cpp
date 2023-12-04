// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "ScenarioAcceleratorKeyPressed.h"

#include "AppWindow.h"
#include "CheckFailure.h"

using namespace Microsoft::WRL;

static constexpr WCHAR c_samplePath[] = L"ScenarioAcceleratorKeyPressed.html";
ScenarioAcceleratorKeyPressed::ScenarioAcceleratorKeyPressed(AppWindow* appWindow)
    : m_appWindow(appWindow), m_controller(appWindow->GetWebViewController()),
      m_webView(appWindow->GetWebView())
{
    m_sampleUri = m_appWindow->GetLocalUri(c_samplePath);
    wil::com_ptr<ICoreWebView2Settings> settings;
    CHECK_FAILURE(m_webView->get_Settings(&settings));
    m_settings3 = settings.try_query<ICoreWebView2Settings3>();
    // Setup the web message received event handler before navigating to
    // ensure we don't miss any messages.
    CHECK_FAILURE(m_webView->add_WebMessageReceived(
        Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args)
            {
                wil::unique_cotaskmem_string uri;
                CHECK_FAILURE(args->get_Source(&uri));

                // Always validate that the origin of the message is what you expect.
                if (uri.get() != m_sampleUri)
                {
                    return S_OK;
                }
                wil::unique_cotaskmem_string messageRaw;
                CHECK_FAILURE(args->TryGetWebMessageAsString(&messageRaw));
                std::wstring message = messageRaw.get();
                std::wstring reply;

                if (message.compare(0, 26, L"DisableBrowserAccelerators") == 0)
                {
                    CHECK_FAILURE(m_settings3->put_AreBrowserAcceleratorKeysEnabled(FALSE));
                    MessageBox(
                        nullptr,
                        L"Browser-specific accelerator keys, for example: \n"
                        L"Ctrl+F and F3 for Find on Page\n"
                        L"Ctrl+P for Print\n"
                        L"Ctrl+R and F5 for Reload\n"
                        L"Ctrl+Plus and Ctrl+Minus for zooming\n"
                        L"Ctrl+Shift-C and F12 for DevTools\n"
                        L"Special keys for browser functions, such as Back, Forward, and "
                        L"Search \n"
                        L"will be disabled after the next navigation except for F7.",
                        L"Settings change", MB_OK);
                }
                else if (message.compare(0, 25, L"EnableBrowserAccelerators") == 0)
                {
                    CHECK_FAILURE(m_settings3->put_AreBrowserAcceleratorKeysEnabled(TRUE));
                    MessageBox(
                        nullptr,
                        L"Browser-specific accelerator keys, for example: \n"
                        L"Ctrl+F and F3 for Find on Page\n"
                        L"Ctrl+R and F5 for Reload\n"
                        L"Ctrl+Plus and Ctrl+Minus for zooming\n"
                        L"Ctrl+Shift-C and F12 for DevTools\n"
                        L"Special keys for browser functions, such as Back, Forward, and "
                        L"Search \n"
                        L"will be enabled after the next navigation except for Ctr + P.",
                        L"Settings change", MB_OK);
                }
                return S_OK;
            })
            .Get(),
        &m_webMessageReceivedToken));

    //! [IsBrowserAcceleratorKeyEnabled]
    if (m_settings3)
    {
        // Register a handler for the AcceleratorKeyPressed event.
        CHECK_FAILURE(m_controller->add_AcceleratorKeyPressed(
            Callback<ICoreWebView2AcceleratorKeyPressedEventHandler>(
                [this](
                    ICoreWebView2Controller* sender,
                    ICoreWebView2AcceleratorKeyPressedEventArgs* args) -> HRESULT
                {
                    COREWEBVIEW2_KEY_EVENT_KIND kind;
                    CHECK_FAILURE(args->get_KeyEventKind(&kind));
                    // We only care about key down events.
                    if (kind == COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN ||
                        kind == COREWEBVIEW2_KEY_EVENT_KIND_SYSTEM_KEY_DOWN)
                    {
                        UINT key;
                        CHECK_FAILURE(args->get_VirtualKey(&key));

                        wil::com_ptr<ICoreWebView2AcceleratorKeyPressedEventArgs2> args2;

                        args->QueryInterface(IID_PPV_ARGS(&args2));
                        if (args2)
                        {
                            if (key == 'P' && (GetKeyState(VK_CONTROL) < 0))
                            {
                                // tell the browser to skip the key
                                CHECK_FAILURE(args2->put_IsBrowserAcceleratorKeyEnabled(FALSE));
                            }
                            if (key == VK_F7)
                            {
                                // tell the browser to process the key
                                CHECK_FAILURE(args2->put_IsBrowserAcceleratorKeyEnabled(TRUE));
                            }
                        }
                    }
                    return S_OK;
                })
                .Get(),
            &m_acceleratorKeyPressedToken));
    }
    //! [IsBrowserAcceleratorKeyEnabled]

    // Turn off this scenario if we navigate away from the sample page
    CHECK_FAILURE(m_webView->add_ContentLoading(
        Callback<ICoreWebView2ContentLoadingEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2ContentLoadingEventArgs* args) -> HRESULT
            {
                wil::unique_cotaskmem_string uri;
                sender->get_Source(&uri);
                if (uri.get() != m_sampleUri)
                {
                    m_appWindow->DeleteComponent(this);
                }
                return S_OK;
            })
            .Get(),
        &m_contentLoadingToken));

    CHECK_FAILURE(m_webView->Navigate(m_sampleUri.c_str()));
}

ScenarioAcceleratorKeyPressed::~ScenarioAcceleratorKeyPressed()
{
    m_webView->remove_WebMessageReceived(m_webMessageReceivedToken);
    m_controller->remove_AcceleratorKeyPressed(m_acceleratorKeyPressedToken);
    m_webView->remove_ContentLoading(m_contentLoadingToken);
}
