// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "AppWindow.h"
#include "CheckFailure.h"
#include "ScenarioWebViewEventMonitor.h"
#include <WebView2.h>
#include <regex>
#include <string>
#include <strstream>

using namespace Microsoft::WRL;
using namespace std;

static constexpr wchar_t c_samplePath[] = L"ScenarioWebViewEventMonitor.html";

ScenarioWebViewEventMonitor::ScenarioWebViewEventMonitor(AppWindow* appWindowEventSource)
    : m_appWindowEventSource(appWindowEventSource),
      m_webviewEventSource(appWindowEventSource->GetWebView())
{
    m_sampleUri = m_appWindowEventSource->GetLocalUri(c_samplePath);
    m_appWindowEventView = new AppWindow(m_sampleUri, [this]() -> void {
        InitializeEventView(m_appWindowEventView->GetWebView());
    });
}

ScenarioWebViewEventMonitor::~ScenarioWebViewEventMonitor()
{
    m_webviewEventSource->remove_NavigationStarting(m_navigationStartingToken);
    m_webviewEventSource->remove_SourceChanged(m_sourceChangedToken);
    m_webviewEventSource->remove_ContentLoading(m_contentLoadingToken);
    m_webviewEventSource->remove_HistoryChanged(m_historyChangedToken);
    m_webviewEventSource->remove_NavigationCompleted(m_navigationCompletedToken);
    m_webviewEventSource->remove_DocumentTitleChanged(m_documentTitleChangedToken);
    m_webviewEventSource->remove_WebMessageReceived(m_webMessageReceivedToken);
    m_webviewEventSource->remove_NewWindowRequested(m_newWindowRequestedToken);
    EnableWebResourceRequestedEvent(false);

    m_webviewEventView->remove_WebMessageReceived(m_eventViewWebMessageReceivedToken);
}

std::wstring WebErrorStatusToString(COREWEBVIEW2_WEB_ERROR_STATUS status)
{
    switch (status)
    {
#define STATUS_ENTRY(statusValue)                                                              \
    case statusValue:                                                                          \
        return L#statusValue;

        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_UNKNOWN);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_COMMON_NAME_IS_INCORRECT);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_EXPIRED);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_CLIENT_CERTIFICATE_CONTAINS_ERRORS);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_REVOKED);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_IS_INVALID);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_SERVER_UNREACHABLE);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_TIMEOUT);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_ERROR_HTTP_INVALID_SERVER_RESPONSE);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_CONNECTION_ABORTED);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_CONNECTION_RESET);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_DISCONNECTED);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_CANNOT_CONNECT);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_HOST_NAME_NOT_RESOLVED);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_OPERATION_CANCELED);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_REDIRECT_FAILED);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_UNEXPECTED_ERROR);

#undef STATUS_ENTRY
    }

    return L"ERROR";
}

std::wstring BoolToString(BOOL value)
{
    return value ? L"true" : L"false";
}

std::wstring EncodeQuote(std::wstring raw)
{
    return L"\"" + regex_replace(raw, wregex(L"\""), L"\\\"") + L"\"";
}

//! [HttpRequestHeaderIterator]
std::wstring RequestHeadersToJsonString(ICoreWebView2HttpRequestHeaders* requestHeaders)
{
    wil::com_ptr<ICoreWebView2HttpHeadersCollectionIterator> iterator;
    CHECK_FAILURE(requestHeaders->GetIterator(&iterator));
    BOOL hasCurrent = FALSE;
    std::wstring result = L"[";

    while (SUCCEEDED(iterator->get_HasCurrentHeader(&hasCurrent)) && hasCurrent)
    {
        wil::unique_cotaskmem_string name;
        wil::unique_cotaskmem_string value;

        CHECK_FAILURE(iterator->GetCurrentHeader(&name, &value));
        result += L"{\"name\": " + EncodeQuote(name.get())
            + L", \"value\": " + EncodeQuote(value.get()) + L"}";

        BOOL hasNext = FALSE;
        CHECK_FAILURE(iterator->MoveNext(&hasNext));
        if (hasNext)
        {
            result += L", ";
        }
    }

    return result + L"]";
}
//! [HttpRequestHeaderIterator]

std::wstring RequestToJsonString(ICoreWebView2WebResourceRequest* request)
{
    wil::com_ptr<IStream> content;
    CHECK_FAILURE(request->get_Content(&content));
    wil::com_ptr<ICoreWebView2HttpRequestHeaders> headers;
    CHECK_FAILURE(request->get_Headers(&headers));
    wil::unique_cotaskmem_string method;
    CHECK_FAILURE(request->get_Method(&method));
    wil::unique_cotaskmem_string uri;
    CHECK_FAILURE(request->get_Uri(&uri));

    std::wstring result = L"{";

    result += L"\"content\": ";
    result += (content == nullptr ? L"null" : L"\"...\"");
    result += L", ";

    result += L"\"headers\": " + RequestHeadersToJsonString(headers.get()) + L", ";
    result += L"\"method\": " + EncodeQuote(method.get()) + L", ";
    result += L"\"uri\": " + EncodeQuote(uri.get()) + L" ";

    result += L"}";

    return result;
}

std::wstring WebViewPropertiesToJsonString(ICoreWebView2* webview)
{
    wil::unique_cotaskmem_string documentTitle;
    CHECK_FAILURE(webview->get_DocumentTitle(&documentTitle));
    wil::unique_cotaskmem_string source;
    CHECK_FAILURE(webview->get_Source(&source));

    std::wstring result = L", \"webview\": {"
        L"\"documentTitle\": " + EncodeQuote(documentTitle.get()) + L", "
        + L"\"source\": " + EncodeQuote(source.get()) + L" "
        + L"}";

    return result;
}

void ScenarioWebViewEventMonitor::EnableWebResourceRequestedEvent(bool enable)
{
    if (!enable && m_webResourceRequestedToken.value != 0)
    {
        m_webviewEventSource->remove_WebResourceRequested(m_webResourceRequestedToken);
        m_webResourceRequestedToken.value = 0;
    }
    else if (enable && m_webResourceRequestedToken.value == 0)
    {
        m_webviewEventSource->AddWebResourceRequestedFilter(
            L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
        m_webviewEventSource->add_WebResourceRequested(
            Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                [this](ICoreWebView2* webview, ICoreWebView2WebResourceRequestedEventArgs* args)
                    -> HRESULT {
                    wil::com_ptr<ICoreWebView2WebResourceRequest> webResourceRequest;
                    CHECK_FAILURE(args->get_Request(&webResourceRequest));
                    wil::com_ptr<ICoreWebView2WebResourceResponse> webResourceResponse;
                    CHECK_FAILURE(args->get_Response(&webResourceResponse));

                    std::wstring message = L"{ \"kind\": \"event\", \"name\": "
                                           L"\"WebResourceRequested\", \"args\": {"
                        L"\"request\": " + RequestToJsonString(webResourceRequest.get()) + L", "
                        L"\"response\": null"
                        L"}";

                    message += WebViewPropertiesToJsonString(m_webviewEventSource.get());
                    message += L"}";
                    PostEventMessage(message);

                    return S_OK;
                })
                .Get(),
            &m_webResourceRequestedToken);
    }
}

void ScenarioWebViewEventMonitor::InitializeEventView(ICoreWebView2* webviewEventView)
{
    m_webviewEventView = webviewEventView;

    m_webviewEventView->add_WebMessageReceived(
        Callback<ICoreWebView2WebMessageReceivedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args)
                -> HRESULT {
                wil::unique_cotaskmem_string source;
                CHECK_FAILURE(args->get_Source(&source));
                wil::unique_cotaskmem_string webMessageAsString;
                if (SUCCEEDED(args->TryGetWebMessageAsString(&webMessageAsString)))
                {
                    if (wcscmp(source.get(), m_sampleUri.c_str()) == 0)
                    {
                        if (wcscmp(webMessageAsString.get(), L"webResourceRequested,on") == 0)
                        {
                            EnableWebResourceRequestedEvent(true);
                        }
                        else if (
                            wcscmp(webMessageAsString.get(), L"webResourceRequested,off") == 0)
                        {
                            EnableWebResourceRequestedEvent(false);
                        }
                    }
                }

                return S_OK;
            })
            .Get(),
        &m_eventViewWebMessageReceivedToken);

    m_webviewEventSource->add_WebMessageReceived(
        Callback<ICoreWebView2WebMessageReceivedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args)
                -> HRESULT {
                wil::unique_cotaskmem_string source;
                CHECK_FAILURE(args->get_Source(&source));
                wil::unique_cotaskmem_string webMessageAsString;
                HRESULT webMessageAsStringHR =
                    args->TryGetWebMessageAsString(&webMessageAsString);
                wil::unique_cotaskmem_string webMessageAsJson;
                CHECK_FAILURE(args->get_WebMessageAsJson(&webMessageAsJson));

                std::wstring message =
                    L"{ \"kind\": \"event\", \"name\": \"WebMessageReceived\", \"args\": {"
                    L"\"source\": " + EncodeQuote(source.get()) + L", ";

                if (SUCCEEDED(webMessageAsStringHR))
                {
                    message += L"\"webMessageAsString\": " + EncodeQuote(webMessageAsString.get()) + L", ";
                }
                else
                {
                    message += L"\"webMessageAsString\": null, ";
                }

                message += L"\"webMessageAsJson\": " + EncodeQuote(webMessageAsJson.get()) + L" "
                    L"}";
                message += WebViewPropertiesToJsonString(m_webviewEventSource.get());

                message += L"}";
                PostEventMessage(message);

                return S_OK;
            })
            .Get(),
        &m_webMessageReceivedToken);

    m_webviewEventSource->add_NewWindowRequested(
        Callback<ICoreWebView2NewWindowRequestedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2NewWindowRequestedEventArgs* args)
                -> HRESULT {
                BOOL handled = FALSE;
                CHECK_FAILURE(args->get_Handled(&handled));
                BOOL isUserInitiated = FALSE;
                CHECK_FAILURE(args->get_IsUserInitiated(&isUserInitiated));
                wil::unique_cotaskmem_string uri;
                CHECK_FAILURE(args->get_Uri(&uri));

                std::wstring message =
                    L"{ \"kind\": \"event\", \"name\": \"NewWindowRequested\", \"args\": {"
                    L"\"handled\": " + BoolToString(handled) + L", "
                    L"\"isUserInitiated\": " + BoolToString(isUserInitiated) + L", "
                    L"\"uri\": " + EncodeQuote(uri.get()) + L", "
                    L"\"newWindow\": null"
                    L"}"
                    + WebViewPropertiesToJsonString(m_webviewEventSource.get())
                    + L"}";
                PostEventMessage(message);

                return S_OK;
            })
            .Get(),
        &m_newWindowRequestedToken);

    m_webviewEventSource->add_NavigationStarting(
        Callback<ICoreWebView2NavigationStartingEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args)
                -> HRESULT {
                BOOL cancel = FALSE;
                CHECK_FAILURE(args->get_Cancel(&cancel));
                BOOL isRedirected = FALSE;
                CHECK_FAILURE(args->get_IsRedirected(&isRedirected));
                BOOL isUserInitiated = FALSE;
                CHECK_FAILURE(args->get_IsUserInitiated(&isUserInitiated));
                wil::com_ptr<ICoreWebView2HttpRequestHeaders> requestHeaders;
                CHECK_FAILURE(args->get_RequestHeaders(&requestHeaders));
                wil::unique_cotaskmem_string uri;
                CHECK_FAILURE(args->get_Uri(&uri));
                UINT64 navigationId = 0;
                CHECK_FAILURE(args->get_NavigationId(&navigationId));

                std::wstring message =
                    L"{ \"kind\": \"event\", \"name\": \"NavigationStarting\", \"args\": {";

                message += L"\"navigationId\": " + std::to_wstring(navigationId) + L", ";

                message += L"\"cancel\": " + BoolToString(cancel) + L", " +
                    L"\"isRedirected\": " + BoolToString(isRedirected) + L", " +
                    L"\"isUserInitiated\": " + BoolToString(isUserInitiated) + L", " +
                    L"\"requestHeaders\": " + RequestHeadersToJsonString(requestHeaders.get()) + L", " +
                    L"\"uri\": " + EncodeQuote(uri.get()) + L" " +
                    L"}" +
                    WebViewPropertiesToJsonString(m_webviewEventSource.get()) +
                    L"}";

                PostEventMessage(message);

                return S_OK;
            })
            .Get(),
        &m_navigationStartingToken);

    m_webviewEventSource->add_FrameNavigationStarting(
        Callback<ICoreWebView2NavigationStartingEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args)
                -> HRESULT {
                BOOL cancel = FALSE;
                CHECK_FAILURE(args->get_Cancel(&cancel));
                BOOL isRedirected = FALSE;
                CHECK_FAILURE(args->get_IsRedirected(&isRedirected));
                BOOL isUserInitiated = FALSE;
                CHECK_FAILURE(args->get_IsUserInitiated(&isUserInitiated));
                wil::com_ptr<ICoreWebView2HttpRequestHeaders> requestHeaders;
                CHECK_FAILURE(args->get_RequestHeaders(&requestHeaders));
                wil::unique_cotaskmem_string uri;
                CHECK_FAILURE(args->get_Uri(&uri));

                std::wstring message =
                    L"{ \"kind\": \"event\", \"name\": "
                    L"\"FrameNavigationStarting\", \"args\": {"
                    L"\"cancel\": " + BoolToString(cancel) + L", "
                    L"\"isRedirected\": " + BoolToString(isRedirected) + L", "
                    L"\"isUserInitiated\": " + BoolToString(isUserInitiated) + L", "
                    L"\"requestHeaders\": " + RequestHeadersToJsonString(requestHeaders.get()) + L", "
                    L"\"uri\": " + EncodeQuote(uri.get()) + L" "
                    L"}" +
                    WebViewPropertiesToJsonString(m_webviewEventSource.get()) +
                    L"}";

                PostEventMessage(message);

                return S_OK;
            })
            .Get(),
        &m_frameNavigationStartingToken);

    m_webviewEventSource->add_SourceChanged(
        Callback<ICoreWebView2SourceChangedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2SourceChangedEventArgs* args)
                -> HRESULT {
                BOOL isNewDocument = FALSE;
                CHECK_FAILURE(args->get_IsNewDocument(&isNewDocument));

                std::wstring message =
                    L"{ \"kind\": \"event\", \"name\": \"SourceChanged\", \"args\": {";
                message += L"\"isNewDocument\": " + BoolToString(isNewDocument) + L"}" +
                           WebViewPropertiesToJsonString(m_webviewEventSource.get()) + L"}";
                PostEventMessage(message);

                return S_OK;
            })
            .Get(),
        &m_navigationStartingToken);

    m_webviewEventSource->add_ContentLoading(
        Callback<ICoreWebView2ContentLoadingEventHandler>(
            [this](
                ICoreWebView2* sender,
                ICoreWebView2ContentLoadingEventArgs* args) -> HRESULT {
                BOOL isErrorPage = FALSE;
                CHECK_FAILURE(args->get_IsErrorPage(&isErrorPage));
                UINT64 navigationId = 0;
                CHECK_FAILURE(args->get_NavigationId(&navigationId));

                std::wstring message =
                    L"{ \"kind\": \"event\", \"name\": \"ContentLoading\", \"args\": {";

                message += L"\"navigationId\": " + std::to_wstring(navigationId) + L", ";

                message += L"\"isErrorPage\": " + BoolToString(isErrorPage) + L"}" +
                           WebViewPropertiesToJsonString(m_webviewEventSource.get()) + L"}";
                PostEventMessage(message);

                return S_OK;
            })
            .Get(),
        &m_navigationStartingToken);

    m_webviewEventSource->add_HistoryChanged(
        Callback<ICoreWebView2HistoryChangedEventHandler>(
            [this](ICoreWebView2* sender, IUnknown* args) -> HRESULT {
                std::wstring message =
                    L"{ \"kind\": \"event\", \"name\": \"HistoryChanged\", \"args\": {";
                message +=
                    L"}" + WebViewPropertiesToJsonString(m_webviewEventSource.get()) + L"}";
                PostEventMessage(message);

                return S_OK;
            })
            .Get(),
        &m_navigationStartingToken);

    m_webviewEventSource->add_NavigationCompleted(
        Callback<ICoreWebView2NavigationCompletedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args)
                -> HRESULT {
                BOOL isSuccess = FALSE;
                CHECK_FAILURE(args->get_IsSuccess(&isSuccess));
                COREWEBVIEW2_WEB_ERROR_STATUS webErrorStatus;
                CHECK_FAILURE(args->get_WebErrorStatus(&webErrorStatus));
                UINT64 navigationId = 0;
                CHECK_FAILURE(args->get_NavigationId(&navigationId));

                std::wstring message =
                    L"{ \"kind\": \"event\", \"name\": \"NavigationCompleted\", \"args\": {";

                message += L"\"navigationId\": " + std::to_wstring(navigationId) + L", ";

                message +=
                    L"\"isSuccess\": " + BoolToString(isSuccess) + L", "
                    L"\"webErrorStatus\": " + EncodeQuote(WebErrorStatusToString(webErrorStatus)) + L" "
                    L"}" +
                    WebViewPropertiesToJsonString(m_webviewEventSource.get()) +
                    L"}";
                PostEventMessage(message);

                return S_OK;
            })
            .Get(),
        &m_navigationStartingToken);

    m_webviewEventSource->add_DocumentTitleChanged(
        Callback<ICoreWebView2DocumentTitleChangedEventHandler>(
            [this](ICoreWebView2* sender, IUnknown* args) -> HRESULT {
                std::wstring message =
                    L"{ \"kind\": \"event\", \"name\": \"DocumentTitleChanged\", \"args\": {"
                    L"}" +
                    WebViewPropertiesToJsonString(m_webviewEventSource.get()) +
                    L"}";
                PostEventMessage(message);

                return S_OK;
            })
            .Get(),
        &m_navigationStartingToken);
}

void ScenarioWebViewEventMonitor::PostEventMessage(std::wstring message)
{
    m_webviewEventView->PostWebMessageAsJson(message.c_str());
}
