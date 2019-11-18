// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "AppWindow.h"

#include <functional>
#include <string>
#include <vector>
#include "App.h"
#include "CheckFailure.h"
#include "ControlComponent.h"
#include "FileComponent.h"
#include "ProcessComponent.h"
#include "Resource.h"
#include "ScenarioAddRemoteObject.h"
#include "ScenarioWebMessage.h"
#include "ScriptComponent.h"
#include "SettingsComponent.h"
#include "ViewComponent.h"

using namespace Microsoft::WRL;
static constexpr size_t s_maxLoadString = 100;

static constexpr UINT s_runAsyncWindowMessage = WM_APP;

static thread_local size_t s_appInstances = 0;

// Creates a new window which is a copy of the entire app, but on the same thread.
AppWindow::AppWindow(std::wstring initialUri) : m_initialUri(initialUri)
{
    ++s_appInstances;

    WCHAR szTitle[s_maxLoadString]; // The title bar text
    LoadStringW(g_hInstance, IDS_APP_TITLE, szTitle, s_maxLoadString);

    m_mainWindow = CreateWindowExW(
        WS_EX_CONTROLPARENT, GetWindowClass(), szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0,
        CW_USEDEFAULT, 0, nullptr, nullptr, g_hInstance, nullptr);
    SetWindowLongPtr(m_mainWindow, GWLP_USERDATA, (LONG_PTR)this);

    m_toolbar.Initialize(m_mainWindow);

    ShowWindow(m_mainWindow, g_nCmdShow);
    UpdateWindow(m_mainWindow);

    RunAsync([this] { InitializeWebView(kDefaultOption); });
}

// Register the Win32 window class for the app window.
PCWSTR AppWindow::GetWindowClass()
{
    // Only do this once
    static PCWSTR windowClass = [] {
        static WCHAR windowClass[s_maxLoadString];
        LoadStringW(g_hInstance, IDC_WEBVIEW2APISAMPLE, windowClass, s_maxLoadString);

        WNDCLASSEXW wcex;
        wcex.cbSize = sizeof(WNDCLASSEX);

        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WndProcStatic;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = g_hInstance;
        wcex.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_WEBVIEW2APISAMPLE));
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WEBVIEW2APISAMPLE);
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

// Handle Win32 window messages sent to the main window
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
        // Don't resize the app or webview when the app is minimized
        // let WM_SYSCOMMAND to handle it
        if (lParam != 0)
        {
            ResizeEverything();
            return true;
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        return true;
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
    case WM_NCDESTROY:
    {
        SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
        delete this;
        if (--s_appInstances == 0)
        {
            PostQuitMessage(0);
        }
    }
    break;
    //! [RestartManager]
    case WM_QUERYENDSESSION:
    {
        // yes, we can shut down
        // Register how we might be restarted
        RegisterApplicationRestart(L"--restore", RESTART_NO_CRASH | RESTART_NO_HANG);
        *result = TRUE;
        return true;
    }
    break;
    case WM_ENDSESSION:
    {
        if (wParam == TRUE)
        {
            // save app state and exit.
            PostQuitMessage(0);
            return true;
        }
    }
    break;
    //! [RestartManager]
    case WM_KEYDOWN:
    {
        // If bit 30 is set, it means the WM_KEYDOWN message is autorepeated.
        // We want to ignore it in that case.
        if (!(lParam & (1 << 30)))
        {
            if (auto action = GetAcceleratorKeyFunction((UINT)wParam))
            {
                action();
                return true;
            }
        }
    }
    break;
    case WM_COMMAND:
    {
        return ExecuteWebViewCommands(wParam, lParam) || ExecuteAppCommands(wParam, lParam);
    }
    break;
    }

    return false;
}

// Handle commands related to the WebView.
// This will do nothing if the WebView is not initialized.
bool AppWindow::ExecuteWebViewCommands(WPARAM wParam, LPARAM lParam)
{
    if (!m_webView)
        return false;
    switch (LOWORD(wParam))
    {
    case IDM_GET_BROWSER_VERSION_AFTER_CREATION:
    {
        //! [GetBrowserVersionInfo]
        wil::unique_cotaskmem_string version_info;
        m_webViewEnvironment->get_BrowserVersionInfo(&version_info);
        MessageBox(
            m_mainWindow, version_info.get(), L"Browser Version Info After WebView Creation",
            MB_OK);
        //! [GetBrowserVersionInfo]
        return true;
    }
    case IDM_CLOSE_WEBVIEW:
    {
        CloseWebView();
        return true;
    }
    case IDM_SCENARIO_POST_WEB_MESSAGE:
    {
        NewComponent<ScenarioWebMessage>(this);
        return true;
    }
    case IDM_SCENARIO_ADD_REMOTE_OBJECT:
    {
        NewComponent<ScenarioAddRemoteObject>(this);
        return true;
    }
    }
    return false;
}

// Handle commands not related to the WebView, which will work even if the WebView
// is not currently initialized.
bool AppWindow::ExecuteAppCommands(WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
    case IDM_ABOUT:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), m_mainWindow, About);
        return true;
    case IDM_GET_BROWSER_VERSION_BEFORE_CREATION:
    {
        wil::unique_cotaskmem_string version_info;
        GetWebView2BrowserVersionInfo(nullptr, &version_info);
        MessageBox(
            m_mainWindow, version_info.get(), L"Browser Version Info Before WebView Creation",
            MB_OK);
        return true;
    }
    case IDM_EXIT:
        CloseAppWindow();
        return true;
    case IDM_REINIT_INSTALLED:
        InitializeWebView(kUseInstalledBrowser);
        return true;
    case IDM_NEW_WINDOW:
        new AppWindow();
        return true;
    case IDM_NEW_THREAD:
        CreateNewThread();
        return true;
    }
    return false;
}
// Message handler for about dialog.
INT_PTR CALLBACK AppWindow::About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// Decide what to do when an accelerator key is pressed. Instead of immediately performing
// the action, we hand it to the caller so they can decide whether to run it right away
// or running it asynchronously. Will return nullptr if there is no action for the key.
std::function<void()> AppWindow::GetAcceleratorKeyFunction(UINT key)
{
    if (GetKeyState(VK_CONTROL) < 0)
    {
        switch (key)
        {
        case 'N':
            return [this] { new AppWindow(); };
        case 'Q':
            return [this] { CloseAppWindow(); };
        case 'S':
            return [this] {
                if (auto file = GetComponent<FileComponent>())
                {
                    file->SaveScreenshot();
                }
            };
        case 'T':
            return [this] { CreateNewThread(); };
        case 'W':
            return [this] { CloseWebView(); };
        }
    }
    return nullptr;
}

//! [CreateWebView]
// Create or recreate the WebView and its environment.
void AppWindow::InitializeWebView(InitializeWebViewFlags webviewInitFlags)
{
    m_lastUsedInitFlags = webviewInitFlags;
    // To ensure browser switches get applied correctly, we need to close
    // the existing WebView. This will result in a new browser process
    // getting created which will apply the browser switches.
    CloseWebView();

    LPCWSTR subFolder = nullptr;
    LPCWSTR additionalBrowserSwitches = nullptr;
    HRESULT hr = CreateWebView2EnvironmentWithDetails(
        subFolder, nullptr, additionalBrowserSwitches,
        Callback<IWebView2CreateWebView2EnvironmentCompletedHandler>(
            this, &AppWindow::OnCreateEnvironmentCompleted)
            .Get());
    if (!SUCCEEDED(hr))
    {
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            MessageBox(
                m_mainWindow,
                L"Couldn't find Edge installation. "
                "Do you have a version installed that's compatible with this "
                "WebView2 SDK version?",
                nullptr, MB_OK);
        }
        else
        {
            ShowFailure(hr, L"Failed to create webview environment");
        }
    }
}

// This is the callback passed to CreateWebViewEnvironmnetWithDetails.
// Here we simply create the WebView.
HRESULT AppWindow::OnCreateEnvironmentCompleted(
    HRESULT result, IWebView2Environment* environment)
{
    CHECK_FAILURE(result);

    CHECK_FAILURE(environment->QueryInterface(IID_PPV_ARGS(&m_webViewEnvironment)));
        CHECK_FAILURE(m_webViewEnvironment->CreateWebView(
            m_mainWindow, Callback<IWebView2CreateWebViewCompletedHandler>(
                              this, &AppWindow::OnCreateWebViewCompleted)
                              .Get()));
    return S_OK;
}
//! [CreateWebView]

// This is the callback passed to CreateWebView. Here we initialize all WebView-related
// state and register most of our event handlers with the WebView.
HRESULT AppWindow::OnCreateWebViewCompleted(HRESULT result, IWebView2WebView* webview)
{
    if (result == S_OK)
    {
        // We should check for failure here because if this app is using a newer
        // SDK version compared to the install of the Edge browser, the Edge
        // browser might not have support for the latest version of the
        // IWebView2WebViewN interface.
        CHECK_FAILURE(webview->QueryInterface(IID_PPV_ARGS(&m_webView)));
        // Create components. These will be deleted when the WebView is closed.
        NewComponent<FileComponent>(this);
        NewComponent<ProcessComponent>(this);
        NewComponent<ScriptComponent>(this);
        NewComponent<SettingsComponent>(
            this, m_webViewEnvironment.get(), m_oldSettingsComponent.get());
        m_oldSettingsComponent = nullptr;
        NewComponent<ViewComponent>(this);
        NewComponent<ControlComponent>(this, &m_toolbar);

        // We have a few of our own event handlers to register here as well
        RegisterEventHandlers();

        // Set the initial size of the WebView
        ResizeEverything();

        if (m_onWebViewFirstInitialized)
        {
            m_onWebViewFirstInitialized();
            m_onWebViewFirstInitialized = nullptr;
        }

        if (!m_initialUri.empty())
        {
            m_webView->Navigate(m_initialUri.c_str());
        }
    }
    else
    {
        ShowFailure(result, L"Failed to create webview");
    }
    return S_OK;
}
void AppWindow::ReinitializeWebView()
{
    // Save the settings component from being deleted when the WebView is closed, so we can
    // copy its properties to the next settings component.
    m_oldSettingsComponent = MoveComponent<SettingsComponent>();
    // Workaround for Edge Bug 23006983
    RunAsync([this] { InitializeWebView(m_lastUsedInitFlags); });
}

void AppWindow::ReinitializeWebViewWithNewBrowser()
{
    // Save the settings component from being deleted when the WebView is closed, so we can
    // copy its properties to the next settings component.
    m_oldSettingsComponent = MoveComponent<SettingsComponent>();

    // Use the reference to the web view before we close it
    UINT webviewProcessId = 0;
    m_webView->get_BrowserProcessId(&webviewProcessId);

    // We need to close the current webviews and wait for the browser_process to exit
    // This is so the new webviews don't use the old browser exe
    CloseWebView();

    // Make sure the browser process inside webview is closed
    ProcessComponent::EnsureProcessIsClosed(webviewProcessId, 2000);

    InitializeWebView(m_lastUsedInitFlags);
}

void AppWindow::RestartApp()
{
    // Use the reference to the web view before we close the app window
    UINT webviewProcessId = 0;
    m_webView->get_BrowserProcessId(&webviewProcessId);

    // To restart the app completely, first we close the current App Window
    CloseAppWindow();

    // Make sure the browser process inside webview is closed
    ProcessComponent::EnsureProcessIsClosed(webviewProcessId, 2000);

    // Get the command line arguments used to start this app
    // so we can re-create the process with them
    LPWSTR args = GetCommandLineW();

    STARTUPINFOW startup_info = {0};
    startup_info.cb = sizeof(startup_info);
    PROCESS_INFORMATION temp_process_info = {};
    // Start a new process
    if (!::CreateProcess(
            nullptr, args,
            nullptr, // default process attributes
            nullptr, // default thread attributes
            FALSE,   // do not inherit handles
            0,
            nullptr, // no environment
            nullptr, // default current directory
            &startup_info, &temp_process_info))
    {
        // Log some error information if desired
    }

    // Terminate this current process
    ::exit(0);
}

void AppWindow::RegisterEventHandlers()
{
    //! [NewWindowRequested]
    // Register a handler for the NewWindowRequested event.
    // This handler will defer the event, create a new app window, and then once the
    // new window is ready, it'll provide that new window's WebView as the response to
    // the request.
    CHECK_FAILURE(m_webView->add_NewWindowRequested(
        Callback<IWebView2NewWindowRequestedEventHandler>(
            [this](IWebView2WebView* sender, IWebView2NewWindowRequestedEventArgs* args) {
                wil::com_ptr<IWebView2Deferral> deferral;
                CHECK_FAILURE(args->GetDeferral(&deferral));

                auto newAppWindow = new AppWindow(L"");
                newAppWindow->m_onWebViewFirstInitialized = [args, deferral, newAppWindow]() {
                    CHECK_FAILURE(args->put_NewWindow(newAppWindow->m_webView.get()));
                    CHECK_FAILURE(args->put_Handled(TRUE));
                    CHECK_FAILURE(deferral->Complete());
                };

                return S_OK;
            })
            .Get(),
        &m_newWindowRequestedToken));
    //! [NewWindowRequested]

    //! [NewVersionAvailable]
    // After the environment is successfully created,
    // register a handler for the NewVersionAvailable event.
    // This handler tells when there is a new Edge version available on the machine.
    CHECK_FAILURE(m_webViewEnvironment->add_NewVersionAvailable(
        Callback<IWebView2NewVersionAvailableEventHandler>(
            [this](IWebView2Environment* sender, IWebView2NewVersionAvailableEventArgs* args)
                -> HRESULT {
                // Get the version value from args
                wil::unique_cotaskmem_string newVersion;
                CHECK_FAILURE(args->get_NewVersion(&newVersion));
                std::wstring message = L"We detected there is a new version for the browser.";
                message += L"\n\nVersion number: ";
                message += newVersion.get();
                message += L"\n\n";
                message += L"Do you want to restart the app? \n\n";
                message += L"Click No if you only want to re-create the webviews. \n";
                message += L"Click Cancel for no action. \n";

                int response = MessageBox(
                    m_mainWindow, message.c_str(), L"New available version", MB_YESNOCANCEL);

                if (response == IDYES)
                {
                    RestartApp();
                }
                else if (response == IDNO)
                {
                    ReinitializeWebViewWithNewBrowser();
                }
                else
                {
                    // do nothing
                }

                return S_OK;
            })
            .Get(),
        &m_newVersionAvailableToken));
    //! [NewVersionAvailable]
}

// Updates the sizing and positioning of everything in the window.
void AppWindow::ResizeEverything()
{
    RECT availableBounds = {0};
    GetClientRect(m_mainWindow, &availableBounds);
        availableBounds = m_toolbar.Resize(availableBounds);
    if (auto view = GetComponent<ViewComponent>())
    {
        view->SetBounds(availableBounds);
    }
}

//! [Close]
// Close the WebView and deinitialize related state. This doesn't close the app window.
void AppWindow::CloseWebView()
{
    DeleteAllComponents();
    if (m_webView)
    {
        m_webView->Close();
        m_webView = nullptr;
    }
    m_webViewEnvironment = nullptr;
}
//! [Close]

void AppWindow::CloseAppWindow()
{
    CloseWebView();
    DestroyWindow(m_mainWindow);
}

void AppWindow::DeleteComponent(ComponentBase* component)
{
    for (auto iter = m_components.begin(); iter != m_components.end(); iter++)
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
    // Delete components in reverse order of initialization.
    while (!m_components.empty())
    {
        m_components.pop_back();
    }
}

template <class ComponentType> std::unique_ptr<ComponentType> AppWindow::MoveComponent()
{
    for (auto iter = m_components.begin(); iter != m_components.end(); iter++)
    {
        if (dynamic_cast<ComponentType*>(iter->get()))
        {
            auto wanted = reinterpret_cast<std::unique_ptr<ComponentType>&&>(std::move(*iter));
            m_components.erase(iter);
            return std::move(wanted);
        }
    }
    return nullptr;
}

void AppWindow::SetTitleText(PCWSTR titleText)
{
    SetWindowText(m_mainWindow, titleText);
}

RECT AppWindow::GetWindowBounds()
{
    RECT hwndBounds = {0};
    GetClientRect(m_mainWindow, &hwndBounds);
    return hwndBounds;
}

std::wstring AppWindow::GetLocalPath(std::wstring relativePath)
{
    WCHAR rawPath[MAX_PATH];
    GetModuleFileNameW(g_hInstance, rawPath, MAX_PATH);
    std::wstring path(rawPath);

    std::size_t index = path.find_last_of(L"\\") + 1;
    path.replace(index, path.length(), relativePath);
    return path;
}

std::wstring AppWindow::GetLocalUri(std::wstring relativePath)
{
    std::wstring path = GetLocalPath(relativePath);

    wil::com_ptr<IUri> uri;
    CHECK_FAILURE(CreateUri(path.c_str(), Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME, 0, &uri));

    wil::unique_bstr uriBstr;
    CHECK_FAILURE(uri->GetAbsoluteUri(&uriBstr));
    return std::wstring(uriBstr.get());
}

void AppWindow::RunAsync(std::function<void()> callback)
{
    auto* task = new std::function<void()>(callback);
    PostMessage(m_mainWindow, s_runAsyncWindowMessage, reinterpret_cast<WPARAM>(task), 0);
}
