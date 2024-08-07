// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "ScenarioSaveAs.h"

#include "App.h"
#include "AppWindow.h"
#include "CheckFailure.h"
#include "Shlwapi.h"
#include "TextInputDialog.h"
#include "resource.h"

using namespace Microsoft::WRL;

ScenarioSaveAs::ScenarioSaveAs(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    if (m_webView)
    {
        m_webView2_25 = m_webView.try_query<ICoreWebView2_25>();
    }
}

std::initializer_list<COREWEBVIEW2_SAVE_AS_KIND> saveAsKind{
    COREWEBVIEW2_SAVE_AS_KIND_DEFAULT, COREWEBVIEW2_SAVE_AS_KIND_HTML_ONLY,
    COREWEBVIEW2_SAVE_AS_KIND_SINGLE_FILE, COREWEBVIEW2_SAVE_AS_KIND_COMPLETE};
// Sync with COREWEBVIEW2_SAVE_AS_KIND.
std::array<std::wstring, 4> saveAsKindString{
    L"DEFAULT", L"HTML_ONLY", L"SIGNLE_FILE", L"COMPLETE"};
// Sync with COREWEBVIEW2_SAVE_AS_UI_RESULTS.
std::array<std::wstring, 5> saveAsUIResultString{
    L"SUCCESS", L"INVALID_PATH", L"FILE_ALREADY_EXISTS", L"KIND_NOT_SUPPORTED", L"CANCELLED"};

//! [ToggleSilent]
// Turn on/off Silent SaveAs, which won't show the system default save as dialog.
// This example hides the default save as dialog and shows a customized dialog.
bool ScenarioSaveAs::ToggleSilent()
{
    if (!m_webView2_25)
        return false;
    m_silentSaveAs = !m_silentSaveAs;
    if (m_silentSaveAs && m_saveAsUIShowingToken.value == 0)
    {
        // Register a handler for the `SaveAsUIShowing` event.
        m_webView2_25->add_SaveAsUIShowing(
            Callback<ICoreWebView2SaveAsUIShowingEventHandler>(
                [this](ICoreWebView2* sender, ICoreWebView2SaveAsUIShowingEventArgs* args)
                    -> HRESULT
                {
                    // Hide the system default save as dialog.
                    CHECK_FAILURE(args->put_SuppressDefaultDialog(TRUE));

                    auto showCustomizedDialog = [this, args = wil::make_com_ptr(args)]
                    {
                        // Preview the content mime type, optional.
                        wil::unique_cotaskmem_string mimeType;
                        CHECK_FAILURE(args->get_ContentMimeType(&mimeType));

                        SaveAsDialog dialog(m_appWindow->GetMainWindow(), saveAsKind);
                        if (dialog.confirmed)
                        {
                            // Set the SaveAsFilePath, Kind, AllowReplace for the event
                            // args from this customized dialog inputs, optional. If nothing
                            // needs to input, the event args will provide default values.
                            CHECK_FAILURE(args->put_SaveAsFilePath(dialog.path.c_str()));
                            CHECK_FAILURE(args->put_Kind(dialog.selectedKind));
                            CHECK_FAILURE(args->put_AllowReplace(dialog.allowReplace));

                            BOOL allowReplace;
                            CHECK_FAILURE(args->get_AllowReplace(&allowReplace));
                            wil::unique_cotaskmem_string path;
                            CHECK_FAILURE(args->get_SaveAsFilePath(&path));
                            COREWEBVIEW2_SAVE_AS_KIND selectedKind;
                            CHECK_FAILURE(args->get_Kind(&selectedKind));

                            // Preview silent save as event args inputs, optional.
                            MessageBox(
                                m_appWindow->GetMainWindow(),
                                (std::wstring(L"Content Mime Type: ") + mimeType.get() + L"\n" +
                                 L"Fullpath: " + path.get() + L"\n" + L"Allow Replace: " +
                                 (allowReplace ? L"true" : L"false") + L"\n" +
                                 L"Selected Save As Kind: " + saveAsKindString[selectedKind])
                                    .c_str(),
                                L"Silent Save As Parameters Preview", MB_OK);
                        }
                        else
                        {
                            // Save As cancelled from this customized dialog.
                            CHECK_FAILURE(args->put_Cancel(TRUE));
                        }
                    };

                    wil::com_ptr<ICoreWebView2Deferral> deferral;
                    CHECK_FAILURE(args->GetDeferral(&deferral));

                    m_appWindow->RunAsync(
                        [deferral, showCustomizedDialog]()
                        {
                            showCustomizedDialog();
                            CHECK_FAILURE(deferral->Complete());
                        });
                    return S_OK;
                })
                .Get(),
            &m_saveAsUIShowingToken);
    }
    else
    {
        // Unregister the handler for the `SaveAsUIShowing` event.
        m_webView2_25->remove_SaveAsUIShowing(m_saveAsUIShowingToken);
        m_saveAsUIShowingToken.value = 0;
    }
    MessageBox(
        m_appWindow->GetMainWindow(),
        (m_silentSaveAs ? L"Silent Save As Enabled" : L"Silent Save As Disabled"), L"Info",
        MB_OK);
    return true;
}
//! [ToggleSilent]

//! [ProgrammaticSaveAs]
// Call ShowSaveAsUI method to trigger the programmatic save as.
bool ScenarioSaveAs::ProgrammaticSaveAs()
{
    if (!m_webView2_25)
        return false;
    m_webView2_25->ShowSaveAsUI(
        Callback<ICoreWebView2ShowSaveAsUICompletedHandler>(
            [this](HRESULT errorCode, COREWEBVIEW2_SAVE_AS_UI_RESULT result) -> HRESULT
            {
                // Show ShowSaveAsUI returned result, optional.
                MessageBox(
                    m_appWindow->GetMainWindow(),
                    (L"ShowSaveAsUI " + saveAsUIResultString[result]).c_str(), L"Info", MB_OK);
                return S_OK;
            })
            .Get());
    return true;
}
//! [ProgrammaticSaveAs]

bool ScenarioSaveAs::HandleWindowMessage(
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result)
{
    if (message == WM_COMMAND)
    {
        switch (LOWORD(wParam))
        {
        case IDM_SCENARIO_SAVE_AS_TOGGLE_SILENT:
        {
            return ToggleSilent();
        }
        case IDM_SCENARIO_SAVE_AS_PROGRAMMATIC:
        {
            return ProgrammaticSaveAs();
        }
        }
    }
    return false;
}

ScenarioSaveAs::~ScenarioSaveAs()
{
    if (m_webView2_25)
    {
        m_webView2_25->remove_SaveAsUIShowing(m_saveAsUIShowingToken);
    }
}

static INT_PTR CALLBACK DlgProcStatic(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto self = (SaveAsDialog*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    switch (message)
    {
    case WM_INITDIALOG:
    {
        self = (SaveAsDialog*)lParam;
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)self);
        HWND hwndList = GetDlgItem(hDlg, IDC_SAVE_AS_KIND);
        for (COREWEBVIEW2_SAVE_AS_KIND kind : self->kinds)
        {
            SendMessage(hwndList, CB_ADDSTRING, kind, (LPARAM)saveAsKindString[kind].c_str());
            SendMessage(hwndList, CB_SETITEMDATA, kind, (LPARAM)kind);
        }
        SendMessage(hwndList, CB_SETCURSEL, 0, 0);
        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
    {
        if (LOWORD(wParam) == IDOK)
        {
            HWND hwndList = GetDlgItem(hDlg, IDC_SAVE_AS_KIND);

            wchar_t path[MAX_PATH] = {};
            int length = GetWindowTextLength(GetDlgItem(hDlg, IDC_EDIT_SAVE_AS_DIRECTORY)) +
                         GetWindowTextLength(GetDlgItem(hDlg, IDC_EDIT_SAVE_AS_FILENAME));
            wchar_t directory[MAX_PATH] = {};
            GetDlgItemText(hDlg, IDC_EDIT_SAVE_AS_DIRECTORY, directory, length + 1);
            wchar_t filename[MAX_PATH] = {};
            GetDlgItemText(hDlg, IDC_EDIT_SAVE_AS_FILENAME, filename, length + 1);
            PathCombineW(path, directory, filename);
            self->path = path;

            self->allowReplace = IsDlgButtonChecked(hDlg, IDC_CHECK_SAVE_AS_ALLOW_REPLACE);

            int index = (int)SendMessage(hwndList, CB_GETCURSEL, 0, 0);
            self->selectedKind =
                (COREWEBVIEW2_SAVE_AS_KIND)SendMessage(hwndList, CB_GETITEMDATA, index, 0);

            self->confirmed = true;
        }
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    case WM_NCDESTROY:
        SetWindowLongPtr(hDlg, GWLP_USERDATA, NULL);
        return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}

SaveAsDialog::SaveAsDialog(HWND parent, std::initializer_list<COREWEBVIEW2_SAVE_AS_KIND> kinds)
    : kinds(kinds)
{
    DialogBoxParam(
        g_hInstance, MAKEINTRESOURCE(IDD_SAVE_CONTENT_AS), parent, DlgProcStatic, (LPARAM)this);
}