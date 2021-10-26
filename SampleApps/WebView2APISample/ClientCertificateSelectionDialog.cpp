// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <ctime>
#include <Windows.h>

#include "stdafx.h"

#include "App.h"
#include "resource.h"
#include "ClientCertificateSelectionDialog.h"

std::wstring UnixEpochToDateTime(double value) {
    WCHAR rawResult[32] = {};
    std::time_t rawTime = std::time_t(value);
    struct tm timeStruct = {};
    gmtime_s(&timeStruct, &rawTime);
    _wasctime_s(rawResult, 32, &timeStruct);
    std::wstring result(rawResult);
    return result;
}

static INT_PTR CALLBACK ClientCertificateSelectionBoxDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    // self is read from GLWP_USERDATA pointer for the messages. Initially self is set to null
    // and gets assigned in the first invocation of WM_INITDIALOG.
    auto* self = (ClientCertificateSelectionDialog*)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        // Set self and then write it to GLWP_USERDATA using lParam.
        self = (ClientCertificateSelectionDialog*)lParam;
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)self);

        SetWindowText(hDlg, self->title);

        std::wstring siteInfo = L"Site " + std::wstring(self->host) + L":" + std::to_wstring(self->port) + L" needs your credentials:";
        SetDlgItemText(hDlg, IDC_CERTIFICATE_LBL, siteInfo.c_str());

        // Add items to list.
        HWND hwndList = GetDlgItem(hDlg, IDC_CERTIFICATE_LIST);
        for (size_t i = 0; i < self->clientCertificates.size(); i++)
        {
            std::wstring certNameIssuer = std::wstring((self->clientCertificates[i].DisplayName).get()) + L", " + std::wstring((self->clientCertificates[i].Issuer).get());
            int pos = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)certNameIssuer.c_str());
            // Set the index of the client certificate as item data.
            // This enables us to retrieve the item from the array
            // even after the items are sorted by the list box.
            SendMessage(hwndList, LB_SETITEMDATA, pos, (LPARAM)i);
        }
        // Set input focus to the list box.
        SetFocus(hwndList);
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_CERTIFICATE_OK:
        case ID_CERTIFICATE_CANCEL:
            if (self && LOWORD(wParam) == ID_CERTIFICATE_OK)
            {
                HWND hwndList = GetDlgItem(hDlg, IDC_CERTIFICATE_LIST);

                // Get selected index.
                int lbItem = (int)SendMessage(hwndList, LB_GETCURSEL, 0, 0);
                // Get item index before listbox sorted items alphabetically.
                int i = (int)SendMessage(hwndList, LB_GETITEMDATA, lbItem, 0);
                self->selectedItem = i;
                self->confirmed = true;
            }

            if (LOWORD(wParam) == ID_CERTIFICATE_OK || LOWORD(wParam) == ID_CERTIFICATE_CANCEL)
            {
                SetWindowLongPtr(hDlg, GWLP_USERDATA, NULL);
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;
        case IDC_CERTIFICATE_LIST:
        {
            switch (HIWORD(wParam))
            {
            case LBN_SELCHANGE:
            {
                HWND hwndList = GetDlgItem(hDlg, IDC_CERTIFICATE_LIST);

                // Get selected index.
                int lbItem = (int)SendMessage(hwndList, LB_GETCURSEL, 0, 0);

                // Get item data.
                int i = (int)SendMessage(hwndList, LB_GETITEMDATA, lbItem, 0);

                TCHAR buff[MAX_PATH];
                StringCbPrintf(buff, ARRAYSIZE(buff),
                    TEXT("Subject: %s\nValid From: %s\nValid To: %s\nCertificate Kind: %s"),
                    self->clientCertificates[i].Subject, UnixEpochToDateTime(self->clientCertificates[i].ValidFrom).c_str(), UnixEpochToDateTime(self->clientCertificates[i].ValidTo).c_str(), self->clientCertificates[i].CertificateKind);

                SetDlgItemText(hDlg, IDC_CERTIFICATE_STATIC, buff);
                return TRUE;
            }
            }
        }
            return TRUE;
        }
    case WM_NCDESTROY:
        SetWindowLongPtr(hDlg, GWLP_USERDATA, NULL);
        return (INT_PTR)TRUE;
    }

    return FALSE;
}

ClientCertificateSelectionDialog::ClientCertificateSelectionDialog(
    HWND parent,
    PCWSTR title,
    PCWSTR host,
    INT port,
    const std::vector<ClientCertificate> &clientCertificates) :
    title(title), host(host), port(port), confirmed(false), clientCertificates(clientCertificates)
{
    DialogBoxParam(
        g_hInstance, MAKEINTRESOURCE(IDD_CERTIFICATE_DIALOG), parent, ClientCertificateSelectionBoxDlg,
        (LPARAM)this);
}
