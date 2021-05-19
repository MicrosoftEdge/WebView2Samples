// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "stdafx.h"

#include "ScenarioCustomDownloadExperience.h"

#include "AppWindow.h"
#include "CheckFailure.h"
#include "TextInputDialog.h"

using namespace Microsoft::WRL;

ScenarioCustomDownloadExperience::ScenarioCustomDownloadExperience(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    //! [DownloadStarting]
    // Register a handler for the `DownloadStarting` event.
    // This example hides the default download dialog and shows a dialog box instead.
    // The dialog box displays the default result file path and allows the user to specify a different path.
    // Selecting `OK` will save the download to the chosen path.
    // Selecting `CANCEL` will cancel the download.
    m_demoUri = L"https://demo.smartscreen.msft.net/";

    m_webViewExperimental2 = m_webView.try_query<ICoreWebView2Experimental2>();
    if (m_webViewExperimental2) {
        CHECK_FAILURE(m_webViewExperimental2->add_DownloadStarting(
            Callback<ICoreWebView2ExperimentalDownloadStartingEventHandler>(
                [this](
                    ICoreWebView2* sender,
                    ICoreWebView2ExperimentalDownloadStartingEventArgs* args) -> HRESULT
                {
                    // We avoid potential reentrancy from running a message loop in the download
                    // starting event handler by showing our download dialog via this lambda run
                    // asynchronously later outside of this event handler. Note that a long running
                    // synchronous UI prompt or other blocking item on the UI thread can potentially
                    // block the WebView2 from doing anything.
                    auto showDialog = [this, args]
                    {
                        // Hide the default download dialog.
                        CHECK_FAILURE(args->put_Handled(TRUE));

                        wil::com_ptr<ICoreWebView2ExperimentalDownloadOperation> download;
                        CHECK_FAILURE(args->get_DownloadOperation(&download));

                        INT64 totalBytesToReceive = 0;
                        CHECK_FAILURE(download->get_TotalBytesToReceive(&totalBytesToReceive));

                        wil::unique_cotaskmem_string uri;
                        CHECK_FAILURE(download->get_Uri(&uri));

                        wil::unique_cotaskmem_string mimeType;
                        CHECK_FAILURE(download->get_MimeType(&mimeType));

                        wil::unique_cotaskmem_string contentDisposition;
                        CHECK_FAILURE(download->get_ContentDisposition(&contentDisposition));

                        // Get the suggested path from the event args.
                        wil::unique_cotaskmem_string resultFilePath;
                        CHECK_FAILURE(args->get_ResultFilePath(&resultFilePath));

                        std::wstring prompt =
                            std::wstring(
                                L"Enter result file path or select `OK` to use default path. "
                                L"Select `Cancel` to cancel the download.");

                        std::wstring description = std::wstring(L"URI: ") + uri.get() + L"\r\n" +
                                                    L"Mime type: " + mimeType.get() + L"\r\n";
                        if (totalBytesToReceive >= 0)
                        {
                            description = description + L"Total bytes to receive: " +
                                          std::to_wstring(totalBytesToReceive) + L"\r\n";
                        }

                        TextInputDialog dialog(
                            m_appWindow->GetMainWindow(), L"Download Starting", prompt.c_str(),
                            description.c_str(), resultFilePath.get());
                        if (dialog.confirmed)
                        {
                            // If user selects `OK`, the download will complete normally.
                            // Result file path will be updated if a new one was provided.
                            CHECK_FAILURE(args->put_ResultFilePath(dialog.input.c_str()));
                            UpdateProgress(download.get());
                        }
                        else
                        {
                            // If user selects `Cancel`, the download will be canceled.
                            CHECK_FAILURE(args->put_Cancel(TRUE));
                        }
                    };

                    // Obtain a deferral for the event so that the CoreWebView2
                    // doesn't examine the properties we set on the event args until
                    // after we call the Complete method asynchronously later.
                    wil::com_ptr<ICoreWebView2Deferral> deferral;
                    CHECK_FAILURE(args->GetDeferral(&deferral));

                    // This function can be called to show the download dialog and
                    // complete the event at a later time, allowing the developer to
                    // perform async work before the event completes.
                    m_completeDeferredDownloadEvent = [showDialog, deferral] {
                        showDialog();
                        CHECK_FAILURE(deferral->Complete());
                    };

                    return S_OK;
                })
                .Get(),
            &m_downloadStartingToken));
    }
    //! [DownloadStarting]

    // Turn off this scenario if we navigate away from the demo page.
    CHECK_FAILURE(m_webView->add_ContentLoading(
        Callback<ICoreWebView2ContentLoadingEventHandler>(
            [this](
                ICoreWebView2* sender, ICoreWebView2ContentLoadingEventArgs* args) -> HRESULT {
                wil::unique_cotaskmem_string uri;
                sender->get_Source(&uri);
                if (uri.get() != m_demoUri)
                {
                    m_appWindow->DeleteComponent(this);
                }
                return S_OK;
            })
            .Get(),
        &m_contentLoadingToken));

    CHECK_FAILURE(m_appWindow->GetWebView()->Navigate(m_demoUri.c_str()));
}

bool ScenarioCustomDownloadExperience::HandleWindowMessage(
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result)
{
    if (message == WM_COMMAND)
    {
        switch (LOWORD(wParam))
        {
        case IDM_SCENARIO_COMPLETE_DEFERRED_DOWNLOAD:
            CompleteDownloadDeferral();
            return true;
        }
    }
    return false;
}

void ScenarioCustomDownloadExperience::CompleteDownloadDeferral()
{
    if (m_completeDeferredDownloadEvent)
    {
        m_completeDeferredDownloadEvent();
        m_completeDeferredDownloadEvent = nullptr;
    }
}

void ScenarioCustomDownloadExperience::UpdateProgress(ICoreWebView2ExperimentalDownloadOperation* download)
{
    //! [BytesReceivedChanged]
    CHECK_FAILURE(download->add_BytesReceivedChanged(
        Callback<ICoreWebView2ExperimentalBytesReceivedChangedEventHandler>(
            [this](ICoreWebView2ExperimentalDownloadOperation* download, IUnknown* args) -> HRESULT {
                // Here developer can update UI to show progress of a download using
                // `download->get_BytesReceived` and
                // `download->get_TotalBytesToReceive`.
                return S_OK;
            })
            .Get(),
        &m_bytesReceivedChangedToken));
    //! [BytesReceivedChanged]

    //! [StateChanged]
    CHECK_FAILURE(download->add_StateChanged(
        Callback<ICoreWebView2ExperimentalStateChangedEventHandler>(
          [this](ICoreWebView2ExperimentalDownloadOperation* download,
            IUnknown* args) -> HRESULT {
                COREWEBVIEW2_DOWNLOAD_STATE downloadState;
                CHECK_FAILURE(download->get_State(&downloadState));
                switch (downloadState)
                {
                case COREWEBVIEW2_DOWNLOAD_STATE_IN_PROGRESS:
                    break;
                case COREWEBVIEW2_DOWNLOAD_STATE_INTERRUPTED:
                    // Here developer can take different actions based on `download->InterruptReason`.
                    // For example, show an error message to the end user.
                    CompleteDownload(download);
                    break;
                case COREWEBVIEW2_DOWNLOAD_STATE_COMPLETED:
                    CompleteDownload(download);
                    break;
                }
                return S_OK;
          })
          .Get(),
        &m_stateChangedToken));
    //! [StateChanged]
}

void ScenarioCustomDownloadExperience::CompleteDownload(ICoreWebView2ExperimentalDownloadOperation* download)
{
    // Close download progress dialog here.

    // Unsubscribe from download events.
    CHECK_FAILURE(download->remove_BytesReceivedChanged(
        m_bytesReceivedChangedToken));
    CHECK_FAILURE(download->remove_StateChanged(m_stateChangedToken));
}

ScenarioCustomDownloadExperience::~ScenarioCustomDownloadExperience()
{
    CHECK_FAILURE(m_webViewExperimental2->remove_DownloadStarting(m_downloadStartingToken));
    CHECK_FAILURE(m_webView->remove_ContentLoading(m_contentLoadingToken));
}