// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "AppWindow.h"

#include <DispatcherQueue.h>
#include <functional>
#include <regex>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <ShObjIdl_core.h>
#include <Shellapi.h>
#include <ShlObj_core.h>
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
#include "ScenarioAddHostObject.h"
#include "ScenarioAuthentication.h"
#include "ScenarioClientCertificateRequested.h"
#include "ScenarioCookieManagement.h"
#include "ScenarioCustomDownloadExperience.h"
#include "ScenarioDOMContentLoaded.h"
#include "ScenarioIFrameDevicePermission.h"
#include "ScenarioNavigateWithWebResourceRequest.h"
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
    AppWindow* appWindow = (AppWindow*) lpParameter;

    int returnCode = 2; // Download failed
    // Use fwlink to download WebView2 Bootstrapper at runtime and invoke installation
    // Broken/Invalid Https Certificate will fail to download
    // Use of the download link below is governed by the below terms. You may acquire the link for your use at https://developer.microsoft.com/microsoft-edge/webview2/.
    // Microsoft owns all legal right, title, and interest in and to the
    // WebView2 Runtime Bootstrapper ("Software") and related documentation,
    // including any intellectual property in the Software. You must acquire all
    // code, including any code obtained from a Microsoft URL, under a separate
    // license directly from Microsoft, including a Microsoft download site
    // (e.g., https://developer.microsoft.com/microsoft-edge/webview2/).
    HRESULT hr = URLDownloadToFile(NULL, L"https://go.microsoft.com/fwlink/p/?LinkId=2124703", L".\\MicrosoftEdgeWebview2Setup.exe", 0, 0);
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
            returnCode = 0; // Install successfull
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
        SetDlgItemText(hDlg, IDC_EDIT_DOWNLOAD_PATH,
            app->GetWebViewOption().downloadPath.c_str());
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

            int downloadPathLength = GetWindowTextLength(
                GetDlgItem(hDlg, IDC_EDIT_DOWNLOAD_PATH));
            wchar_t downloadPath[MAX_PATH] = {};
            GetDlgItemText(hDlg, IDC_EDIT_DOWNLOAD_PATH, downloadPath,
                downloadPathLength + 1);

            WebViewCreateOption opt(std::wstring(std::move(text)), inPrivate,
                std::wstring(std::move(downloadPath)),
                WebViewCreateEntry::EVER_FROM_CREATE_WITH_OPTION_MENU);

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
    UINT creationModeId,
    const WebViewCreateOption& opt,
    const std::wstring& initialUri,
    const std::wstring& userDataFolderParam,
    bool isMainWindow,
    std::function<void()> webviewCreatedCallback,
    bool customWindowRect,
    RECT windowRect,
    bool shouldHaveToolbar
    )
    : m_creationModeId(creationModeId),
      m_webviewOption(opt),
      m_initialUri(initialUri),
      m_onWebViewFirstInitialized(webviewCreatedCallback)
{
    // Initialize COM as STA.
    CHECK_FAILURE(OleInitialize(NULL));

    ++s_appInstances;

    WCHAR szTitle[s_maxLoadString]; // The title bar text
    LoadStringW(g_hInstance, IDS_APP_TITLE, szTitle, s_maxLoadString);
    m_appTitle = szTitle;

    if (userDataFolderParam.length() > 0)
    {
        m_userDataFolder = userDataFolderParam;
    }

    if (customWindowRect)
    {
        m_mainWindow = CreateWindowExW(
            WS_EX_CONTROLPARENT, GetWindowClass(), szTitle, WS_OVERLAPPEDWINDOW, windowRect.left,
            windowRect.top, windowRect.right-windowRect.left, windowRect.bottom-windowRect.top, nullptr, nullptr, g_hInstance, nullptr);
    }
    else
    {
        m_mainWindow = CreateWindowExW(
            WS_EX_CONTROLPARENT, GetWindowClass(), szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
            0, CW_USEDEFAULT, 0, nullptr, nullptr, g_hInstance, nullptr);
    }

    m_appBackgroundImageHandle = (HBITMAP)LoadImage(
        g_hInstance, MAKEINTRESOURCE(IDI_WEBVIEW2_BACKGROUND), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
    GetObject(m_appBackgroundImageHandle, sizeof(m_appBackgroundImage), &m_appBackgroundImage);
    m_memHdc = CreateCompatibleDC(GetDC(m_mainWindow));
    SelectObject(m_memHdc, m_appBackgroundImageHandle);

    SetWindowLongPtr(m_mainWindow, GWLP_USERDATA, (LONG_PTR)this);

#ifdef USE_WEBVIEW2_WIN10
    //! [TextScaleChanged1]
    if (winrt::try_get_activation_factory<winrt::Windows::UI::ViewManagement::UISettings>())
    {
        m_uiSettings = winrt::Windows::UI::ViewManagement::UISettings();
        m_uiSettings.TextScaleFactorChanged({ this, &AppWindow::OnTextScaleChanged });
    }
    //! [TextScaleChanged1]
#endif

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
        RunAsync([this] {
            InitializeWebView();
        });
    }
    else
    {
        if (isMainWindow) {
            AddRef();
            CreateThread(0, 0, DownloadAndInstallWV2RT, (void*) this, 0, 0);
        }
        else
        {
            MessageBox(m_mainWindow, L"WebView Runtime not installed", L"WebView Runtime Installation status", MB_OK);
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
    //! [DPIChanged]
    case WM_DPICHANGED:
    {
        m_toolbar.UpdateDpiAndTextScale();
        if (auto view = GetComponent<ViewComponent>())
        {
            view->UpdateDpiAndTextScale();
        }

        RECT* const newWindowSize = reinterpret_cast<RECT*>(lParam);
        SetWindowPos(hWnd,
            nullptr,
            newWindowSize->left,
            newWindowSize->top,
            newWindowSize->right - newWindowSize->left,
            newWindowSize->bottom - newWindowSize->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
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

        StretchBlt(hdc, m_appBackgroundImageRect.left, m_appBackgroundImageRect.top,
            m_appBackgroundImageRect.right, m_appBackgroundImageRect.bottom, m_memHdc,
            0, 0, m_appBackgroundImage.bmWidth, m_appBackgroundImage.bmHeight, SRCCOPY);

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
        auto environment7 =
            m_webViewEnvironment.try_query<ICoreWebView2Environment7>();
        CHECK_FEATURE_RETURN(environment7);
        wil::unique_cotaskmem_string userDataFolder;
        environment7->get_UserDataFolder(&userDataFolder);
        MessageBox(
            m_mainWindow, userDataFolder.get(), L"User Data Folder",
            MB_OK);
        //! [GetUserDataFolder]
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
                [this](HRESULT errorCode,
                   ICoreWebView2ExperimentalUpdateRuntimeResult* result) -> HRESULT {
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
    case IDM_CREATION_MODE_VISUAL_DCOMP:
    case IDM_CREATION_MODE_TARGET_DCOMP:
#ifdef USE_WEBVIEW2_WIN10
    case IDM_CREATION_MODE_VISUAL_WINCOMP:
#endif
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
        bool isCurrentlyTopMost = (GetWindowLong(m_mainWindow, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
        SetWindowPos(m_mainWindow, isCurrentlyTopMost ? HWND_NOTOPMOST : HWND_TOPMOST,
            0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
        return true;
    }
    case IDM_TOGGLE_EXCLUSIVE_USER_DATA_FOLDER_ACCESS:
        ToggleExclusiveUserDataFolderAccess();
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
        return ClearBrowsingData(
            (COREWEBVIEW2_BROWSING_DATA_KINDS)(COREWEBVIEW2_BROWSING_DATA_KINDS_GENERAL_AUTOFILL |
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
    }
    return false;
}
//! [ClearBrowsingData]
bool AppWindow::ClearBrowsingData(COREWEBVIEW2_BROWSING_DATA_KINDS dataKinds)
{
    auto webView2Experimental8 =
        m_webView.try_query<ICoreWebView2Experimental8>();
    CHECK_FEATURE_RETURN(webView2Experimental8);
    wil::com_ptr<ICoreWebView2ExperimentalProfile> webView2ExperimentalProfile;
    CHECK_FAILURE(webView2Experimental8->get_Profile(&webView2ExperimentalProfile));
    CHECK_FEATURE_RETURN(webView2ExperimentalProfile);
    auto webView2ExperimentalProfile4 = webView2ExperimentalProfile.try_query<ICoreWebView2ExperimentalProfile4>();
    CHECK_FEATURE_RETURN(webView2ExperimentalProfile4);
    // Clear the browsing data from the last hour.
    double endTime = (double)std::time(nullptr);
    double startTime = endTime - 3600.0;
    CHECK_FAILURE(webView2ExperimentalProfile4->ClearBrowsingDataInTimeRange(
        dataKinds, startTime, endTime,
        Callback<ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler>(
            [this](HRESULT error) -> HRESULT {
                AsyncMessageBox(L"Completed", L"Clear Browsing Data");
                return S_OK;
            })
            .Get()));
    return true;
}
//! [ClearBrowsingData]

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
                          L"created after all webviews are closed." :
                          L"AAD single sign on will be disabled for new WebView"
                          L" created after all webviews are closed.",
        L"AAD SSO change",
        MB_OK);
}

void AppWindow::ToggleExclusiveUserDataFolderAccess()
{
    m_ExclusiveUserDataFolderAccess = !m_ExclusiveUserDataFolderAccess;
    MessageBox(
        nullptr,
        m_ExclusiveUserDataFolderAccess ?
            L"Will request exclusive access to user data folder "
            L"for new WebView created after all webviews are closed." :
            L"Will not request exclusive access to user data folder "
            L"for new WebView created after all webviews are closed.",
        L"Exclusive User Data Folder Access change", MB_OK);
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
            return [this] {
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
#ifdef USE_WEBVIEW2_WIN10
    m_wincompCompositor = nullptr;
#endif
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
#ifdef USE_WEBVIEW2_WIN10
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
#endif
    //! [CreateCoreWebView2EnvironmentWithOptions]
    auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
    CHECK_FAILURE(
        options->put_AllowSingleSignOnUsingOSPrimaryAccount(
        m_AADSSOEnabled ? TRUE : FALSE));
    CHECK_FAILURE(options->put_ExclusiveUserDataFolderAccess(
        m_ExclusiveUserDataFolderAccess ? TRUE : FALSE));
    if (!m_language.empty())
        CHECK_FAILURE(options->put_Language(m_language.c_str()));

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
                    m_mainWindow, L"User data folder cannot be created because a file with the same name already exists.", nullptr, MB_OK);
            }
            break;
            case E_ACCESSDENIED:
            {
                MessageBox(
                    m_mainWindow, L"Unable to create user data folder, Access Denied.", nullptr, MB_OK);
            }
            break;
            case E_FAIL:
            {
                MessageBox(
                    m_mainWindow, L"Edge runtime unable to start", nullptr, MB_OK);
            }
            break;
            default:
            {
                ShowFailure(
                    hr, L"Failed to create WebView2 environment");
            }
        }
    }
}
// This is the callback passed to CreateWebViewEnvironmentWithOptions.
// Here we simply create the WebView.
HRESULT AppWindow::OnCreateEnvironmentCompleted(
    HRESULT result, ICoreWebView2Environment* environment)
{
    CHECK_FAILURE(result);
    m_webViewEnvironment = environment;

    if (m_webviewOption.entry == WebViewCreateEntry::EVER_FROM_CREATE_WITH_OPTION_MENU)
    {
        return CreateControllerWithOptions();
    }

    auto webViewEnvironment3 =
        m_webViewEnvironment.try_query<ICoreWebView2Environment3>();
#ifdef USE_WEBVIEW2_WIN10
    if (webViewEnvironment3 && (m_dcompDevice || m_wincompCompositor))
#else
    if (webViewEnvironment3 && m_dcompDevice)
#endif
    {
        CHECK_FAILURE(webViewEnvironment3->CreateCoreWebView2CompositionController(
            m_mainWindow,
            Callback<
                ICoreWebView2CreateCoreWebView2CompositionControllerCompletedHandler>(
                [this](
                    HRESULT result,
                    ICoreWebView2CompositionController* compositionController) -> HRESULT {
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
    auto webViewEnvironment8 =
        m_webViewEnvironment.try_query<ICoreWebView2ExperimentalEnvironment8>();
    if (!webViewEnvironment8)
    {
        FeatureNotAvailable();
        return S_OK;
    }

    Microsoft::WRL::ComPtr<ICoreWebView2ExperimentalControllerOptions> options;
    HRESULT hr = webViewEnvironment8->CreateCoreWebView2ControllerOptions(
        m_webviewOption.profile.c_str(), m_webviewOption.isInPrivate, options.GetAddressOf());
    if (hr == E_INVALIDARG)
    {
        ShowFailure(hr, L"Unable to create WebView2 due to an invalid profile name.");
        CloseAppWindow();
        return S_OK;
    }
    CHECK_FAILURE(hr);
    //! [CreateControllerWithOptions]

#ifdef USE_WEBVIEW2_WIN10
    if (m_dcompDevice || m_wincompCompositor)
#else
    if (m_dcompDevice)
#endif
    {
        CHECK_FAILURE(webViewEnvironment8->CreateCoreWebView2CompositionControllerWithOptions(
            m_mainWindow, options.Get(),
            Callback<ICoreWebView2CreateCoreWebView2CompositionControllerCompletedHandler>(
                [this](
                    HRESULT result,
                    ICoreWebView2CompositionController* compositionController) -> HRESULT {
                    auto controller =
                        wil::com_ptr<ICoreWebView2CompositionController>(compositionController)
                            .query<ICoreWebView2Controller>();
                    return OnCreateCoreWebView2ControllerCompleted(result, controller.get());
                })
                .Get()));
    }
    else
    {
        CHECK_FAILURE(webViewEnvironment8->CreateCoreWebView2ControllerWithOptions(
            m_mainWindow, options.Get(),
            Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                this, &AppWindow::OnCreateCoreWebView2ControllerCompleted)
                .Get()));
    }

    return S_OK;
}

void AppWindow::SetAppIcon(bool inPrivate)
{
    HICON newSmallIcon = nullptr;
    HICON newBigIcon = nullptr;
    if (inPrivate)
    {
        static HICON smallInPrivateIcon = reinterpret_cast<HICON>(LoadImage(
            g_hInstance, MAKEINTRESOURCEW(IDI_WEBVIEW2APISAMPLE_INPRIVATE), IMAGE_ICON, 16, 16,
            LR_DEFAULTCOLOR));
        static HICON bigInPrivateIcon = reinterpret_cast<HICON>(LoadImage(
            g_hInstance, MAKEINTRESOURCEW(IDI_WEBVIEW2APISAMPLE_INPRIVATE), IMAGE_ICON, 32, 32,
            LR_DEFAULTCOLOR));
        newSmallIcon = smallInPrivateIcon;
        newBigIcon = bigInPrivateIcon;
    }
    else
    {
        static HICON smallIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_WEBVIEW2APISAMPLE));
        static HICON bigIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_SMALL));
        newSmallIcon = smallIcon;
        newBigIcon = bigIcon;
    }
    reinterpret_cast<HICON>(SendMessage(
        m_mainWindow, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(newSmallIcon)));
    reinterpret_cast<HICON>(
        SendMessage(m_mainWindow, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(newBigIcon)));
}

// This is the callback passed to CreateCoreWebView2Controller. Here we initialize all WebView-related
// state and register most of our event handlers with the WebView.
HRESULT AppWindow::OnCreateCoreWebView2ControllerCompleted(HRESULT result, ICoreWebView2Controller* controller)
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
        auto webview2Experimental8 = coreWebView2.try_query<ICoreWebView2Experimental8>();
        if (webview2Experimental8)
        {
            wil::com_ptr<ICoreWebView2ExperimentalProfile> profile;
            CHECK_FAILURE(webview2Experimental8->get_Profile(&profile));
            wil::unique_cotaskmem_string profile_path;
            CHECK_FAILURE(profile->get_ProfilePath(&profile_path));
            std::wstring str(profile_path.get());
            m_profileDirName = str.substr(str.find_last_of(L'\\') + 1);
            BOOL inPrivate = FALSE;
            CHECK_FAILURE(profile->get_IsInPrivateModeEnabled(&inPrivate));
            if (!m_webviewOption.downloadPath.empty())
            {
                auto webView2ExperimentalProfile3 =
                    profile.try_query<ICoreWebView2ExperimentalProfile3>();
                CHECK_FAILURE(webView2ExperimentalProfile3->
                    put_DefaultDownloadFolderPath(
                        m_webviewOption.downloadPath.c_str()));
            }

            // update window title with m_profileDirName
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
            this, m_dcompDevice.get(),
#ifdef USE_WEBVIEW2_WIN10
            m_wincompCompositor,
#endif
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
            std::wstring initialUri = m_initialUri.empty() ? AppStartPage::GetUri(this) : m_initialUri;
            CHECK_FAILURE(m_webView->Navigate(initialUri.c_str()));
        }
    }
    else if (result == E_ABORT)
    {
        // Webview creation was aborted because the user closed this window.
        // No need to report this as an error.
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
            [this](ICoreWebView2* sender, IUnknown* args) -> HRESULT {
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
            [this](ICoreWebView2* sender, ICoreWebView2NewWindowRequestedEventArgs* args) {
                if (!m_shouldHandleNewWindowRequest)
                {
                    args->put_Handled(FALSE);
                    return S_OK;
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
                newAppWindow->m_onWebViewFirstInitialized = [args, deferral, newAppWindow]() {
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
        Callback<ICoreWebView2WindowCloseRequestedEventHandler>([this](
                                                                    ICoreWebView2* sender,
                                                                    IUnknown* args) {
            if (m_isPopupWindow)
            {
                CloseAppWindow();
            }
            return S_OK;
        }).Get(),
        nullptr));
    //! [WindowCloseRequested]

    //! [NewBrowserVersionAvailable]
    // After the environment is successfully created,
    // register a handler for the NewBrowserVersionAvailable event.
    // This handler tells when there is a new Edge version available on the machine.
    CHECK_FAILURE(m_webViewEnvironment->add_NewBrowserVersionAvailable(
        Callback<ICoreWebView2NewBrowserVersionAvailableEventHandler>(
            [this](ICoreWebView2Environment* sender, IUnknown* args) -> HRESULT {
                 // Don't block the event handler with a message box
                RunAsync([this]
                {
                    std::wstring message = L"We detected there is a new version for the browser.";
                    if (m_webView)
                    {
                        message += L"Do you want to restart the app? \n\n";
                        message += L"Click No if you only want to re-create the webviews. \n";
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
}

// Updates the sizing and positioning of everything in the window.
void AppWindow::ResizeEverything()
{
    RECT availableBounds = {0};
    GetClientRect(m_mainWindow, &availableBounds);

    if (!m_containsFullscreenElement)
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
            int selection = MessageBox(m_mainWindow,
                L"Print to PDF is in progress. Continue closing?",
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
        environment5 =
            m_webViewEnvironment.try_query<ICoreWebView2Environment5>();
    }
    if (cleanupUserDataFolder && environment5)
    {
        // Before closing the WebView, register a handler with code to run once the
        // browser process and associated processes are terminated.
        CHECK_FAILURE(environment5->add_BrowserProcessExited(
            Callback<ICoreWebView2BrowserProcessExitedEventHandler>(
                [environment5, this](
                    ICoreWebView2Environment* sender,
                    ICoreWebView2BrowserProcessExitedEventArgs* args) {
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
    m_profileDirName = L"";
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
    // https://docs.microsoft.com/microsoft-edge/webview2/reference/win32/webview2-idl#createcorewebview2environmentwithoptions
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

void AppWindow::UpdateAppTitle() {
    std::wstring str(m_appTitle);
    if (!m_profileDirName.empty())
    {
        str += L" - " + m_profileDirName;
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
    {
        MessageBox(m_mainWindow, message.c_str(), title.c_str(), MB_OK);
    });
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
    thread_local winSystem::DispatcherQueueController dispatcherQueueController{ nullptr };

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
            winSystem::DispatcherQueueController controller{ nullptr };
            DispatcherQueueOptions options
            {
                sizeof(DispatcherQueueOptions),
                DQTYPE_THREAD_CURRENT,
                DQTAT_COM_STA
            };
            hr = fnCreateDispatcherQueueController(
                options, reinterpret_cast<ABI::Windows::System::IDispatcherQueueController**>(
                             winrt::put_abi(controller)));
            dispatcherQueueController = controller;
        }
    }

    return hr;
}

#ifdef USE_WEBVIEW2_WIN10
//! [TextScaleChanged2]
void AppWindow::OnTextScaleChanged(
    winrt::Windows::UI::ViewManagement::UISettings const& settings,
    winrt::Windows::Foundation::IInspectable const& args)
{
    RunAsync([this] {
        m_toolbar.UpdateDpiAndTextScale();
        if (auto view = GetComponent<ViewComponent>())
        {
            view->UpdateDpiAndTextScale();
        }
    });
}
//! [TextScaleChanged2]
#endif

void AppWindow::UpdateCreationModeMenu()
{
    HMENU hMenu = GetMenu(m_mainWindow);
    CheckMenuRadioItem(
        hMenu,
        IDM_CREATION_MODE_WINDOWED,
#ifdef USE_WEBVIEW2_WIN10
        IDM_CREATION_MODE_VISUAL_WINCOMP,
#else
        IDM_CREATION_MODE_TARGET_DCOMP,
#endif
        m_creationModeId,
        MF_BYCOMMAND);
}

double AppWindow::GetDpiScale()
{
    return DpiUtil::GetDpiForWindow(m_mainWindow) * 1.0f / USER_DEFAULT_SCREEN_DPI;
}

double AppWindow::GetTextScale()
{
#ifdef USE_WEBVIEW2_WIN10
    return m_uiSettings ? m_uiSettings.TextScaleFactor() : 1.0f;
#else
    return 1.0f;
#endif
}

void AppWindow::AddRef()
{
    InterlockedIncrement((LONG *)&m_refCount);
}

void AppWindow::Release()
{
    uint32_t refCount = InterlockedDecrement((LONG *)&m_refCount);
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
            RunAsync([this] {
                InitializeWebView();
            });
        }
        else if (return_code == 1)
        {
            MessageBox(m_mainWindow, L"WebView Runtime failed to Install", L"WebView Runtime Installation status", MB_OK);
        }
        else if (return_code == 2)
        {
            MessageBox(m_mainWindow, L"WebView Bootstrapper failled to download", L"WebView Bootstrapper Download status", MB_OK);
        }
    }
}
