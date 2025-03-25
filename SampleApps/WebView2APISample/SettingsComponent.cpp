// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "SettingsComponent.h"

#include "CheckFailure.h"
#include "ScenarioPermissionManagement.h"
#include "TextInputDialog.h"
#include <gdiplus.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <windows.h>
#include <sstream>

using namespace Microsoft::WRL;

SettingsComponent::SettingsComponent(
    AppWindow* appWindow, ICoreWebView2Environment* environment, SettingsComponent* old)
    : m_appWindow(appWindow), m_webViewEnvironment(environment),
      m_webView(appWindow->GetWebView())
{
    CHECK_FAILURE(m_webView->get_Settings(&m_settings));

    m_settings2 = m_settings.try_query<ICoreWebView2Settings2>();
    m_settings3 = m_settings.try_query<ICoreWebView2Settings3>();
    m_settings4 = m_settings.try_query<ICoreWebView2Settings4>();
    m_settings5 = m_settings.try_query<ICoreWebView2Settings5>();
    m_settings6 = m_settings.try_query<ICoreWebView2Settings6>();
    m_settings7 = m_settings.try_query<ICoreWebView2Settings7>();
    m_settings8 = m_settings.try_query<ICoreWebView2Settings8>();
    m_controller = m_appWindow->GetWebViewController();
    m_controller3 = m_controller.try_query<ICoreWebView2Controller3>();
    m_webView2_5 = m_webView.try_query<ICoreWebView2_5>();
    m_webView2_11 = m_webView.try_query<ICoreWebView2_11>();
    m_webView2_12 = m_webView.try_query<ICoreWebView2_12>();
    m_webView2_13 = m_webView.try_query<ICoreWebView2_13>();
    m_webView2_14 = m_webView.try_query<ICoreWebView2_14>();
    m_webView2_15 = m_webView.try_query<ICoreWebView2_15>();
    m_webView2_18 = m_webView.try_query<ICoreWebView2_18>();
    m_webView2_22 = m_webView.try_query<ICoreWebView2_22>();

    // Copy old settings if desired
    if (old)
    {
        BOOL setting;
        CHECK_FAILURE(old->m_settings->get_IsScriptEnabled(&setting));
        CHECK_FAILURE(m_settings->put_IsScriptEnabled(setting));
        CHECK_FAILURE(old->m_settings->get_IsWebMessageEnabled(&setting));
        CHECK_FAILURE(m_settings->put_IsWebMessageEnabled(setting));
        CHECK_FAILURE(old->m_settings->get_AreDefaultScriptDialogsEnabled(&setting));
        CHECK_FAILURE(m_settings->put_AreDefaultScriptDialogsEnabled(setting));
        CHECK_FAILURE(old->m_settings->get_IsStatusBarEnabled(&setting));
        CHECK_FAILURE(m_settings->put_IsStatusBarEnabled(setting));
        CHECK_FAILURE(old->m_settings->get_AreDevToolsEnabled(&setting));
        CHECK_FAILURE(m_settings->put_AreDevToolsEnabled(setting));
        CHECK_FAILURE(old->m_settings->get_IsBuiltInErrorPageEnabled(&setting));
        CHECK_FAILURE(m_settings->put_IsBuiltInErrorPageEnabled(setting));
        CHECK_FAILURE(old->m_settings->get_AreDefaultScriptDialogsEnabled(&setting));
        CHECK_FAILURE(m_settings->put_AreDefaultScriptDialogsEnabled(setting));
        CHECK_FAILURE(old->m_settings->get_AreDefaultContextMenusEnabled(&setting));
        CHECK_FAILURE(m_settings->put_AreDefaultContextMenusEnabled(setting));
        CHECK_FAILURE(old->m_settings->get_AreHostObjectsAllowed(&setting));
        CHECK_FAILURE(m_settings->put_AreHostObjectsAllowed(setting));
        CHECK_FAILURE(old->m_settings->get_IsZoomControlEnabled(&setting));
        CHECK_FAILURE(m_settings->put_IsZoomControlEnabled(setting));
        if (old->m_settings2 && m_settings2)
        {
            LPWSTR user_agent;
            CHECK_FAILURE(old->m_settings2->get_UserAgent(&user_agent));
            CHECK_FAILURE(m_settings2->put_UserAgent(user_agent));
        }
        if (old->m_settings3 && m_settings3)
        {
            CHECK_FAILURE(old->m_settings3->get_AreBrowserAcceleratorKeysEnabled(&setting));
            CHECK_FAILURE(m_settings3->put_AreBrowserAcceleratorKeysEnabled(setting));
        }
        if (old->m_settings4 && m_settings4)
        {
            CHECK_FAILURE(old->m_settings4->get_IsPasswordAutosaveEnabled(&setting));
            CHECK_FAILURE(m_settings4->put_IsPasswordAutosaveEnabled(setting));
            CHECK_FAILURE(old->m_settings4->get_IsGeneralAutofillEnabled(&setting));
            CHECK_FAILURE(m_settings4->put_IsGeneralAutofillEnabled(setting));
        }
        if (old->m_settings5 && m_settings5)
        {
            CHECK_FAILURE(old->m_settings5->get_IsPinchZoomEnabled(&setting));
            CHECK_FAILURE(m_settings5->put_IsPinchZoomEnabled(setting));
        }
        if (old->m_settings6 && m_settings6)
        {
            CHECK_FAILURE(old->m_settings6->get_IsSwipeNavigationEnabled(&setting));
            CHECK_FAILURE(m_settings6->put_IsSwipeNavigationEnabled(setting));
        }
        if (old->m_settings7 && m_settings7)
        {
            COREWEBVIEW2_PDF_TOOLBAR_ITEMS hiddenPdfToolbarItems;
            CHECK_FAILURE(old->m_settings7->get_HiddenPdfToolbarItems(&hiddenPdfToolbarItems));
            CHECK_FAILURE(m_settings7->put_HiddenPdfToolbarItems(hiddenPdfToolbarItems));
        }
        if (old->m_settings8 && m_settings8)
        {
            CHECK_FAILURE(old->m_settings8->get_IsReputationCheckingRequired(&setting));
            CHECK_FAILURE(m_settings8->put_IsReputationCheckingRequired(setting));
        }
        SetBlockImages(old->m_blockImages);
        SetReplaceImages(old->m_replaceImages);
        m_isScriptEnabled = old->m_isScriptEnabled;
        m_blockedSitesSet = old->m_blockedSitesSet;
        m_blockedSites = std::move(old->m_blockedSites);
        EnableCustomClientCertificateSelection();
        ToggleCustomServerCertificateSupport();
    }

    //! [NavigationStarting]
    // Register a handler for the NavigationStarting event.
    // This handler will check the domain being navigated to, and if the domain
    // matches a list of blocked sites, it will cancel the navigation and
    // possibly display a warning page.  It will also disable JavaScript on
    // selected websites.
    CHECK_FAILURE(m_webView->add_NavigationStarting(
        Callback<ICoreWebView2NavigationStartingEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args)
                -> HRESULT
            {
                wil::unique_cotaskmem_string uri;
                CHECK_FAILURE(args->get_Uri(&uri));

                if (ShouldBlockUri(uri.get()))
                {
                    CHECK_FAILURE(args->put_Cancel(true));

                    // If the user clicked a link to navigate, show a warning page.
                    BOOL userInitiated;
                    CHECK_FAILURE(args->get_IsUserInitiated(&userInitiated));
                    //! [NavigateToString]
                    static const PCWSTR htmlContent =
                        L"<h1>Domain Blocked</h1>"
                        L"<p>You've attempted to navigate to a domain in the blocked "
                        L"sites list. Press back to return to the previous page.</p>";
                    CHECK_FAILURE(sender->NavigateToString(htmlContent));
                    //! [NavigateToString]
                }
                //! [IsScriptEnabled]
                // Changes to settings will apply at the next navigation, which includes the
                // navigation after a NavigationStarting event.  We can use this to change
                // settings according to what site we're visiting.
                if (ShouldBlockScriptForUri(uri.get()))
                {
                    m_settings->put_IsScriptEnabled(FALSE);
                }
                else
                {
                    m_settings->put_IsScriptEnabled(m_isScriptEnabled);
                }
                //! [IsScriptEnabled]
                //! [UserAgent]
                if (m_settings2)
                {
                    static const PCWSTR url_compare_example = L"fourthcoffee.com";
                    wil::unique_bstr domain = GetDomainOfUri(uri.get());
                    const wchar_t* domains = domain.get();

                    if (wcscmp(url_compare_example, domains) == 0)
                    {
                        SetUserAgent(L"example_navigation_ua");
                    }
                }
                //! [UserAgent]
                // [NavigationKind]
                wil::com_ptr<ICoreWebView2NavigationStartingEventArgs3> args3;
                if (SUCCEEDED(args->QueryInterface(IID_PPV_ARGS(&args3))))
                {
                    COREWEBVIEW2_NAVIGATION_KIND kind =
                        COREWEBVIEW2_NAVIGATION_KIND_NEW_DOCUMENT;
                    CHECK_FAILURE(args3->get_NavigationKind(&kind));
                }
                // ! [NavigationKind]
                return S_OK;
            })
            .Get(),
        &m_navigationStartingToken));
    //! [NavigationStarting]

    //! [FrameNavigationStarting]
    // Register a handler for the FrameNavigationStarting event.
    // This handler will prevent a frame from navigating to a blocked domain.
    CHECK_FAILURE(m_webView->add_FrameNavigationStarting(
        Callback<ICoreWebView2NavigationStartingEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args)
                -> HRESULT
            {
                wil::unique_cotaskmem_string uri;
                CHECK_FAILURE(args->get_Uri(&uri));

                if (ShouldBlockUri(uri.get()))
                {
                    CHECK_FAILURE(args->put_Cancel(true));
                }
                return S_OK;
            })
            .Get(),
        &m_frameNavigationStartingToken));
    //! [FrameNavigationStarting]

    //! [FaviconChanged]
    // Register a handler for the FaviconUriChanged event.
    // This will provided the current favicon of the page, as well
    // as any changes that occur during the page lifetime
    if (m_webView2_15)
    {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;

        // Initialize GDI+.
        Gdiplus::GdiplusStartup(&gdiplusToken_, &gdiplusStartupInput, NULL);
        CHECK_FAILURE(m_webView2_15->add_FaviconChanged(
            Callback<ICoreWebView2FaviconChangedEventHandler>(
                [this](ICoreWebView2* sender, IUnknown* args) -> HRESULT
                {
                    if (m_faviconChanged)
                    {
                        wil::unique_cotaskmem_string url;
                        Microsoft::WRL::ComPtr<ICoreWebView2_15> webview2;
                        CHECK_FAILURE(sender->QueryInterface(IID_PPV_ARGS(&webview2)));

                        CHECK_FAILURE(webview2->get_FaviconUri(&url));
                        std::wstring strUrl(url.get());

                        webview2->GetFavicon(
                            COREWEBVIEW2_FAVICON_IMAGE_FORMAT_PNG,
                            Callback<ICoreWebView2GetFaviconCompletedHandler>(
                                [this,
                                 strUrl](HRESULT errorCode, IStream* iconStream) -> HRESULT
                                {
                                    CHECK_FAILURE(errorCode);
                                    Gdiplus::Bitmap iconBitmap(iconStream);
                                    wil::unique_hicon icon;
                                    if (iconBitmap.GetHICON(&icon) == Gdiplus::Status::Ok)
                                    {
                                        m_favicon = std::move(icon);
                                        SendMessage(
                                            m_appWindow->GetMainWindow(), WM_SETICON,
                                            ICON_SMALL, (LPARAM)m_favicon.get());
                                        m_statusBar.Show(strUrl);
                                    }
                                    else
                                    {
                                        SendMessage(
                                            m_appWindow->GetMainWindow(), WM_SETICON,
                                            ICON_SMALL, (LPARAM)IDC_NO);
                                        m_statusBar.Show(L"No Icon");
                                    }

                                    return S_OK;
                                })
                                .Get());
                    }
                    return S_OK;
                })
                .Get(),
            &m_faviconChangedToken));
    }
    //! [FaviconChanged]

    //! [ScriptDialogOpening]
    // Register a handler for the ScriptDialogOpening event.
    // This handler will set up a custom prompt dialog for the user.  Because
    // running a message loop inside of an event handler causes problems, we
    // defer the event and handle it asynchronously.
    CHECK_FAILURE(m_webView->add_ScriptDialogOpening(
        Callback<ICoreWebView2ScriptDialogOpeningEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2ScriptDialogOpeningEventArgs* args)
                -> HRESULT
            {
                AppWindow* appWindow = m_appWindow;
                wil::com_ptr<ICoreWebView2ScriptDialogOpeningEventArgs> eventArgs = args;
                wil::com_ptr<ICoreWebView2Deferral> deferral;
                CHECK_FAILURE(args->GetDeferral(&deferral));
                appWindow->RunAsync(
                    [appWindow, eventArgs, deferral]
                    {
                        wil::unique_cotaskmem_string uri;
                        COREWEBVIEW2_SCRIPT_DIALOG_KIND type;
                        wil::unique_cotaskmem_string message;
                        wil::unique_cotaskmem_string defaultText;

                        CHECK_FAILURE(eventArgs->get_Uri(&uri));
                        CHECK_FAILURE(eventArgs->get_Kind(&type));
                        CHECK_FAILURE(eventArgs->get_Message(&message));
                        CHECK_FAILURE(eventArgs->get_DefaultText(&defaultText));

                        std::wstring promptString =
                            std::wstring(L"The page at '") + uri.get() + L"' says:";
                        TextInputDialog dialog(
                            appWindow->GetMainWindow(), L"Script Dialog", promptString.c_str(),
                            message.get(), defaultText.get(),
                            /* readonly */ type != COREWEBVIEW2_SCRIPT_DIALOG_KIND_PROMPT);
                        if (dialog.confirmed)
                        {
                            CHECK_FAILURE(eventArgs->put_ResultText(dialog.input.c_str()));
                            CHECK_FAILURE(eventArgs->Accept());
                        }
                        deferral->Complete();
                    });
                return S_OK;
            })
            .Get(),
        &m_scriptDialogOpeningToken));
    //! [ScriptDialogOpening]

    //! [PermissionRequested0]
    // Register a handler for the PermissionRequested event.
    // This handler prompts the user to allow or deny the request, and remembers
    // the user's choice for later.
    CHECK_FAILURE(m_webView->add_PermissionRequested(
        Callback<ICoreWebView2PermissionRequestedEventHandler>(
            this, &SettingsComponent::OnPermissionRequested)
            .Get(),
        &m_permissionRequestedToken));
    //! [PermissionRequested0]

    if (m_webView2_12)
    {
        //! [StatusBarTextChanged]
        m_statusBar.Initialize(appWindow);
        // Registering a listener for status bar message changes
        CHECK_FAILURE(m_webView2_12->add_StatusBarTextChanged(
            Microsoft::WRL::Callback<ICoreWebView2StatusBarTextChangedEventHandler>(
                [this](ICoreWebView2* sender, IUnknown* args) -> HRESULT
                {
                    if (m_customStatusBar)
                    {
                        wil::unique_cotaskmem_string value;
                        Microsoft::WRL::ComPtr<ICoreWebView2_12> wv;
                        CHECK_FAILURE(sender->QueryInterface(IID_PPV_ARGS(&wv)));

                        CHECK_FAILURE(wv->get_StatusBarText(&value));
                        std::wstring valueString = value.get();
                        if (valueString.length() != 0)
                        {
                            m_statusBar.Show(valueString);
                        }
                        else
                        {
                            m_statusBar.Hide();
                        }
                    }

                    return S_OK;
                })
                .Get(),
            &m_statusBarTextChangedToken));
        //! [StatusBarTextChanged]
    }
}

//! [PermissionRequested1]
HRESULT SettingsComponent::OnPermissionRequested(
    ICoreWebView2* sender, ICoreWebView2PermissionRequestedEventArgs* args)
{
    // Obtain a deferral for the event so that the CoreWebView2
    // doesn't examine the properties we set on the event args until
    // after we call the Complete method asynchronously later.
    wil::com_ptr<ICoreWebView2Deferral> deferral;
    CHECK_FAILURE(args->GetDeferral(&deferral));

    // Do not save state to the profile so that the PermissionRequested event is
    // always raised and the app is in control of all permission requests. In
    // this example, the app listens to all requests and caches permission on
    // its own to decide whether to show custom UI to the user.
    wil::com_ptr<ICoreWebView2PermissionRequestedEventArgs3> extended_args;
    CHECK_FAILURE(args->QueryInterface(IID_PPV_ARGS(&extended_args)));
    CHECK_FAILURE(extended_args->put_SavesInProfile(FALSE));

    // Do the rest asynchronously, to avoid calling MessageBox in an event handler.
    m_appWindow->RunAsync(
        [this, deferral, args = wil::com_ptr<ICoreWebView2PermissionRequestedEventArgs>(args)]
        {
            wil::unique_cotaskmem_string uri;
            COREWEBVIEW2_PERMISSION_KIND kind = COREWEBVIEW2_PERMISSION_KIND_UNKNOWN_PERMISSION;
            BOOL userInitiated = FALSE;
            CHECK_FAILURE(args->get_Uri(&uri));
            CHECK_FAILURE(args->get_PermissionKind(&kind));
            CHECK_FAILURE(args->get_IsUserInitiated(&userInitiated));

            COREWEBVIEW2_PERMISSION_STATE state = COREWEBVIEW2_PERMISSION_STATE_DEFAULT;

            auto cached_key = std::make_tuple(std::wstring(uri.get()), kind, userInitiated);
            auto cached_permission = m_cached_permissions.find(cached_key);
            if (cached_permission != m_cached_permissions.end())
            {
                state =
                    (cached_permission->second ? COREWEBVIEW2_PERMISSION_STATE_ALLOW
                                               : COREWEBVIEW2_PERMISSION_STATE_DENY);
            }
            else
            {
                std::wstring message = L"An iframe has requested device permission for ";
                message += PermissionKindToString(kind);
                message += L" to the website at ";
                message += uri.get();
                message += L"?\n\n";
                message += L"Do you want to grant permission?\n";
                message +=
                    (userInitiated ? L"This request came from a user gesture."
                                   : L"This request did not come from a user gesture.");

                int response = MessageBox(
                    nullptr, message.c_str(), L"Permission Request",
                    MB_YESNOCANCEL | MB_ICONWARNING);
                switch (response)
                {
                case IDYES:
                    m_cached_permissions[cached_key] = true;
                    state = COREWEBVIEW2_PERMISSION_STATE_ALLOW;
                    break;
                case IDNO:
                    m_cached_permissions[cached_key] = false;
                    state = COREWEBVIEW2_PERMISSION_STATE_DENY;
                    break;
                default:
                    state = COREWEBVIEW2_PERMISSION_STATE_DEFAULT;
                    break;
                }
            }
            CHECK_FAILURE(args->put_State(state));
            CHECK_FAILURE(deferral->Complete());
        });
    return S_OK;
}
//! [PermissionRequested1]

bool SettingsComponent::HandleWindowMessage(
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result)
{
    if (message == WM_COMMAND)
    {
        switch (LOWORD(wParam))
        {
        case ID_BLOCKEDSITES:
        {
            ChangeBlockedSites();
            return true;
        }
        case ID_CUSTOM_DATA_PARTITION:
        {
            SetCustomDataPartitionId();
            return true;
        }
        case ID_SETTINGS_SETUSERAGENT:
        {
            CHECK_FEATURE_RETURN(m_settings2);
            ChangeUserAgent();
            return true;
        }
        case IDM_TOGGLE_JAVASCRIPT:
        {
            m_isScriptEnabled = !m_isScriptEnabled;
            m_settings->put_IsScriptEnabled(m_isScriptEnabled);
            MessageBox(
                nullptr,
                (std::wstring(L"JavaScript will be ") +
                 (m_isScriptEnabled ? L"enabled" : L"disabled") +
                 L" after the next navigation.")
                    .c_str(),
                L"Settings change", MB_OK);
            return true;
        }
        case IDM_TOGGLE_WEB_MESSAGING:
        {
            BOOL isWebMessageEnabled;
            m_settings->get_IsWebMessageEnabled(&isWebMessageEnabled);
            m_settings->put_IsWebMessageEnabled(!isWebMessageEnabled);
            MessageBox(
                nullptr,
                (std::wstring(L"Web Messaging will be ") +
                 (!isWebMessageEnabled ? L"enabled" : L"disabled") +
                 L" after the next navigation.")
                    .c_str(),
                L"Settings change", MB_OK);
            return true;
        }
        case ID_SETTINGS_STATUS_BAR_ENABLED:
        {
            BOOL isStatusBarEnabled;
            m_settings->get_IsStatusBarEnabled(&isStatusBarEnabled);
            m_settings->put_IsStatusBarEnabled(!isStatusBarEnabled);
            MessageBox(
                nullptr,
                (std::wstring(L"Status bar will be ") +
                 +(!isStatusBarEnabled ? L"enabled" : L"disabled") +
                 L" after the next navigation.")
                    .c_str(),
                L"Settings change", MB_OK);
            return true;
        }
        case ID_SETTINGS_TOGGLE_POST_STATUS_BAR_ACTIVITY:
        {
            m_customStatusBar = !m_customStatusBar;
            return true;
        }
        case ID_SETTINGS_TOGGLE_POST_FAVICON_CHANGED:
        {
            m_faviconChanged = !m_faviconChanged;
            return true;
        }
        case ID_SETTINGS_DEV_TOOLS_ENABLED:
        {
            BOOL areDevToolsEnabled = FALSE;
            m_settings->get_AreDevToolsEnabled(&areDevToolsEnabled);
            m_settings->put_AreDevToolsEnabled(!areDevToolsEnabled);
            MessageBox(
                nullptr,
                (std::wstring(L"Dev tools will be ") +
                 (!areDevToolsEnabled ? L"enabled" : L"disabled") +
                 L" after the next navigation.")
                    .c_str(),
                L"Settings change", MB_OK);
            return true;
        }
        case IDM_TOGGLE_DEFAULT_SCRIPT_DIALOGS:
        {
            BOOL currentlyEnabled;
            m_settings->get_AreDefaultScriptDialogsEnabled(&currentlyEnabled);
            m_settings->put_AreDefaultScriptDialogsEnabled(!currentlyEnabled);
            MessageBox(
                nullptr,
                (std::wstring(L"Default script dialogs will be") +
                 (!currentlyEnabled ? L"enabled" : L"disabled") +
                 L" after the next navigation.")
                    .c_str(),
                L"Settings change", MB_OK);
            return true;
        }
        case ID_SETTINGS_BLOCKALLIMAGES:
        {
            SetBlockImages(!m_blockImages);
            MessageBox(
                nullptr,
                (std::wstring(L"Image blocking has been ") +
                 (m_blockImages ? L"enabled." : L"disabled."))
                    .c_str(),
                L"Settings change", MB_OK);
            return true;
        }
        case ID_SETTINGS_REPLACEALLIMAGES:
        {
            SetReplaceImages(!m_replaceImages);
            MessageBox(
                nullptr,
                (std::wstring(L"Image replacing has been ") +
                 (m_replaceImages ? L"enabled." : L"disabled."))
                    .c_str(),
                L"Settings change", MB_OK);
            return true;
        }
        case ID_SETTINGS_CONTEXT_MENUS_ENABLED:
        {
            //! [DisableContextMenu]
            BOOL allowContextMenus;
            CHECK_FAILURE(m_settings->get_AreDefaultContextMenusEnabled(&allowContextMenus));
            if (allowContextMenus)
            {
                CHECK_FAILURE(m_settings->put_AreDefaultContextMenusEnabled(FALSE));
                MessageBox(
                    nullptr, L"Context menus will be disabled after the next navigation.",
                    L"Settings change", MB_OK);
            }
            else
            {
                CHECK_FAILURE(m_settings->put_AreDefaultContextMenusEnabled(TRUE));
                MessageBox(
                    nullptr, L"Context menus will be enabled after the next navigation.",
                    L"Settings change", MB_OK);
            }
            //! [DisableContextMenu]
            return true;
        }
        case ID_TOGGLE_CUSTOM_CONTEXT_MENU:
        {
            m_allowCustomMenus = !m_allowCustomMenus;
            //! [EnableCustomMenu]
            if (m_allowCustomMenus)
            {
                m_webView2_11->add_ContextMenuRequested(
                    Callback<ICoreWebView2ContextMenuRequestedEventHandler>(
                        [this](
                            ICoreWebView2* sender,
                            ICoreWebView2ContextMenuRequestedEventArgs* eventArgs)
                        {
                            wil::com_ptr<ICoreWebView2ContextMenuRequestedEventArgs> args =
                                eventArgs;
                            wil::com_ptr<ICoreWebView2ContextMenuItemCollection> items;
                            CHECK_FAILURE(args->get_MenuItems(&items));

                            UINT32 itemsCount;
                            CHECK_FAILURE(items->get_Count(&itemsCount));

                            wil::com_ptr<ICoreWebView2ContextMenuTarget> target;
                            CHECK_FAILURE(args->get_ContextMenuTarget(&target));

                            COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND targetKind;
                            CHECK_FAILURE(target->get_Kind(&targetKind));
                            // Custom UI Context Menu rendered if invoked on selected text
                            if (targetKind ==
                                COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND_SELECTED_TEXT)
                            {
                                auto showMenu = [this, args, itemsCount, items, target]
                                {
                                    CHECK_FAILURE(args->put_Handled(true));
                                    HMENU hPopupMenu = CreatePopupMenu();
                                    AddMenuItems(hPopupMenu, items);
                                    HWND hWnd;
                                    m_controller->get_ParentWindow(&hWnd);
                                    SetForegroundWindow(hWnd);
                                    RECT rct;
                                    GetClientRect(hWnd, &rct);
                                    POINT topLeft;
                                    topLeft.x = rct.left;
                                    topLeft.y = rct.top;
                                    ClientToScreen(hWnd, &topLeft);
                                    POINT p;
                                    CHECK_FAILURE(args->get_Location(&p));
                                    RECT bounds;
                                    CHECK_FAILURE(m_controller->get_Bounds(&bounds));
                                    double scale;
                                    m_controller3->get_RasterizationScale(&scale);
                                    INT32 selectedCommandId = TrackPopupMenu(
                                        hPopupMenu,
                                        TPM_TOPALIGN | TPM_LEFTALIGN | TPM_RETURNCMD,
                                        bounds.left + topLeft.x + ((int)(p.x * scale)),
                                        bounds.top + topLeft.y + ((int)(p.y * scale)), 0, hWnd,
                                        NULL);
                                    CHECK_FAILURE(
                                        args->put_SelectedCommandId(selectedCommandId));
                                };
                                wil::com_ptr<ICoreWebView2Deferral> deferral;
                                CHECK_FAILURE(args->GetDeferral(&deferral));
                                m_appWindow->RunAsync(
                                    [deferral, showMenu]()
                                    {
                                        showMenu();
                                        CHECK_FAILURE(deferral->Complete());
                                    });
                            }
                            // Removing the 'Save image as' context menu item for image context
                            // selections.
                            else if (targetKind == COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND_IMAGE)
                            {
                                UINT32 removeIndex = itemsCount;
                                wil::com_ptr<ICoreWebView2ContextMenuItem> current;
                                for (UINT32 i = 0; i < itemsCount; i++)
                                {
                                    CHECK_FAILURE(items->GetValueAtIndex(i, &current));
                                    wil::unique_cotaskmem_string name;
                                    CHECK_FAILURE(current->get_Name(&name));
                                    if (std::wstring(L"saveImageAs") == name.get())
                                    {
                                        removeIndex = i;
                                    }
                                }
                                if (removeIndex < itemsCount)
                                {
                                    CHECK_FAILURE(items->RemoveValueAtIndex(removeIndex));
                                }
                            }
                            // Adding a custom context menu item for the page that will display
                            // the page's URI.
                            else if (targetKind == COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND_PAGE)
                            {
                                // Custom items should be reused whenever possible.
                                if (!m_displayPageUrlContextSubMenuItem)
                                {
                                    wil::com_ptr<ICoreWebView2Environment9> webviewEnvironment;
                                    CHECK_FAILURE(
                                        m_appWindow->GetWebViewEnvironment()->QueryInterface(
                                            IID_PPV_ARGS(&webviewEnvironment)));
                                    wil::com_ptr<ICoreWebView2ContextMenuItem>
                                        displayPageUrlItem;
                                    wil::com_ptr<IStream> iconStream;
                                    CHECK_FAILURE(SHCreateStreamOnFileEx(
                                        L"small.ico", STGM_READ, FILE_ATTRIBUTE_NORMAL, FALSE,
                                        nullptr, &iconStream));
                                    CHECK_FAILURE(webviewEnvironment->CreateContextMenuItem(
                                        L"Display Page Url", iconStream.get(),
                                        COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND_COMMAND,
                                        &displayPageUrlItem));
                                    CHECK_FAILURE(displayPageUrlItem->add_CustomItemSelected(
                                        Callback<ICoreWebView2CustomItemSelectedEventHandler>(
                                            [appWindow = m_appWindow, target](
                                                ICoreWebView2ContextMenuItem* sender,
                                                IUnknown* args)
                                            {
                                                wil::unique_cotaskmem_string pageUri;
                                                CHECK_FAILURE(target->get_PageUri(&pageUri));
                                                appWindow->AsyncMessageBox(
                                                    pageUri.get(), L"Display Page Uri");
                                                return S_OK;
                                            })
                                            .Get(),
                                        nullptr));
                                    CHECK_FAILURE(webviewEnvironment->CreateContextMenuItem(
                                        L"New Submenu", nullptr,
                                        COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND_SUBMENU,
                                        &m_displayPageUrlContextSubMenuItem));
                                    wil::com_ptr<ICoreWebView2ContextMenuItemCollection>
                                        m_displayPageUrlContextSubMenuItemChildren;
                                    CHECK_FAILURE(
                                        m_displayPageUrlContextSubMenuItem->get_Children(
                                            &m_displayPageUrlContextSubMenuItemChildren));
                                    m_displayPageUrlContextSubMenuItemChildren
                                        ->InsertValueAtIndex(0, displayPageUrlItem.get());
                                }

                                CHECK_FAILURE(items->InsertValueAtIndex(
                                    itemsCount, m_displayPageUrlContextSubMenuItem.get()));
                            }
                            // If type is not an image, video, or page, the regular Edge context
                            // menu will be displayed
                            return S_OK;
                        })
                        .Get(),
                    &m_contextMenuRequestedToken);

                MessageBox(
                    nullptr, L"Custom Context menus are now enabled.", L"Settings change",
                    MB_OK);
            }
            else
            {
                m_webView2_11->remove_ContextMenuRequested(m_contextMenuRequestedToken);
                MessageBox(
                    nullptr, L"Custom Context menus are now disabled.", L"Settings change",
                    MB_OK);
            }
            //! [EnableCustomMenu]
            return true;
        }
        case ID_SETTINGS_HOST_OBJECTS_ALLOWED:
        {
            //! [HostObjectsAccess]
            BOOL allowHostObjects;
            CHECK_FAILURE(m_settings->get_AreHostObjectsAllowed(&allowHostObjects));
            if (allowHostObjects)
            {
                CHECK_FAILURE(m_settings->put_AreHostObjectsAllowed(FALSE));
                MessageBox(
                    nullptr,
                    L"Access to host objects will be denied after the next navigation.",
                    L"Settings change", MB_OK);
            }
            else
            {
                CHECK_FAILURE(m_settings->put_AreHostObjectsAllowed(TRUE));
                MessageBox(
                    nullptr,
                    L"Access to host objects will be allowed after the next navigation.",
                    L"Settings change", MB_OK);
            }
            //! [HostObjectsAccess]
            return true;
        }
        case ID_SETTINGS_ZOOM_ENABLED:
        {
            //! [DisableZoomControl]
            BOOL zoomControlEnabled;
            CHECK_FAILURE(m_settings->get_IsZoomControlEnabled(&zoomControlEnabled));
            if (zoomControlEnabled)
            {
                CHECK_FAILURE(m_settings->put_IsZoomControlEnabled(FALSE));
                MessageBox(
                    nullptr, L"Zoom control will be disabled after the next navigation.",
                    L"Settings change", MB_OK);
            }
            else
            {
                CHECK_FAILURE(m_settings->put_IsZoomControlEnabled(TRUE));
                MessageBox(
                    nullptr, L"Zoom control will be enabled after the next navigation.",
                    L"Settings change", MB_OK);
            }
            //! [DisableZoomControl]
            return true;
        }
        case ID_SETTINGS_PROFILE_DELETE:
        {
            //! [DeleteProfile]
            CHECK_FEATURE_RETURN(m_webView2_13);
            wil::com_ptr<ICoreWebView2Profile> webView2ProfileBase;
            m_webView2_13->get_Profile(&webView2ProfileBase);
            CHECK_FEATURE_RETURN(webView2ProfileBase);
            auto webView2Profile = webView2ProfileBase.try_query<ICoreWebView2Profile8>();
            CHECK_FEATURE_RETURN(webView2Profile);
            webView2Profile->Delete();
            //! [DeleteProfile]
            return true;
        }
        case ID_SETTINGS_PINCH_ZOOM_ENABLED:
        {
            //! [TogglePinchZoomEnabled]
            CHECK_FEATURE_RETURN(m_settings5);

            BOOL pinchZoomEnabled;
            CHECK_FAILURE(m_settings5->get_IsPinchZoomEnabled(&pinchZoomEnabled));
            if (pinchZoomEnabled)
            {
                CHECK_FAILURE(m_settings5->put_IsPinchZoomEnabled(FALSE));
                MessageBox(
                    nullptr, L"Pinch Zoom is disabled after the next navigation.",
                    L"Settings change", MB_OK);
            }
            else
            {
                CHECK_FAILURE(m_settings5->put_IsPinchZoomEnabled(TRUE));
                MessageBox(
                    nullptr, L"Pinch Zoom is enabled after the next navigation.",
                    L"Settings change", MB_OK);
            }
            //! [TogglePinchZoomEnabled]
            return true;
        }
        case ID_SETTINGS_BUILTIN_ERROR_PAGE_ENABLED:
        {
            //! [BuiltInErrorPageEnabled]
            BOOL enabled;
            CHECK_FAILURE(m_settings->get_IsBuiltInErrorPageEnabled(&enabled));
            if (enabled)
            {
                CHECK_FAILURE(m_settings->put_IsBuiltInErrorPageEnabled(FALSE));
                MessageBox(
                    nullptr, L"Built-in error page will be disabled for future navigation.",
                    L"Settings change", MB_OK);
            }
            else
            {
                CHECK_FAILURE(m_settings->put_IsBuiltInErrorPageEnabled(TRUE));
                MessageBox(
                    nullptr, L"Built-in error page will be enabled for future navigation.",
                    L"Settings change", MB_OK);
            }
            //! [BuiltInErrorPageEnabled]
            return true;
        }
        case ID_SETTINGS_PASSWORD_AUTOSAVE_ENABLED:
        {
            //! [PasswordAutosaveEnabled]
            CHECK_FEATURE_RETURN(m_settings4);

            BOOL enabled;
            CHECK_FAILURE(m_settings4->get_IsPasswordAutosaveEnabled(&enabled));
            if (enabled)
            {
                CHECK_FAILURE(m_settings4->put_IsPasswordAutosaveEnabled(FALSE));
                MessageBox(
                    nullptr, L"Password autosave will be disabled immediately.",
                    L"Settings change", MB_OK);
            }
            else
            {
                CHECK_FAILURE(m_settings4->put_IsPasswordAutosaveEnabled(TRUE));
                MessageBox(
                    nullptr, L"Password autosave will be enabled immediately.",
                    L"Settings change", MB_OK);
            }
            //! [PasswordAutosaveEnabled]
            return true;
        }
        case ID_SETTINGS_GENERAL_AUTOFILL_ENABLED:
        {
            //! [GeneralAutofillEnabled]
            CHECK_FEATURE_RETURN(m_settings4);

            BOOL enabled;
            CHECK_FAILURE(m_settings4->get_IsGeneralAutofillEnabled(&enabled));
            if (enabled)
            {
                CHECK_FAILURE(m_settings4->put_IsGeneralAutofillEnabled(FALSE));
                MessageBox(
                    nullptr, L"General autofill will be disabled immediately.",
                    L"Settings change", MB_OK);
            }
            else
            {
                CHECK_FAILURE(m_settings4->put_IsGeneralAutofillEnabled(TRUE));
                MessageBox(
                    nullptr, L"General autofill will be enabled immediately.",
                    L"Settings change", MB_OK);
            }
            //! [GeneralAutofillEnabled]
            return true;
        }
        case ID_TOGGLE_CLIENT_CERTIFICATE_REQUESTED:
        {
            EnableCustomClientCertificateSelection();
            MessageBox(
                nullptr,
                (std::wstring(L"Custom client certificate selection has been ") +
                 (m_ClientCertificateRequestedToken.value != 0 ? L"enabled." : L"disabled."))
                    .c_str(),
                L"Custom client certificate selection", MB_OK);
            return true;
        }
        case ID_SETTINGS_BROWSER_ACCELERATOR_KEYS_ENABLED:
        {
            //! [AreBrowserAcceleratorKeysEnabled]
            CHECK_FEATURE_RETURN(m_settings3);

            BOOL enabled;
            CHECK_FAILURE(m_settings3->get_AreBrowserAcceleratorKeysEnabled(&enabled));
            if (enabled)
            {
                CHECK_FAILURE(m_settings3->put_AreBrowserAcceleratorKeysEnabled(FALSE));
                MessageBox(
                    nullptr,
                    L"Browser-specific accelerator keys will be disabled after the next "
                    L"navigation.",
                    L"Settings change", MB_OK);
            }
            else
            {
                CHECK_FAILURE(m_settings3->put_AreBrowserAcceleratorKeysEnabled(TRUE));
                MessageBox(
                    nullptr,
                    L"Browser-specific accelerator keys will be enabled after the next "
                    L"navigation.",
                    L"Settings change", MB_OK);
            }
            //! [AreBrowserAcceleratorKeysEnabled]
            return true;
        }
        case ID_SETTINGS_SWIPE_NAVIGATION_ENABLED:
        {
            //! [ToggleSwipeNavigationEnabled]
            CHECK_FEATURE_RETURN(m_settings6);
            BOOL swipeNavigationEnabled;
            CHECK_FAILURE(m_settings6->get_IsSwipeNavigationEnabled(&swipeNavigationEnabled));
            if (swipeNavigationEnabled)
            {
                CHECK_FAILURE(m_settings6->put_IsSwipeNavigationEnabled(FALSE));
                MessageBox(
                    nullptr, L"Swipe to navigate is disabled after the next navigation.",
                    L"Settings change", MB_OK);
            }
            else
            {
                CHECK_FAILURE(m_settings6->put_IsSwipeNavigationEnabled(TRUE));
                MessageBox(
                    nullptr, L"Swipe to navigate is enabled after the next navigation.",
                    L"Settings change", MB_OK);
            }
            //! [ToggleSwipeNavigationEnabled]
            return true;
        }
        case ID_SETTINGS_TOGGLE_HIDE_PDF_TOOLBAR_ITEMS:
        {
            //! [ToggleHidePdfToolbarItems]
            CHECK_FEATURE_RETURN(m_settings7);

            COREWEBVIEW2_PDF_TOOLBAR_ITEMS hiddenPdfToolbarItems;
            CHECK_FAILURE(m_settings7->get_HiddenPdfToolbarItems(&hiddenPdfToolbarItems));
            if (hiddenPdfToolbarItems ==
                COREWEBVIEW2_PDF_TOOLBAR_ITEMS::COREWEBVIEW2_PDF_TOOLBAR_ITEMS_NONE)
            {
                CHECK_FAILURE(
                    m_settings7->put_HiddenPdfToolbarItems(
                        COREWEBVIEW2_PDF_TOOLBAR_ITEMS::COREWEBVIEW2_PDF_TOOLBAR_ITEMS_PRINT |
                        COREWEBVIEW2_PDF_TOOLBAR_ITEMS::COREWEBVIEW2_PDF_TOOLBAR_ITEMS_SAVE) |
                    COREWEBVIEW2_PDF_TOOLBAR_ITEMS::COREWEBVIEW2_PDF_TOOLBAR_ITEMS_BOOKMARKS |
                    COREWEBVIEW2_PDF_TOOLBAR_ITEMS::COREWEBVIEW2_PDF_TOOLBAR_ITEMS_FIT_PAGE |
                    COREWEBVIEW2_PDF_TOOLBAR_ITEMS::COREWEBVIEW2_PDF_TOOLBAR_ITEMS_PAGE_LAYOUT |
                    COREWEBVIEW2_PDF_TOOLBAR_ITEMS::COREWEBVIEW2_PDF_TOOLBAR_ITEMS_ROTATE |
                    COREWEBVIEW2_PDF_TOOLBAR_ITEMS::COREWEBVIEW2_PDF_TOOLBAR_ITEMS_SEARCH |
                    COREWEBVIEW2_PDF_TOOLBAR_ITEMS::COREWEBVIEW2_PDF_TOOLBAR_ITEMS_ZOOM_IN |
                    COREWEBVIEW2_PDF_TOOLBAR_ITEMS::COREWEBVIEW2_PDF_TOOLBAR_ITEMS_ZOOM_OUT |
                    COREWEBVIEW2_PDF_TOOLBAR_ITEMS::COREWEBVIEW2_PDF_TOOLBAR_ITEMS_SAVE_AS |
                    COREWEBVIEW2_PDF_TOOLBAR_ITEMS::
                        COREWEBVIEW2_PDF_TOOLBAR_ITEMS_PAGE_SELECTOR |
                    COREWEBVIEW2_PDF_TOOLBAR_ITEMS::COREWEBVIEW2_PDF_TOOLBAR_ITEMS_FULL_SCREEN |
                    COREWEBVIEW2_PDF_TOOLBAR_ITEMS::
                        COREWEBVIEW2_PDF_TOOLBAR_ITEMS_MORE_SETTINGS);
                MessageBox(
                    nullptr,
                    L"PDF toolbar print and save buttons are hidden after the next navigation.",
                    L"Settings change", MB_OK);
            }
            else
            {
                CHECK_FAILURE(m_settings7->put_HiddenPdfToolbarItems(
                    COREWEBVIEW2_PDF_TOOLBAR_ITEMS::COREWEBVIEW2_PDF_TOOLBAR_ITEMS_NONE));
                MessageBox(
                    nullptr,
                    L"PDF toolbar print and save buttons are shown after the next navigation.",
                    L"Settings change", MB_OK);
            }
            //! [ToggleHidePdfToolbarItems]
            return true;
        }
        case ID_TOGGLE_ALLOW_EXTERNAL_DROP:
        {
            //! [ToggleAllowExternalDrop]
            wil::com_ptr<ICoreWebView2Controller> controller =
                m_appWindow->GetWebViewController();
            wil::com_ptr<ICoreWebView2Controller4> controller4 =
                controller.try_query<ICoreWebView2Controller4>();
            if (controller4)
            {
                BOOL allowExternalDrop;
                CHECK_FAILURE(controller4->get_AllowExternalDrop(&allowExternalDrop));
                if (allowExternalDrop)
                {
                    CHECK_FAILURE(controller4->put_AllowExternalDrop(FALSE));
                    MessageBox(
                        nullptr, L"WebView disallows dropping files now.",
                        L"WebView AllowExternalDrop property changed", MB_OK);
                }
                else
                {
                    CHECK_FAILURE(controller4->put_AllowExternalDrop(TRUE));
                    MessageBox(
                        nullptr, L"WebView allows dropping files now.",
                        L"WebView AllowExternalDrop property changed", MB_OK);
                }
            }
            //! [ToggleAllowExternalDrop]
            return true;
        }
        case ID_SETTINGS_SMART_SCREEN_ENABLED:
        {
            //! [ToggleSmartScreen]
            CHECK_FEATURE_RETURN(m_settings8);

            BOOL is_smart_screen_enabled;
            CHECK_FAILURE(
                m_settings8->get_IsReputationCheckingRequired(&is_smart_screen_enabled));
            if (is_smart_screen_enabled)
            {
                CHECK_FAILURE(m_settings8->put_IsReputationCheckingRequired(false));
                MessageBox(
                    nullptr, L"SmartScreen is disabled after the next navigation.",
                    L"Settings change", MB_OK);
            }
            else
            {
                CHECK_FAILURE(m_settings8->put_IsReputationCheckingRequired(true));
                MessageBox(
                    nullptr, L"SmartScreen is enabled after the next navigation.",
                    L"Settings change", MB_OK);
            }
            //! [ToggleSmartScreen]
            return true;
        }
        case ID_SETTINGS_PROFILE_PASSWORD_AUTOSAVE_ENABLED:
        {
            //! [ToggleProfilePasswordAutosaveEnabled]
            // Get the profile object.
            auto webView2_13 = m_webView.try_query<ICoreWebView2_13>();
            CHECK_FEATURE_RETURN(webView2_13);
            wil::com_ptr<ICoreWebView2Profile> webView2Profile;
            CHECK_FAILURE(webView2_13->get_Profile(&webView2Profile));
            CHECK_FEATURE_RETURN(webView2Profile);
            auto webView2Profile6 = webView2Profile.try_query<ICoreWebView2Profile6>();
            CHECK_FEATURE_RETURN(webView2Profile6);

            BOOL enabled;
            CHECK_FAILURE(webView2Profile6->get_IsPasswordAutosaveEnabled(&enabled));
            // Set password-autosave property to the opposite value to current value.
            if (enabled)
            {
                CHECK_FAILURE(webView2Profile6->put_IsPasswordAutosaveEnabled(FALSE));
                MessageBox(
                    nullptr,
                    L"Password autosave will be disabled immediately in all "
                    L"WebView2 with the same profile.",
                    L"Profile settings change", MB_OK);
            }
            else
            {
                CHECK_FAILURE(webView2Profile6->put_IsPasswordAutosaveEnabled(TRUE));
                MessageBox(
                    nullptr,
                    L"Password autosave will be enabled immediately in all "
                    L"WebView2 with the same profile.",
                    L"Profile settings change", MB_OK);
            }
            //! [ToggleProfilePasswordAutosaveEnabled]
            return true;
        }
        case ID_SETTINGS_PROFILE_GENERAL_AUTOFILL_ENABLED:
        {
            //! [ToggleProfileGeneralAutofillEnabled]
            // Get the profile object.
            auto webView2_13 = m_webView.try_query<ICoreWebView2_13>();
            CHECK_FEATURE_RETURN(webView2_13);
            wil::com_ptr<ICoreWebView2Profile> webView2Profile;
            CHECK_FAILURE(webView2_13->get_Profile(&webView2Profile));
            CHECK_FEATURE_RETURN(webView2Profile);
            auto webView2Profile6 = webView2Profile.try_query<ICoreWebView2Profile6>();
            CHECK_FEATURE_RETURN(webView2Profile6);

            BOOL enabled;
            CHECK_FAILURE(webView2Profile6->get_IsGeneralAutofillEnabled(&enabled));
            // Set general-autofill property to the opposite value to current value.
            if (enabled)
            {
                CHECK_FAILURE(webView2Profile6->put_IsGeneralAutofillEnabled(FALSE));
                MessageBox(
                    nullptr,
                    L"General autofill will be disabled immediately in all WebView2 with the "
                    L"same profile.",
                    L"Profile settings change", MB_OK);
            }
            else
            {
                CHECK_FAILURE(webView2Profile6->put_IsGeneralAutofillEnabled(TRUE));
                MessageBox(
                    nullptr,
                    L"General autofill will be enabled immediately in all WebView2 with the "
                    L"same profile.",
                    L"Profile settings change", MB_OK);
            }
            //! [ToggleProfileGeneralAutofillEnabled]
            return true;
        }
        case ID_TOGGLE_LAUNCHING_EXTERNAL_URI_SCHEME_ENABLED:
        {
            //! [ToggleLaunchingExternalUriScheme]
            m_launchingExternalUriScheme = !m_launchingExternalUriScheme;
            CHECK_FEATURE_RETURN(m_webView2_18);
            if (m_launchingExternalUriScheme)
            {
                CHECK_FAILURE(m_webView2_18->add_LaunchingExternalUriScheme(
                    Callback<ICoreWebView2LaunchingExternalUriSchemeEventHandler>(
                        [this](
                            ICoreWebView2* sender,
                            ICoreWebView2LaunchingExternalUriSchemeEventArgs* args)
                        {
                            auto showDialog = [this, args]
                            {
                                wil::unique_cotaskmem_string uri;
                                CHECK_FAILURE(args->get_Uri(&uri));
                                if (wcscmp(uri.get(), L"calculator://") == 0)
                                {
                                    // Set the event args to cancel the event and launch the
                                    // calculator app. This will always allow the external
                                    // URI scheme launch.
                                    args->put_Cancel(true);
                                    std::wstring schemeUrl = L"calculator://";
                                    SHELLEXECUTEINFO info = {sizeof(info)};
                                    info.fMask = SEE_MASK_NOASYNC;
                                    info.lpVerb = L"open";
                                    info.lpFile = schemeUrl.c_str();
                                    info.nShow = SW_SHOWNORMAL;
                                    ::ShellExecuteEx(&info);
                                }
                                else if (wcscmp(uri.get(), L"malicious://") == 0)
                                {
                                    // Always block the request in this case by cancelling
                                    // the event.
                                    args->put_Cancel(true);
                                }
                                else if (wcscmp(uri.get(), L"contoso://") == 0)
                                {
                                    // To display a custom dialog we cancel the launch,
                                    // display a custom dialog, and then manually launch the
                                    // external URI scheme depending on the user's
                                    // selection.
                                    args->put_Cancel(true);
                                    wil::unique_cotaskmem_string initiatingOrigin;
                                    CHECK_FAILURE(
                                        args->get_InitiatingOrigin(&initiatingOrigin));
                                    std::wstring message =
                                        L"Launching External URI Scheme request";
                                    std::wstring initiatingOriginString =
                                        initiatingOrigin.get();
                                    if (initiatingOriginString.empty())
                                    {
                                        message += L" from ";
                                        message += initiatingOriginString;
                                    }
                                    message += L" to ";
                                    message += uri.get();
                                    message += L"?\n\n";
                                    message += L"Do you want to grant permission?\n";
                                    int response = MessageBox(
                                        nullptr, message.c_str(),
                                        L"Launching External URI Scheme",
                                        MB_YESNO | MB_ICONWARNING);
                                    if (response == IDYES)
                                    {
                                        std::wstring schemeUrl = uri.get();
                                        SHELLEXECUTEINFO info = {sizeof(info)};
                                        info.fMask = SEE_MASK_NOASYNC;
                                        info.lpVerb = L"open";
                                        info.lpFile = schemeUrl.c_str();
                                        info.nShow = SW_SHOWNORMAL;
                                        ::ShellExecuteEx(&info);
                                    }
                                }
                                else
                                {
                                    // Do not cancel the event, allowing the request to use
                                    // the default dialog.
                                }
                                return S_OK;
                            };
                            showDialog();
                            return S_OK;
                            // A deferral may be taken for the event so that the
                            // CoreWebView2 doesn't examine the properties we set on the
                            // event args until after we call the Complete method
                            // asynchronously later. This will give the user more time to
                            // decide whether to launch the external URI scheme or not.
                            // A deferral doesn't need to be taken in this case, so taking
                            // a deferral is commented out here.
                            // wil::com_ptr<ICoreWebView2Deferral> deferral;
                            // CHECK_FAILURE(args->GetDeferral(&deferral));

                            // m_appWindow->RunAsync(
                            //     [deferral, showDialog]()
                            //     {
                            //         showDialog();
                            // CHECK_FAILURE(deferral->Complete());
                            //     });
                            // return S_OK;
                        })
                        .Get(),
                    &m_launchingExternalUriSchemeToken));
            }
            else
            {
                m_webView2_18->remove_LaunchingExternalUriScheme(
                    m_launchingExternalUriSchemeToken);
            }
            MessageBox(
                nullptr,
                (std::wstring(L"Launching External URI Scheme support has been ") +
                 (m_launchingExternalUriScheme ? L"enabled." : L"disabled."))
                    .c_str(),
                L"Launching External URI Scheme", MB_OK);
            return true;
            //! [ToggleLaunchingExternalUriScheme]
        }
        case ID_TOGGLE_CUSTOM_SERVER_CERTIFICATE_SUPPORT:
        {
            ToggleCustomServerCertificateSupport();
            MessageBox(
                nullptr,
                (std::wstring(L"Custom server certificate support has been ") +
                 (m_ServerCertificateErrorToken.value != 0 ? L"enabled." : L"disabled."))
                    .c_str(),
                L"Custom server certificate support", MB_OK);
            return true;
        }
        case ID_CLEAR_SERVER_CERTIFICATE_ERROR_ACTIONS:
        {
            CHECK_FEATURE_RETURN(m_webView2_14);
            // This example clears `AlwaysAllow` response that are added for proceeding with TLS
            // certificate errors.
            CHECK_FAILURE(m_webView2_14->ClearServerCertificateErrorActions(
                Callback<ICoreWebView2ClearServerCertificateErrorActionsCompletedHandler>(
                    [this](HRESULT result) -> HRESULT
                    {
                        auto showDialog = [result]
                        {
                            MessageBox(
                                nullptr,
                                (result == S_OK)
                                    ? L"Clear server certificate error actions are succeeded."
                                    : L"Clear server certificate error actions are failed.",
                                L"Clear server certificate error actions", MB_OK);
                        };

                        m_appWindow->RunAsync([showDialog]() { showDialog(); });

                        return S_OK;
                    })
                    .Get()));
            return true;
        }
        case IDM_TRACKING_PREVENTION_LEVEL_NONE:
            SetTrackingPreventionLevel(COREWEBVIEW2_TRACKING_PREVENTION_LEVEL_NONE);
            return true;
        case IDM_TRACKING_PREVENTION_LEVEL_BASIC:
            SetTrackingPreventionLevel(COREWEBVIEW2_TRACKING_PREVENTION_LEVEL_BASIC);
            return true;
        case IDM_TRACKING_PREVENTION_LEVEL_BALANCED:
            SetTrackingPreventionLevel(COREWEBVIEW2_TRACKING_PREVENTION_LEVEL_BALANCED);
            return true;
        case IDM_TRACKING_PREVENTION_LEVEL_STRICT:
            SetTrackingPreventionLevel(COREWEBVIEW2_TRACKING_PREVENTION_LEVEL_STRICT);
            return true;
        case ID_SETTINGS_NON_CLIENT_REGION_SUPPORT_ENABLED:
        {
            //![ToggleNonClientRegionSupportEnabled]
            BOOL nonClientRegionSupportEnabled;
            wil::com_ptr<ICoreWebView2Settings9> settings;
            settings = m_settings.try_query<ICoreWebView2Settings9>();
            CHECK_FEATURE_RETURN(settings);
            CHECK_FAILURE(
                settings->get_IsNonClientRegionSupportEnabled(&nonClientRegionSupportEnabled));
            if (nonClientRegionSupportEnabled)
            {
                CHECK_FAILURE(settings->put_IsNonClientRegionSupportEnabled(FALSE));
                MessageBox(
                    nullptr,
                    L"Non-client region support will be disabled after the next navigation",
                    L"Settings change", MB_OK);
            }
            else
            {
                CHECK_FAILURE(settings->put_IsNonClientRegionSupportEnabled(TRUE));
                MessageBox(
                    nullptr,
                    L"Non-client region support will be enabled after the next navigation",
                    L"Settings change", MB_OK);
            }
            //! [ToggleNonClientRegionSupportEnabled]
            return true;
        }
        }
    }
    return false;
}

void SettingsComponent::AddMenuItems(
    HMENU hPopupMenu, wil::com_ptr<ICoreWebView2ContextMenuItemCollection> items)
{
    wil::com_ptr<ICoreWebView2ContextMenuItem> current;
    UINT32 itemsCount;
    CHECK_FAILURE(items->get_Count(&itemsCount));
    for (UINT32 i = 0; i < itemsCount; i++)
    {
        CHECK_FAILURE(items->GetValueAtIndex(i, &current));
        COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND kind;
        CHECK_FAILURE(current->get_Kind(&kind));
        wil::unique_cotaskmem_string label;
        CHECK_FAILURE(current->get_Label(&label));
        std::wstring labelString = label.get();
        wil::unique_cotaskmem_string shortcut;
        CHECK_FAILURE(current->get_ShortcutKeyDescription(&shortcut));
        std::wstring shortcutString = shortcut.get();
        if (!shortcutString.empty())
        {
            // L"\t" will right align the shortcut string
            labelString = labelString + L"\t" + shortcutString;
        }
        BOOL isEnabled;
        CHECK_FAILURE(current->get_IsEnabled(&isEnabled));
        BOOL isChecked;
        CHECK_FAILURE(current->get_IsChecked(&isChecked));
        INT32 commandId;
        CHECK_FAILURE(current->get_CommandId(&commandId));
        if (kind == COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND_SEPARATOR)
        {
            AppendMenu(hPopupMenu, MF_SEPARATOR, 0, nullptr);
        }
        else if (kind == COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND_SUBMENU)
        {
            HMENU newMenu = CreateMenu();
            wil::com_ptr<ICoreWebView2ContextMenuItemCollection> submenuItems;
            CHECK_FAILURE(current->get_Children(&submenuItems));
            AddMenuItems(newMenu, submenuItems);
            AppendMenu(hPopupMenu, MF_POPUP, (UINT_PTR)newMenu, labelString.c_str());
        }
        else if (kind == COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND_COMMAND)
        {
            if (isEnabled)
            {
                AppendMenu(
                    hPopupMenu, MF_BYPOSITION | MF_STRING, commandId, labelString.c_str());
            }
            else
            {
                AppendMenu(hPopupMenu, MF_GRAYED | MF_STRING, commandId, labelString.c_str());
            }
        }
        else if (
            kind == COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND_CHECK_BOX ||
            kind == COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND_RADIO)
        {
            if (isEnabled)
            {
                if (isChecked)
                {
                    AppendMenu(
                        hPopupMenu, MF_CHECKED | MF_STRING, commandId, labelString.c_str());
                }
                else
                {
                    AppendMenu(
                        hPopupMenu, MF_BYPOSITION | MF_STRING, commandId, labelString.c_str());
                }
            }
            else
            {
                if (isChecked)
                {
                    AppendMenu(
                        hPopupMenu, MF_CHECKED | MF_GRAYED | MF_STRING, commandId,
                        labelString.c_str());
                }
                else
                {
                    AppendMenu(
                        hPopupMenu, MF_GRAYED | MF_STRING, commandId, labelString.c_str());
                }
            }
        }
    }
}

// Prompt the user for a list of blocked domains
void SettingsComponent::ChangeBlockedSites()
{
    std::wstring blockedSitesString;
    if (m_blockedSitesSet)
    {
        for (auto& site : m_blockedSites)
        {
            if (!blockedSitesString.empty())
            {
                blockedSitesString += L";";
            }
            blockedSitesString += site;
        }
    }
    else
    {
        blockedSitesString = L"foo.com;bar.org";
    }

    TextInputDialog dialog(
        m_appWindow->GetMainWindow(), L"Blocked Sites", L"Sites:",
        L"Enter hostnames to block, separated by semicolons.", blockedSitesString.c_str());
    if (dialog.confirmed)
    {
        m_blockedSitesSet = true;
        m_blockedSites.clear();
        dialog.input.erase(
            std::remove_if(dialog.input.begin(), dialog.input.end(), isspace),
            dialog.input.end());
        size_t begin = 0;
        size_t end = 0;
        while (end != std::wstring::npos)
        {
            end = dialog.input.find(L';', begin);
            if (end != begin)
            {
                m_blockedSites.push_back(dialog.input.substr(begin, end - begin));
            }
            begin = end + 1;
        }
    }
}

// Check the URI's domain against the blocked sites list
bool SettingsComponent::ShouldBlockUri(PWSTR uri)
{
    wil::unique_bstr domain = GetDomainOfUri(uri);

    for (auto site = m_blockedSites.begin(); site != m_blockedSites.end(); site++)
    {
        if (wcscmp(site->c_str(), domain.get()) == 0)
        {
            return true;
        }
    }
    return false;
}

void SettingsComponent::SetCustomDataPartitionId()
{
    //! [CustomDataPartitionId]
    wil::com_ptr<ICoreWebView2Experimental20> webViewStaging20;
    webViewStaging20 = m_webView.try_query<ICoreWebView2Experimental20>();
    if (webViewStaging20)
    {
        wil::unique_cotaskmem_string partitionId;
        CHECK_FAILURE(webViewStaging20->get_CustomDataPartitionId(&partitionId));
        TextInputDialog dialog(
            m_appWindow->GetMainWindow(), L"Custom Datga Partition Id",
            L"Custom Datga Partition Id:",
            L"Enter data storage partition id, or leave blank to clear data storage "
            L"partition.",
            std::wstring(partitionId.get()));
        if (dialog.confirmed)
        {
            webViewStaging20->put_CustomDataPartitionId(dialog.input.c_str());
        }
    }
    //! [CustomDataPartitionId]
}

// Decide whether a website should have script disabled.  Since we're only using this
// for sample code and we don't actually want to break any websites, just return false.
bool SettingsComponent::ShouldBlockScriptForUri(PWSTR uri)
{
    return false;
}

// Turn on or off image blocking by adding or removing a WebResourceRequested handler
// which selectively intercepts requests for images.
void SettingsComponent::SetBlockImages(bool blockImages)
{
    if (blockImages != m_blockImages)
    {
        m_blockImages = blockImages;

        //! [WebResourceRequested0]
        if (m_blockImages)
        {
            CHECK_FEATURE_RETURN_EMPTY(m_webView2_22);
            m_webView2_22->AddWebResourceRequestedFilterWithRequestSourceKinds(
                L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_IMAGE,
                COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS_DOCUMENT);
            CHECK_FAILURE(m_webView->add_WebResourceRequested(
                Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                    [this](
                        ICoreWebView2* sender, ICoreWebView2WebResourceRequestedEventArgs* args)
                    {
                        COREWEBVIEW2_WEB_RESOURCE_CONTEXT resourceContext;
                        CHECK_FAILURE(args->get_ResourceContext(&resourceContext));
                        // Ensure that the type is image
                        if (resourceContext != COREWEBVIEW2_WEB_RESOURCE_CONTEXT_IMAGE)
                        {
                            return E_INVALIDARG;
                        }
                        // Override the response with an empty one to block the image.
                        // If put_Response is not called, the request will
                        // continue as normal.
                        wil::com_ptr<ICoreWebView2WebResourceResponse> response;
                        wil::com_ptr<ICoreWebView2Environment> environment;
                        wil::com_ptr<ICoreWebView2_2> webview2;
                        CHECK_FAILURE(m_webView->QueryInterface(IID_PPV_ARGS(&webview2)));
                        CHECK_FAILURE(webview2->get_Environment(&environment));
                        CHECK_FAILURE(environment->CreateWebResourceResponse(
                            nullptr, 403 /*NoContent*/, L"Blocked", L"Content-Type: image/jpeg",
                            &response));
                        CHECK_FAILURE(args->put_Response(response.get()));
                        return S_OK;
                    })
                    .Get(),
                &m_webResourceRequestedTokenForImageBlocking));
        }
        else
        {
            CHECK_FAILURE(m_webView->remove_WebResourceRequested(
                m_webResourceRequestedTokenForImageBlocking));
        }
        //! [WebResourceRequested0]
    }
}

// Turn on or off image replacing by adding or removing a WebResourceRequested handler
// which selectively intercepts requests for images. It will replace all images with another
// image.
void SettingsComponent::SetReplaceImages(bool replaceImages)
{
    if (replaceImages != m_replaceImages)
    {
        m_replaceImages = replaceImages;
        //! [WebResourceRequested1]
        if (m_replaceImages)
        {
            CHECK_FEATURE_RETURN_EMPTY(m_webView2_22);
            m_webView2_22->AddWebResourceRequestedFilterWithRequestSourceKinds(
                L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_IMAGE,
                COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS_DOCUMENT);
            CHECK_FAILURE(m_webView->add_WebResourceRequested(
                Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                    [this](
                        ICoreWebView2* sender, ICoreWebView2WebResourceRequestedEventArgs* args)
                    {
                        COREWEBVIEW2_WEB_RESOURCE_CONTEXT resourceContext;
                        CHECK_FAILURE(args->get_ResourceContext(&resourceContext));
                        // Ensure that the type is image
                        if (resourceContext != COREWEBVIEW2_WEB_RESOURCE_CONTEXT_IMAGE)
                        {
                            return E_INVALIDARG;
                        }
                        // Override the response with an another image.
                        // If put_Response is not called, the request will
                        // continue as normal.
                        // It's not required for this scenario, but generally you should examine
                        // relevant HTTP request headers just like an HTTP server would do when
                        // producing a response stream.
                        wil::com_ptr<IStream> stream;
                        CHECK_FAILURE(SHCreateStreamOnFileEx(
                            L"assets/EdgeWebView2-80.jpg", STGM_READ, FILE_ATTRIBUTE_NORMAL,
                            FALSE, nullptr, &stream));
                        wil::com_ptr<ICoreWebView2WebResourceResponse> response;
                        wil::com_ptr<ICoreWebView2Environment> environment;
                        wil::com_ptr<ICoreWebView2_2> webview2;
                        CHECK_FAILURE(m_webView->QueryInterface(IID_PPV_ARGS(&webview2)));
                        CHECK_FAILURE(webview2->get_Environment(&environment));
                        CHECK_FAILURE(environment->CreateWebResourceResponse(
                            stream.get(), 200, L"OK", L"Content-Type: image/jpeg", &response));
                        CHECK_FAILURE(args->put_Response(response.get()));
                        return S_OK;
                    })
                    .Get(),
                &m_webResourceRequestedTokenForImageReplacing));
        }
        else
        {
            CHECK_FAILURE(m_webView->remove_WebResourceRequested(
                m_webResourceRequestedTokenForImageReplacing));
        }
        //! [WebResourceRequested1]
    }
}

// Prompt the user for a new User Agent string
void SettingsComponent::ChangeUserAgent()
{
    if (m_settings2)
    {
        LPWSTR user_agent;
        CHECK_FAILURE(m_settings2->get_UserAgent(&user_agent));
        TextInputDialog dialog(
            m_appWindow->GetMainWindow(), L"User Agent", L"User agent:",
            L"Enter user agent, or leave blank to restore default.",
            m_changeUserAgent ? m_overridingUserAgent.c_str() : user_agent);
        if (dialog.confirmed)
        {
            SetUserAgent(dialog.input);
        }
    }
}

// Register a WebResourceRequested handler which adds a custom User-Agent
// HTTP header to all requests.
void SettingsComponent::SetUserAgent(const std::wstring& userAgent)
{
    if (m_settings2)
    {
        m_overridingUserAgent = userAgent;
        if (m_overridingUserAgent.empty())
        {
            m_changeUserAgent = false;
        }
        else
        {
            m_changeUserAgent = true;
            CHECK_FAILURE(m_settings2->put_UserAgent(m_overridingUserAgent.c_str()));
        }
    }
}

// Turn off client certificate selection dialog using ClientCertificateRequested event handler
// that disables the dialog. This example hides the default client certificate dialog and
// always chooses the last certificate without prompting the user.
//! [ClientCertificateRequested1]
void SettingsComponent::EnableCustomClientCertificateSelection()
{
    if (m_webView2_5)
    {
        if (m_ClientCertificateRequestedToken.value == 0)
        {
            CHECK_FAILURE(m_webView2_5->add_ClientCertificateRequested(
                Callback<ICoreWebView2ClientCertificateRequestedEventHandler>(
                    [this](
                        ICoreWebView2* sender,
                        ICoreWebView2ClientCertificateRequestedEventArgs* args)
                    {
                        wil::com_ptr<ICoreWebView2ClientCertificateCollection>
                            certificateCollection;
                        CHECK_FAILURE(
                            args->get_MutuallyTrustedCertificates(&certificateCollection));

                        UINT certificateCollectionCount = 0;
                        CHECK_FAILURE(
                            certificateCollection->get_Count(&certificateCollectionCount));
                        wil::com_ptr<ICoreWebView2ClientCertificate> certificate = nullptr;

                        if (certificateCollectionCount > 0)
                        {
                            // There is no significance to the order, picking a certificate
                            // arbitrarily.
                            CHECK_FAILURE(certificateCollection->GetValueAtIndex(
                                certificateCollectionCount - 1, &certificate));
                            // Continue with the selected certificate to respond to the server.
                            CHECK_FAILURE(args->put_SelectedCertificate(certificate.get()));
                            CHECK_FAILURE(args->put_Handled(TRUE));
                        }
                        else
                        {
                            // Continue without a certificate to respond to the server if
                            // certificate collection is empty.
                            CHECK_FAILURE(args->put_Handled(TRUE));
                        }
                        return S_OK;
                    })
                    .Get(),
                &m_ClientCertificateRequestedToken));
        }
        else
        {
            CHECK_FAILURE(m_webView2_5->remove_ClientCertificateRequested(
                m_ClientCertificateRequestedToken));
            m_ClientCertificateRequestedToken.value = 0;
        }
    }
}
//! [ClientCertificateRequested1]

// Function to validate the server certificate for untrusted root or self-signed certificate.
// You may also choose to defer server certificate validation.
static bool ValidateServerCertificate(ICoreWebView2Certificate* certificate)
{
    // You may want to validate certificates in different ways depending on your app and
    // scenario. One way might be the following:
    // First, get the list of host app trusted certificates and its thumbprint.
    //
    // Then get the last chain element using
    // `ICoreWebView2Certificate::get_PemEncodedIssuerCertificateChain` that contains the raw
    // data of the untrusted root CA/self-signed certificate. Get the untrusted root CA/self
    // signed certificate thumbprint from the raw certificate data and validate the thumbprint
    // against the host app trusted certificate list.
    //
    // Finally, return true if it exists in the host app's certificate trusted list, or
    // otherwise return false.
    return true;
}

//! [ServerCertificateErrorDetected1]
// When WebView2 doesn't trust a TLS certificate but host app does, this example bypasses
// the default TLS interstitial page using the ServerCertificateErrorDetected event handler and
// continues the request to a server. Otherwise, cancel the request.
void SettingsComponent::ToggleCustomServerCertificateSupport()
{
    if (m_webView2_14)
    {
        if (m_ServerCertificateErrorToken.value == 0)
        {
            CHECK_FAILURE(m_webView2_14->add_ServerCertificateErrorDetected(
                Callback<ICoreWebView2ServerCertificateErrorDetectedEventHandler>(
                    [this](
                        ICoreWebView2* sender,
                        ICoreWebView2ServerCertificateErrorDetectedEventArgs* args)
                    {
                        COREWEBVIEW2_WEB_ERROR_STATUS errorStatus;
                        CHECK_FAILURE(args->get_ErrorStatus(&errorStatus));

                        wil::com_ptr<ICoreWebView2Certificate> certificate = nullptr;
                        CHECK_FAILURE(args->get_ServerCertificate(&certificate));

                        // Continues the request to a server with a TLS certificate if the error
                        // status is of type
                        // `COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_IS_INVALID` and trusted by
                        // the host app.
                        if (errorStatus ==
                                COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_IS_INVALID &&
                            ValidateServerCertificate(certificate.get()))
                        {
                            CHECK_FAILURE(args->put_Action(
                                COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION_ALWAYS_ALLOW));
                        }
                        else
                        {
                            // Cancel the request for other TLS certificate error types or if
                            // untrusted by the host app.
                            CHECK_FAILURE(args->put_Action(
                                COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION_CANCEL));
                        }
                        return S_OK;
                    })
                    .Get(),
                &m_ServerCertificateErrorToken));
        }
        else
        {
            CHECK_FAILURE(m_webView2_14->remove_ServerCertificateErrorDetected(
                m_ServerCertificateErrorToken));
            m_ServerCertificateErrorToken.value = 0;
        }
    }
    else
    {
        FeatureNotAvailable();
    }
}
//! [ServerCertificateErrorDetected1]

//! [SetTrackingPreventionLevel]
void SettingsComponent::SetTrackingPreventionLevel(COREWEBVIEW2_TRACKING_PREVENTION_LEVEL value)
{
    wil::com_ptr<ICoreWebView2_13> webView2_13;
    webView2_13 = m_webView.try_query<ICoreWebView2_13>();

    if (webView2_13)
    {
        wil::com_ptr<ICoreWebView2Profile> profile;
        CHECK_FAILURE(webView2_13->get_Profile(&profile));

        auto profile3 = profile.try_query<ICoreWebView2Profile3>();
        if (profile3)
        {
            CHECK_FAILURE(profile3->put_PreferredTrackingPreventionLevel(value));
            MessageBox(
                nullptr, L"Tracking prevention level is set successfully",
                L"Tracking Prevention Level", MB_OK);
        }
    }
}
//! [SetTrackingPreventionLevel]

SettingsComponent::~SettingsComponent()
{
    m_webView->remove_NavigationStarting(m_navigationStartingToken);
    m_webView->remove_FrameNavigationStarting(m_frameNavigationStartingToken);
    m_webView->remove_WebResourceRequested(m_webResourceRequestedTokenForImageBlocking);
    m_webView->remove_ScriptDialogOpening(m_scriptDialogOpeningToken);
    m_webView->remove_PermissionRequested(m_permissionRequestedToken);
}
// Take advantage of urlmon's URI library to parse a URI
wil::unique_bstr GetDomainOfUri(PWSTR uri)
{
    wil::com_ptr<IUri> uriObject;
    HRESULT hr = CreateUri(
        uri, Uri_CREATE_CANONICALIZE | Uri_CREATE_NO_DECODE_EXTRA_INFO, 0, &uriObject);
    if (FAILED(hr))
    {
        return nullptr;
    }
    wil::unique_bstr domain;
    uriObject->GetHost(&domain);
    return domain;
}
