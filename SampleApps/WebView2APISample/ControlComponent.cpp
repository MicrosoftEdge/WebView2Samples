// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "ControlComponent.h"
#include "ScenarioWebViewEventMonitor.h"

#include "App.h"
#include "CheckFailure.h"
#include <regex>
#include <shlwapi.h>

using namespace Microsoft::WRL;

ControlComponent::ControlComponent(AppWindow* appWindow, Toolbar* toolbar)
    : m_appWindow(appWindow), m_controller(appWindow->GetWebViewController()),
      m_webView(appWindow->GetWebView()), m_toolbar(toolbar)
{
    m_toolbar->SetItemEnabled(Toolbar::Item_AddressBar, true);
    m_toolbar->SetItemEnabled(Toolbar::Item_GoButton, true);

    // Register a handler for the NavigationStarting event.
    // This handler just enables the Cancel button.
    CHECK_FAILURE(m_webView->add_NavigationStarting(
        Callback<ICoreWebView2NavigationStartingEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args)
            -> HRESULT {
                m_toolbar->SetItemEnabled(Toolbar::Item_CancelButton, true);
                return S_OK;
            })
            .Get(),
        &m_navigationStartingToken));

    //! [SourceChanged]
    // Register a handler for the SourceChanged event.
    // This handler will read the webview's source URI and update
    // the app's address bar.
    CHECK_FAILURE(m_webView->add_SourceChanged(
        Callback<ICoreWebView2SourceChangedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2SourceChangedEventArgs* args)
                -> HRESULT {
                wil::unique_cotaskmem_string uri;
                sender->get_Source(&uri);
                if (wcscmp(uri.get(), L"about:blank") == 0)
                {
                    uri = wil::make_cotaskmem_string(L"");
                }
                SetWindowText(GetAddressBar(), uri.get());

                return S_OK;
            })
            .Get(),
        &m_sourceChangedToken));
    //! [SourceChanged]

    //! [HistoryChanged]
    // Register a handler for the HistoryChanged event.
    // Update the Back, Forward buttons.
    CHECK_FAILURE(m_webView->add_HistoryChanged(
        Callback<ICoreWebView2HistoryChangedEventHandler>(
            [this](ICoreWebView2* sender, IUnknown* args) -> HRESULT {
                BOOL canGoBack;
                BOOL canGoForward;
                sender->get_CanGoBack(&canGoBack);
                sender->get_CanGoForward(&canGoForward);
                m_toolbar->SetItemEnabled(Toolbar::Item_BackButton, canGoBack);
                m_toolbar->SetItemEnabled(Toolbar::Item_ForwardButton, canGoForward);

                return S_OK;
            })
            .Get(),
        &m_historyChangedToken));
    //! [HistoryChanged]

    //! [NavigationCompleted]
    // Register a handler for the NavigationCompleted event.
    // Check whether the navigation succeeded, and if not, do something.
    // Also update the Cancel buttons.
    CHECK_FAILURE(m_webView->add_NavigationCompleted(
        Callback<ICoreWebView2NavigationCompletedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args)
                -> HRESULT {
                BOOL success;
                CHECK_FAILURE(args->get_IsSuccess(&success));
                if (!success)
                {
                    COREWEBVIEW2_WEB_ERROR_STATUS webErrorStatus;
                    CHECK_FAILURE(args->get_WebErrorStatus(&webErrorStatus));
                    if (webErrorStatus == COREWEBVIEW2_WEB_ERROR_STATUS_DISCONNECTED)
                    {
                        // Do something here if you want to handle a specific error case.
                        // In most cases this isn't necessary, because the WebView will
                        // display its own error page automatically.
                    }
                }
                m_toolbar->SetItemEnabled(Toolbar::Item_CancelButton, false);
                m_toolbar->SetItemEnabled(Toolbar::Item_ReloadButton, true);
                return S_OK;
            })
            .Get(),
        &m_navigationCompletedToken));
    //! [NavigationCompleted]

    //! [FrameNavigationCompleted]
    // Register a handler for the FrameNavigationCompleted event.
    // Check whether the navigation succeeded, and if not, do something.
    CHECK_FAILURE(m_webView->add_FrameNavigationCompleted(
        Callback<ICoreWebView2NavigationCompletedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args)
                -> HRESULT {
                BOOL success;
                CHECK_FAILURE(args->get_IsSuccess(&success));
                if (!success)
                {
                    COREWEBVIEW2_WEB_ERROR_STATUS webErrorStatus;
                    CHECK_FAILURE(args->get_WebErrorStatus(&webErrorStatus));
                    // The web page can cancel its own iframe loads, so we'll ignore that.
                    if (webErrorStatus != COREWEBVIEW2_WEB_ERROR_STATUS_OPERATION_CANCELED)
                    {
                        m_appWindow->AsyncMessageBox(
                            L"Iframe navigation failed: "
                                + WebErrorStatusToString(webErrorStatus),
                            L"Navigation Failure");
                    }
                }
                return S_OK;
            })
            .Get(),
        &m_frameNavigationCompletedToken));
    //! [FrameNavigationCompleted]

    //! [MoveFocusRequested]
    // Register a handler for the MoveFocusRequested event.
    // This event will be fired when the user tabs out of the webview.
    // The handler will focus another window in the app, depending on which
    // direction the focus is being shifted.
    CHECK_FAILURE(m_controller->add_MoveFocusRequested(
        Callback<ICoreWebView2MoveFocusRequestedEventHandler>(
            [this](
                ICoreWebView2Controller* sender,
                ICoreWebView2MoveFocusRequestedEventArgs* args) -> HRESULT {
                if (!g_autoTabHandle)
                {
                    COREWEBVIEW2_MOVE_FOCUS_REASON reason;
                    CHECK_FAILURE(args->get_Reason(&reason));

                    if (reason == COREWEBVIEW2_MOVE_FOCUS_REASON_NEXT)
                    {
                        TabForwards(-1);
                    }
                    else if (reason == COREWEBVIEW2_MOVE_FOCUS_REASON_PREVIOUS)
                    {
                        TabBackwards(m_tabbableWindows.size());
                    }
                    CHECK_FAILURE(args->put_Handled(TRUE));
                }
                return S_OK;
            })
            .Get(),
        &m_moveFocusRequestedToken));
    //! [MoveFocusRequested]

    // Replace the window procs on some toolbar elements to customize their behavior
    for (auto hwnd : m_toolbar->GetItems())
    {
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        auto originalWndProc =
            (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)ChildWndProcStatic);
        m_tabbableWindows.emplace_back(hwnd, originalWndProc);
    }

    //! [AcceleratorKeyPressed]
    // Register a handler for the AcceleratorKeyPressed event.
    CHECK_FAILURE(m_controller->add_AcceleratorKeyPressed(
        Callback<ICoreWebView2AcceleratorKeyPressedEventHandler>(
            [this](
                ICoreWebView2Controller* sender,
                ICoreWebView2AcceleratorKeyPressedEventArgs* args) -> HRESULT {
                COREWEBVIEW2_KEY_EVENT_KIND kind;
                CHECK_FAILURE(args->get_KeyEventKind(&kind));
                // We only care about key down events.
                if (kind == COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN ||
                    kind == COREWEBVIEW2_KEY_EVENT_KIND_SYSTEM_KEY_DOWN)
                {
                    UINT key;
                    CHECK_FAILURE(args->get_VirtualKey(&key));
                    // Check if the key is one we want to handle.
                    std::function<void()> action = m_appWindow->GetAcceleratorKeyFunction(key);
                    if (action)
                    {
                        // Keep the browser from handling this key, whether it's autorepeated or
                        // not.
                        CHECK_FAILURE(args->put_Handled(TRUE));

                        // Filter out autorepeated keys.
                        COREWEBVIEW2_PHYSICAL_KEY_STATUS status;
                        CHECK_FAILURE(args->get_PhysicalKeyStatus(&status));
                        if (!status.WasKeyDown)
                        {
                            // Perform the action asynchronously to avoid blocking the
                            // browser process's event queue.
                            m_appWindow->RunAsync(action);
                        }
                    }
                }
                return S_OK;
            })
            .Get(),
        &m_acceleratorKeyPressedToken));
    //! [AcceleratorKeyPressed]
}

bool ControlComponent::HandleWindowMessage(
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result)
{
    if (message == WM_COMMAND)
    {
        switch (LOWORD(wParam))
        {
        case IDM_FOCUS_SET:
            CHECK_FAILURE(m_controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC));
            return true;
        case IDM_FOCUS_TAB_IN:
            CHECK_FAILURE(m_controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_NEXT));
            return true;
        case IDM_FOCUS_REVERSE_TAB_IN:
            CHECK_FAILURE(m_controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PREVIOUS));
            return true;
        case IDM_TOGGLE_TAB_HANDLING:
            g_autoTabHandle = !g_autoTabHandle;
            MessageBox(
                nullptr,
                (std::wstring(L"Tab handling by host app is now ") +
                    (g_autoTabHandle ? L"disabled." : L"enabled."))
                .c_str(),
                L"", MB_OK);
            return true;
        case IDE_ADDRESSBAR_GO:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                NavigateToAddressBar();
                return true;
            }
        case IDE_BACK:
            CHECK_FAILURE(m_webView->GoBack());
            return true;
        case IDE_FORWARD:
            CHECK_FAILURE(m_webView->GoForward());
            return true;
        case IDE_ADDRESSBAR_RELOAD:
            CHECK_FAILURE(m_webView->Reload());
            return true;
        case IDE_CANCEL:
            CHECK_FAILURE(m_webView->Stop());
            return true;
        }
    }
    return false;
}

// We replace the address bar and go buttons' wndproc with this.
LRESULT CALLBACK
ControlComponent::ChildWndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (auto self = reinterpret_cast<ControlComponent*>(GetWindowLongPtr(hWnd, GWLP_USERDATA)))
    {
        LRESULT result = 0;
        if (self->HandleChildWindowMessage(hWnd, message, wParam, lParam, &result))
        {
            return result;
        }
        // Continue with original window proc if we didn't handle this message
        for (auto pair : self->m_tabbableWindows)
        {
            if (hWnd == pair.first)
            {
                return CallWindowProc(pair.second, hWnd, message, wParam, lParam);
            }
        }
        // This should never be called on a window we don't know about.
        FAIL_FAST();
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

bool ControlComponent::HandleChildWindowMessage(
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result)
{
    // If not calling IsDialogMessage to handle tab traversal automatically,
    // detect tab traversal and cycle focus through address bar, go button, and
    // elements in WebView.
    if (message == WM_KEYDOWN)
    {
        //! [MoveFocus1]
        if (wParam == VK_TAB)
        {
            // Find out if the window is one we've customized for tab handling
            for (size_t i = 0; i < m_tabbableWindows.size(); i++)
            {
                if (m_tabbableWindows[i].first == hWnd)
                {
                    if (GetKeyState(VK_SHIFT) < 0)
                    {
                        TabBackwards(i);
                    }
                    else
                    {
                        TabForwards(i);
                    }
                    return true;
                }
            }
        }
        //! [MoveFocus1]
        else if ((wParam == VK_RETURN) && (hWnd == GetAddressBar()))
        {
            // Handle pressing Enter in address bar
            NavigateToAddressBar();
            return true;
        }
        else
        {
            // If bit 30 is set, it means the WM_KEYDOWN message is autorepeated.
            // We want to ignore it in that case.
            if (!(lParam & (1 << 30)))
            {
                if (auto action = m_appWindow->GetAcceleratorKeyFunction((UINT)wParam))
                {
                    action();
                    return true;
                }
            }
        }
    }
    else if ((message == WM_CHAR) && ((wParam == VK_TAB) || (wParam == VK_RETURN)))
    {
        // Ignore Tab and return char messages to avoid the ding sound.
        return true;
    }
    else if ((message == WM_GETDLGCODE) && (wParam == VK_RETURN))
    {
        // When calling IsDialogMessage to handle tab traversal automatically, tell
        // the system that we want to handle the VK_RETURN. This let's the app to
        // Navigate the WebView when Enter is pressed in the address bar.
        *result = DLGC_WANTALLKEYS;
        return true;
    }
    return false;
}

//! [Navigate]
void ControlComponent::NavigateToAddressBar()
{
    int length = GetWindowTextLength(GetAddressBar());
    std::wstring uri(length, 0);
    PWSTR buffer = const_cast<PWSTR>(uri.data());
    GetWindowText(GetAddressBar(), buffer, length + 1);

    HRESULT hr = m_webView->Navigate(uri.c_str());
    if (hr == E_INVALIDARG)
    {
        // An invalid URI was provided.
        if (uri.find(L' ') == std::wstring::npos
            && uri.find(L'.') != std::wstring::npos)
        {
            // If it contains a dot and no spaces, try tacking http:// on the front.
            hr = m_webView->Navigate((L"http://" + uri).c_str());
        }
        else
        {
            // Otherwise treat it as a web search. 
            std::wstring urlEscaped(2048, ' ');
            DWORD dwEscaped = (DWORD)urlEscaped.length();
            UrlEscapeW(uri.c_str(), &urlEscaped[0], &dwEscaped, URL_ESCAPE_ASCII_URI_COMPONENT);
            hr = m_webView->Navigate(
                (L"https://bing.com/search?q=" +
                 std::regex_replace(urlEscaped, std::wregex(L"(?:%20)+"), L"+"))
                    .c_str());
        }
    }
    if (hr != E_INVALIDARG) {
        CHECK_FAILURE(hr);
    }
}
//! [Navigate]

//! [MoveFocus2]
void ControlComponent::TabForwards(size_t currentIndex)
{
    // Find first enabled window after the active one
    for (size_t i = currentIndex + 1; i < m_tabbableWindows.size(); i++)
    {
        HWND hwnd = m_tabbableWindows.at(i).first;
        if (IsWindowEnabled(hwnd))
        {
            SetFocus(hwnd);
            return;
        }
    }
    // If this is the last enabled window, tab forwards into the WebView.
    m_controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_NEXT);
}

void ControlComponent::TabBackwards(size_t currentIndex)
{
    // Find first enabled window before the active one
    for (size_t i = currentIndex - 1; i >= 0 && i < m_tabbableWindows.size(); i--)
    {
        HWND hwnd = m_tabbableWindows.at(i).first;
        if (IsWindowEnabled(hwnd))
        {
            SetFocus(hwnd);
            return;
        }
    }
    // If this is the last enabled window, tab forwards into the WebView.
    CHECK_FAILURE(m_controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PREVIOUS));
}
//! [MoveFocus2]

ControlComponent::~ControlComponent()
{
    m_webView->remove_NavigationStarting(m_navigationStartingToken);
    m_webView->remove_SourceChanged(m_sourceChangedToken);
    m_webView->remove_HistoryChanged(m_historyChangedToken);
    m_webView->remove_NavigationCompleted(m_navigationCompletedToken);
    m_webView->remove_FrameNavigationCompleted(m_frameNavigationCompletedToken);
    m_controller->remove_MoveFocusRequested(m_moveFocusRequestedToken);
    m_controller->remove_AcceleratorKeyPressed(m_acceleratorKeyPressedToken);

    // Undo our modifications to the toolbar elements
    for (auto pair : m_tabbableWindows)
    {
        SetWindowLongPtr(pair.first, GWLP_USERDATA, (LONG_PTR)nullptr);
        SetWindowLongPtr(pair.first, GWLP_WNDPROC, (LONG_PTR)pair.second);
    }

    SetWindowText(GetAddressBar(), L"");
    m_toolbar->DisableAllItems();
}

HWND ControlComponent::GetAddressBar()
{
    return m_toolbar->GetItem(Toolbar::Item_AddressBar);
}
