// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include "ComponentBase.h"
#include "Toolbar.h"
#include "resource.h"
#include <functional>
#include <memory>
#include <ole2.h>
#include <string>
#include <vector>
#include <winnt.h>
class SettingsComponent;

class AppWindow
{
public:
    AppWindow(
        std::wstring initialUri = L"https://www.bing.com/",
        std::function<void()> webviewCreatedCallback = nullptr);

    ICoreWebView2Host* GetWebViewHost()
    {
        return m_host.get();
    }
    ICoreWebView2* GetWebView()
    {
        return m_webView.get();
    }
    HWND GetMainWindow()
    {
        return m_mainWindow;
    }
    void SetTitleText(PCWSTR titleText);
    RECT GetWindowBounds();
    std::wstring GetLocalUri(std::wstring path);
    std::function<void()> GetAcceleratorKeyFunction(UINT key);

    void ReinitializeWebView();

    template <class ComponentType, class... Args> void NewComponent(Args&&... args);

    template <class ComponentType> ComponentType* GetComponent();

    void DeleteComponent(ComponentBase* scenario);

    void RunAsync(std::function<void(void)> callback);
private:
    static PCWSTR GetWindowClass();

    static INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    static LRESULT CALLBACK
    WndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    bool HandleWindowMessage(
        HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result);

    bool ExecuteWebViewCommands(WPARAM wParam, LPARAM lParam);
    bool ExecuteAppCommands(WPARAM wParam, LPARAM lParam);

    void ResizeEverything();
    void InitializeWebView();
    HRESULT OnCreateEnvironmentCompleted(HRESULT result, ICoreWebView2Environment* environment);
    HRESULT OnCreateCoreWebView2HostCompleted(HRESULT result, ICoreWebView2Host* host);
    HRESULT DeleteFileRecursive(std::wstring path);
    void RegisterEventHandlers();
    void ReinitializeWebViewWithNewBrowser();
    void RestartApp();
    void CloseWebView(bool cleanupUserDataFolder = false);
    void CloseAppWindow();

    std::wstring GetLocalPath(std::wstring path);

    void DeleteAllComponents();

    template <class ComponentType> std::unique_ptr<ComponentType> MoveComponent();

    std::wstring m_initialUri;
    HWND m_mainWindow = nullptr;
    Toolbar m_toolbar;
    std::function<void()> m_onWebViewFirstInitialized;

    // The following is state that belongs with the webview, and should
    // be reinitialized along with it.  Everything here is undefined when
    // m_webView is null.
    wil::com_ptr<ICoreWebView2Environment> m_webViewEnvironment;
    wil::com_ptr<ICoreWebView2Host> m_host;
    wil::com_ptr<ICoreWebView2> m_webView;

    // All components are deleted when the WebView is closed.
    std::vector<std::unique_ptr<ComponentBase>> m_components;

    // This state is preserved between WebViews so we can recreate
    // a new WebView based on the settings of the old one.
    std::unique_ptr<SettingsComponent> m_oldSettingsComponent;
    // Fullscreen related code
    WINDOWPLACEMENT m_previousPlacement;
    HMENU m_hMenu;
    BOOL m_containsFullscreenElement = FALSE;
    bool m_fullScreenAllowed = true;
    bool m_isPopupWindow = false;
    void EnterFullScreen();
    void ExitFullScreen();
};

template <class ComponentType, class... Args> void AppWindow::NewComponent(Args&&... args)
{
    m_components.emplace_back(new ComponentType(std::forward<Args>(args)...));
}

template <class ComponentType> ComponentType* AppWindow::GetComponent()
{
    for (auto& component : m_components)
    {
        if (auto wanted = dynamic_cast<ComponentType*>(component.get()))
        {
            return wanted;
        }
    }
    return nullptr;
}
