// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include "ComponentBase.h"
#include "resource.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

class AppWindow
{
public:
    AppWindow(const std::wstring& userDataFolderParam = L"");
    ~AppWindow();

    ICoreWebView2Controller* GetWebViewController()
    {
        return m_controller.get();
    }
    ICoreWebView2* GetWebView()
    {
        return m_webView.get();
    }
    ICoreWebView2Environment* GetWebViewEnvironment()
    {
        return m_webViewEnvironment.get();
    }
    HWND GetMainWindow()
    {
        return m_mainWindow;
    }

    std::wstring GetLocalUri(std::wstring path, bool useVirtualHostName = true);

    template <class ComponentType, class... Args> void NewComponent(Args&&... args);
    template <class ComponentType> ComponentType* GetComponent();
    void DeleteComponent(ComponentBase* component);

    // Runs a function by posting it to the event loop
    void RunAsync(std::function<void(void)> callback);

private:
    static PCWSTR GetWindowClass();
    static LRESULT CALLBACK WndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    bool HandleWindowMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result);

    void ResizeEverything();
    void InitializeWebView();
    HRESULT OnCreateEnvironmentCompleted(HRESULT result, ICoreWebView2Environment* environment);
    HRESULT OnCreateCoreWebView2ControllerCompleted(HRESULT result, ICoreWebView2Controller* controller);
    void RegisterEventHandlers();
    bool CloseWebView();
    void CloseAppWindow();
    std::wstring GetLocalPath(std::wstring path, bool keep_exe_path);
    void DeleteAllComponents();

    std::wstring m_userDataFolder;
    HWND m_mainWindow = nullptr;

    wil::com_ptr<ICoreWebView2Environment> m_webViewEnvironment;
    wil::com_ptr<ICoreWebView2Controller> m_controller;
    wil::com_ptr<ICoreWebView2> m_webView;

    std::vector<std::unique_ptr<ComponentBase>> m_components;
};

// Creates and registers a component on this AppWindow.
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
