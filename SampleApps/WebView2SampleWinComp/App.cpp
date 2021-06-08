// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pch.h"
#include "framework.h"
#include "App.h"
#include "Appwindow.h"
#include "CompositionHost.h"
#define MAX_LOADSTRING 100
#define WM_LBUTTONDOWN 0x0201

// Global Variables:
HINSTANCE hInst;                                // current instance
int nCmdShow;
int RunMessagePump();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       m_nCmdShow)
{
    hInst = hInstance;
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    nCmdShow = m_nCmdShow;

    AppWindow appWindow;
    int retVal = RunMessagePump();
    return retVal;
}

// Run the message pump for one thread.
int RunMessagePump()
{
    HACCEL hAccelTable = LoadAccelerators(hInst, MAKEINTRESOURCE(IDC_WEBVIEW2SAMPLEWINCOMP));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}
