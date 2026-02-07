// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "AppWindow.h"

#include <Shellapi.h>
#include <ShlObj_core.h>
#include <functional>
#include <regex>
#include <sstream>
#include <string>

#include "App.h"
#include "CheckFailure.h"
#include "DpiUtil.h"
#include "Resource.h"
#include "ScenarioSLM.h"

using namespace Microsoft::WRL;

static constexpr size_t s_maxLoadString = 100;
static constexpr UINT s_runAsyncWindowMessage = WM_APP;
static thread_local size_t s_appInstances = 0;

AppWindow::AppWindow(const std::wstring& userDataFolderParam)
{
    // Initialize COM as STA.
    CHECK_FAILURE(OleInitialize(NULL));

    ++s_appInstances;

    WCHAR szTitle[s_maxLoadString];
    LoadStringW(g_hInstance, IDS_APP_TITLE, szTitle, s_maxLoadString);

    if (!userDataFolderParam.empty())
    {
        m_userDataFolder = userDataFolderParam;
    }

    m_mainWindow = CreateWindowExW(
        WS_EX_CONTROLPARENT, GetWindowClass(), szTitle,
        WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        CW_USEDEFAULT, 0, 1024, 768,
        nullptr, nullptr, g_hInstance, nullptr);

    SetWindowLongPtr(m_mainWindow, GWLP_USERDATA, (LONG_PTR)this);

    DpiUtil::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    ShowWindow(m_mainWindow, g_nCmdShow);
    UpdateWindow(m_mainWindow);

    // Check if WebView2 Runtime is installed
    wil::unique_cotaskmem_string version_info;
    HRESULT hr = GetAvailableCoreWebView2BrowserVersionString(nullptr, &version_info);
    if (hr == S_OK && version_info != nullptr)
    {
        RunAsync([this] { InitializeWebView(); });
    }
    else
    {
        MessageBox(
            m_mainWindow, L"WebView2 Runtime not installed. Please install it first.",
            L"WebView2 Runtime Required", MB_OK | MB_ICONERROR);
    }
}

AppWindow::~AppWindow()
{
    DeleteAllComponents();
}

// Register the Win32 window class for the app window.
PCWSTR AppWindow::GetWindowClass()
{
    static PCWSTR windowClass = []
    {
        static WCHAR windowClass[s_maxLoadString];
        LoadStringW(g_hInstance, IDC_WEBVIEW2APISAMPLEONNX, windowClass, s_maxLoadString);

        WNDCLASSEXW wcex;
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WndProcStatic;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = g_hInstance;
        wcex.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_WEBVIEW2APISAMPLEONNX));
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszMenuName = nullptr;  // No menu for this simplified app
        wcex.lpszClassName = windowClass;
        wcex.hIconSm = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_SMALL));

        RegisterClassExW(&wcex);
        return windowClass;
    }();
    return windowClass;
}

LRESULT CALLBACK AppWindow::WndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (auto app = (AppWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA))
    {
        LRESULT result = 0;
        if (app->HandleWindowMessage(hWnd, message, wParam, lParam, &result))
        {
            return result;
        }
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

bool AppWindow::HandleWindowMessage(
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result)
{
    // Give all components a chance to handle the message first.
    for (auto& component : m_components)
    {
        if (component->HandleWindowMessage(hWnd, message, wParam, lParam, result))
        {
            return true;
        }
    }

    switch (message)
    {
    case WM_SIZE:
    {
        if (lParam != 0)
        {
            ResizeEverything();
            return true;
        }
    }
    break;

    case s_runAsyncWindowMessage:
    {
        auto* task = reinterpret_cast<std::function<void()>*>(wParam);
        (*task)();
        delete task;
        return true;
    }
    break;

    case WM_CLOSE:
    {
        CloseAppWindow();
        return true;
    }
    break;

    case WM_NCDESTROY:
    {
        SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
        if (--s_appInstances == 0)
        {
            PostQuitMessage(0);
        }
    }
    break;
    }
    return false;
}

void AppWindow::ResizeEverything()
{
    RECT availableBounds = {0};
    GetClientRect(m_mainWindow, &availableBounds);

    if (m_controller)
    {
        m_controller->put_Bounds(availableBounds);
    }
}

void AppWindow::InitializeWebView()
{
    CloseWebView();

    auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();

    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr, m_userDataFolder.empty() ? nullptr : m_userDataFolder.c_str(), options.Get(),
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            this, &AppWindow::OnCreateEnvironmentCompleted)
            .Get());

    if (!SUCCEEDED(hr))
    {
        switch (hr)
        {
        case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
            MessageBox(m_mainWindow, L"WebView2 Runtime not found.", nullptr, MB_OK);
            break;
        default:
            ShowFailure(hr, L"Failed to create WebView2 environment");
            break;
        }
    }
}

HRESULT AppWindow::OnCreateEnvironmentCompleted(
    HRESULT result, ICoreWebView2Environment* environment)
{
    if (result != S_OK)
    {
        ShowFailure(result, L"Failed to create environment object.");
        return S_OK;
    }
    m_webViewEnvironment = environment;

    CHECK_FAILURE(m_webViewEnvironment->CreateCoreWebView2Controller(
        m_mainWindow, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                          this, &AppWindow::OnCreateCoreWebView2ControllerCompleted)
                          .Get()));

    return S_OK;
}

HRESULT AppWindow::OnCreateCoreWebView2ControllerCompleted(
    HRESULT result, ICoreWebView2Controller* controller)
{
    if (result == S_OK)
    {
        m_controller = controller;
        wil::com_ptr<ICoreWebView2> coreWebView2;
        CHECK_FAILURE(m_controller->get_CoreWebView2(&coreWebView2));
        coreWebView2.query_to(&m_webView);

        // Setup host resource mapping for local files
        wil::com_ptr<ICoreWebView2_3> webView3;
        if (SUCCEEDED(m_webView->QueryInterface(IID_PPV_ARGS(&webView3))))
        {
            webView3->SetVirtualHostNameToFolderMapping(
                L"appassets.example", L"assets",
                COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY_CORS);
        }

        RegisterEventHandlers();
        ResizeEverything();

        // Create the SLM scenario
        NewComponent<ScenarioSLM>(this);
    }
    else
    {
        ShowFailure(result, L"Failed to create WebView2 controller.");
    }
    return S_OK;
}

void AppWindow::RegisterEventHandlers()
{
    // Register any event handlers here if needed
}

bool AppWindow::CloseWebView()
{
    DeleteAllComponents();
    if (m_controller)
    {
        m_controller->Close();
        m_controller = nullptr;
        m_webView = nullptr;
    }
    m_webViewEnvironment = nullptr;
    return true;
}

void AppWindow::CloseAppWindow()
{
    CloseWebView();
    DestroyWindow(m_mainWindow);
}

std::wstring AppWindow::GetLocalPath(std::wstring relativePath, bool keep_exe_path)
{
    WCHAR rawPath[MAX_PATH];
    GetModuleFileNameW(g_hInstance, rawPath, MAX_PATH);
    std::wstring path(rawPath);
    if (keep_exe_path)
    {
        path.append(relativePath);
    }
    else
    {
        std::size_t index = path.find_last_of(L"\\") + 1;
        path.replace(index, path.length(), relativePath);
    }
    return path;
}

std::wstring AppWindow::GetLocalUri(std::wstring relativePath, bool useVirtualHostName)
{
    wil::com_ptr<ICoreWebView2_3> webView3;
    if (useVirtualHostName && m_webView && 
        SUCCEEDED(m_webView->QueryInterface(IID_PPV_ARGS(&webView3))))
    {
        const std::wstring localFileRootUrl = L"https://appassets.example/";
        return localFileRootUrl + std::regex_replace(relativePath, std::wregex(L"\\\\"), L"/");
    }
    else
    {
        std::wstring path = GetLocalPath(L"assets\\" + relativePath, false);
        wil::com_ptr<IUri> uri;
        CHECK_FAILURE(CreateUri(path.c_str(), Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME, 0, &uri));

        wil::unique_bstr uriBstr;
        CHECK_FAILURE(uri->GetAbsoluteUri(&uriBstr));
        return std::wstring(uriBstr.get());
    }
}

void AppWindow::RunAsync(std::function<void()> callback)
{
    auto* task = new std::function<void()>(std::move(callback));
    PostMessage(m_mainWindow, s_runAsyncWindowMessage, reinterpret_cast<WPARAM>(task), 0);
}

void AppWindow::DeleteComponent(ComponentBase* component)
{
    for (auto iter = m_components.begin(); iter != m_components.end(); ++iter)
    {
        if (iter->get() == component)
        {
            m_components.erase(iter);
            return;
        }
    }
}

void AppWindow::DeleteAllComponents()
{
    m_components.clear();
}
