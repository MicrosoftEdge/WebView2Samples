// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "FileComponent.h"

#include "CheckFailure.h"
#include <shlwapi.h>
#include <sstream>

using namespace Microsoft::WRL;

FileComponent::FileComponent(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    //! [DocumentTitleChanged]
    // Register a handler for the DocumentTitleChanged event.
    // This handler just announces the new title on the window's title bar.
    CHECK_FAILURE(m_webView->add_DocumentTitleChanged(
        Callback<ICoreWebView2DocumentTitleChangedEventHandler>(
            [this](ICoreWebView2* sender, IUnknown* args) -> HRESULT {
                wil::unique_cotaskmem_string title;
                CHECK_FAILURE(sender->get_DocumentTitle(&title));
                m_appWindow->SetDocumentTitle(title.get());
                return S_OK;
            })
            .Get(),
        &m_documentTitleChangedToken));
    //! [DocumentTitleChanged]
}

bool FileComponent::HandleWindowMessage(
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result)
{
    if (message == WM_COMMAND)
    {
        switch (LOWORD(wParam))
        {
        case IDM_SAVE_SCREENSHOT:
            SaveScreenshot();
            return true;
        case IDM_PRINT_TO_PDF_LANDSCAPE:
            m_enableLandscape = true;
        case IDM_PRINT_TO_PDF_PORTRAIT:
            if (m_printToPdfInProgress)
            {
                MessageBox(
                    m_appWindow->GetMainWindow(), L"Print to PDF is in progress.",
                    L"Print to PDF", MB_OK);
            }
            else
            {
                PrintToPdf(m_enableLandscape);
            }
            m_enableLandscape = false;
            return true;
        case IDM_GET_DOCUMENT_TITLE:
        {
            wil::unique_cotaskmem_string title;
            m_webView->get_DocumentTitle(&title);
            MessageBox(m_appWindow->GetMainWindow(), title.get(), L"Document Title", MB_OK);
            return true;
        }
        }
    }
    return false;
}

//! [CapturePreview]
// Show the user a file selection dialog, then save a screenshot of the WebView
// to the selected file.
void FileComponent::SaveScreenshot()
{
    WCHAR defaultName[MAX_PATH] = L"WebView2_Screenshot.png";
    OPENFILENAME openFileName = CreateOpenFileName(defaultName, L"PNG File\0*.png\0");
    if (GetSaveFileName(&openFileName))
    {
        wil::com_ptr<IStream> stream;
        CHECK_FAILURE(SHCreateStreamOnFileEx(
            defaultName, STGM_READWRITE | STGM_CREATE, FILE_ATTRIBUTE_NORMAL, TRUE, nullptr,
            &stream));

        CHECK_FAILURE(m_webView->CapturePreview(
            COREWEBVIEW2_CAPTURE_PREVIEW_IMAGE_FORMAT_PNG, stream.get(),
            Callback<ICoreWebView2CapturePreviewCompletedHandler>(
                [appWindow{m_appWindow}](HRESULT error_code) -> HRESULT {
                    CHECK_FAILURE(error_code);
                    appWindow->AsyncMessageBox(L"Preview Captured", L"Preview Captured");
                    return S_OK;
                })
                .Get()));
    }
}
//! [CapturePreview]

//! [PrintToPdf]
// Shows the user a file selection dialog, then uses the selected path when
// printing to PDF. If `enableLandscape` is true, the page is printed
// in landscape mode, otherwise the page is printed in portrait mode.
void FileComponent::PrintToPdf(bool enableLandscape)
{
    WCHAR defaultName[MAX_PATH] = L"WebView2_PrintedPdf.pdf";
    OPENFILENAME openFileName = CreateOpenFileName(defaultName, L"PDF File\0*.pdf\0");
    if (GetSaveFileName(&openFileName))
    {
        wil::com_ptr<ICoreWebView2PrintSettings> printSettings = nullptr;
        if (enableLandscape)
        {
            wil::com_ptr<ICoreWebView2Environment6> webviewEnvironment6;
            CHECK_FAILURE(m_appWindow->GetWebViewEnvironment()->QueryInterface(
                IID_PPV_ARGS(&webviewEnvironment6)));
            if (webviewEnvironment6)
            {
                CHECK_FAILURE(webviewEnvironment6->CreatePrintSettings(&printSettings));
                CHECK_FAILURE(
                    printSettings->put_Orientation(COREWEBVIEW2_PRINT_ORIENTATION_LANDSCAPE));
            }
        }

        wil::com_ptr<ICoreWebView2_7> webview2_7;
        CHECK_FAILURE(m_webView->QueryInterface(IID_PPV_ARGS(&webview2_7)));
        if (webview2_7)
        {
            m_printToPdfInProgress = true;
            CHECK_FAILURE(webview2_7->PrintToPdf(
                openFileName.lpstrFile, printSettings.get(),
                Callback<ICoreWebView2PrintToPdfCompletedHandler>(
                    [this](HRESULT errorCode, BOOL isSuccessful) -> HRESULT {
                        CHECK_FAILURE(errorCode);
                        m_printToPdfInProgress = false;
                        m_appWindow->AsyncMessageBox(
                            (isSuccessful) ? L"Print to PDF succeeded"
                                           : L"Print to PDF failed",
                            L"Print to PDF Completed");
                        return S_OK;
                    })
                    .Get()));
        }
    }
}
//! [PrintToPdf]

bool FileComponent::IsPrintToPdfInProgress()
{
    return m_printToPdfInProgress;
}

OPENFILENAME FileComponent::CreateOpenFileName(LPWSTR defaultName, LPCWSTR filter)
{
    OPENFILENAME openFileName = {};
    openFileName.lStructSize = sizeof(openFileName);
    openFileName.hwndOwner = nullptr;
    openFileName.hInstance = nullptr;
    openFileName.lpstrFile = defaultName;
    openFileName.lpstrFilter = filter;
    openFileName.nMaxFile = MAX_PATH;
    openFileName.Flags = OFN_OVERWRITEPROMPT;
    return openFileName;
}

FileComponent::~FileComponent()
{
    m_webView->remove_DocumentTitleChanged(m_documentTitleChangedToken);
}
