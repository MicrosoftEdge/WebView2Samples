// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include <iomanip>
#include <sstream>

#include "ScenarioSharedBuffer.h"

#include "AppWindow.h"
#include "CheckFailure.h"

using namespace Microsoft::WRL;

static constexpr WCHAR c_samplePath[] = L"ScenarioSharedBuffer.html";

ScenarioSharedBuffer::ScenarioSharedBuffer(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    m_webView18 = m_webView.try_query<ICoreWebView2Experimental18>();
    if (!m_webView18)
    {
        // Feature not supported.
        return;
    }

    m_sampleUri = m_appWindow->GetLocalUri(c_samplePath);

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

    CHECK_FAILURE(m_webView->add_WebMessageReceived(
        Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args)
            {
                WebViewMessageReceived(args, false);
                return S_OK;
            })
            .Get(),
        &m_webMessageReceivedToken));

    wil::com_ptr<ICoreWebView2_4> webview2_4 = m_webView.try_query<ICoreWebView2_4>();
    if (webview2_4)
    {
        CHECK_FAILURE(webview2_4->add_FrameCreated(
            Callback<ICoreWebView2FrameCreatedEventHandler>(
                [this](
                    ICoreWebView2* sender, ICoreWebView2FrameCreatedEventArgs* args) -> HRESULT
                {
                    wil::com_ptr<ICoreWebView2Frame> webviewFrame;
                    CHECK_FAILURE(args->get_Frame(&webviewFrame));
                    wil::com_ptr<ICoreWebView2Frame2> webviewFrame2 =
                        webviewFrame.try_query<ICoreWebView2Frame2>();
                    if (!webviewFrame2)
                    {
                        return S_OK;
                    }
                    CHECK_FAILURE(webviewFrame2->add_WebMessageReceived(
                        Microsoft::WRL::Callback<
                            ICoreWebView2FrameWebMessageReceivedEventHandler>(
                            [this](
                                ICoreWebView2Frame* sender,
                                ICoreWebView2WebMessageReceivedEventArgs* args)
                            {
                                WebViewMessageReceived(args, true);
                                return S_OK;
                            })
                            .Get(),
                        nullptr));
                    m_webviewFrame4 = webviewFrame.try_query<ICoreWebView2ExperimentalFrame4>();
                    return S_OK;
                })
                .Get(),
            &m_frameCreatedToken));
    }

    // Changes to CoreWebView2 settings apply to the next document to which we navigate.
    CHECK_FAILURE(m_webView->Navigate(m_sampleUri.c_str()));
}

void ScenarioSharedBuffer::DisplaySharedBufferData()
{
    if (!m_sharedBuffer)
    {
        m_appWindow->AsyncMessageBox(L"Share Buffer not setup", L"Shared Buffer Data");
        return;
    }
    BYTE* buffer = nullptr;
    UINT64 size = 0;
    CHECK_FAILURE(m_sharedBuffer->get_Buffer(&buffer));
    CHECK_FAILURE(m_sharedBuffer->get_Size(&size));
    // Display first 256 bytes of the data
    std::wstringstream message;
    message << L"Share Buffer Data:" << std::endl;
    for (int i = 0; i < size && i < 256; ++i)
    {
        if (isprint(buffer[i]))
            message << (char)buffer[i];
        else
            message << "0x" << std::hex << std::setfill(L'0') << std::setw(2) << buffer[i];
    }
    message << std::endl;
    m_appWindow->AsyncMessageBox(std::move(message.str()), L"Shared Buffer Data");
}

void ScenarioSharedBuffer::WebViewMessageReceived(
    ICoreWebView2WebMessageReceivedEventArgs* args, bool fromFrame)
{
    wil::unique_cotaskmem_string uri;
    CHECK_FAILURE(args->get_Source(&uri));

    // Always validate that the origin of the message is what you expect.
    if (uri.get() != m_sampleUri)
    {
        // Ignore messages from untrusted sources.
        return;
    }
    wil::unique_cotaskmem_string messageRaw;
    HRESULT hr = args->TryGetWebMessageAsString(&messageRaw);
    if (hr == E_INVALIDARG)
    {
        // Was not a string message. Ignore.
        return;
    }
    // Any other problems are fatal.
    CHECK_FAILURE(hr);
    std::wstring message = messageRaw.get();

    if (message == L"SharedBufferDataUpdated")
    {
        // Shared buffer updated, display it.
        DisplaySharedBufferData();
    }
    else if (message == L"RequestShareBuffer")
    {
        EnsureSharedBuffer();
        if (fromFrame)
        {
            m_webviewFrame4->PostSharedBufferToScript(
                m_sharedBuffer.get(), COREWEBVIEW2_SHARED_BUFFER_ACCESS_READ_WRITE, nullptr);
        }
        else
        {
            m_webView18->PostSharedBufferToScript(
                m_sharedBuffer.get(), COREWEBVIEW2_SHARED_BUFFER_ACCESS_READ_WRITE, nullptr);
        }
    }
    else if (message == L"RequestOneTimeShareBuffer")
    {
        const UINT64 bufferSize = 128;
        BYTE data[] = "some read only data";
        //! [OneTimeShareBuffer]
        wil::com_ptr<ICoreWebView2ExperimentalEnvironment10> environment;
        CHECK_FAILURE(
            m_appWindow->GetWebViewEnvironment()->QueryInterface(IID_PPV_ARGS(&environment)));

        wil::com_ptr<ICoreWebView2ExperimentalSharedBuffer> sharedBuffer;
        CHECK_FAILURE(environment->CreateSharedBuffer(bufferSize, &sharedBuffer));
        // Set data into the shared memory via IStream.
        wil::com_ptr<IStream> stream;
        CHECK_FAILURE(sharedBuffer->OpenStream(&stream));
        CHECK_FAILURE(stream->Write(data, sizeof(data), nullptr));
        PCWSTR additionalDataAsJson = L"{\"myBufferType\":\"bufferType1\"}";
        if (fromFrame)
        {
            m_webviewFrame4->PostSharedBufferToScript(
                sharedBuffer.get(), COREWEBVIEW2_SHARED_BUFFER_ACCESS_READ_ONLY,
                additionalDataAsJson);
        }
        else
        {
            m_webView18->PostSharedBufferToScript(
                sharedBuffer.get(), COREWEBVIEW2_SHARED_BUFFER_ACCESS_READ_ONLY,
                additionalDataAsJson);
        }
        // Explicitly close the one time shared buffer to ensure that the resource is released.
        sharedBuffer->Close();
        //! [OneTimeShareBuffer]
    }
    else
    {
        // Ignore unrecognized messages, but log for further investigation
        // since it suggests a mismatch between the web content and the host.
        OutputDebugString(
            std::wstring(L"Unexpected message from main page:" + message).c_str());
    }
}

void ScenarioSharedBuffer::EnsureSharedBuffer()
{
    if (m_sharedBuffer)
    {
        // already created
        return;
    }
    wil::com_ptr<ICoreWebView2ExperimentalEnvironment10> environment;
    CHECK_FAILURE(
        m_appWindow->GetWebViewEnvironment()->QueryInterface(IID_PPV_ARGS(&environment)));

    const UINT64 size = 128;
    CHECK_FAILURE(environment->CreateSharedBuffer(size, &m_sharedBuffer));
    // Set some data into the shared memory
    BYTE* buffer = nullptr;
    CHECK_FAILURE(m_sharedBuffer->get_Buffer(&buffer));
    BYTE data[] = "some app data";
    memcpy(buffer, data, sizeof(data));
}

ScenarioSharedBuffer::~ScenarioSharedBuffer()
{
    m_webView->remove_ContentLoading(m_contentLoadingToken);
    m_webView->remove_WebMessageReceived(m_webMessageReceivedToken);
    wil::com_ptr<ICoreWebView2_4> webview2_4 = m_webView.try_query<ICoreWebView2_4>();
    if (webview2_4)
    {
        webview2_4->remove_FrameCreated(m_frameCreatedToken);
    }
}
