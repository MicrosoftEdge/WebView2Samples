// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "PermissionDialog.h"

#include "App.h"
#include "ScenarioPermissionManagement.h"
#include "resource.h"

static INT_PTR CALLBACK DlgProcStatic(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto self = (PermissionDialog*)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        self = (PermissionDialog*)lParam;
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)self);
        HWND hwndList = GetDlgItem(hDlg, IDC_PERMISSION_KIND);
        for (COREWEBVIEW2_PERMISSION_KIND kind_enum : self->permissionKinds)
        {
            int pos = (int)SendMessage(
                hwndList, CB_ADDSTRING, 0, (LPARAM)PermissionKindToString(kind_enum).c_str());
            SendMessage(hwndList, CB_SETITEMDATA, pos, (LPARAM)kind_enum);
        }
        hwndList = GetDlgItem(hDlg, IDC_PERMISSION_STATE);
        for (COREWEBVIEW2_PERMISSION_STATE state_enum : self->permissionStates)
        {
            int pos = (int)SendMessage(
                hwndList, CB_ADDSTRING, 0, (LPARAM)PermissionStateToString(state_enum).c_str());
            SendMessage(hwndList, CB_SETITEMDATA, pos, (LPARAM)state_enum);
        }
        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
    {
        if (LOWORD(wParam) == IDOK)
        {
            int length = GetWindowTextLength(GetDlgItem(hDlg, IDC_EDIT_PERMISSION_ORIGIN));
            wchar_t origin[MAX_PATH] = {};
            GetDlgItemText(hDlg, IDC_EDIT_PERMISSION_ORIGIN, origin, length + 1);
            self->origin = origin;

            HWND hwndList = GetDlgItem(hDlg, IDC_PERMISSION_KIND);
            int index = (int)SendMessage(hwndList, CB_GETCURSEL, 0, 0);
            COREWEBVIEW2_PERMISSION_KIND kind =
                (COREWEBVIEW2_PERMISSION_KIND)SendMessage(hwndList, CB_GETITEMDATA, index, 0);
            self->kind = kind;

            hwndList = GetDlgItem(hDlg, IDC_PERMISSION_STATE);
            index = (int)SendMessage(hwndList, CB_GETCURSEL, 0, 0);
            COREWEBVIEW2_PERMISSION_STATE state =
                (COREWEBVIEW2_PERMISSION_STATE)SendMessage(hwndList, CB_GETITEMDATA, index, 0);
            self->state = state;

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

PermissionDialog::PermissionDialog(
    HWND parent, std::vector<COREWEBVIEW2_PERMISSION_KIND> kinds,
    std::vector<COREWEBVIEW2_PERMISSION_STATE> states)
    : permissionKinds(kinds), permissionStates(states)
{
    DialogBoxParam(
        g_hInstance, MAKEINTRESOURCE(IDD_SET_PERMISSION), parent, DlgProcStatic, (LPARAM)this);
}
