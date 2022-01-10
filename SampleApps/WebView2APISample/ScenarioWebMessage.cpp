// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "ScenarioWebMessage.h"

#include "AppWindow.h"
#include "CheckFailure.h"

using namespace Microsoft::WRL;

static constexpr WCHAR c_samplePath[] = L"ScenarioWebMessage.html";

ScenarioWebMessage::ScenarioWebMessage(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    m_sampleUri = m_appWindow->GetLocalUri(c_samplePath);

    //! [IsWebMessageEnabled]
    ComPtr<ICoreWebView2Settings> settings;
    CHECK_FAILURE(m_webView->get_Settings(&settings));

    CHECK_FAILURE(settings->put_IsWebMessageEnabled(TRUE));
    //! [IsWebMessageEnabled]

    //! [WebMessageReceived]
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
            // Ignore messages from untrusted sources.
            return S_OK;
        }
        wil::unique_cotaskmem_string messageRaw;
        HRESULT hr = args->TryGetWebMessageAsString(&messageRaw);
        if (hr == E_INVALIDARG)
        {
            // Was not a string message. Ignore.
            return S_OK;
        }
        // Any other problems are fatal.
        CHECK_FAILURE(hr);
        std::wstring message = messageRaw.get();

        if (message.compare(0, 13, L"SetTitleText ") == 0)
        {
            m_appWindow->SetDocumentTitle(message.substr(13).c_str());
        }
        else if (message.compare(L"GetWindowBounds") == 0)
        {
            RECT bounds = m_appWindow->GetWindowBounds();
            std::wstring reply =
                L"{\"WindowBounds\":\"Left:" + std::to_wstring(bounds.left)
                + L"\\nTop:" + std::to_wstring(bounds.top)
                + L"\\nRight:" + std::to_wstring(bounds.right)
                + L"\\nBottom:" + std::to_wstring(bounds.bottom)
                + L"\"}";
            CHECK_FAILURE(sender->PostWebMessageAsJson(reply.c_str()));
        }
        else
        {
            // Ignore unrecognized messages, but log for further investigation
            // since it suggests a mismatch between the web content and the host.
            OutputDebugString(
                std::wstring(L"Unexpected message from main page:" + message).c_str());
        }
        return S_OK;
    }).Get(), &m_webMessageReceivedToken));
    //! [WebMessageReceived]

    // Turn off this scenario if we navigate away from the sample page
    CHECK_FAILURE(m_webView->add_ContentLoading(
        Callback<ICoreWebView2ContentLoadingEventHandler>(
            [this](
                ICoreWebView2* sender,
                ICoreWebView2ContentLoadingEventArgs* args) -> HRESULT {
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

    wil::com_ptr<ICoreWebView2_4> webview2_4 = m_webView.try_query<ICoreWebView2_4>();
    if (webview2_4)
    {
        CHECK_FAILURE(webview2_4->add_FrameCreated(
            Callback<ICoreWebView2FrameCreatedEventHandler>(
                [this](ICoreWebView2* sender, ICoreWebView2FrameCreatedEventArgs* args) -> HRESULT {
            wil::com_ptr<ICoreWebView2Frame> webviewFrame;
            CHECK_FAILURE(args->get_Frame(&webviewFrame));
            wil::com_ptr<ICoreWebView2Frame2> webviewFrame2 =
                webviewFrame.try_query<ICoreWebView2Frame2>();
            if (!webviewFrame2)
            {
                return S_OK;
            }
            //! [WebMessageReceivedIFrame]
            // Setup the web message received event handler before navigating to
            // ensure we don't miss any messages.
            CHECK_FAILURE(webviewFrame2->add_WebMessageReceived(
                Microsoft::WRL::Callback<ICoreWebView2FrameWebMessageReceivedEventHandler>(
                    [this](
                        ICoreWebView2Frame* sender,
                        ICoreWebView2WebMessageReceivedEventArgs* args) {
                        wil::unique_cotaskmem_string uri;
                        CHECK_FAILURE(args->get_Source(&uri));

                        // Always validate that the origin of the message is what you expect.
                        if (uri.get() != m_sampleUri)
                        {
                            // Ignore messages from untrusted sources.
                            return S_OK;
                        }
                        wil::unique_cotaskmem_string messageRaw;
                        HRESULT hr = args->TryGetWebMessageAsString(&messageRaw);
                        if (hr == E_INVALIDARG)
                        {
                            // Was not a string message. Ignore.
                            return S_OK;
                        }
                        // Any other problems are fatal.
                        CHECK_FAILURE(hr);
                        std::wstring message = messageRaw.get();

                        if (message.compare(0, 13, L"SetTitleText ") == 0)
                        {
                            m_appWindow->SetDocumentTitle(message.substr(13).c_str());
                        }
                        else if (message.compare(L"GetWindowBounds") == 0)
                        {
                            RECT bounds = m_appWindow->GetWindowBounds();
                            std::wstring reply = L"{\"WindowBounds\":\"Left:" +
                                                 std::to_wstring(bounds.left) + L"\\nTop:" +
                                                 std::to_wstring(bounds.top) + L"\\nRight:" +
                                                 std::to_wstring(bounds.right) + L"\\nBottom:" +
                                                 std::to_wstring(bounds.bottom) + L"\"}";
                            wil::com_ptr<ICoreWebView2Frame2> webviewFrame2;
                            if (sender->QueryInterface(IID_PPV_ARGS(&webviewFrame2)) == S_OK)
                            {
                                CHECK_FAILURE(
                                    webviewFrame2->PostWebMessageAsJson(reply.c_str()));
                            }
                        }
                        else
                        {
                            // Ignore unrecognized messages, but log for further investigation
                            // since it suggests a mismatch between the web content and the host.
                            OutputDebugString(
                                std::wstring(L"Unexpected message from frame:" + message).c_str());
                        }
                        return S_OK;
                    })
                    .Get(),
                NULL));
            //! [WebMessageReceivedIFrame]
            return S_OK;
        }).Get(), &m_frameCreatedToken));
    }

    // Changes to ICoreWebView2Settings::IsWebMessageEnabled apply to the next document
    // to which we navigate.
    CHECK_FAILURE(m_webView->Navigate(m_sampleUri.c_str()));
}

ScenarioWebMessage::~ScenarioWebMessage()
{
    m_webView->remove_WebMessageReceived(m_webMessageReceivedToken);
    m_webView->remove_ContentLoading(m_contentLoadingToken);
    wil::com_ptr<ICoreWebView2_4> webview2_4 = m_webView.try_query<ICoreWebView2_4>();
    if (webview2_4)
    {
        webview2_4->remove_FrameCreated(m_frameCreatedToken);
    }
}
