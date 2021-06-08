// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include "pch.h"

#include "framework.h"
#include <regex>

using namespace Microsoft::WRL;

#define MAX_LOADSTRING 100

class CompositionHost;

class AppWindow
{
public:
    AppWindow();

    ICoreWebView2Controller* GetWebViewController()
    {
        return m_controller.get();
    }

    ICoreWebView2CompositionController* GetWebViewCompositionController()
    {
        return m_compositionController.get();
    }

    ICoreWebView2* GetWebView()
    {
        return m_webView.get();
    }

    HWND GetMainWindow()
    {
        return m_mainWindow;
    }

private:
    static LRESULT CALLBACK
    WndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static PCWSTR GetWindowClass();

    bool HandleWindowMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    void InitializeWebView();
    HRESULT OnCreateCoreWebView2ControllerCompleted(
        HRESULT result, ICoreWebView2CompositionController* compositionController);
    void CloseWebView();
    void CloseAppWindow();
    std::wstring GetLocalUri(std::wstring relativePath);
    std::wstring GetLocalPath(std::wstring relativePath, bool keep_exe_path);

    HWND m_mainWindow = nullptr;
    std::unique_ptr<CompositionHost> m_winComp;

    wil::com_ptr<ICoreWebView2Environment> m_webViewEnvironment;
    wil::com_ptr<ICoreWebView2CompositionController> m_compositionController;
    wil::com_ptr<ICoreWebView2Controller> m_controller;
    wil::com_ptr<ICoreWebView2> m_webView;
    std::wstring m_sampleUri;
};
