// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "TextInputDialog.h"

#include "App.h"
#include "resource.h"

static INT_PTR CALLBACK DlgProcStatic(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    auto* self = (TextInputDialog*)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    switch (message)
    {
    case WM_INITDIALOG:
        self = (TextInputDialog*)lParam;
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)self);

        SetWindowText(hDlg, self->title);
        SetDlgItemText(hDlg, IDC_STATIC_LABEL, self->prompt);
        SetDlgItemText(hDlg, IDC_EDIT_DESCRIPTION, self->description);
        SetDlgItemText(hDlg, IDC_EDIT_INPUT, self->input.data());
        if (self->readOnly)
        {
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_INPUT), false);
        }
        return (INT_PTR)TRUE;
    // TODO: don't close dialog if enter is pressed in edit control
    case WM_COMMAND:
        if (self && LOWORD(wParam) == IDOK)
        {
            int length = GetWindowTextLength(GetDlgItem(hDlg, IDC_EDIT_INPUT));
            self->input.resize(length);
            PWSTR data = const_cast<PWSTR>(self->input.data());
            GetDlgItemText(hDlg, IDC_EDIT_INPUT, data, length + 1);
            self->confirmed = true;
        }

        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            SetWindowLongPtr(hDlg, GWLP_USERDATA, NULL);
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    case WM_NCDESTROY:
        SetWindowLongPtr(hDlg, GWLP_USERDATA, NULL);
        return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}

TextInputDialog::TextInputDialog(
    HWND parent,
    PCWSTR title,
    PCWSTR prompt,
    PCWSTR description,
    const std::wstring& defaultInput,
    bool readOnly) :
        title(title), prompt(prompt), description(description), readOnly(readOnly), confirmed(false), input(defaultInput)
{
    DialogBoxParam(
        g_hInstance, MAKEINTRESOURCE(IDD_DIALOG_INPUT),
        parent, DlgProcStatic, (LPARAM)this);
}
