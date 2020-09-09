// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include "ComponentBase.h"
#include "Toolbar.h"
#include "resource.h"
#include <dcomp.h>
#include <functional>
#include <memory>
#include <ole2.h>
#include <string>
#include <vector>
#include <winnt.h>
#ifdef USE_WEBVIEW2_WIN10
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.ViewManagement.h>

namespace winrtComp = winrt::Windows::UI::Composition;
#endif

class SettingsComponent;

class AppWindow
{
public:
    AppWindow(
        UINT creationModeId,
        std::wstring initialUri = L"https://www.bing.com/",
        bool isMainWindow = false,
        std::function<void()> webviewCreatedCallback = nullptr,
        bool customWindowRect = false,
        RECT windowRect = { 0 },
        bool shouldHaveToolbar = true);

    ICoreWebView2Controller* GetWebViewController()
    {
        return m_controller.get();
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
    double GetDpiScale();
#ifdef USE_WEBVIEW2_WIN10
    double GetTextScale();
#endif

    void ReinitializeWebView();

    template <class ComponentType, class... Args> void NewComponent(Args&&... args);

    template <class ComponentType> ComponentType* GetComponent();

    void DeleteComponent(ComponentBase* scenario);

    void RunAsync(std::function<void(void)> callback);

    void AddRef();
    void Release();
    void NotifyClosed();

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
    HRESULT OnCreateCoreWebView2ControllerCompleted(HRESULT result, ICoreWebView2Controller* controller);
    HRESULT DeleteFileRecursive(std::wstring path);
    void RegisterEventHandlers();
    void ReinitializeWebViewWithNewBrowser();
    void RestartApp();
    void CloseWebView(bool cleanupUserDataFolder = false);
    void CloseAppWindow();
    void ChangeLanguage();
    void UpdateCreationModeMenu();
    void ToggleAADSSO();
#ifdef USE_WEBVIEW2_WIN10
    void OnTextScaleChanged(
        winrt::Windows::UI::ViewManagement::UISettings const& uiSettings,
        winrt::Windows::Foundation::IInspectable const& args);
#endif
    std::wstring GetLocalPath(std::wstring path, bool keep_exe_path);
    void DeleteAllComponents();

    template <class ComponentType> std::unique_ptr<ComponentType> MoveComponent();

    std::wstring m_initialUri;
    HWND m_mainWindow = nullptr;
    Toolbar m_toolbar;
    std::function<void()> m_onWebViewFirstInitialized;
    DWORD m_creationModeId = 0;
    int m_refCount = 1;
    bool m_isClosed = false;

    // The following is state that belongs with the webview, and should
    // be reinitialized along with it. Everything here is undefined when
    // m_webView is null.
    wil::com_ptr<ICoreWebView2Environment> m_webViewEnvironment;
    wil::com_ptr<ICoreWebView2Controller> m_controller;
    wil::com_ptr<ICoreWebView2> m_webView;

    // All components are deleted when the WebView is closed.
    std::vector<std::unique_ptr<ComponentBase>> m_components;
    std::unique_ptr<SettingsComponent> m_oldSettingsComponent;

    std::wstring m_language;

    bool m_AADSSOEnabled = false;

    // Fullscreen related code
    RECT m_previousWindowRect;
    HMENU m_hMenu;
    BOOL m_containsFullscreenElement = FALSE;
    bool m_fullScreenAllowed = true;
    bool m_isPopupWindow = false;
    void EnterFullScreen();
    void ExitFullScreen();

    // Compositor creation helper methods
    HRESULT DCompositionCreateDevice2(IUnknown* renderingDevice, REFIID riid, void** ppv);
    HRESULT TryCreateDispatcherQueue();

    wil::com_ptr<IDCompositionDevice> m_dcompDevice;
#ifdef USE_WEBVIEW2_WIN10
    winrtComp::Compositor m_wincompCompositor{ nullptr };
    winrt::Windows::UI::ViewManagement::UISettings m_uiSettings{ nullptr };
#endif
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
