// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "AppWindow.h"

#include <DispatcherQueue.h>
#include <ShObjIdl_core.h>
#include <Shellapi.h>
#include <ShlObj_core.h>
#include <functional>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <winrt/windows.system.h>

#include "App.h"
#include "AppStartPage.h"
#include "AudioComponent.h"
#include "CheckFailure.h"
#include "ControlComponent.h"
#include "DpiUtil.h"
#include "FileComponent.h"
#include "ProcessComponent.h"
#include "Resource.h"
#include "ScenarioAcceleratorKeyPressed.h"
#include "ScenarioAddHostObject.h"
#include "ScenarioAuthentication.h"
#include "ScenarioClientCertificateRequested.h"
#include "ScenarioCookieManagement.h"
#include "ScenarioCustomDownloadExperience.h"
#include "ScenarioCustomScheme.h"
#include "ScenarioCustomSchemeNavigate.h"
#include "ScenarioDefaultBackgroundColor.h"
#include "ScenarioDOMContentLoaded.h"
#include "ScenarioDragDrop.h"
#include "ScenarioDragDropOverride.h"
#include "ScenarioExtensionsManagement.h"
#include "ScenarioFileSystemHandleShare.h"
#include "ScenarioFileTypePolicy.h"
#include "ScenarioIFrameDevicePermission.h"
#include "ScenarioNavigateWithWebResourceRequest.h"
#include "ScenarioNonClientRegionSupport.h"
#include "ScenarioNotificationReceived.h"
#include "ScenarioPermissionManagement.h"
#include "ScenarioSaveAs.h"
#include "ScenarioScreenCapture.h"
#include "ScenarioSharedBuffer.h"
#include "ScenarioSharedWorkerWRR.h"
#include "ScenarioThrottlingControl.h"
#include "ScenarioVirtualHostMappingForPopUpWindow.h"
#include "ScenarioVirtualHostMappingForSW.h"
#include "ScenarioWebMessage.h"
#include "ScenarioWebViewEventMonitor.h"
#include "ScriptComponent.h"
#include "SettingsComponent.h"
#include "TextInputDialog.h"
#include "ViewComponent.h"
using namespace Microsoft::WRL;
static constexpr size_t s_maxLoadString = 100;
static constexpr UINT s_runAsyncWindowMessage = WM_APP;

static thread_local size_t s_appInstances = 0;
// The minimum height and width for Window Features.
// See https://developer.mozilla.org/docs/Web/API/Window/open#Size
static constexpr int s_minNewWindowSize = 100;

// Run Download and Install in another thread so we don't block the UI thread
DWORD WINAPI DownloadAndInstallWV2RT(_In_ LPVOID lpParameter)
{
    AppWindow* appWindow = (AppWindow*)lpParameter;

    int returnCode = 2; // Download failed
    // Use fwlink to download WebView2 Bootstrapper at runtime and invoke installation
    // Broken/Invalid Https Certificate will fail to download
    // Use of the download link below is governed by the below terms. You may acquire the link
    // for your use at https://developer.microsoft.com/microsoft-edge/webview2/. Microsoft owns
    // all legal right, title, and interest in and to the WebView2 Runtime Bootstrapper
    // ("Software") and related documentation, including any intellectual property in the
    // Software. You must acquire all code, including any code obtained from a Microsoft URL,
    // under a separate license directly from Microsoft, including a Microsoft download site
    // (e.g., https://developer.microsoft.com/microsoft-edge/webview2/).
    HRESULT hr = URLDownloadToFile(
        NULL, L"https://go.microsoft.com/fwlink/p/?LinkId=2124703",
        L".\\MicrosoftEdgeWebview2Setup.exe", 0, 0);
    if (hr == S_OK)
    {
        // Either Package the WebView2 Bootstrapper with your app or download it using fwlink
        // Then invoke install at Runtime.
        SHELLEXECUTEINFO shExInfo = {0};
        shExInfo.cbSize = sizeof(shExInfo);
        shExInfo.fMask = SEE_MASK_NOASYNC;
        shExInfo.hwnd = 0;
        shExInfo.lpVerb = L"runas";
        shExInfo.lpFile = L"MicrosoftEdgeWebview2Setup.exe";
        shExInfo.lpParameters = L" /silent /install";
        shExInfo.lpDirectory = 0;
        shExInfo.nShow = 0;
        shExInfo.hInstApp = 0;

        if (ShellExecuteEx(&shExInfo))
        {
            returnCode = 0; // Install successful
        }
        else
        {
            returnCode = 1; // Install failed
        }
    }

    appWindow->InstallComplete(returnCode);
    appWindow->Release();
    return returnCode;
}

static INT_PTR CALLBACK DlgProcStatic(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto app = (AppWindow*)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        app = (AppWindow*)lParam;
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)app);

        SetDlgItemText(hDlg, IDC_EDIT_PROFILE, app->GetWebViewOption().profile.c_str());
        SetDlgItemText(
            hDlg, IDC_EDIT_DOWNLOAD_PATH, app->GetWebViewOption().downloadPath.c_str());
        SetDlgItemText(hDlg, IDC_EDIT_LOCALE, app->GetWebViewOption().scriptLocale.c_str());
        CheckDlgButton(hDlg, IDC_CHECK_INPRIVATE, app->GetWebViewOption().isInPrivate);

        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
    {
        if (LOWORD(wParam) == IDOK)
        {
            int length = GetWindowTextLength(GetDlgItem(hDlg, IDC_EDIT_PROFILE));
            wchar_t text[MAX_PATH] = {};
            GetDlgItemText(hDlg, IDC_EDIT_PROFILE, text, length + 1);
            bool inPrivate = IsDlgButtonChecked(hDlg, IDC_CHECK_INPRIVATE);

            int downloadPathLength =
                GetWindowTextLength(GetDlgItem(hDlg, IDC_EDIT_DOWNLOAD_PATH));
            wchar_t downloadPath[MAX_PATH] = {};
            GetDlgItemText(hDlg, IDC_EDIT_DOWNLOAD_PATH, downloadPath, downloadPathLength + 1);

            int scriptLocaleLength = GetWindowTextLength(GetDlgItem(hDlg, IDC_EDIT_LOCALE));
            wchar_t scriptLocale[MAX_PATH] = {};
            GetDlgItemText(hDlg, IDC_EDIT_LOCALE, scriptLocale, scriptLocaleLength + 1);

            bool useOsRegion = IsDlgButtonChecked(hDlg, IDC_CHECK_USE_OS_REGION);

            WebViewCreateOption opt(
                std::wstring(std::move(text)), inPrivate, std::wstring(std::move(downloadPath)),
                std::wstring(std::move(scriptLocale)),
                WebViewCreateEntry::EVER_FROM_CREATE_WITH_OPTION_MENU, useOsRegion);

            // create app window
            new AppWindow(app->GetCreationModeId(), opt);
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

void WebViewCreateOption::PopupDialog(AppWindow* app)
{
    DialogBoxParam(
        g_hInstance, MAKEINTRESOURCE(IDD_WEBVIEW2_OPTION), app->GetMainWindow(), DlgProcStatic,
        (LPARAM)app);
}

// Creates a new window which is a copy of the entire app, but on the same thread.
AppWindow::AppWindow(
    UINT creationModeId, const WebViewCreateOption& opt, const std::wstring& initialUri,
    const std::wstring& userDataFolderParam, bool isMainWindow,
    std::function<void()> webviewCreatedCallback, bool customWindowRect, RECT windowRect,
    bool shouldHaveToolbar, bool isPopup)
    : m_creationModeId(creationModeId), m_webviewOption(opt), m_initialUri(initialUri),
      m_onWebViewFirstInitialized(webviewCreatedCallback), m_isPopupWindow(isPopup)
{
    // Initialize COM as STA.
    CHECK_FAILURE(OleInitialize(NULL));

    ++s_appInstances;

    WCHAR szTitle[s_maxLoadString]; // The title bar text
    LoadStringW(g_hInstance, IDS_APP_TITLE, szTitle, s_maxLoadString);
    m_appTitle = szTitle;
    DWORD windowStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    if (userDataFolderParam.length() > 0)
    {
        m_userDataFolder = userDataFolderParam;
    }

    if (customWindowRect)
    {
        m_mainWindow = CreateWindowExW(
            WS_EX_CONTROLPARENT, GetWindowClass(), szTitle, windowStyle, windowRect.left,
            windowRect.top, windowRect.right - windowRect.left,
            windowRect.bottom - windowRect.top, nullptr, nullptr, g_hInstance, nullptr);
    }
    else
    {
        m_mainWindow = CreateWindowExW(
            WS_EX_CONTROLPARENT, GetWindowClass(), szTitle, windowStyle, CW_USEDEFAULT, 0,
            CW_USEDEFAULT, 0, nullptr, nullptr, g_hInstance, nullptr);
    }

    m_appBackgroundImageHandle = (HBITMAP)LoadImage(
        g_hInstance, MAKEINTRESOURCE(IDI_WEBVIEW2_BACKGROUND), IMAGE_BITMAP, 0, 0,
        LR_DEFAULTCOLOR);
    GetObject(m_appBackgroundImageHandle, sizeof(m_appBackgroundImage), &m_appBackgroundImage);
    m_memHdc = CreateCompatibleDC(GetDC(m_mainWindow));
    SelectObject(m_memHdc, m_appBackgroundImageHandle);

    SetWindowLongPtr(m_mainWindow, GWLP_USERDATA, (LONG_PTR)this);

    //! [TextScaleChanged1]
    if (winrt::try_get_activation_factory<winrt::Windows::UI::ViewManagement::UISettings>())
    {
        m_uiSettings = winrt::Windows::UI::ViewManagement::UISettings();
        m_uiSettings.TextScaleFactorChanged({this, &AppWindow::OnTextScaleChanged});
    }
    //! [TextScaleChanged1]

    if (shouldHaveToolbar)
    {
        m_toolbar.Initialize(this);
    }

    UpdateCreationModeMenu();
    ShowWindow(m_mainWindow, g_nCmdShow);
    UpdateWindow(m_mainWindow);
    // If no WebView2 Runtime installed, create new thread to do install/download.
    // Otherwise just initialize webview.
    wil::unique_cotaskmem_string version_info;
    HRESULT hr = GetAvailableCoreWebView2BrowserVersionString(nullptr, &version_info);
    if (hr == S_OK && version_info != nullptr)
    {
        RunAsync([this] { InitializeWebView(); });
    }
    else
    {
        if (isMainWindow)
        {
            AddRef();
            CreateThread(0, 0, DownloadAndInstallWV2RT, (void*)this, 0, 0);
        }
        else
        {
            MessageBox(
                m_mainWindow, L"WebView Runtime not installed",
                L"WebView Runtime Installation status", MB_OK);
        }
    }
}
AppWindow::~AppWindow()
{
    DeleteObject(m_appBackgroundImageHandle);
    DeleteDC(m_memHdc);
}

// Register the Win32 window class for the app window.
PCWSTR AppWindow::GetWindowClass()
{
    // Only do this once
    static PCWSTR windowClass = []
    {
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
        wcex.hIconSm = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_WEBVIEW2APISAMPLE));

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
    //! [DPIChanged]
    case WM_DPICHANGED:
    {
        m_toolbar.UpdateDpiAndTextScale();
        if (auto view = GetComponent<ViewComponent>())
        {
            view->UpdateDpiAndTextScale();
        }

        RECT* const newWindowSize = reinterpret_cast<RECT*>(lParam);
        SetWindowPos(
            hWnd, nullptr, newWindowSize->left, newWindowSize->top,
            newWindowSize->right - newWindowSize->left,
            newWindowSize->bottom - newWindowSize->top, SWP_NOZORDER | SWP_NOACTIVATE);
        return true;
    }
    break;
    //! [DPIChanged]
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc;
        BeginPaint(hWnd, &ps);

        hdc = GetDC(hWnd);

        StretchBlt(
            hdc, m_appBackgroundImageRect.left, m_appBackgroundImageRect.top,
            m_appBackgroundImageRect.right, m_appBackgroundImageRect.bottom, m_memHdc, 0, 0,
            m_appBackgroundImage.bmWidth, m_appBackgroundImage.bmHeight, SRCCOPY);

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
    case WM_CLOSE:
    {
        CloseAppWindow();
        return true;
    }
    break;
    case WM_NCDESTROY:
    {
        int retValue = 0;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
        NotifyClosed();
        if (--s_appInstances == 0)
        {
            PostQuitMessage(retValue);
        }
        Release();
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
    case WM_SYSKEYDOWN:
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
        //! [GetBrowserVersionString]
        wil::unique_cotaskmem_string version_info;
        m_webViewEnvironment->get_BrowserVersionString(&version_info);
        MessageBox(
            m_mainWindow, version_info.get(), L"Browser Version Info After WebView Creation",
            MB_OK);
        //! [GetBrowserVersionString]
        return true;
    }
    case IDM_GET_USER_DATA_FOLDER:
    {
        //! [GetUserDataFolder]
        auto environment7 = m_webViewEnvironment.try_query<ICoreWebView2Environment7>();
        CHECK_FEATURE_RETURN(environment7);
        wil::unique_cotaskmem_string userDataFolder;
        environment7->get_UserDataFolder(&userDataFolder);
        MessageBox(m_mainWindow, userDataFolder.get(), L"User Data Folder", MB_OK);
        //! [GetUserDataFolder]
        return true;
    }
    case IDM_GET_FAILURE_REPORT_FOLDER:
    {
        //! [GetFailureReportFolder]
        auto environment11 = m_webViewEnvironment.try_query<ICoreWebView2Environment11>();
        CHECK_FEATURE_RETURN(environment11);
        wil::unique_cotaskmem_string failureReportFolder;
        environment11->get_FailureReportFolderPath(&failureReportFolder);
        MessageBox(m_mainWindow, failureReportFolder.get(), L"Failure Report Folder", MB_OK);
        //! [GetFailureReportFolder]
        return true;
    }
    case IDM_CLOSE_WEBVIEW:
    {
        CloseWebView();
        return true;
    }
    case IDM_CLOSE_WEBVIEW_CLEANUP:
    {
        CloseWebView(true);
        return true;
    }
    case IDM_SCENARIO_POST_WEB_MESSAGE:
    {
        NewComponent<ScenarioWebMessage>(this);
        return true;
    }
    case IDM_SCENARIO_ADD_HOST_OBJECT:
    {
        NewComponent<ScenarioAddHostObject>(this);
        return true;
    }
    case IDM_SCENARIO_WEB_VIEW_EVENT_MONITOR:
    {
        NewComponent<ScenarioWebViewEventMonitor>(this);
        return true;
    }
    case IDM_SCENARIO_JAVA_SCRIPT:
    {
        WCHAR c_scriptPath[] = L"ScenarioJavaScriptDebugIndex.html";
        std::wstring m_scriptUri = GetLocalUri(c_scriptPath, false);
        CHECK_FAILURE(m_webView->Navigate(m_scriptUri.c_str()));
        return true;
    }
    case IDM_SCENARIO_TYPE_SCRIPT:
    {
        WCHAR c_scriptPath[] = L"ScenarioTypeScriptDebugIndex.html";
        std::wstring m_scriptUri = GetLocalUri(c_scriptPath, false);
        CHECK_FAILURE(m_webView->Navigate(m_scriptUri.c_str()));
        return true;
    }
    case IDM_SCENARIO_JAVA_SCRIPT_VIRTUAL:
    {
        WCHAR c_scriptPath[] = L"ScenarioJavaScriptDebugIndex.html";
        std::wstring m_scriptUri = GetLocalUri(c_scriptPath, true);
        CHECK_FAILURE(m_webView->Navigate(m_scriptUri.c_str()));
        return true;
    }
    case IDM_SCENARIO_TYPE_SCRIPT_VIRTUAL:
    {
        WCHAR c_scriptPath[] = L"ScenarioTypeScriptDebugIndex.html";
        std::wstring m_scriptUri = GetLocalUri(c_scriptPath, true);
        CHECK_FAILURE(m_webView->Navigate(m_scriptUri.c_str()));
        return true;
    }
    case IDM_SCENARIO_AUTHENTICATION:
    {
        NewComponent<ScenarioAuthentication>(this);
        return true;
    }
    case IDM_SCENARIO_COOKIE_MANAGEMENT:
    {
        NewComponent<ScenarioCookieManagement>(this);
        return true;
    }
    case IDM_SCENARIO_COOKIE_MANAGEMENT_PROFILE:
    {
        NewComponent<ScenarioCookieManagement>(this, true);
        MessageBox(
            m_mainWindow, L"Got CookieManager from Profile instead of ICoreWebView2.",
            L"CookieManagement", MB_OK);
        return true;
    }
    case IDM_SCENARIO_EXTENSIONS_MANAGEMENT_INSTALL_DEFAULT:
    {
        NewComponent<ScenarioExtensionsManagement>(this, false);
        return true;
    }
    case IDM_SCENARIO_EXTENSIONS_MANAGEMENT_OFFLOAD_DEFAULT:
    {
        NewComponent<ScenarioExtensionsManagement>(this, true);
        return true;
    }
    case IDM_SCENARIO_CUSTOM_SCHEME:
    {
        NewComponent<ScenarioCustomScheme>(this);
        return true;
    }
    case IDM_SCENARIO_CUSTOM_SCHEME_NAVIGATE:
    {
        NewComponent<ScenarioCustomSchemeNavigate>(this);
        return true;
    }
    case IDM_SCENARIO_SHARED_WORKER:
    {
        NewComponent<ScenarioSharedWorkerWRR>(this);
        return true;
    }
    case IDM_SCENARIO_SHARED_BUFFER:
    {
        NewComponent<ScenarioSharedBuffer>(this);
        return true;
    }
    case IDM_SCENARIO_DOM_CONTENT_LOADED:
    {
        NewComponent<ScenarioDOMContentLoaded>(this);
        return true;
    }
    case IDM_SCENARIO_NAVIGATEWITHWEBRESOURCEREQUEST:
    {
        NewComponent<ScenarioNavigateWithWebResourceRequest>(this);
        return true;
    }
    case IDM_SCENARIO_NOTIFICATION:
    {
        NewComponent<ScenarioNotificationReceived>(this);
        return true;
    }
    case IDM_SCENARIO_TESTING_FOCUS:
    {
        WCHAR testingFocusPath[] = L"ScenarioTestingFocus.html";
        std::wstring testingFocusUri = GetLocalUri(testingFocusPath, false);
        CHECK_FAILURE(m_webView->Navigate(testingFocusUri.c_str()));
        return true;
    }
    case IDM_SCENARIO_USE_DEFERRED_DOWNLOAD:
    {
        NewComponent<ScenarioCustomDownloadExperience>(this);
        return true;
    }
    case IDM_SCENARIO_USE_DEFERRED_CUSTOM_CLIENT_CERTIFICATE_DIALOG:
    {
        NewComponent<ScenarioClientCertificateRequested>(this);
        return true;
    }
    case IDM_SCENARIO_VIRTUAL_HOST_MAPPING:
    {
        NewComponent<ScenarioVirtualHostMappingForSW>(this);
        return true;
    }
    case IDM_SCENARIO_VIRTUAL_HOST_MAPPING_POP_UP_WINDOW:
    {
        NewComponent<ScenarioVirtualHostMappingForPopUpWindow>(this);
        return true;
    }
    case IDM_SCENARIO_IFRAME_DEVICE_PERMISSION:
    {
        NewComponent<ScenarioIFrameDevicePermission>(this);
        return true;
    }
    case IDM_SCENARIO_BROWSER_PRINT_PREVIEW:
    {
        return ShowPrintUI(COREWEBVIEW2_PRINT_DIALOG_KIND_BROWSER);
    }
    case IDM_SCENARIO_SYSTEM_PRINT:
    {
        return ShowPrintUI(COREWEBVIEW2_PRINT_DIALOG_KIND_SYSTEM);
    }
    case IDM_SCENARIO_PRINT_TO_DEFAULT_PRINTER:
    {
        return PrintToDefaultPrinter();
    }
    case IDM_SCENARIO_PRINT_TO_PRINTER:
    {
        return PrintToPrinter();
    }
    case IDM_SCENARIO_PRINT_TO_PDF_STREAM:
    {
        return PrintToPdfStream();
    }
    case IDM_SCENARIO_NON_CLIENT_REGION_SUPPORT:
    {
        NewComponent<ScenarioNonClientRegionSupport>(this);
        return true;
    }
    case IDM_SCENARIO_ACCELERATOR_KEY_PRESSED:
    {
        NewComponent<ScenarioAcceleratorKeyPressed>(this);
        return true;
    }
    case IDM_SCENARIO_THROTTLING_CONTROL:
    {
        NewComponent<ScenarioThrottlingControl>(this);
        return true;
    }
    case IDM_SCENARIO_SCREEN_CAPTURE:
    {
        NewComponent<ScenarioScreenCapture>(this);
        return true;
    }
    case IDM_SCENARIO_FILE_TYPE_POLICY:
    {
        NewComponent<ScenarioFileTypePolicy>(this);
        return true;
    }
    case IDM_START_FIND:
    {
        std::wstring searchTerm = L"webview"; // Replace with the actual term
        return Start(searchTerm);
    }
    case IDM_STOP_FIND:
    {
        return StopFind();
    }
    case IDM_GET_MATCHES:
    {
        return GetMatchCount();
    }
    case IDM_GET_ACTIVE_MATCH_INDEX:
    {
        return GetActiveMatchIndex();
    }
    case IDC_FIND_TERM:
    {
        return FindTerm();
    }
    case IDC_IS_CASE_SENSITIVE:
    {
        return IsCaseSensitive();
    }
    case IDC_SHOULD_MATCH_WHOLE_WORD:
    {
        return ShouldMatchWord();
    }
    case IDC_SHOULD_HIGHLIGHT_ALL_MATCHES:
    {
        return ShouldHighlightAllMatches();
    }
    case IDC_SUPPRESS_DEFAULT_FIND_DIALOG:
    {
        return SuppressDefaultFindDialog();
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
        GetAvailableCoreWebView2BrowserVersionString(
            nullptr,
            &version_info);
        MessageBox(
            m_mainWindow, version_info.get(), L"Browser Version Info Before WebView Creation",
            MB_OK);
        return true;
    }
    case IDM_UPDATE_RUNTIME:
    {
        if (!m_webViewEnvironment)
        {
            MessageBox(
                m_mainWindow, L"Need WebView2 environment object to update WebView2 Runtime",
                nullptr, MB_OK);
            return true;
        }
        //! [UpdateRuntime]
        auto experimentalEnvironment3 =
            m_webViewEnvironment.try_query<ICoreWebView2ExperimentalEnvironment3>();
        CHECK_FEATURE_RETURN(experimentalEnvironment3);
        HRESULT hr = experimentalEnvironment3->UpdateRuntime(
            Callback<ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler>(
                [this](HRESULT errorCode, ICoreWebView2ExperimentalUpdateRuntimeResult* result)
                    -> HRESULT
                {
                    HRESULT updateError = E_FAIL;
                    COREWEBVIEW2_UPDATE_RUNTIME_STATUS status =
                        COREWEBVIEW2_UPDATE_RUNTIME_STATUS_FAILED;
                    if ((errorCode == S_OK) && result)
                    {
                        CHECK_FAILURE(result->get_Status(&status));
                        CHECK_FAILURE(result->get_ExtendedError(&updateError));
                    }
                    std::wstringstream formattedMessage;
                    formattedMessage << "UpdateRuntime result (0x" << std::hex << errorCode
                                     << "), status(" << status << "), extendedError("
                                     << updateError << ")";
                    AsyncMessageBox(std::move(formattedMessage.str()), L"UpdateRuntimeResult");
                    return S_OK;
                })
                .Get());
        if (FAILED(hr))
            ShowFailure(hr, L"Call to UpdateRuntime failed");
        //! [UpdateRuntime]
        return true;
    }
    case IDM_EXIT:
        CloseAppWindow();
        return true;
    case IDM_CREATION_MODE_WINDOWED:
    case IDM_CREATION_MODE_HOST_INPUT_PROCESSING:
    case IDM_CREATION_MODE_VISUAL_DCOMP:
    case IDM_CREATION_MODE_TARGET_DCOMP:
    case IDM_CREATION_MODE_VISUAL_WINCOMP:
        m_creationModeId = LOWORD(wParam);
        UpdateCreationModeMenu();
        return true;
    case IDM_REINIT:
        InitializeWebView();
        return true;
    case IDM_NEW_WINDOW:
        new AppWindow(m_creationModeId, GetWebViewOption());
        return true;
    case IDM_NEW_THREAD:
        CreateNewThread(this);
        return true;
    case IDM_SET_LANGUAGE:
        ChangeLanguage();
        return true;
    case IDM_TOGGLE_AAD_SSO:
        ToggleAADSSO();
        return true;
    case IDM_CREATE_WITH_OPTION:
        m_webviewOption.PopupDialog(this);
        return true;
    case IDM_TOGGLE_TOPMOST_WINDOW:
    {
        bool isCurrentlyTopMost =
            (GetWindowLong(m_mainWindow, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
        SetWindowPos(
            m_mainWindow, isCurrentlyTopMost ? HWND_NOTOPMOST : HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
        return true;
    }
    case IDM_TOGGLE_EXCLUSIVE_USER_DATA_FOLDER_ACCESS:
        ToggleExclusiveUserDataFolderAccess();
        return true;
    case IDM_TOGGLE_CUSTOM_CRASH_REPORTING:
        ToggleCustomCrashReporting();
        return true;
    case IDM_TOGGLE_TRACKING_PREVENTION:
        ToggleTrackingPrevention();
        return true;
    case IDM_SCENARIO_CLEAR_BROWSING_DATA_COOKIES:
    {
        return ClearBrowsingData(COREWEBVIEW2_BROWSING_DATA_KINDS_COOKIES);
    }
    case IDM_SCENARIO_CLEAR_BROWSING_DATA_ALL_DOM_STORAGE:
    {
        return ClearBrowsingData(COREWEBVIEW2_BROWSING_DATA_KINDS_ALL_DOM_STORAGE);
    }
    case IDM_SCENARIO_CLEAR_BROWSING_DATA_ALL_SITE:
    {
        return ClearBrowsingData(COREWEBVIEW2_BROWSING_DATA_KINDS_ALL_SITE);
    }
    case IDM_SCENARIO_CLEAR_BROWSING_DATA_DISK_CACHE:
    {
        return ClearBrowsingData(COREWEBVIEW2_BROWSING_DATA_KINDS_DISK_CACHE);
    }
    case IDM_SCENARIO_CLEAR_BROWSING_DATA_DOWNLOAD_HISTORY:
    {
        return ClearBrowsingData(COREWEBVIEW2_BROWSING_DATA_KINDS_DOWNLOAD_HISTORY);
    }
    case IDM_SCENARIO_CLEAR_BROWSING_DATA_AUTOFILL:
    {
        return ClearBrowsingData((
            COREWEBVIEW2_BROWSING_DATA_KINDS)(COREWEBVIEW2_BROWSING_DATA_KINDS_GENERAL_AUTOFILL |
                                              COREWEBVIEW2_BROWSING_DATA_KINDS_PASSWORD_AUTOSAVE));
    }
    case IDM_SCENARIO_CLEAR_BROWSING_DATA_BROWSING_HISTORY:
    {
        return ClearBrowsingData(COREWEBVIEW2_BROWSING_DATA_KINDS_BROWSING_HISTORY);
    }
    case IDM_SCENARIO_CLEAR_BROWSING_DATA_ALL_PROFILE:
    {
        return ClearBrowsingData(COREWEBVIEW2_BROWSING_DATA_KINDS_ALL_PROFILE);
    }
    case IDM_SCENARIO_CLEAR_CUSTOM_DATA_PARTITION:
    {
        return ClearCustomDataPartition();
    }
    }
    return false;
}
//! [ClearBrowsingData]
bool AppWindow::ClearBrowsingData(COREWEBVIEW2_BROWSING_DATA_KINDS dataKinds)
{
    auto webView2_13 = m_webView.try_query<ICoreWebView2_13>();
    CHECK_FEATURE_RETURN(webView2_13);
    wil::com_ptr<ICoreWebView2Profile> webView2Profile;
    CHECK_FAILURE(webView2_13->get_Profile(&webView2Profile));
    CHECK_FEATURE_RETURN(webView2Profile);
    auto webView2Profile2 = webView2Profile.try_query<ICoreWebView2Profile2>();
    CHECK_FEATURE_RETURN(webView2Profile2);
    // Clear the browsing data from the last hour.
    double endTime = (double)std::time(nullptr);
    double startTime = endTime - 3600.0;
    CHECK_FAILURE(webView2Profile2->ClearBrowsingDataInTimeRange(
        dataKinds, startTime, endTime,
        Callback<ICoreWebView2ClearBrowsingDataCompletedHandler>(
            [this](HRESULT error) -> HRESULT
            {
                AsyncMessageBox(L"Completed", L"Clear Browsing Data");
                return S_OK;
            })
            .Get()));
    return true;
}
//! [ClearBrowsingData]

//! [ClearCustomDataPartition]
bool AppWindow::ClearCustomDataPartition()
{
    auto webView2_13 = m_webView.try_query<ICoreWebView2_13>();
    CHECK_FEATURE_RETURN(webView2_13);
    wil::com_ptr<ICoreWebView2Profile> webView2Profile;
    CHECK_FAILURE(webView2_13->get_Profile(&webView2Profile));
    CHECK_FEATURE_RETURN(webView2Profile);
    auto webView2Profile7 = webView2Profile.try_query<ICoreWebView2ExperimentalProfile7>();
    CHECK_FEATURE_RETURN(webView2Profile7);
    std::wstring partitionToClearData;
    wil::com_ptr<ICoreWebView2Experimental20> webViewStaging20;
    webViewStaging20 = m_webView.try_query<ICoreWebView2Experimental20>();
    wil::unique_cotaskmem_string partitionId;
    CHECK_FAILURE(webViewStaging20->get_CustomDataPartitionId(&partitionId));
    if (!partitionId.get() || !*partitionId.get())
    {
        TextInputDialog dialog(
            GetMainWindow(), L"Custom Data Partition Id", L"Custom Data Partition Id:",
            L"Enter custom data partition id");
        if (dialog.confirmed)
        {
            partitionToClearData = dialog.input.c_str();
        }
    }
    else
    {
        // Use partition id from the WebView.
        partitionToClearData = partitionId.get();
    }
    if (!partitionToClearData.empty())
    {
        CHECK_FAILURE(webView2Profile7->ClearCustomDataPartition(
            partitionToClearData.c_str(),
            Callback<ICoreWebView2ExperimentalClearCustomDataPartitionCompletedHandler>(
                [this](HRESULT result) -> HRESULT
                {
                    if (SUCCEEDED(result))
                    {
                        AsyncMessageBox(L"Completed", L"Clear Custom Data Partition");
                    }
                    else
                    {
                        std::wstringstream message;
                        message << L"Failed: " << std::to_wstring(result) << L"(0x" << std::hex
                                << result << L")" << std::endl;
                        AsyncMessageBox(message.str(), L"Clear Custom Data Partition");
                    }
                    return S_OK;
                })
                .Get()));
    }
    return true;
}
//! [ClearCustomDataPartition]

// Prompt the user for a new language string
void AppWindow::ChangeLanguage()
{
    TextInputDialog dialog(
        GetMainWindow(), L"Language", L"Language:",
        L"Enter a language to use for WebView, or leave blank to restore default.",
        m_language.empty() ? L"zh-cn" : m_language.c_str());
    if (dialog.confirmed)
    {
        m_language = (dialog.input);
    }
}

// Toggle AAD SSO enabled
void AppWindow::ToggleAADSSO()
{
    m_AADSSOEnabled = !m_AADSSOEnabled;
    MessageBox(
        nullptr,
        m_AADSSOEnabled ? L"AAD single sign on will be enabled for new WebView "
                          L"created after all webviews are closed."
                        : L"AAD single sign on will be disabled for new WebView"
                          L" created after all webviews are closed.",
        L"AAD SSO change", MB_OK);
}

void AppWindow::ToggleExclusiveUserDataFolderAccess()
{
    m_ExclusiveUserDataFolderAccess = !m_ExclusiveUserDataFolderAccess;
    MessageBox(
        nullptr,
        m_ExclusiveUserDataFolderAccess
            ? L"Will request exclusive access to user data folder "
              L"for new WebView created after all webviews are closed."
            : L"Will not request exclusive access to user data folder "
              L"for new WebView created after all webviews are closed.",
        L"Exclusive User Data Folder Access change", MB_OK);
}

void AppWindow::ToggleCustomCrashReporting()
{
    m_CustomCrashReportingEnabled = !m_CustomCrashReportingEnabled;
    MessageBox(
        nullptr,
        m_CustomCrashReportingEnabled ? L"Crash reporting will be disabled for new WebView "
                                        L"created after all webviews are closed."
                                      : L"Crash reporting will be enabled for new WebView "
                                        L"created after all webviews are closed.",
        L"Custom Crash Reporting", MB_OK);
}

void AppWindow::ToggleTrackingPrevention()
{
    m_TrackingPreventionEnabled = !m_TrackingPreventionEnabled;
    MessageBox(
        nullptr,
        m_TrackingPreventionEnabled ? L"Tracking prevention will be enabled for new WebView "
                                      L"created after all webviews are closed."
                                    : L"Tracking prevention will be disabled for new WebView "
                                      L"created after all webviews are closed.",
        L"Tracking Prevention", MB_OK);
}

//! [ShowPrintUI]
// Shows the user a print dialog. If `printDialogKind` is
// COREWEBVIEW2_PRINT_DIALOG_KIND_BROWSER, opens a browser print preview dialog,
// COREWEBVIEW2_PRINT_DIALOG_KIND_SYSTEM opens a system print dialog.
bool AppWindow::ShowPrintUI(COREWEBVIEW2_PRINT_DIALOG_KIND printDialogKind)
{
    auto webView2_16 = m_webView.try_query<ICoreWebView2_16>();
    CHECK_FEATURE_RETURN(webView2_16);
    CHECK_FAILURE(webView2_16->ShowPrintUI(printDialogKind));
    return true;
}
//! [ShowPrintUI]

// This example prints the current web page without a print dialog to default printer
// with the default print settings.
bool AppWindow::PrintToDefaultPrinter()
{
    wil::com_ptr<ICoreWebView2_16> webView2_16;
    CHECK_FAILURE(m_webView->QueryInterface(IID_PPV_ARGS(&webView2_16)));

    wil::unique_cotaskmem_string title;
    CHECK_FAILURE(m_webView->get_DocumentTitle(&title));

    // Passing nullptr for `ICoreWebView2PrintSettings` results in default print settings used.
    // Prints current web page with the default page and printer settings.
    CHECK_FAILURE(webView2_16->Print(
        nullptr, Callback<ICoreWebView2PrintCompletedHandler>(
                     [title = std::move(title),
                      this](HRESULT errorCode, COREWEBVIEW2_PRINT_STATUS printStatus) -> HRESULT
                     {
                         std::wstring message = L"";
                         if (errorCode == S_OK &&
                             printStatus == COREWEBVIEW2_PRINT_STATUS_SUCCEEDED)
                         {
                             message = L"Printing " + std::wstring(title.get()) +
                                       L" document to printer is succeeded";
                         }
                         else if (
                             errorCode == S_OK &&
                             printStatus == COREWEBVIEW2_PRINT_STATUS_PRINTER_UNAVAILABLE)
                         {
                             message = L"Printer is not available, offline or error state";
                         }
                         else if (errorCode == E_ABORT)
                         {
                             message = L"Printing " + std::wstring(title.get()) +
                                       L" document is in progress";
                         }
                         else
                         {
                             message = L"Printing " + std::wstring(title.get()) +
                                       L" document to printer is failed";
                         }

                         AsyncMessageBox(message, L"Print to default printer");

                         return S_OK;
                     })
                     .Get()));
    return true;
}

// Function to get printer name by displaying printer text input dialog to the user.
// User has to specify the desired printer name by querying the installed printers list on the
// OS to print the web page. You may also choose to display printers list to the user and return
// user selected printer.
std::wstring AppWindow::GetPrinterName()
{
    std::wstring printerName;

    TextInputDialog dialog(
        GetMainWindow(), L"Printer Name", L"Printer Name:",
        L"Specify a printer name from the installed printers list on the OS.", L"");

    if (dialog.confirmed)
    {
        printerName = (dialog.input).c_str();
    }
    return printerName;

    // or
    //
    // Use win32 EnumPrinters function to get locally installed printers.
    // Display the printer list to the user and get the user desired printer to print.
    // Return the user selected printer name.
}

// Function to get print settings for the selected printer.
// You may also choose get the capabilties from the native printer API, display to the user to
// get the print settings for the current web page and for the selected printer.
SamplePrintSettings AppWindow::GetSelectedPrinterPrintSettings(std::wstring printerName)
{
    SamplePrintSettings samplePrintSettings;
    samplePrintSettings.PrintBackgrounds = true;
    samplePrintSettings.HeaderAndFooter = true;

    return samplePrintSettings;

    // or
    //
    // Use win32 DeviceCapabilitiesA function to get the capabilities of the selected printer.
    // Display the printer capabilities to the user along with the page settings.
    // Return the user selected settings.
}

//! [PrintToPrinter]
// This example prints the current web page to a specified printer with the settings.
bool AppWindow::PrintToPrinter()
{
    std::wstring printerName = GetPrinterName();
    // Host apps custom print settings based on the user selection.
    SamplePrintSettings samplePrintSettings = GetSelectedPrinterPrintSettings(printerName);

    wil::com_ptr<ICoreWebView2_16> webView2_16;
    CHECK_FAILURE(m_webView->QueryInterface(IID_PPV_ARGS(&webView2_16)));

    wil::com_ptr<ICoreWebView2Environment6> webviewEnvironment6;
    CHECK_FAILURE(m_webViewEnvironment->QueryInterface(IID_PPV_ARGS(&webviewEnvironment6)));

    wil::com_ptr<ICoreWebView2PrintSettings> printSettings;
    CHECK_FAILURE(webviewEnvironment6->CreatePrintSettings(&printSettings));

    wil::com_ptr<ICoreWebView2PrintSettings2> printSettings2;
    CHECK_FAILURE(printSettings->QueryInterface(IID_PPV_ARGS(&printSettings2)));

    CHECK_FAILURE(printSettings->put_Orientation(samplePrintSettings.Orientation));
    CHECK_FAILURE(printSettings2->put_Copies(samplePrintSettings.Copies));
    CHECK_FAILURE(printSettings2->put_PagesPerSide(samplePrintSettings.PagesPerSide));
    CHECK_FAILURE(printSettings2->put_PageRanges(samplePrintSettings.Pages.c_str()));
    if (samplePrintSettings.Media == COREWEBVIEW2_PRINT_MEDIA_SIZE_CUSTOM)
    {
        CHECK_FAILURE(printSettings->put_PageWidth(samplePrintSettings.PaperWidth));
        CHECK_FAILURE(printSettings->put_PageHeight(samplePrintSettings.PaperHeight));
    }
    CHECK_FAILURE(printSettings2->put_ColorMode(samplePrintSettings.ColorMode));
    CHECK_FAILURE(printSettings2->put_Collation(samplePrintSettings.Collation));
    CHECK_FAILURE(printSettings2->put_Duplex(samplePrintSettings.Duplex));
    CHECK_FAILURE(printSettings->put_ScaleFactor(samplePrintSettings.ScaleFactor));
    CHECK_FAILURE(
        printSettings->put_ShouldPrintBackgrounds(samplePrintSettings.PrintBackgrounds));
    CHECK_FAILURE(
        printSettings->put_ShouldPrintBackgrounds(samplePrintSettings.PrintBackgrounds));
    CHECK_FAILURE(
        printSettings->put_ShouldPrintHeaderAndFooter(samplePrintSettings.HeaderAndFooter));
    CHECK_FAILURE(printSettings->put_HeaderTitle(samplePrintSettings.HeaderTitle.c_str()));
    CHECK_FAILURE(printSettings->put_FooterUri(samplePrintSettings.FooterUri.c_str()));
    CHECK_FAILURE(printSettings2->put_PrinterName(printerName.c_str()));

    wil::unique_cotaskmem_string title;
    CHECK_FAILURE(m_webView->get_DocumentTitle(&title));

    CHECK_FAILURE(webView2_16->Print(
        printSettings.get(),
        Callback<ICoreWebView2PrintCompletedHandler>(
            [title = std::move(title),
             this](HRESULT errorCode, COREWEBVIEW2_PRINT_STATUS printStatus) -> HRESULT
            {
                std::wstring message = L"";
                if (errorCode == S_OK && printStatus == COREWEBVIEW2_PRINT_STATUS_SUCCEEDED)
                {
                    message = L"Printing " + std::wstring(title.get()) +
                              L" document to printer is succeeded";
                }
                else if (
                    errorCode == S_OK &&
                    printStatus == COREWEBVIEW2_PRINT_STATUS_PRINTER_UNAVAILABLE)
                {
                    message = L"Selected printer is not found, not available, offline or "
                              L"error state.";
                }
                else if (errorCode == E_INVALIDARG)
                {
                    message = L"Invalid settings provided for the specified printer";
                }
                else if (errorCode == E_ABORT)
                {
                    message = L"Printing " + std::wstring(title.get()) +
                              L" document already in progress";
                }
                else
                {
                    message = L"Printing " + std::wstring(title.get()) +
                              L" document to printer is failed";
                }

                AsyncMessageBox(message, L"Print to printer");

                return S_OK;
            })
            .Get()));
    return true;
}
//! [PrintToPrinter]

// Function to display current web page pdf data in a custom print preview dialog.
static void DisplayPdfDataInPrintDialog(IStream* pdfData)
{
    // You can display the printable pdf data in a custom print preview dialog to the end user.
}

//! [PrintToPdfStream]
// This example prints the Pdf data of the current page to a stream.
bool AppWindow::PrintToPdfStream()
{
    wil::com_ptr<ICoreWebView2_16> webView2_16;
    CHECK_FAILURE(m_webView->QueryInterface(IID_PPV_ARGS(&webView2_16)));

    wil::unique_cotaskmem_string title;
    CHECK_FAILURE(m_webView->get_DocumentTitle(&title));

    // Passing nullptr for `ICoreWebView2PrintSettings` results in default print settings used.
    CHECK_FAILURE(webView2_16->PrintToPdfStream(
        nullptr,
        Callback<ICoreWebView2PrintToPdfStreamCompletedHandler>(
            [title = std::move(title), this](HRESULT errorCode, IStream* pdfData) -> HRESULT
            {
                DisplayPdfDataInPrintDialog(pdfData);

                std::wstring message =
                    L"Printing " + std::wstring(title.get()) + L" document to PDF Stream " +
                    ((errorCode == S_OK && pdfData != nullptr) ? L"succedded" : L"failed");

                AsyncMessageBox(message, L"Print to PDF Stream");

                return S_OK;
            })
            .Get()));
    return true;
}
//! [PrintToPdfStream]

//! [Start]
bool AppWindow::Start(const std::wstring& searchTerm)
{
    auto webView2Experimental29 = m_webView.try_query<ICoreWebView2Experimental29>();
    CHECK_FEATURE_RETURN(webView2Experimental29);

    wil::com_ptr<ICoreWebView2ExperimentalFind> webView2find;
    CHECK_FAILURE(webView2Experimental29->get_Find(&webView2find));
    SetupFindEventHandlers(webView2find);

    // Initialize find configuration/settings
    if (!findOptions)
    {
        findOptions = SetDefaultFindOptions();
    }

    // Check if the search term has changed to determine if UI needs to be updated
    if (m_findOnPageLastSearchTerm != searchTerm)
    {
        m_findOnPageLastSearchTerm =
            searchTerm; // Update the last search term for future comparisons
    }

    // Start the find operation
    CHECK_FAILURE(webView2find->Start(
        findOptions.get(), Callback<ICoreWebView2ExperimentalFindStartCompletedHandler>(
                               [this](HRESULT result) -> HRESULT { return S_OK; })
                               .Get()));
    return true;
}
//! [Start]

//! [FindNext]
bool AppWindow::FindNext()
{
    auto webView2Experimental29 = m_webView.try_query<ICoreWebView2Experimental29>();
    CHECK_FEATURE_RETURN(webView2Experimental29);
    wil::com_ptr<ICoreWebView2ExperimentalFind> webView2find;
    CHECK_FAILURE(webView2Experimental29->get_Find(&webView2find));
    CHECK_FAILURE(webView2find->FindNext());

    return true;
}
//! [FindNext]

//! [FindPrevious]
bool AppWindow::FindPrevious()
{
    auto webView2Experimental29 = m_webView.try_query<ICoreWebView2Experimental29>();
    CHECK_FEATURE_RETURN(webView2Experimental29);
    wil::com_ptr<ICoreWebView2ExperimentalFind> webView2find;
    CHECK_FAILURE(webView2Experimental29->get_Find(&webView2find));

    CHECK_FAILURE(webView2find->FindPrevious());

    return true;
}
//! [FindPrevious]

//! [Stop]
bool AppWindow::StopFind()
{
    auto webView2Experimental29 = m_webView.try_query<ICoreWebView2Experimental29>();
    CHECK_FEATURE_RETURN(webView2Experimental29);
    wil::com_ptr<ICoreWebView2ExperimentalFind> webView2find;
    CHECK_FAILURE(webView2Experimental29->get_Find(&webView2find));
    // Note, I am not 100% sure if this is the best way to remove the event handlers
    // Because it would rely on the user clicking stopfind. Maybe put in destructor or when
    // startfind completes?
    CHECK_FAILURE(webView2find->remove_MatchCountChanged(m_matchCountChangedToken));
    CHECK_FAILURE(webView2find->remove_ActiveMatchIndexChanged(m_activeMatchIndexChangedToken));
    CHECK_FAILURE(webView2find->Stop());
    return true;
}
//! [Stop]

//! [MatchCount]
bool AppWindow::GetMatchCount()
{
    auto webView2Experimental29 = m_webView.try_query<ICoreWebView2Experimental29>();
    CHECK_FEATURE_RETURN(webView2Experimental29);
    wil::com_ptr<ICoreWebView2ExperimentalFind> webView2find;
    CHECK_FAILURE(webView2Experimental29->get_Find(&webView2find));
    int32_t matchCount;
    CHECK_FAILURE(webView2find->get_MatchCount(&matchCount));

    std::wstring matchCountStr = L"Match Count: " + std::to_wstring(matchCount);
    MessageBox(m_mainWindow, matchCountStr.c_str(), L"Find Operation", MB_OK);

    return true;
}
//! [MatchCount]

//! [ActiveMatchIndex]
bool AppWindow::GetActiveMatchIndex()
{
    auto webView2Experimental29 = m_webView.try_query<ICoreWebView2Experimental29>();
    CHECK_FEATURE_RETURN(webView2Experimental29);
    wil::com_ptr<ICoreWebView2ExperimentalFind> webView2find;
    CHECK_FAILURE(webView2Experimental29->get_Find(&webView2find));
    int32_t activeMatchIndex;
    CHECK_FAILURE(webView2find->get_ActiveMatchIndex(&activeMatchIndex));

    std::wstring activeMatchIndexStr =
        L"Active Match Index: " + std::to_wstring(activeMatchIndex);
    MessageBox(m_mainWindow, activeMatchIndexStr.c_str(), L"Find Operation", MB_OK);

    return true;
}
//! [ActiveMatchIndex]

//! [IsCaseSensitive]
bool AppWindow::IsCaseSensitive()
{
    auto webView2Experimental29 = m_webView.try_query<ICoreWebView2Experimental29>();
    CHECK_FEATURE_RETURN(webView2Experimental29);
    wil::com_ptr<ICoreWebView2ExperimentalFind> webView2find;
    CHECK_FAILURE(webView2Experimental29->get_Find(&webView2find));

    auto m_env18 = m_webViewEnvironment.try_query<ICoreWebView2ExperimentalEnvironment18>();
    CHECK_FEATURE_RETURN(m_env18);
    CHECK_FAILURE(webView2find->Stop());

    if (!findOptions)
    {
        findOptions = SetDefaultFindOptions();
    }
    BOOL caseSensitive;
    findOptions->get_IsCaseSensitive(&caseSensitive);
    CHECK_FAILURE(findOptions->put_IsCaseSensitive(!caseSensitive));

    CHECK_FAILURE(webView2find->Start(
        findOptions.get(), Callback<ICoreWebView2ExperimentalFindStartCompletedHandler>(
                               [this](HRESULT result) -> HRESULT { return S_OK; })
                               .Get()));
    return true;
}
//! [IsCaseSensitive]

//! [ShouldHighlightAllMatches]
bool AppWindow::ShouldHighlightAllMatches()
{
    auto webView2Experimental29 = m_webView.try_query<ICoreWebView2Experimental29>();
    CHECK_FEATURE_RETURN(webView2Experimental29);
    wil::com_ptr<ICoreWebView2ExperimentalFind> webView2find;
    CHECK_FAILURE(webView2Experimental29->get_Find(&webView2find));

    auto m_env18 = m_webViewEnvironment.try_query<ICoreWebView2ExperimentalEnvironment18>();
    CHECK_FEATURE_RETURN(m_env18);
    CHECK_FAILURE(webView2find->Stop());

    if (!findOptions)
    {
        findOptions = SetDefaultFindOptions();
    }
    BOOL shouldHighlightAllMatches;
    CHECK_FAILURE(findOptions->get_ShouldHighlightAllMatches(&shouldHighlightAllMatches));
    CHECK_FAILURE(findOptions->put_ShouldHighlightAllMatches(!shouldHighlightAllMatches));
    CHECK_FAILURE(webView2find->Start(
        findOptions.get(), Callback<ICoreWebView2ExperimentalFindStartCompletedHandler>(
                               [this](HRESULT result) -> HRESULT { return S_OK; })
                               .Get()));
    return true;
}
//! [ShouldHighlightAllMatches]

//! [ShouldMatchWord]
bool AppWindow::ShouldMatchWord()
{
    auto webView2Experimental29 = m_webView.try_query<ICoreWebView2Experimental29>();
    CHECK_FEATURE_RETURN(webView2Experimental29);
    wil::com_ptr<ICoreWebView2ExperimentalFind> webView2find;
    CHECK_FAILURE(webView2Experimental29->get_Find(&webView2find));

    auto m_env18 = m_webViewEnvironment.try_query<ICoreWebView2ExperimentalEnvironment18>();
    CHECK_FEATURE_RETURN(m_env18);
    CHECK_FAILURE(webView2find->Stop());

    if (!findOptions)
    {
        findOptions = SetDefaultFindOptions();
    }
    BOOL shouldMatchWord;
    findOptions->get_ShouldMatchWord(&shouldMatchWord);
    CHECK_FAILURE(findOptions->put_ShouldMatchWord(!shouldMatchWord));
    CHECK_FAILURE(webView2find->Start(
        findOptions.get(), Callback<ICoreWebView2ExperimentalFindStartCompletedHandler>(
                               [this](HRESULT result) -> HRESULT { return S_OK; })
                               .Get()));
    return true;
}
//! [ShouldMatchWord]

//! [SuppressDefaultFindDialog]
bool AppWindow::SuppressDefaultFindDialog()
{
    auto webView2Experimental29 = m_webView.try_query<ICoreWebView2Experimental29>();
    CHECK_FEATURE_RETURN(webView2Experimental29);
    wil::com_ptr<ICoreWebView2ExperimentalFind> webView2find;
    CHECK_FAILURE(webView2Experimental29->get_Find(&webView2find));

    auto m_env18 = m_webViewEnvironment.try_query<ICoreWebView2ExperimentalEnvironment18>();
    CHECK_FEATURE_RETURN(m_env18);
    CHECK_FAILURE(webView2find->Stop());

    if (!findOptions)
    {
        findOptions = SetDefaultFindOptions();
    }
    BOOL suppressDefaultDialog;
    findOptions->get_SuppressDefaultFindDialog(&suppressDefaultDialog);
    CHECK_FAILURE(findOptions->put_SuppressDefaultFindDialog(!suppressDefaultDialog));
    CHECK_FAILURE(webView2find->Stop());
    CHECK_FAILURE(webView2find->Start(
        findOptions.get(), Callback<ICoreWebView2ExperimentalFindStartCompletedHandler>(
                               [this](HRESULT result) -> HRESULT { return S_OK; })
                               .Get()));
    return true;
}
//! [SuppressDefaultFindDialog]

//! [FindTerm]
bool AppWindow::FindTerm()
{
    auto webView2Experimental29 = m_webView.try_query<ICoreWebView2Experimental29>();
    CHECK_FEATURE_RETURN(webView2Experimental29);
    wil::com_ptr<ICoreWebView2ExperimentalFind> webView2find;
    CHECK_FAILURE(webView2Experimental29->get_Find(&webView2find));

    TextInputDialog dialog(
        GetMainWindow(), L"Find on Page Term", L"Find on Page Term:", L"Enter Find Term");

    auto m_env18 = m_webViewEnvironment.try_query<ICoreWebView2ExperimentalEnvironment18>();
    CHECK_FEATURE_RETURN(m_env18);
    CHECK_FAILURE(webView2find->Stop());

    if (!findOptions)
    {
        findOptions = SetDefaultFindOptions();
    }
    CHECK_FAILURE(findOptions->put_FindTerm(dialog.input.c_str()));
    CHECK_FAILURE(webView2find->Start(
        findOptions.get(), Callback<ICoreWebView2ExperimentalFindStartCompletedHandler>(
                               [this](HRESULT result) -> HRESULT { return S_OK; })
                               .Get()));
    return true;
}
//! [FindTerm]

wil::com_ptr<ICoreWebView2ExperimentalFindOptions> AppWindow::SetDefaultFindOptions()
{
    auto m_env18 = m_webViewEnvironment.try_query<ICoreWebView2ExperimentalEnvironment18>();

    // Initialize find configuration/settings
    wil::com_ptr<ICoreWebView2ExperimentalFindOptions> findOptions;
    CHECK_FAILURE(m_env18->CreateFindOptions(&findOptions));
    CHECK_FAILURE(findOptions->put_FindTerm(L"Webview2"));
    CHECK_FAILURE(findOptions->put_IsCaseSensitive(false));
    CHECK_FAILURE(findOptions->put_ShouldMatchWord(false));
    CHECK_FAILURE(findOptions->put_SuppressDefaultFindDialog(false));
    CHECK_FAILURE(findOptions->put_ShouldHighlightAllMatches(true));

    return findOptions;
}

void AppWindow::SetupFindEventHandlers(wil::com_ptr<ICoreWebView2ExperimentalFind> webView2find)
{
    CHECK_FAILURE(webView2find->add_MatchCountChanged(
        Callback<ICoreWebView2ExperimentalFindMatchCountChangedEventHandler>(
            [this](ICoreWebView2ExperimentalFind* sender, IUnknown* args) -> HRESULT
            {
                int matchCount = 0;
                if (sender)
                {
                    CHECK_FAILURE(sender->get_MatchCount(&matchCount));
                }
                return S_OK;
            })
            .Get(),
        &m_matchCountChangedToken));

    CHECK_FAILURE(webView2find->add_ActiveMatchIndexChanged(
        Callback<ICoreWebView2ExperimentalFindActiveMatchIndexChangedEventHandler>(
            [this](ICoreWebView2ExperimentalFind* sender, IUnknown* args) -> HRESULT
            {
                int activeIndex = -1;
                if (sender)
                {
                    CHECK_FAILURE(sender->get_ActiveMatchIndex(&activeIndex));
                }
                return S_OK;
            })
            .Get(),
        &m_activeMatchIndexChangedToken));
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
            return [this] { new AppWindow(m_creationModeId, GetWebViewOption()); };
        case 'Q':
            return [this] { CloseAppWindow(); };
        case 'S':
            return [this]
            {
                if (auto file = GetComponent<FileComponent>())
                {
                    file->SaveScreenshot();
                }
            };
        case 'T':
            return [this] { CreateNewThread(this); };
        case 'W':
            return [this] { CloseWebView(); };
        }
    }
    if (GetKeyState(VK_MENU) < 0) // VK_MENU == Alt key
    {
        switch (key)
        {
        case 'D': // Alt+D focuses and selects the address bar, like the browser.
            return [this] { m_toolbar.SelectAddressBar(); };
        }
    }
    return nullptr;
}

//! [CreateCoreWebView2Controller]
// Create or recreate the WebView and its environment.
void AppWindow::InitializeWebView()
{
    // To ensure browser switches get applied correctly, we need to close
    // the existing WebView. This will result in a new browser process
    // getting created which will apply the browser switches.
    CloseWebView();
    m_dcompDevice = nullptr;
    m_wincompCompositor = nullptr;
    LPCWSTR subFolder = nullptr;

    if (m_creationModeId == IDM_CREATION_MODE_VISUAL_DCOMP ||
        m_creationModeId == IDM_CREATION_MODE_TARGET_DCOMP)
    {
        HRESULT hr = DCompositionCreateDevice2(nullptr, IID_PPV_ARGS(&m_dcompDevice));
        if (!SUCCEEDED(hr))
        {
            MessageBox(
                m_mainWindow,
                L"Attempting to create WebView using DComp Visual is not supported.\r\n"
                "DComp device creation failed.\r\n"
                "Current OS may not support DComp.",
                L"Create with Windowless DComp Visual Failed", MB_OK);
            return;
        }
    }
    else if (m_creationModeId == IDM_CREATION_MODE_VISUAL_WINCOMP)
    {
        HRESULT hr = TryCreateDispatcherQueue();
        if (!SUCCEEDED(hr))
        {
            MessageBox(
                m_mainWindow,
                L"Attempting to create WebView using WinComp Visual is not supported.\r\n"
                "WinComp compositor creation failed.\r\n"
                "Current OS may not support WinComp.",
                L"Create with Windowless WinComp Visual Failed", MB_OK);
            return;
        }
        m_wincompCompositor = winrtComp::Compositor();
    }
    //! [CreateCoreWebView2EnvironmentWithOptions]

    std::wstring args;
    args.append(L"--enable-features=ThirdPartyStoragePartitioning,PartitionedCookies");
    auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
    options->put_AdditionalBrowserArguments(args.c_str());
    CHECK_FAILURE(
        options->put_AllowSingleSignOnUsingOSPrimaryAccount(m_AADSSOEnabled ? TRUE : FALSE));
    CHECK_FAILURE(options->put_ExclusiveUserDataFolderAccess(
        m_ExclusiveUserDataFolderAccess ? TRUE : FALSE));
    if (!m_language.empty())
        CHECK_FAILURE(options->put_Language(m_language.c_str()));
    CHECK_FAILURE(options->put_IsCustomCrashReportingEnabled(
        m_CustomCrashReportingEnabled ? TRUE : FALSE));

    //! [CoreWebView2CustomSchemeRegistration]
    Microsoft::WRL::ComPtr<ICoreWebView2EnvironmentOptions4> options4;
    if (options.As(&options4) == S_OK)
    {
        const WCHAR* allowedOrigins[1] = {L"https://*.example.com"};

        auto customSchemeRegistration =
            Microsoft::WRL::Make<CoreWebView2CustomSchemeRegistration>(L"custom-scheme");
        customSchemeRegistration->SetAllowedOrigins(1, allowedOrigins);
        auto customSchemeRegistration2 =
            Microsoft::WRL::Make<CoreWebView2CustomSchemeRegistration>(L"wv2rocks");
        customSchemeRegistration2->put_TreatAsSecure(TRUE);
        customSchemeRegistration2->SetAllowedOrigins(1, allowedOrigins);
        customSchemeRegistration2->put_HasAuthorityComponent(TRUE);
        auto customSchemeRegistration3 =
            Microsoft::WRL::Make<CoreWebView2CustomSchemeRegistration>(
                L"custom-scheme-not-in-allowed-origins");
        ICoreWebView2CustomSchemeRegistration* registrations[3] = {
            customSchemeRegistration.Get(), customSchemeRegistration2.Get(),
            customSchemeRegistration3.Get()};
        options4->SetCustomSchemeRegistrations(
            2, static_cast<ICoreWebView2CustomSchemeRegistration**>(registrations));
    }
    //! [CoreWebView2CustomSchemeRegistration]

    Microsoft::WRL::ComPtr<ICoreWebView2EnvironmentOptions5> options5;
    if (options.As(&options5) == S_OK)
    {
        CHECK_FAILURE(
            options5->put_EnableTrackingPrevention(m_TrackingPreventionEnabled ? TRUE : FALSE));
    }

    Microsoft::WRL::ComPtr<ICoreWebView2EnvironmentOptions6> options6;
    if (options.As(&options6) == S_OK)
    {
        CHECK_FAILURE(options6->put_AreBrowserExtensionsEnabled(TRUE));
    }

    Microsoft::WRL::ComPtr<ICoreWebView2EnvironmentOptions8> options8;
    if (options.As(&options8) == S_OK)
    {
        COREWEBVIEW2_SCROLLBAR_STYLE style = COREWEBVIEW2_SCROLLBAR_STYLE_FLUENT_OVERLAY;
        CHECK_FAILURE(options8->put_ScrollBarStyle(style));
    }

    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
        subFolder, m_userDataFolder.c_str(), options.Get(),
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            this, &AppWindow::OnCreateEnvironmentCompleted)
            .Get());
    //! [CreateCoreWebView2EnvironmentWithOptions]
    if (!SUCCEEDED(hr))
    {
        switch (hr)
        {
        case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
        {
            MessageBox(
                m_mainWindow,
                L"Couldn't find Edge WebView2 Runtime. "
                "Do you have a version installed?",
                nullptr, MB_OK);
        }
        break;
        case HRESULT_FROM_WIN32(ERROR_FILE_EXISTS):
        {
            MessageBox(
                m_mainWindow,
                L"User data folder cannot be created because a file with the same name already "
                L"exists.",
                nullptr, MB_OK);
        }
        break;
        case E_ACCESSDENIED:
        {
            MessageBox(
                m_mainWindow, L"Unable to create user data folder, Access Denied.", nullptr,
                MB_OK);
        }
        break;
        case E_FAIL:
        {
            MessageBox(m_mainWindow, L"Edge runtime unable to start", nullptr, MB_OK);
        }
        break;
        default:
        {
            ShowFailure(hr, L"Failed to create WebView2 environment");
        }
        }
    }
}
// This is the callback passed to CreateWebViewEnvironmentWithOptions.
// Here we simply create the WebView.
HRESULT AppWindow::OnCreateEnvironmentCompleted(
    HRESULT result, ICoreWebView2Environment* environment)
{
    if (result != S_OK)
    {
        ShowFailure(result, L"Failed to create environment object.");
        return S_OK;
    }
    m_webViewEnvironment = environment;

    if (m_webviewOption.entry == WebViewCreateEntry::EVER_FROM_CREATE_WITH_OPTION_MENU ||
        m_creationModeId == IDM_CREATION_MODE_HOST_INPUT_PROCESSING
    )
    {
        return CreateControllerWithOptions();
    }
    auto webViewEnvironment3 = m_webViewEnvironment.try_query<ICoreWebView2Environment3>();

    if (webViewEnvironment3 && (m_dcompDevice || m_wincompCompositor))
    {
        CHECK_FAILURE(webViewEnvironment3->CreateCoreWebView2CompositionController(
            m_mainWindow,
            Callback<ICoreWebView2CreateCoreWebView2CompositionControllerCompletedHandler>(
                [this](
                    HRESULT result,
                    ICoreWebView2CompositionController* compositionController) -> HRESULT
                {
                    auto controller =
                        wil::com_ptr<ICoreWebView2CompositionController>(compositionController)
                            .query<ICoreWebView2Controller>();
                    return OnCreateCoreWebView2ControllerCompleted(result, controller.get());
                })
                .Get()));
    }
    else
    {
        CHECK_FAILURE(m_webViewEnvironment->CreateCoreWebView2Controller(
            m_mainWindow, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                              this, &AppWindow::OnCreateCoreWebView2ControllerCompleted)
                              .Get()));
    }

    return S_OK;
}
//! [CreateCoreWebView2Controller]

HRESULT AppWindow::CreateControllerWithOptions()
{
    //! [CreateControllerWithOptions]
    auto webViewEnvironment10 = m_webViewEnvironment.try_query<ICoreWebView2Environment10>();
    if (!webViewEnvironment10)
    {
        FeatureNotAvailable();
        return S_OK;
    }

    wil::com_ptr<ICoreWebView2ControllerOptions> options;
    // The validation of parameters occurs when setting the properties.
    HRESULT hr = webViewEnvironment10->CreateCoreWebView2ControllerOptions(&options);
    if (hr == E_INVALIDARG)
    {
        ShowFailure(hr, L"Unable to create WebView2 due to an invalid profile name.");
        CloseAppWindow();
        return S_OK;
    }
    CHECK_FAILURE(hr);
    //! [CreateControllerWithOptions]

    // If call 'put_ProfileName' with an invalid profile name, the 'E_INVALIDARG' returned
    // immediately. ProfileName could be reused.
    CHECK_FAILURE(options->put_ProfileName(m_webviewOption.profile.c_str()));
    CHECK_FAILURE(options->put_IsInPrivateModeEnabled(m_webviewOption.isInPrivate));

    //! [ScriptLocaleSetting]
    wil::com_ptr<ICoreWebView2ControllerOptions2> webView2ControllerOptions2;
    if (SUCCEEDED(options->QueryInterface(IID_PPV_ARGS(&webView2ControllerOptions2))))
    {
        if (m_webviewOption.useOSRegion)
        {
            wchar_t osLocale[LOCALE_NAME_MAX_LENGTH] = {0};
            GetUserDefaultLocaleName(osLocale, LOCALE_NAME_MAX_LENGTH);
            CHECK_FAILURE(webView2ControllerOptions2->put_ScriptLocale(osLocale));
        }
        else if (!m_webviewOption.scriptLocale.empty())
        {
            CHECK_FAILURE(webView2ControllerOptions2->put_ScriptLocale(
                m_webviewOption.scriptLocale.c_str()));
        }
    }
    //! [ScriptLocaleSetting]

    //! [AllowHostInputProcessing]
    if (m_creationModeId == IDM_CREATION_MODE_HOST_INPUT_PROCESSING)
    {
        wil::com_ptr<ICoreWebView2ExperimentalControllerOptions2>
            webView2ExperimentalControllerOptions2;
        if (SUCCEEDED(
                options->QueryInterface(IID_PPV_ARGS(&webView2ExperimentalControllerOptions2))))
        {
            CHECK_FAILURE(
                webView2ExperimentalControllerOptions2->put_AllowHostInputProcessing(TRUE));
        }
    }
    //! [AllowHostInputProcessing]

    //! [DefaultBackgroundColor]
    if (m_webviewOption.bg_color)
    {
        wil::com_ptr<ICoreWebView2ExperimentalControllerOptions3> experimentalControllerOptions3;
        if (SUCCEEDED(options->QueryInterface(IID_PPV_ARGS(&experimentalControllerOptions3))))
        {
            CHECK_FAILURE(
                experimentalControllerOptions3->put_DefaultBackgroundColor(*m_webviewOption.bg_color));
        }
    }
    //! [DefaultBackgroundColor]

    if (m_dcompDevice || m_wincompCompositor)
    {
        //! [OnCreateCoreWebView2ControllerCompleted]
        CHECK_FAILURE(webViewEnvironment10->CreateCoreWebView2CompositionControllerWithOptions(
            m_mainWindow, options.get(),
            Callback<ICoreWebView2CreateCoreWebView2CompositionControllerCompletedHandler>(
                [this](
                    HRESULT result,
                    ICoreWebView2CompositionController* compositionController) -> HRESULT
                {
                    auto controller =
                        wil::com_ptr<ICoreWebView2CompositionController>(compositionController)
                            .query<ICoreWebView2Controller>();
                    return OnCreateCoreWebView2ControllerCompleted(result, controller.get());
                })
                .Get()));
        //! [OnCreateCoreWebView2ControllerCompleted]
    }
    else
    {
        CHECK_FAILURE(webViewEnvironment10->CreateCoreWebView2ControllerWithOptions(
            m_mainWindow, options.get(),
            Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                this, &AppWindow::OnCreateCoreWebView2ControllerCompleted)
                .Get()));
    }

    return S_OK;
}

void AppWindow::SetAppIcon(bool inPrivate)
{
    int iconID = inPrivate ? IDI_WEBVIEW2APISAMPLE_INPRIVATE : IDI_WEBVIEW2APISAMPLE;

    HICON newSmallIcon = reinterpret_cast<HICON>(
        LoadImage(g_hInstance, MAKEINTRESOURCEW(iconID), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
    HICON newBigIcon = reinterpret_cast<HICON>(
        LoadImage(g_hInstance, MAKEINTRESOURCEW(iconID), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR));

    reinterpret_cast<HICON>(SendMessage(
        m_mainWindow, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(newSmallIcon)));
    reinterpret_cast<HICON>(
        SendMessage(m_mainWindow, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(newBigIcon)));
}

// This is the callback passed to CreateCoreWebView2Controller. Here we initialize all
// WebView-related state and register most of our event handlers with the WebView.
HRESULT AppWindow::OnCreateCoreWebView2ControllerCompleted(
    HRESULT result, ICoreWebView2Controller* controller)
{
    if (result == S_OK)
    {
        m_controller = controller;
        wil::com_ptr<ICoreWebView2> coreWebView2;
        CHECK_FAILURE(m_controller->get_CoreWebView2(&coreWebView2));
        // We should check for failure here because if this app is using a newer
        // SDK version compared to the install of the Edge browser, the Edge
        // browser might not have support for the latest version of the
        // ICoreWebView2_N interface.
        coreWebView2.query_to(&m_webView);
        // Save PID of the browser process serving last WebView created from our
        // CoreWebView2Environment. We know the controller was created with
        // S_OK, and it hasn't been closed (we haven't called Close and no
        // ProcessFailed event could have been raised yet) so the PID is
        // available.
        CHECK_FAILURE(m_webView->get_BrowserProcessId(&m_newestBrowserPid));
        //! [CoreWebView2Profile]
        auto webView2_13 = coreWebView2.try_query<ICoreWebView2_13>();
        if (webView2_13)
        {
            wil::com_ptr<ICoreWebView2Profile> profile;
            CHECK_FAILURE(webView2_13->get_Profile(&profile));
            wil::unique_cotaskmem_string profile_name;
            CHECK_FAILURE(profile->get_ProfileName(&profile_name));
            m_profileName = profile_name.get();
            BOOL inPrivate = FALSE;
            CHECK_FAILURE(profile->get_IsInPrivateModeEnabled(&inPrivate));
            if (!m_webviewOption.downloadPath.empty())
            {
                CHECK_FAILURE(profile->put_DefaultDownloadFolderPath(
                    m_webviewOption.downloadPath.c_str()));
            }

            // update window title with m_profileName
            UpdateAppTitle();

            // update window icon
            SetAppIcon(inPrivate);
        }
        //! [CoreWebView2Profile]
        // Create components. These will be deleted when the WebView is closed.
        NewComponent<FileComponent>(this);
        NewComponent<ProcessComponent>(this);
        NewComponent<ScriptComponent>(this);
        NewComponent<SettingsComponent>(
            this, m_webViewEnvironment.get(), m_oldSettingsComponent.get());
        m_oldSettingsComponent = nullptr;
        NewComponent<ViewComponent>(
            this, m_dcompDevice.get(), m_wincompCompositor,
            m_creationModeId == IDM_CREATION_MODE_TARGET_DCOMP);
        NewComponent<AudioComponent>(this);
        NewComponent<ControlComponent>(this, &m_toolbar);
        m_webView3 = coreWebView2.try_query<ICoreWebView2_3>();
        if (m_webView3)
        {
            //! [AddVirtualHostNameToFolderMapping]
            // Setup host resource mapping for local files.
            m_webView3->SetVirtualHostNameToFolderMapping(
                L"appassets.example", L"assets",
                COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY_CORS);
            //! [AddVirtualHostNameToFolderMapping]
        }
        NewComponent<ScenarioPermissionManagement>(this);
        NewComponent<ScenarioNotificationReceived>(this);
        NewComponent<ScenarioSaveAs>(this);

        // We have a few of our own event handlers to register here as well
        RegisterEventHandlers();

        // Set the initial size of the WebView
        ResizeEverything();

        if (m_onWebViewFirstInitialized)
        {
            m_onWebViewFirstInitialized();
            m_onWebViewFirstInitialized = nullptr;
        }

        if (m_initialUri != L"none")
        {
            std::wstring initialUri =
                m_initialUri.empty() ? AppStartPage::GetUri(this) : m_initialUri;
            CHECK_FAILURE(m_webView->Navigate(initialUri.c_str()));
        }
    }
    else if (result == E_ABORT)
    {
        // Webview creation was aborted because the user closed this window.
        // No need to report this as an error.
    }
    else if (result == HRESULT_FROM_WIN32(ERROR_DELETE_PENDING))
    {
        RunAsync(
            [this, result]()
            {
                ShowFailure(
                    result,
                    L"Failed to create webview, because the profile's name has been marked as "
                    L"deleted, please use a different profile's name");
                m_webviewOption.PopupDialog(this);
                CloseAppWindow();
            });
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
    InitializeWebView();
}

void AppWindow::ReinitializeWebViewWithNewBrowser()
{
    if (!m_webView)
    {
        ReinitializeWebView();
        return;
    }
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

    InitializeWebView();
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
    //! [ContainsFullScreenElementChanged]
    // Register a handler for the ContainsFullScreenChanged event.
    CHECK_FAILURE(m_webView->add_ContainsFullScreenElementChanged(
        Callback<ICoreWebView2ContainsFullScreenElementChangedEventHandler>(
            [this](ICoreWebView2* sender, IUnknown* args) -> HRESULT
            {
                CHECK_FAILURE(
                    sender->get_ContainsFullScreenElement(&m_containsFullscreenElement));
                if (m_containsFullscreenElement)
                {
                    EnterFullScreen();
                }
                else
                {
                    ExitFullScreen();
                }
                return S_OK;
            })
            .Get(),
        nullptr));
    //! [ContainsFullScreenElementChanged]

    //! [NewWindowRequested]
    // Register a handler for the NewWindowRequested event.
    // This handler will defer the event, create a new app window, and then once the
    // new window is ready, it'll provide that new window's WebView as the response to
    // the request.
    CHECK_FAILURE(m_webView->add_NewWindowRequested(
        Callback<ICoreWebView2NewWindowRequestedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2NewWindowRequestedEventArgs* args)
            {
                if (!m_shouldHandleNewWindowRequest)
                {
                    args->put_Handled(FALSE);
                    return S_OK;
                }
                wil::com_ptr<ICoreWebView2NewWindowRequestedEventArgs> args_as_comptr = args;
                auto args3 =
                    args_as_comptr.try_query<ICoreWebView2NewWindowRequestedEventArgs3>();
                if (args3)
                {
                    wil::com_ptr<ICoreWebView2FrameInfo> frame_info;
                    CHECK_FAILURE(args3->get_OriginalSourceFrameInfo(&frame_info));
                    wil::unique_cotaskmem_string source;
                    CHECK_FAILURE(frame_info->get_Source(&source));
                    // The host can decide how to open based on source frame info,
                    // such as URI.
                    static const wchar_t* browser_launching_domain = L"www.example.com";
                    wil::unique_bstr source_domain = GetDomainOfUri(source.get());
                    const wchar_t* source_domain_as_wchar = source_domain.get();
                    if (source_domain_as_wchar &&
                        wcscmp(browser_launching_domain, source_domain_as_wchar) == 0)
                    {
                        // Open the URI in the default browser.
                        wil::unique_cotaskmem_string target_uri;
                        CHECK_FAILURE(args->get_Uri(&target_uri));
                        ShellExecute(
                            nullptr, L"open", target_uri.get(), nullptr, nullptr,
                            SW_SHOWNORMAL);
                        CHECK_FAILURE(args->put_Handled(TRUE));
                        return S_OK;
                    }
                }

                wil::com_ptr<ICoreWebView2Deferral> deferral;
                CHECK_FAILURE(args->GetDeferral(&deferral));
                AppWindow* newAppWindow;

                wil::com_ptr<ICoreWebView2WindowFeatures> windowFeatures;
                CHECK_FAILURE(args->get_WindowFeatures(&windowFeatures));

                RECT windowRect = {0};
                UINT32 left = 0;
                UINT32 top = 0;
                UINT32 height = 0;
                UINT32 width = 0;
                BOOL shouldHaveToolbar = true;

                BOOL hasPosition = FALSE;
                BOOL hasSize = FALSE;
                CHECK_FAILURE(windowFeatures->get_HasPosition(&hasPosition));
                CHECK_FAILURE(windowFeatures->get_HasSize(&hasSize));

                bool useDefaultWindow = true;

                if (!!hasPosition && !!hasSize)
                {
                    CHECK_FAILURE(windowFeatures->get_Left(&left));
                    CHECK_FAILURE(windowFeatures->get_Top(&top));
                    CHECK_FAILURE(windowFeatures->get_Height(&height));
                    CHECK_FAILURE(windowFeatures->get_Width(&width));
                    useDefaultWindow = false;
                }
                CHECK_FAILURE(windowFeatures->get_ShouldDisplayToolbar(&shouldHaveToolbar));

                windowRect.left = left;
                windowRect.right =
                    left + (width < s_minNewWindowSize ? s_minNewWindowSize : width);
                windowRect.top = top;
                windowRect.bottom =
                    top + (height < s_minNewWindowSize ? s_minNewWindowSize : height);

                // passing "none" as uri as its a noinitialnavigation
                if (!useDefaultWindow)
                {
                    newAppWindow = new AppWindow(
                        m_creationModeId, GetWebViewOption(), L"none", m_userDataFolder, false,
                        nullptr, true, windowRect, !!shouldHaveToolbar);
                }
                else
                {
                    newAppWindow = new AppWindow(m_creationModeId, GetWebViewOption(), L"none");
                }
                newAppWindow->m_isPopupWindow = true;
                newAppWindow->m_onWebViewFirstInitialized = [args, deferral, newAppWindow]()
                {
                    CHECK_FAILURE(args->put_NewWindow(newAppWindow->m_webView.get()));
                    CHECK_FAILURE(args->put_Handled(TRUE));
                    CHECK_FAILURE(deferral->Complete());
                };
                return S_OK;
            })
            .Get(),
        nullptr));
    //! [NewWindowRequested]

    //! [WindowCloseRequested]
    // Register a handler for the WindowCloseRequested event.
    // This handler will close the app window if it is not the main window.
    CHECK_FAILURE(m_webView->add_WindowCloseRequested(
        Callback<ICoreWebView2WindowCloseRequestedEventHandler>(
            [this](ICoreWebView2* sender, IUnknown* args)
            {
                if (m_isPopupWindow)
                {
                    CloseAppWindow();
                }
                return S_OK;
            })
            .Get(),
        nullptr));
    //! [WindowCloseRequested]

    //! [NewBrowserVersionAvailable]
    // After the environment is successfully created,
    // register a handler for the NewBrowserVersionAvailable event.
    // This handler tells when there is a new Edge version available on the machine.
    CHECK_FAILURE(m_webViewEnvironment->add_NewBrowserVersionAvailable(
        Callback<ICoreWebView2NewBrowserVersionAvailableEventHandler>(
            [this](ICoreWebView2Environment* sender, IUnknown* args) -> HRESULT
            {
                // Don't block the event handler with a message box
                RunAsync(
                    [this]
                    {
                        std::wstring message =
                            L"We detected there is a new version for the browser.";
                        if (m_webView)
                        {
                            message += L"Do you want to restart the app? \n\n";
                            message +=
                                L"Click No if you only want to re-create the webviews. \n";
                            message += L"Click Cancel for no action. \n";
                        }
                        int response = MessageBox(
                            m_mainWindow, message.c_str(), L"New available version",
                            m_webView ? MB_YESNOCANCEL : MB_OK);

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
                    });

                return S_OK;
            })
            .Get(),
        nullptr));
    //! [NewBrowserVersionAvailable]

    //! [RestartRequested]
    // After the environment is successfully created,
    // register a handler for
    auto exp_env15 = m_webViewEnvironment.try_query<ICoreWebView2ExperimentalEnvironment15>();
    CHECK_FAILURE(exp_env15->add_RestartRequested(
        Callback<ICoreWebView2ExperimentalRestartRequestedEventHandler>(
            [this](
                ICoreWebView2Environment* sender,
                ICoreWebView2ExperimentalRestartRequestedEventArgs* args) -> HRESULT
            {
                COREWEBVIEW2_RESTART_REQUESTED_PRIORITY priority;
                args->get_Priority(&priority);
                if (priority == COREWEBVIEW2_RESTART_REQUESTED_PRIORITY_NORMAL)
                {
                    // Remaind user to restart the app when they get a chance.
                    // Don't force user to restart.
                    MessageBox(
                        m_mainWindow, L"Please restart your app when you get a chance",
                        L"WebView Restart Requested", MB_OK);
                }
                else if (priority == COREWEBVIEW2_RESTART_REQUESTED_PRIORITY_HIGH)
                {
                    // Don't block the event handler with a message box
                    RunAsync(
                        [this]()
                        {
                            std::wstring message = L"We detected there is a critical "
                                                   L"update for WebView2 runtime.";
                            if (m_webView)
                            {
                                message += L"Do you want to restart the app? \n\n";
                                message += L"Click No if you only want to re-create the "
                                           L"webviews. \n";
                                message += L"Click Cancel for no action. \n";
                            }
                            int response = MessageBox(
                                m_mainWindow, message.c_str(), L"Critical Update Avaliable",
                                m_webView ? MB_YESNOCANCEL : MB_OK);

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
                        });
                }

                return S_OK;
            })
            .Get(),
        nullptr));
    //! [RestartRequested]

    //! [ProfileDeleted]
    auto webView2_13 = m_webView.try_query<ICoreWebView2_13>();
    CHECK_FEATURE_RETURN_EMPTY(webView2_13);
    wil::com_ptr<ICoreWebView2Profile> webView2Profile;
    CHECK_FAILURE(webView2_13->get_Profile(&webView2Profile));
    CHECK_FEATURE_RETURN_EMPTY(webView2Profile);
    auto webView2Profile8 = webView2Profile.try_query<ICoreWebView2Profile8>();
    CHECK_FEATURE_RETURN_EMPTY(webView2Profile8);
    CHECK_FAILURE(webView2Profile8->add_Deleted(
        Microsoft::WRL::Callback<ICoreWebView2ProfileDeletedEventHandler>(
            [this](ICoreWebView2Profile* sender, IUnknown* args)
            {
                RunAsync(
                    [this]()
                    {
                        std::wstring message = L"The profile has been marked for deletion. Any "
                                               L"associated webview2 objects will be closed.";
                        MessageBox(m_mainWindow, message.c_str(), L"webview2 closed", MB_OK);
                        CloseAppWindow();
                    });
                return S_OK;
            })
            .Get(),
        nullptr));
    //! [ProfileDeleted]
}

// Updates the sizing and positioning of everything in the window.
void AppWindow::ResizeEverything()
{
    RECT availableBounds = {0};
    GetClientRect(m_mainWindow, &availableBounds);

    if (!m_containsFullscreenElement
    )
    {
        availableBounds = m_toolbar.Resize(availableBounds);
    }
    if (auto view = GetComponent<ViewComponent>())
    {
        view->SetBounds(availableBounds);
    }
    m_appBackgroundImageRect = availableBounds;
}

//! [Close]
// Close the WebView and deinitialize related state. This doesn't close the app window.
bool AppWindow::CloseWebView(bool cleanupUserDataFolder)
{
    if (auto file = GetComponent<FileComponent>())
    {
        if (file->IsPrintToPdfInProgress())
        {
            int selection = MessageBox(
                m_mainWindow, L"Print to PDF is in progress. Continue closing?",
                L"Print to PDF", MB_YESNO);
            if (selection == IDNO)
            {
                return false;
            }
        }
    }
    // 1. Delete components.
    DeleteAllComponents();

    // 2. If cleanup needed and BrowserProcessExited event interface available,
    // register to cleanup upon browser exit.
    wil::com_ptr<ICoreWebView2Environment5> environment5;
    if (m_webViewEnvironment)
    {
        environment5 = m_webViewEnvironment.try_query<ICoreWebView2Environment5>();
    }
    if (cleanupUserDataFolder && environment5)
    {
        // Before closing the WebView, register a handler with code to run once the
        // browser process and associated processes are terminated.
        CHECK_FAILURE(environment5->add_BrowserProcessExited(
            Callback<ICoreWebView2BrowserProcessExitedEventHandler>(
                [environment5, this](
                    ICoreWebView2Environment* sender,
                    ICoreWebView2BrowserProcessExitedEventArgs* args)
                {
                    COREWEBVIEW2_BROWSER_PROCESS_EXIT_KIND kind;
                    UINT32 pid;
                    CHECK_FAILURE(args->get_BrowserProcessExitKind(&kind));
                    CHECK_FAILURE(args->get_BrowserProcessId(&pid));

                    // If a new WebView is created from this CoreWebView2Environment after
                    // the browser has exited but before our handler gets to run, a new
                    // browser process will be created and lock the user data folder
                    // again. Do not attempt to cleanup the user data folder in these
                    // cases. We check the PID of the exited browser process against the
                    // PID of the browser process to which our last CoreWebView2 attached.
                    if (pid == m_newestBrowserPid)
                    {
                        // Watch for graceful browser process exit. Let ProcessFailed event
                        // handler take care of failed browser process termination.
                        if (kind == COREWEBVIEW2_BROWSER_PROCESS_EXIT_KIND_NORMAL)
                        {
                            CHECK_FAILURE(environment5->remove_BrowserProcessExited(
                                m_browserExitedEventToken));
                            // Release the environment only after the handler is invoked.
                            // Otherwise, there will be no environment to raise the event when
                            // the collection of WebView2 Runtime processes exit.
                            m_webViewEnvironment = nullptr;
                            RunAsync([this]() { CleanupUserDataFolder(); });
                        }
                    }
                    else
                    {
                        // The exiting process is not the last in use. Do not attempt cleanup
                        // as we might still have a webview open over the user data folder.
                        // Do not block from event handler.
                        AsyncMessageBox(
                            L"A new browser process prevented cleanup of the user data folder.",
                            L"Cleanup User Data Folder");
                    }

                    return S_OK;
                })
                .Get(),
            &m_browserExitedEventToken));
    }

    // 3. Close the webview.
    if (m_controller)
    {
        m_controller->Close();
        m_controller = nullptr;
        m_webView = nullptr;
        m_webView3 = nullptr;
    }

    // 4. If BrowserProcessExited event interface is not available, release
    // environment and proceed to cleanup immediately. If the interface is
    // available, release environment only if not waiting for the event.
    if (!environment5)
    {
        m_webViewEnvironment = nullptr;
        if (cleanupUserDataFolder)
        {
            CleanupUserDataFolder();
        }
    }
    else if (!cleanupUserDataFolder)
    {
        // Release the environment object here only if no cleanup is needed.
        // If cleanup is needed, the environment object release is deferred
        // until the browser process exits, otherwise the handler for the
        // BrowserProcessExited event will not be called.
        m_webViewEnvironment = nullptr;
    }

    // reset profile name
    m_profileName = L"";
    m_documentTitle = L"";
    return true;
}
//! [Close]

void AppWindow::CleanupUserDataFolder()
{
    // For non-UWP apps, the default user data folder {Executable File Name}.WebView2
    // is in the same directory next to the app executable. If end
    // developers specify userDataFolder during WebView environment
    // creation, they would need to pass in that explicit value here.
    // For more information about userDataFolder:
    // https://learn.microsoft.com/microsoft-edge/webview2/reference/win32/webview2-idl#createcorewebview2environmentwithoptions
    WCHAR userDataFolder[MAX_PATH] = L"";
    // Obtain the absolute path for relative paths that include "./" or "../"
    _wfullpath(userDataFolder, GetLocalPath(L".WebView2", true).c_str(), MAX_PATH);
    std::wstring userDataFolderPath(userDataFolder);

    std::wstring message = L"Are you sure you want to clean up the user data folder at\n";
    message += userDataFolderPath;
    message += L"\n?\nWarning: This action is not reversible.\n\n";
    message += L"Click No if there are other open WebView instances.\n";

    if (MessageBox(m_mainWindow, message.c_str(), L"Cleanup User Data Folder", MB_YESNO) ==
        IDYES)
    {
        CHECK_FAILURE(DeleteFileRecursive(userDataFolderPath));
    }
}

HRESULT AppWindow::DeleteFileRecursive(std::wstring path)
{
    wil::com_ptr<IFileOperation> fileOperation;
    CHECK_FAILURE(
        CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_ALL, IID_PPV_ARGS(&fileOperation)));

    // Turn off all UI from being shown to the user during the operation.
    CHECK_FAILURE(fileOperation->SetOperationFlags(FOF_NO_UI));

    wil::com_ptr<IShellItem> userDataFolder;
    CHECK_FAILURE(
        SHCreateItemFromParsingName(path.c_str(), NULL, IID_PPV_ARGS(&userDataFolder)));

    // Add the operation
    CHECK_FAILURE(fileOperation->DeleteItem(userDataFolder.get(), NULL));
    userDataFolder.reset();

    // Perform the operation to delete the directory
    CHECK_FAILURE(fileOperation->PerformOperations());

    CHECK_FAILURE(fileOperation->Release());
    OleUninitialize();
    return S_OK;
}

void AppWindow::CloseAppWindow()
{
    if (m_onAppWindowClosing)
    {
        m_onAppWindowClosing();
        m_onAppWindowClosing = nullptr;
    }
    if (!CloseWebView())
    {
        return;
    }
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

void AppWindow::SetDocumentTitle(PCWSTR titleText)
{
    m_documentTitle = titleText;
    UpdateAppTitle();
}

std::wstring AppWindow::GetDocumentTitle()
{
    return m_documentTitle;
}

void AppWindow::UpdateAppTitle()
{
    std::wstring str(m_appTitle);
    if (!m_profileName.empty())
    {
        str += L" - " + m_profileName;
    }
    if (!m_documentTitle.empty())
    {
        str += L" - " + m_documentTitle;
    }
    SetWindowText(m_mainWindow, str.c_str());
}

RECT AppWindow::GetWindowBounds()
{
    RECT hwndBounds = {0};
    GetClientRect(m_mainWindow, &hwndBounds);
    return hwndBounds;
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

std::wstring AppWindow::GetLocalUri(
    std::wstring relativePath, bool useVirtualHostName /*= true*/)
{
    if (useVirtualHostName && m_webView3)
    {
        //! [LocalUrlUsage]
        const std::wstring localFileRootUrl = L"https://appassets.example/";
        return localFileRootUrl + regex_replace(relativePath, std::wregex(L"\\\\"), L"/");
        //! [LocalUrlUsage]
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

void AppWindow::AsyncMessageBox(std::wstring message, std::wstring title)
{
    RunAsync([this, message = std::move(message), title = std::move(title)]
             { MessageBox(m_mainWindow, message.c_str(), title.c_str(), MB_OK); });
}

void AppWindow::EnterFullScreen()
{
    DWORD style = GetWindowLong(m_mainWindow, GWL_STYLE);
    MONITORINFO monitor_info = {sizeof(monitor_info)};
    m_hMenu = ::GetMenu(m_mainWindow);
    ::SetMenu(m_mainWindow, nullptr);
    if (GetWindowRect(m_mainWindow, &m_previousWindowRect) &&
        GetMonitorInfo(
            MonitorFromWindow(m_mainWindow, MONITOR_DEFAULTTOPRIMARY), &monitor_info))
    {
        SetWindowLong(m_mainWindow, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
        SetWindowPos(
            m_mainWindow, HWND_TOP, monitor_info.rcMonitor.left, monitor_info.rcMonitor.top,
            monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
            monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

void AppWindow::ExitFullScreen()
{
    DWORD style = GetWindowLong(m_mainWindow, GWL_STYLE);
    ::SetMenu(m_mainWindow, m_hMenu);
    SetWindowLong(m_mainWindow, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
    SetWindowPos(
        m_mainWindow, NULL, m_previousWindowRect.left, m_previousWindowRect.top,
        m_previousWindowRect.right - m_previousWindowRect.left,
        m_previousWindowRect.bottom - m_previousWindowRect.top,
        SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
}

// We have our own implementation of DCompositionCreateDevice2 that dynamically
// loads dcomp.dll to create the device. Not having a static dependency on dcomp.dll
// enables the sample app to run on versions of Windows that don't support dcomp.
HRESULT AppWindow::DCompositionCreateDevice2(IUnknown* renderingDevice, REFIID riid, void** ppv)
{
    HRESULT hr = E_FAIL;
    static decltype(::DCompositionCreateDevice2)* fnCreateDCompDevice2 = nullptr;
    if (fnCreateDCompDevice2 == nullptr)
    {
        HMODULE hmod = ::LoadLibraryEx(L"dcomp.dll", nullptr, 0);
        if (hmod != nullptr)
        {
            fnCreateDCompDevice2 = reinterpret_cast<decltype(::DCompositionCreateDevice2)*>(
                ::GetProcAddress(hmod, "DCompositionCreateDevice2"));
        }
    }
    if (fnCreateDCompDevice2 != nullptr)
    {
        hr = fnCreateDCompDevice2(renderingDevice, riid, ppv);
    }
    return hr;
}

// WinRT APIs cannot run without a DispatcherQueue. This helper function creates a
// DispatcherQueueController (which instantiates a DispatcherQueue under the covers) that will
// manage tasks for the WinRT APIs. The DispatcherQueue implementation lives in
// CoreMessaging.dll Similar to dcomp.dll, we load CoreMessaging.dll dynamically so the sample
// app can run on versions of windows that don't have CoreMessaging.
HRESULT AppWindow::TryCreateDispatcherQueue()
{
    namespace winSystem = winrt::Windows::System;

    HRESULT hr = S_OK;
    thread_local winSystem::DispatcherQueueController dispatcherQueueController{nullptr};

    if (dispatcherQueueController == nullptr)
    {
        hr = E_FAIL;
        static decltype(::CreateDispatcherQueueController)* fnCreateDispatcherQueueController =
            nullptr;
        if (fnCreateDispatcherQueueController == nullptr)
        {
            HMODULE hmod = ::LoadLibraryEx(L"CoreMessaging.dll", nullptr, 0);
            if (hmod != nullptr)
            {
                fnCreateDispatcherQueueController =
                    reinterpret_cast<decltype(::CreateDispatcherQueueController)*>(
                        ::GetProcAddress(hmod, "CreateDispatcherQueueController"));
            }
        }
        if (fnCreateDispatcherQueueController != nullptr)
        {
            winSystem::DispatcherQueueController controller{nullptr};
            DispatcherQueueOptions options{
                sizeof(DispatcherQueueOptions), DQTYPE_THREAD_CURRENT, DQTAT_COM_STA};
            hr = fnCreateDispatcherQueueController(
                options, reinterpret_cast<ABI::Windows::System::IDispatcherQueueController**>(
                             winrt::put_abi(controller)));
            dispatcherQueueController = controller;
        }
    }

    return hr;
}

//! [TextScaleChanged2]
void AppWindow::OnTextScaleChanged(
    winrt::Windows::UI::ViewManagement::UISettings const& settings,
    winrt::Windows::Foundation::IInspectable const& args)
{
    RunAsync(
        [this]
        {
            m_toolbar.UpdateDpiAndTextScale();
            if (auto view = GetComponent<ViewComponent>())
            {
                view->UpdateDpiAndTextScale();
            }
        });
}
//! [TextScaleChanged2]

void AppWindow::UpdateCreationModeMenu()
{
    HMENU hMenu = GetMenu(m_mainWindow);
    CheckMenuRadioItem(
        hMenu, IDM_CREATION_MODE_WINDOWED, IDM_CREATION_MODE_HOST_INPUT_PROCESSING,
        m_creationModeId, MF_BYCOMMAND);
}

double AppWindow::GetDpiScale()
{
    return DpiUtil::GetDpiForWindow(m_mainWindow) * 1.0f / USER_DEFAULT_SCREEN_DPI;
}

double AppWindow::GetTextScale()
{
    return m_uiSettings ? m_uiSettings.TextScaleFactor() : 1.0f;
}

void AppWindow::AddRef()
{
    InterlockedIncrement((LONG*)&m_refCount);
}

void AppWindow::Release()
{
    uint32_t refCount = InterlockedDecrement((LONG*)&m_refCount);
    if (refCount == 0)
    {
        delete this;
    }
}

void AppWindow::NotifyClosed()
{
    m_isClosed = true;
}

void AppWindow::EnableHandlingNewWindowRequest(bool enable)
{
    m_shouldHandleNewWindowRequest = enable;
}

void AppWindow::InstallComplete(int return_code)
{
    if (!m_isClosed)
    {
        if (return_code == 0)
        {
            RunAsync([this] { InitializeWebView(); });
        }
        else if (return_code == 1)
        {
            MessageBox(
                m_mainWindow, L"WebView Runtime failed to Install",
                L"WebView Runtime Installation status", MB_OK);
        }
        else if (return_code == 2)
        {
            MessageBox(
                m_mainWindow, L"WebView Bootstrapper failled to download",
                L"WebView Bootstrapper Download status", MB_OK);
        }
    }
}
