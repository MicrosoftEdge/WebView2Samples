// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "SettingsComponent.h"

#include "CheckFailure.h"
#include "TextInputDialog.h"
#include <shlwapi.h>

using namespace Microsoft::WRL;

// Some utility functions
static wil::unique_bstr GetDomainOfUri(PWSTR uri);
static PCWSTR NameOfPermissionKind(COREWEBVIEW2_PERMISSION_KIND kind);

SettingsComponent::SettingsComponent(
    AppWindow* appWindow, ICoreWebView2Environment* environment, SettingsComponent* old)
    : m_appWindow(appWindow), m_webViewEnvironment(environment),
      m_webView(appWindow->GetWebView())
{
    CHECK_FAILURE(m_webView->get_Settings(&m_settings));

    m_settings2 = m_settings.try_query<ICoreWebView2Settings2>();
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
        if (old->m_settings2 && m_settings2)
        {
            LPWSTR user_agent;
            CHECK_FAILURE(old->m_settings2->get_UserAgent(&user_agent));
            CHECK_FAILURE(m_settings2->put_UserAgent(user_agent));
        }
        SetBlockImages(old->m_blockImages);
        SetReplaceImages(old->m_replaceImages);
        m_deferScriptDialogs = old->m_deferScriptDialogs;
        m_isScriptEnabled = old->m_isScriptEnabled;
        m_blockedSitesSet = old->m_blockedSitesSet;
        m_blockedSites = std::move(old->m_blockedSites);
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
                -> HRESULT {
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
                -> HRESULT {
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

    //! [ScriptDialogOpening]
    // Register a handler for the ScriptDialogOpening event.
    // This handler will set up a custom prompt dialog for the user,
    // and may defer the event if the setting to defer dialogs is enabled.
    CHECK_FAILURE(m_webView->add_ScriptDialogOpening(
        Callback<ICoreWebView2ScriptDialogOpeningEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2ScriptDialogOpeningEventArgs* args)
                -> HRESULT {
                wil::com_ptr<ICoreWebView2ScriptDialogOpeningEventArgs> eventArgs = args;
                auto showDialog = [this, eventArgs] {
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
                        m_appWindow->GetMainWindow(), L"Script Dialog", promptString.c_str(),
                        message.get(), defaultText.get(),
                        /* readonly */ type != COREWEBVIEW2_SCRIPT_DIALOG_KIND_PROMPT);
                    if (dialog.confirmed)
                    {
                        CHECK_FAILURE(eventArgs->put_ResultText(dialog.input.c_str()));
                        CHECK_FAILURE(eventArgs->Accept());
                    }
                };

                if (m_deferScriptDialogs)
                {
                    wil::com_ptr<ICoreWebView2Deferral> deferral;
                    CHECK_FAILURE(args->GetDeferral(&deferral));
                    m_completeDeferredDialog = [showDialog, deferral] {
                        showDialog();
                        CHECK_FAILURE(deferral->Complete());
                    };
                }
                else
                {
                    showDialog();
                }

                return S_OK;
            })
            .Get(),
        &m_scriptDialogOpeningToken));
    //! [ScriptDialogOpening]

    //! [PermissionRequested]
    // Register a handler for the PermissionRequested event.
    // This handler prompts the user to allow or deny the request.
    CHECK_FAILURE(m_webView->add_PermissionRequested(
        Callback<ICoreWebView2PermissionRequestedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2PermissionRequestedEventArgs* args)
                -> HRESULT {
                wil::unique_cotaskmem_string uri;
                COREWEBVIEW2_PERMISSION_KIND kind =
                    COREWEBVIEW2_PERMISSION_KIND_UNKNOWN_PERMISSION;
                BOOL userInitiated = FALSE;

                CHECK_FAILURE(args->get_Uri(&uri));
                CHECK_FAILURE(args->get_PermissionKind(&kind));
                CHECK_FAILURE(args->get_IsUserInitiated(&userInitiated));

                std::wstring message = L"Do you want to grant permission for ";
                message += NameOfPermissionKind(kind);
                message += L" to the website at ";
                message += uri.get();
                message += L"?\n\n";
                message +=
                    (userInitiated ? L"This request came from a user gesture."
                                   : L"This request did not come from a user gesture.");

                int response = MessageBox(
                    nullptr, message.c_str(), L"Permission Request",
                    MB_YESNOCANCEL | MB_ICONWARNING);

                COREWEBVIEW2_PERMISSION_STATE state =
                    response == IDYES
                        ? COREWEBVIEW2_PERMISSION_STATE_ALLOW
                        : response == IDNO ? COREWEBVIEW2_PERMISSION_STATE_DENY
                                           : COREWEBVIEW2_PERMISSION_STATE_DEFAULT;
                CHECK_FAILURE(args->put_State(state));

                return S_OK;
            })
            .Get(),
        &m_permissionRequestedToken));
    //! [PermissionRequested]
}

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
        case IDM_USE_DEFAULT_SCRIPT_DIALOGS:
        {
            BOOL defaultCurrentlyEnabled;
            m_settings->get_AreDefaultScriptDialogsEnabled(&defaultCurrentlyEnabled);
            if (!defaultCurrentlyEnabled)
            {
                m_settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                MessageBox(
                    nullptr, L"Default script dialogs will be used after the next navigation.",
                    L"Settings change", MB_OK);
            }
            return true;
        }
        case IDM_USE_CUSTOM_SCRIPT_DIALOGS:
        {
            BOOL defaultCurrentlyEnabled;
            m_settings->get_AreDefaultScriptDialogsEnabled(&defaultCurrentlyEnabled);
            if (defaultCurrentlyEnabled)
            {
                m_settings->put_AreDefaultScriptDialogsEnabled(FALSE);
                m_deferScriptDialogs = false;
                MessageBox(
                    nullptr,
                    L"Custom script dialogs without deferral will be used after the next "
                    L"navigation.",
                    L"Settings change", MB_OK);
            }
            else if (m_deferScriptDialogs)
            {
                m_deferScriptDialogs = false;
                MessageBox(
                    nullptr, L"Custom script dialogs without deferral will be used now.",
                    L"Settings change", MB_OK);
            }
            return true;
        }
        case IDM_USE_DEFERRED_SCRIPT_DIALOGS:
        {
            BOOL defaultCurrentlyEnabled;
            m_settings->get_AreDefaultScriptDialogsEnabled(&defaultCurrentlyEnabled);
            if (defaultCurrentlyEnabled)
            {
                m_settings->put_AreDefaultScriptDialogsEnabled(FALSE);
                m_deferScriptDialogs = true;
                MessageBox(
                    nullptr,
                    L"Custom script dialogs with deferral will be used after the next "
                    L"navigation.",
                    L"Settings change", MB_OK);
            }
            else if (!m_deferScriptDialogs)
            {
                m_deferScriptDialogs = true;
                MessageBox(
                    nullptr, L"Custom script dialogs with deferral will be used now.",
                    L"Settings change", MB_OK);
            }
            return true;
        }
        case IDM_COMPLETE_JAVASCRIPT_DIALOG:
        {
            CompleteScriptDialogDeferral();
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
        }
    }
    return false;
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
            m_webView->AddWebResourceRequestedFilter(
                L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_IMAGE);
            CHECK_FAILURE(m_webView->add_WebResourceRequested(
                Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                    [this](
                        ICoreWebView2* sender,
                        ICoreWebView2WebResourceRequestedEventArgs* args) {
                        COREWEBVIEW2_WEB_RESOURCE_CONTEXT resourceContext;
                        CHECK_FAILURE(args->get_ResourceContext(&resourceContext));
                        // Ensure that the type is image
                        if (resourceContext != COREWEBVIEW2_WEB_RESOURCE_CONTEXT_IMAGE)
                        {
                            return E_INVALIDARG;
                        }
                        // Override the response with an empty one to block the image.
                        // If put_Response is not called, the request will continue as normal.
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
            m_webView->AddWebResourceRequestedFilter(
                L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_IMAGE);
            CHECK_FAILURE(m_webView->add_WebResourceRequested(
                Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                    [this](
                        ICoreWebView2* sender,
                        ICoreWebView2WebResourceRequestedEventArgs* args) {
                        COREWEBVIEW2_WEB_RESOURCE_CONTEXT resourceContext;
                        CHECK_FAILURE(args->get_ResourceContext(&resourceContext));
                        // Ensure that the type is image
                        if (resourceContext != COREWEBVIEW2_WEB_RESOURCE_CONTEXT_IMAGE)
                        {
                            return E_INVALIDARG;
                        }
                        // Override the response with an another image.
                        // If put_Response is not called, the request will continue as normal.
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

void SettingsComponent::CompleteScriptDialogDeferral()
{
    if (m_completeDeferredDialog)
    {
        m_completeDeferredDialog();
        m_completeDeferredDialog = nullptr;
    }
}

SettingsComponent::~SettingsComponent()
{
    m_webView->remove_NavigationStarting(m_navigationStartingToken);
    m_webView->remove_FrameNavigationStarting(m_frameNavigationStartingToken);
    m_webView->remove_WebResourceRequested(m_webResourceRequestedTokenForImageBlocking);
    m_webView->remove_ScriptDialogOpening(m_scriptDialogOpeningToken);
    m_webView->remove_PermissionRequested(m_permissionRequestedToken);
}

// Take advantage of urlmon's URI library to parse a URI
static wil::unique_bstr GetDomainOfUri(PWSTR uri)
{
    wil::com_ptr<IUri> uriObject;
    CreateUri(uri, Uri_CREATE_CANONICALIZE | Uri_CREATE_NO_DECODE_EXTRA_INFO, 0, &uriObject);
    wil::unique_bstr domain;
    uriObject->GetHost(&domain);
    return domain;
}

static PCWSTR NameOfPermissionKind(COREWEBVIEW2_PERMISSION_KIND kind)
{
    switch (kind)
    {
    case COREWEBVIEW2_PERMISSION_KIND_MICROPHONE:
        return L"Microphone";
    case COREWEBVIEW2_PERMISSION_KIND_CAMERA:
        return L"Camera";
    case COREWEBVIEW2_PERMISSION_KIND_GEOLOCATION:
        return L"Geolocation";
    case COREWEBVIEW2_PERMISSION_KIND_NOTIFICATIONS:
        return L"Notifications";
    case COREWEBVIEW2_PERMISSION_KIND_OTHER_SENSORS:
        return L"Generic Sensors";
    case COREWEBVIEW2_PERMISSION_KIND_CLIPBOARD_READ:
        return L"Clipboard Read";
    default:
        return L"Unknown resources";
    }
}
