// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "ScenarioScreenCapture.h"

#include "AppWindow.h"
#include "CheckFailure.h"

using namespace Microsoft::WRL;

static constexpr WCHAR c_samplePath[] = L"ScenarioScreenCapture.html";

ScenarioScreenCapture::ScenarioScreenCapture(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    m_sampleUri = m_appWindow->GetLocalUri(c_samplePath);

    //! [ScreenCaptureStarting0]
    m_webViewExperimental26 = m_webView.try_query<ICoreWebView2Experimental26>();
    if (m_webViewExperimental26)
    {
        m_webViewExperimental26->add_ScreenCaptureStarting(
            Callback<ICoreWebView2ExperimentalScreenCaptureStartingEventHandler>(
                [this](
                    ICoreWebView2* sender,
                    ICoreWebView2ExperimentalScreenCaptureStartingEventArgs* args) -> HRESULT
                {
                    // Get Frame Info
                    wil::com_ptr<ICoreWebView2FrameInfo> frameInfo;
                    CHECK_FAILURE(args->get_OriginalSourceFrameInfo(&frameInfo));

                    wil::com_ptr<ICoreWebView2FrameInfo2> frameInfo2;
                    CHECK_FAILURE(frameInfo->QueryInterface(IID_PPV_ARGS(&frameInfo2)));

                    UINT32 source_frameId;
                    CHECK_FAILURE(frameInfo2->get_FrameId(&source_frameId));

                    BOOL cancel = m_mainFramePermission == FALSE;
                    CHECK_FAILURE(args->put_Cancel(cancel));
                    return S_OK;
                })
                .Get(),
            &m_screenCaptureStartingToken);
    }
    //! [ScreenCaptureStarting0]

    //! [ScreenCaptureStarting1]
    m_webView4 = m_webView.try_query<ICoreWebView2_4>();
    if (m_webView4)
    {
        CHECK_FAILURE(m_webView4->add_FrameCreated(
            Callback<ICoreWebView2FrameCreatedEventHandler>(
                [this](
                    ICoreWebView2* sender, ICoreWebView2FrameCreatedEventArgs* args) -> HRESULT
                {
                    wil::com_ptr<ICoreWebView2Frame> webviewFrame;
                    CHECK_FAILURE(args->get_Frame(&webviewFrame));

                    auto frame5 = webviewFrame.try_query<ICoreWebView2Frame5>();

                    UINT32 frameId;
                    frame5->get_FrameId(&frameId);

                    m_screenCaptureFrameIdPermission[frameId] = TRUE;

                    wil::com_ptr<ICoreWebView2Frame2> webviewFrame2 =
                        webviewFrame.try_query<ICoreWebView2Frame2>();
                    if (!webviewFrame2)
                    {
                        return S_OK;
                    }

                    bool cancel_on_frame = false;

                    CHECK_FAILURE(webviewFrame2->add_WebMessageReceived(
                        Microsoft::WRL::Callback<
                            ICoreWebView2FrameWebMessageReceivedEventHandler>(
                            [this, frameId](
                                ICoreWebView2Frame* sender,
                                ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT
                            {
                                BOOL cancel = false;

                                wil::unique_cotaskmem_string messageRaw;
                                HRESULT hr = args->TryGetWebMessageAsString(&messageRaw);
                                if (hr == E_INVALIDARG)
                                {
                                    // Was not a string message. Ignore.
                                    return hr;
                                }
                                // Any other problems are fatal.
                                CHECK_FAILURE(hr);
                                std::wstring message = messageRaw.get();

                                if (message == L"EnableScreenCapture")
                                {
                                    cancel = false;
                                }
                                else if (message == L"DisableScreenCapture")
                                {
                                    cancel = true;
                                }
                                else
                                {
                                    // Ignore unrecognized messages, but log for further
                                    // investigation since it suggests a mismatch between the
                                    // web content and the host.
                                    OutputDebugString(
                                        std::wstring(
                                            L"Unexpected message from main page:" + message)
                                            .c_str());
                                }

                                m_screenCaptureFrameIdPermission[frameId] = (cancel == FALSE);

                                return S_OK;
                            })
                            .Get(),
                        nullptr));

                    m_experimentalFrame6 =
                        webviewFrame.try_query<ICoreWebView2ExperimentalFrame6>();

                    m_experimentalFrame6->add_ScreenCaptureStarting(
                        Callback<
                            ICoreWebView2ExperimentalFrameScreenCaptureStartingEventHandler>(
                            [this](
                                ICoreWebView2Frame* sender,
                                ICoreWebView2ExperimentalScreenCaptureStartingEventArgs* args)
                                -> HRESULT
                            {
                                args->put_Handled(TRUE);

                                bool cancel = FALSE;

                                // Get Frame Info
                                wil::com_ptr<ICoreWebView2FrameInfo> frameInfo;
                                CHECK_FAILURE(args->get_OriginalSourceFrameInfo(&frameInfo));

                                wil::com_ptr<ICoreWebView2FrameInfo2> frameInfo2;
                                CHECK_FAILURE(
                                    frameInfo->QueryInterface(IID_PPV_ARGS(&frameInfo2)));

                                // Frame Source
                                wil::unique_cotaskmem_string frameSource;
                                CHECK_FAILURE(frameInfo->get_Source(&frameSource));

                                UINT32 source_frameId;
                                CHECK_FAILURE(frameInfo2->get_FrameId(&source_frameId));

                                cancel =
                                    (m_screenCaptureFrameIdPermission[source_frameId] == FALSE);

                                CHECK_FAILURE(args->put_Cancel(cancel));
                                return S_OK;
                            })
                            .Get(),
                        &m_frameScreenCaptureStartingToken);

                    return S_OK;
                })
                .Get(),
            &m_frameCreatedToken));
    }
    else
    {
        FeatureNotAvailable();
    }
    //! [ScreenCaptureStarting1]

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
                -> HRESULT
            {
                BOOL cancel = FALSE;
                wil::unique_cotaskmem_string uri;
                CHECK_FAILURE(args->get_Source(&uri));

                // Always validate that the origin of the message is what you
                // expect.
                if (uri.get() != m_sampleUri)
                {
                    // Ignore messages from untrusted sources.
                    return E_INVALIDARG;
                }
                wil::unique_cotaskmem_string messageRaw;
                HRESULT hr = args->TryGetWebMessageAsString(&messageRaw);
                if (hr == E_INVALIDARG)
                {
                    // Was not a string message. Ignore.
                    return hr;
                }
                // Any other problems are fatal.
                CHECK_FAILURE(hr);
                std::wstring message = messageRaw.get();

                if (message == L"EnableScreenCapture")
                {
                    cancel = FALSE;
                }
                else if (message == L"DisableScreenCapture")
                {
                    cancel = TRUE;
                }
                else
                {
                    // Ignore unrecognized messages, but log for further
                    // investigation since it suggests a mismatch between the
                    // web content and the host.
                    OutputDebugString(
                        std::wstring(L"Unexpected message from main page:" + message).c_str());
                }
                m_mainFramePermission = (cancel == FALSE);
                return S_OK;
            })
            .Get(),
        &m_webMessageReceivedToken));

    CHECK_FAILURE(m_webView->Navigate(m_sampleUri.c_str()));
}

ScenarioScreenCapture::~ScenarioScreenCapture()
{
    m_webView->remove_ContentLoading(m_contentLoadingToken);
    m_webView->remove_WebMessageReceived(m_webMessageReceivedToken);
    if (m_webViewExperimental26)
    {
        CHECK_FAILURE(m_webViewExperimental26->remove_ScreenCaptureStarting(
            m_screenCaptureStartingToken));
    }
    if (m_experimentalFrame6)
    {
        CHECK_FAILURE(m_experimentalFrame6->remove_ScreenCaptureStarting(
            m_frameScreenCaptureStartingToken));
    }
    if (m_webView4)
    {
        CHECK_FAILURE(m_webView4->remove_FrameCreated(m_frameCreatedToken));
    }
}
