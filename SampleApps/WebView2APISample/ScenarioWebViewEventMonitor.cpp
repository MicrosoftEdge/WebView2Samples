// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING 1

#include "AppWindow.h"
#include "CheckFailure.h"
#include "ScenarioPermissionManagement.h"
#include "ScenarioWebViewEventMonitor.h"
#include <WebView2.h>
#include <codecvt>
#include <locale>
#include <regex>
#include <string>

using namespace Microsoft::WRL;
using namespace std;

static constexpr wchar_t c_samplePath[] = L"ScenarioWebViewEventMonitor.html";
const int first_level_iframe_depth = 1;

std::wstring WebResourceSourceToString(COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS source)
{
    switch (source)
    {
    case COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS_DOCUMENT:
        return L"\"main\"";
    case COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS_SHARED_WORKER:
        return L"\"shared_worker\"";
    case COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS_SERVICE_WORKER:
        return L"\"service_worker\"";
    default:
        return L"\"unknown_source\"";
    }
}

ScenarioWebViewEventMonitor::ScenarioWebViewEventMonitor(AppWindow* appWindowEventSource)
    : m_appWindowEventSource(appWindowEventSource),
      m_webviewEventSource(appWindowEventSource->GetWebView()),
      m_controllerEventSource(appWindowEventSource->GetWebViewController())
{
    m_sampleUri = m_appWindowEventSource->GetLocalUri(c_samplePath);
    m_appWindowEventView = new AppWindow(
        IDM_CREATION_MODE_WINDOWED, appWindowEventSource->GetWebViewOption(),
        m_sampleUri, appWindowEventSource->GetUserDataFolder(),
        false, [this]() -> void { InitializeEventView(m_appWindowEventView->GetWebView()); });
    // Delete this component when the event monitor window closes.
    // Since this is a component of the event source window, it will automatically
    // be deleted when the event source webview is closed.
    m_appWindowEventView->SetOnAppWindowClosing([&]{
        m_appWindowEventSource->DeleteComponent(this);
    });
    m_webviewEventSource2 = m_webviewEventSource.query<ICoreWebView2_2>();
}

ScenarioWebViewEventMonitor::~ScenarioWebViewEventMonitor()
{
    m_webviewEventSource->remove_NavigationStarting(m_navigationStartingToken);
    m_webviewEventSource->remove_FrameNavigationStarting(m_frameNavigationStartingToken);
    m_webviewEventSource->remove_SourceChanged(m_sourceChangedToken);
    m_webviewEventSource->remove_ContentLoading(m_contentLoadingToken);
    m_webviewEventSource->remove_HistoryChanged(m_historyChangedToken);
    m_webviewEventSource->remove_FrameNavigationCompleted(m_frameNavigationCompletedToken);
    m_webviewEventSource->remove_NavigationCompleted(m_navigationCompletedToken);
    m_webviewEventSource->remove_DocumentTitleChanged(m_documentTitleChangedToken);
    m_webviewEventSource->remove_WebMessageReceived(m_webMessageReceivedToken);
    m_webviewEventSource->remove_NewWindowRequested(m_newWindowRequestedToken);
    m_webviewEventSource2->remove_DOMContentLoaded(m_DOMContentLoadedToken);
    if (m_webviewEventSource4)
    {
        m_webviewEventSource4->remove_DownloadStarting(m_downloadStartingToken);
        m_webviewEventSource4->remove_FrameCreated(m_frameCreatedToken);
    }
    m_controllerEventSource->remove_GotFocus(m_gotFocusToken);
    m_controllerEventSource->remove_LostFocus(m_lostFocusToken);
    EnableWebResourceRequestedEvent(false);
    EnableWebResourceResponseReceivedEvent(false);

    m_webviewEventView->remove_WebMessageReceived(m_eventViewWebMessageReceivedToken);
    if (m_webViewEventSource9) {
        m_webViewEventSource9->remove_IsDefaultDownloadDialogOpenChanged(
            m_isDefaultDownloadDialogOpenChangedToken);
    }

    // Clear our app window's reference to this.
    m_appWindowEventView->SetOnAppWindowClosing(nullptr);
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
    std::wstring encoded;
    // Allocate 10 more chars to reduce memory re-allocation
    // due to adding potential escaping chars.
    encoded.reserve(raw.length() + 10);
    encoded.push_back(L'"');
    for (size_t i = 0; i < raw.length(); ++i)
    {
        // Escape chars as listed in https://tc39.es/ecma262/#sec-json.stringify.
        switch (raw[i])
        {
        case '\b':
            encoded.append(L"\\b");
            break;
        case '\f':
            encoded.append(L"\\f");
            break;
        case '\n':
            encoded.append(L"\\n");
            break;
        case '\r':
            encoded.append(L"\\r");
            break;
        case '\t':
            encoded.append(L"\\t");
            break;
        case '\\':
            encoded.append(L"\\\\");
            break;
        case '"':
            encoded.append(L"\\\"");
            break;
        default:
            encoded.push_back(raw[i]);
        }
    }
    encoded.push_back(L'"');
    return encoded;
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

std::wstring ResponseHeadersToJsonString(ICoreWebView2HttpResponseHeaders* responseHeaders)
{
    wil::com_ptr<ICoreWebView2HttpHeadersCollectionIterator> iterator;
    CHECK_FAILURE(responseHeaders->GetIterator(&iterator));
    BOOL hasCurrent = FALSE;
    std::wstring result = L"[";

    while (SUCCEEDED(iterator->get_HasCurrentHeader(&hasCurrent)) && hasCurrent)
    {
        wil::unique_cotaskmem_string name;
        wil::unique_cotaskmem_string value;

        CHECK_FAILURE(iterator->GetCurrentHeader(&name, &value));
        result += EncodeQuote(std::wstring(name.get()) + L": " + value.get());

        BOOL hasNext = FALSE;
        CHECK_FAILURE(iterator->MoveNext(&hasNext));
        if (hasNext)
        {
            result += L", ";
        }
    }

    return result + L"]";
}

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

std::wstring GetPreviewOfContent(IStream* content, bool& readAll)
{
    char buffer[50];
    unsigned long read;
    content->Read(buffer, 50U, &read);
    readAll = read < 50;

    WCHAR converted[50];
    CHECK_FAILURE(MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, buffer, 50, converted, 50));
    return std::wstring(converted);
}

std::wstring ResponseToJsonString(
    ICoreWebView2WebResourceResponseView* response, IStream* content)
{
    wil::com_ptr<ICoreWebView2HttpResponseHeaders> headers;
    CHECK_FAILURE(response->get_Headers(&headers));
    int statusCode;
    CHECK_FAILURE(response->get_StatusCode(&statusCode));
    wil::unique_cotaskmem_string reasonPhrase;
    CHECK_FAILURE(response->get_ReasonPhrase(&reasonPhrase));
    BOOL containsContentType = FALSE;
    headers->Contains(L"Content-Type", &containsContentType);
    wil::unique_cotaskmem_string contentType;
    bool isBinaryContent = true;
    if (containsContentType)
    {
        headers->GetHeader(L"Content-Type", &contentType);
        if (wcsncmp(L"text/", contentType.get(), ARRAYSIZE(L"text/")) == 0)
        {
            isBinaryContent = false;
        }
    }
    std::wstring result = L"{";

    result += L"\"content\": ";
    if (!content)
    {
        result += L"null";
    }
    else
    {
        if (isBinaryContent)
        {
            result += EncodeQuote(L"BINARY_DATA");
        }
        else
        {
            bool readAll = false;
            result += EncodeQuote(GetPreviewOfContent(content, readAll));
            if (!readAll)
            {
              result += L"...";
            }
        }
    }
    result += L", ";

    result += L"\"headers\": " + ResponseHeadersToJsonString(headers.get()) + L", ";
    result += L"\"status\": ";
    WCHAR statusCodeString[4];
    _itow_s(statusCode, statusCodeString, 4, 10);
    result += statusCodeString;
    result += L", ";
    result += L"\"reason\": " + EncodeQuote(reasonPhrase.get()) + L" ";

    result += L"}";

    return result;
}

std::wstring WebViewPropertiesToJsonString(ICoreWebView2* webview)
{
    wil::unique_cotaskmem_string documentTitle;
    CHECK_FAILURE(webview->get_DocumentTitle(&documentTitle));
    wil::unique_cotaskmem_string source;
    CHECK_FAILURE(webview->get_Source(&source));
    BOOL canGoBack = FALSE;
    CHECK_FAILURE(webview->get_CanGoBack(&canGoBack));
    BOOL canGoForward = FALSE;
    CHECK_FAILURE(webview->get_CanGoForward(&canGoForward));

    std::wstring result = L", \"webview\": {"
        L"\"documentTitle\": " + EncodeQuote(documentTitle.get()) + L", "
        + L"\"source\": " + EncodeQuote(source.get()) + L", "
        + L"\"canGoBack\": " + BoolToString(canGoBack) + L", "
        + L"\"canGoForward\": " + BoolToString(canGoForward)
        + L"}";

    return result;
}

std::wstring NavigationStartingArgsToJsonString(
    ICoreWebView2* webview, ICoreWebView2NavigationStartingEventArgs* args,
    const std::wstring& eventName)
{
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
        L"{ \"kind\": \"event\", \"name\": \"" + eventName + L"\", \"args\": {";

    message += L"\"navigationId\": " + std::to_wstring(navigationId) + L", ";

    message += L"\"cancel\": " + BoolToString(cancel) + L", " + L"\"isRedirected\": " +
               BoolToString(isRedirected) + L", " + L"\"isUserInitiated\": " +
               BoolToString(isUserInitiated) + L", " + L"\"requestHeaders\": " +
               RequestHeadersToJsonString(requestHeaders.get()) + L", " + L"\"uri\": " +
               EncodeQuote(uri.get()) + L" " + L"}" + WebViewPropertiesToJsonString(webview) +
               L"}";

    return message;
}

std::wstring ContentLoadingArgsToJsonString(
    ICoreWebView2* webview, ICoreWebView2ContentLoadingEventArgs* args,
    const std::wstring& eventName)
{
    BOOL isErrorPage = FALSE;
    CHECK_FAILURE(args->get_IsErrorPage(&isErrorPage));
    UINT64 navigationId = 0;
    CHECK_FAILURE(args->get_NavigationId(&navigationId));

    std::wstring message =
        L"{ \"kind\": \"event\", \"name\": \"" + eventName + L"\", \"args\": {";

    message += L"\"navigationId\": " + std::to_wstring(navigationId) + L", ";
    message += L"\"isErrorPage\": " + BoolToString(isErrorPage) + L"}" +
               WebViewPropertiesToJsonString(webview) + L"}";
    return message;
}

std::wstring NavigationCompletedArgsToJsonString(
    ICoreWebView2* webview, ICoreWebView2NavigationCompletedEventArgs* args,
    const std::wstring& eventName)
{
    BOOL isSuccess = FALSE;
    CHECK_FAILURE(args->get_IsSuccess(&isSuccess));
    COREWEBVIEW2_WEB_ERROR_STATUS webErrorStatus;
    CHECK_FAILURE(args->get_WebErrorStatus(&webErrorStatus));
    UINT64 navigationId = 0;
    CHECK_FAILURE(args->get_NavigationId(&navigationId));

    std::wstring message =
        L"{ \"kind\": \"event\", \"name\": \"" + eventName + L"\", \"args\": {";

    message += L"\"navigationId\": " + std::to_wstring(navigationId) + L", ";

    message += L"\"isSuccess\": " + BoolToString(isSuccess) +
               L", "
               L"\"webErrorStatus\": " +
               EncodeQuote(WebErrorStatusToString(webErrorStatus)) +
               L" "
               L"}" +
               WebViewPropertiesToJsonString(webview) + L"}";
    return message;
}

std::wstring DOMContentLoadedArgsToJsonString(
    ICoreWebView2* webview, ICoreWebView2DOMContentLoadedEventArgs* args,
    const std::wstring& eventName)
{
    UINT64 navigationId = 0;
    CHECK_FAILURE(args->get_NavigationId(&navigationId));

    std::wstring message =
        L"{ \"kind\": \"event\", \"name\": \"" + eventName + L"\", \"args\": {";

    message += L"\"navigationId\": " + std::to_wstring(navigationId);

    message += L"}" + WebViewPropertiesToJsonString(webview) + L"}";
    return message;
}

std::wstring WebResourceRequestedToJsonString(
    ICoreWebView2WebResourceRequest* webResourceRequest,
    const std::wstring& source = std::wstring())
{
    std::wstring message = L"{ \"kind\": \"event\", \"name\": "
                           L"\"WebResourceRequested\", \"args\": {"
                           L"\"request\": " +
                           RequestToJsonString(webResourceRequest) +
                           L", "
                           L"\"response\": null" +
                           source + L"}";

    return message;
}

void ScenarioWebViewEventMonitor::EnableWebResourceResponseReceivedEvent(bool enable) {
    if (!enable && m_webResourceResponseReceivedToken.value != 0)
    {
        m_webviewEventSource2->remove_WebResourceResponseReceived(m_webResourceResponseReceivedToken);
        m_webResourceResponseReceivedToken.value = 0;
    }
    else if (enable && m_webResourceResponseReceivedToken.value == 0)
    {
        m_webviewEventSource2->add_WebResourceResponseReceived(
            Callback<ICoreWebView2WebResourceResponseReceivedEventHandler>(
                [this](
                    ICoreWebView2* webview,
                    ICoreWebView2WebResourceResponseReceivedEventArgs* args) noexcept -> HRESULT
                {
                    wil::com_ptr<ICoreWebView2WebResourceRequest> webResourceRequest;
                    CHECK_FAILURE(args->get_Request(&webResourceRequest));
                    wil::com_ptr<ICoreWebView2WebResourceResponseView>
                        webResourceResponse;
                    CHECK_FAILURE(args->get_Response(&webResourceResponse));
                    //! [GetContent]
                    webResourceResponse->GetContent(
                        Callback<
                            ICoreWebView2WebResourceResponseViewGetContentCompletedHandler>(
                            [this, webResourceRequest,
                             webResourceResponse](HRESULT result, IStream* content) {
                                std::wstring message =
                                    L"{ \"kind\": \"event\", \"name\": "
                                    L"\"WebResourceResponseReceived\", \"args\": {"
                                    L"\"request\": " +
                                    RequestToJsonString(webResourceRequest.get()) +
                                    L", "
                                    L"\"response\": " +
                                    ResponseToJsonString(webResourceResponse.get(), content) +
                                    L"}";

                                message +=
                                    WebViewPropertiesToJsonString(m_webviewEventSource.get());
                                message += L"}";
                                PostEventMessage(message);
                                return S_OK;
                            })
                            .Get());
                    //! [GetContent]
                    return S_OK;
                })
                .Get(),
            &m_webResourceResponseReceivedToken);
    }
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
        auto webView2_22 = m_webviewEventSource.try_query<ICoreWebView2_22>();
        if (webView2_22)
        {
            webView2_22->AddWebResourceRequestedFilterWithRequestSourceKinds(
                L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL,
                COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS_ALL);
        }

        m_webviewEventSource->add_WebResourceRequested(
            Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                [this](
                    ICoreWebView2* webview,
                    ICoreWebView2WebResourceRequestedEventArgs* args) noexcept -> HRESULT
                {
                    wil::com_ptr<ICoreWebView2WebResourceRequest> webResourceRequest;
                    CHECK_FAILURE(args->get_Request(&webResourceRequest));
                    wil::com_ptr<ICoreWebView2WebResourceResponse> webResourceResponse;
                    CHECK_FAILURE(args->get_Response(&webResourceResponse));

                    std::wstring message =
                        WebResourceRequestedToJsonString(webResourceRequest.get());

                    wil::com_ptr<ICoreWebView2WebResourceRequestedEventArgs> argsPtr = args;
                    wil::com_ptr<ICoreWebView2WebResourceRequestedEventArgs2>
                        webResourceRequestArgs =
                            argsPtr.try_query<ICoreWebView2WebResourceRequestedEventArgs2>();
                    if (webResourceRequestArgs)
                    {
                        COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS requestedSourceKind =
                            COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS_ALL;
                        CHECK_FAILURE(webResourceRequestArgs->get_RequestedSourceKind(
                            &requestedSourceKind));
                        if (requestedSourceKind !=
                            COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS_ALL)
                        {
                            std::wstring source = L", \"source\": " + WebResourceSourceToString(
                                                                          requestedSourceKind);

                            message = WebResourceRequestedToJsonString(
                                webResourceRequest.get(), source);
                        }
                    }
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
            [this](
                ICoreWebView2* sender,
                ICoreWebView2WebMessageReceivedEventArgs* args) noexcept -> HRESULT
            {
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
                        else if (wcscmp(webMessageAsString.get(), L"webResourceRequested,off") == 0)
                        {
                            EnableWebResourceRequestedEvent(false);
                        }
                        else if (wcscmp(webMessageAsString.get(), L"webResourceResponseReceived,on") == 0)
                        {
                            EnableWebResourceResponseReceivedEvent(true);
                        }
                        else if (
                            wcscmp(webMessageAsString.get(), L"webResourceResponseReceived,off") == 0)
                        {
                            EnableWebResourceResponseReceivedEvent(false);
                        }
                    }
                }

                return S_OK;
            })
            .Get(),
        &m_eventViewWebMessageReceivedToken);

    m_webviewEventSource->add_WebMessageReceived(
        Callback<ICoreWebView2WebMessageReceivedEventHandler>(
            [this](
                ICoreWebView2* sender,
                ICoreWebView2WebMessageReceivedEventArgs* args) noexcept -> HRESULT
            {
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
            [this](
                ICoreWebView2* sender,
                ICoreWebView2NewWindowRequestedEventArgs* args) noexcept -> HRESULT
            {
                BOOL handled = FALSE;
                CHECK_FAILURE(args->get_Handled(&handled));
                BOOL isUserInitiated = FALSE;
                CHECK_FAILURE(args->get_IsUserInitiated(&isUserInitiated));
                wil::unique_cotaskmem_string uri;
                CHECK_FAILURE(args->get_Uri(&uri));

                wil::com_ptr<ICoreWebView2NewWindowRequestedEventArgs2>
                    args2;
                wil::unique_cotaskmem_string name;
                std::wstring encodedName = EncodeQuote(L"");

                if (SUCCEEDED(args->QueryInterface(IID_PPV_ARGS(&args2))))
                {
                    CHECK_FAILURE(args2->get_Name(&name));
                    encodedName = EncodeQuote(name.get());
                }

                wil::com_ptr<ICoreWebView2NewWindowRequestedEventArgs3> args3;
                std::wstring frameName = EncodeQuote(L"");
                std::wstring frameUri = EncodeQuote(L"");
                if (SUCCEEDED(args->QueryInterface(IID_PPV_ARGS(&args3))))
                {
                    wil::com_ptr<ICoreWebView2FrameInfo> frame_info;
                    CHECK_FAILURE(args3->get_OriginalSourceFrameInfo(&frame_info));
                    wil::unique_cotaskmem_string name;
                    CHECK_FAILURE(frame_info->get_Name(&name));
                    frameName = EncodeQuote(name.get());
                    wil::unique_cotaskmem_string source;
                    CHECK_FAILURE(frame_info->get_Source(&source));
                    frameUri = EncodeQuote(source.get());
                }

                std::wstring message =
                    L"{ \"kind\": \"event\", \"name\": \"NewWindowRequested\", \"args\": {"
                    L"\"handled\": " +
                    BoolToString(handled) +
                    L", "
                    L"\"isUserInitiated\": " +
                    BoolToString(isUserInitiated) +
                    L", "
                    L"\"uri\": " +
                    EncodeQuote(uri.get()) +
                    L", "
                    L"\"name\": " +
                    encodedName +
                    L", "
                    L"\"newWindow\": null" +
                    L", "
                    L"\"frameName\": " +
                    frameName +
                    L", "
                    L"\"frameUri\": " +
                    frameUri + L"}" +
                    WebViewPropertiesToJsonString(m_webviewEventSource.get()) + L"}";
                PostEventMessage(message);

                return S_OK;
            })
            .Get(),
        &m_newWindowRequestedToken);

    m_webviewEventSource->add_NavigationStarting(
        Callback<ICoreWebView2NavigationStartingEventHandler>(
            [this](
                ICoreWebView2* sender,
                ICoreWebView2NavigationStartingEventArgs* args) noexcept -> HRESULT
            {
                std::wstring message =
                    NavigationStartingArgsToJsonString(sender, args, L"NavigationStarting");
                PostEventMessage(message);

                return S_OK;
            })
            .Get(),
        &m_navigationStartingToken);

    m_webviewEventSource->add_FrameNavigationStarting(
        Callback<ICoreWebView2NavigationStartingEventHandler>(
            [this](
                ICoreWebView2* sender,
                ICoreWebView2NavigationStartingEventArgs* args) noexcept -> HRESULT
            {
                std::wstring message = NavigationStartingArgsToJsonString(
                    sender, args, L"FrameNavigationStarting");
                PostEventMessage(message);

                return S_OK;
            })
            .Get(),
        &m_frameNavigationStartingToken);

    m_webviewEventSource->add_SourceChanged(
        Callback<ICoreWebView2SourceChangedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2SourceChangedEventArgs* args) noexcept
            -> HRESULT
            {
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
        &m_sourceChangedToken);

    m_webviewEventSource->add_ContentLoading(
        Callback<ICoreWebView2ContentLoadingEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2ContentLoadingEventArgs* args) noexcept
            -> HRESULT
            {
                std::wstring message =
                    ContentLoadingArgsToJsonString(sender, args, L"ContentLoading");
                PostEventMessage(message);

                return S_OK;
            })
            .Get(),
        &m_contentLoadingToken);

    m_webviewEventSource->add_HistoryChanged(
        Callback<ICoreWebView2HistoryChangedEventHandler>(
            [this](ICoreWebView2* sender, IUnknown* args) noexcept -> HRESULT
            {
                std::wstring message =
                    L"{ \"kind\": \"event\", \"name\": \"HistoryChanged\", \"args\": {";
                message +=
                    L"}" + WebViewPropertiesToJsonString(m_webviewEventSource.get()) + L"}";
                PostEventMessage(message);

                return S_OK;
            })
            .Get(),
        &m_historyChangedToken);

    m_webviewEventSource->add_NavigationCompleted(
        Callback<ICoreWebView2NavigationCompletedEventHandler>(
            [this](
                ICoreWebView2* sender,
                ICoreWebView2NavigationCompletedEventArgs* args) noexcept -> HRESULT
            {
                std::wstring message =
                    NavigationCompletedArgsToJsonString(sender, args, L"NavigationCompleted");
                PostEventMessage(message);

                return S_OK;
            })
            .Get(),
        &m_navigationCompletedToken);

    m_webviewEventSource->add_FrameNavigationCompleted(
        Callback<ICoreWebView2NavigationCompletedEventHandler>(
            [this](
                ICoreWebView2* sender,
                ICoreWebView2NavigationCompletedEventArgs* args) noexcept -> HRESULT
            {
                std::wstring message = NavigationCompletedArgsToJsonString(
                    sender, args, L"FrameNavigationCompleted");
                PostEventMessage(message);

                return S_OK;
            })
            .Get(),
        &m_frameNavigationCompletedToken);

    m_webviewEventSource2->add_DOMContentLoaded(
        Callback<ICoreWebView2DOMContentLoadedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2DOMContentLoadedEventArgs* args) noexcept
            -> HRESULT
            {
                std::wstring message =
                    DOMContentLoadedArgsToJsonString(sender, args, L"DOMContentLoaded");
                PostEventMessage(message);

                return S_OK;
            })
            .Get(),
        &m_DOMContentLoadedToken);

    m_webviewEventSource->add_DocumentTitleChanged(
        Callback<ICoreWebView2DocumentTitleChangedEventHandler>(
            [this](ICoreWebView2* sender, IUnknown* args) noexcept -> HRESULT
            {
                std::wstring message =
                    L"{ \"kind\": \"event\", \"name\": \"DocumentTitleChanged\", \"args\": {"
                    L"}" +
                    WebViewPropertiesToJsonString(m_webviewEventSource.get()) +
                    L"}";
                PostEventMessage(message);

                return S_OK;
            })
            .Get(),
        &m_documentTitleChangedToken);

    m_webviewEventSource4 = m_webviewEventSource.try_query<ICoreWebView2_4>();
    if (m_webviewEventSource4) {
        m_webviewEventSource4->add_DownloadStarting(
            Callback<ICoreWebView2DownloadStartingEventHandler>(
                [this](
                    ICoreWebView2* sender,
                    ICoreWebView2DownloadStartingEventArgs* args) noexcept -> HRESULT
                {
                    wil::com_ptr<ICoreWebView2DownloadOperation> download;
                    CHECK_FAILURE(args->get_DownloadOperation(&download));

                    BOOL cancel = FALSE;
                    CHECK_FAILURE(args->get_Cancel(&cancel));

                    INT64 totalBytesToReceive = 0;
                    CHECK_FAILURE(
                        download->get_TotalBytesToReceive(&totalBytesToReceive));

                    wil::unique_cotaskmem_string uri;
                    CHECK_FAILURE(download->get_Uri(&uri));

                    wil::unique_cotaskmem_string mimeType;
                    CHECK_FAILURE(download->get_MimeType(&mimeType));

                    wil::unique_cotaskmem_string contentDisposition;
                    CHECK_FAILURE(download->get_ContentDisposition(&contentDisposition));

                    wil::unique_cotaskmem_string resultFilePath;
                    CHECK_FAILURE(args->get_ResultFilePath(&resultFilePath));

                    COREWEBVIEW2_DOWNLOAD_STATE state;
                    CHECK_FAILURE(download->get_State(&state));

                    BOOL handled = FALSE;
                    CHECK_FAILURE(args->get_Handled(&handled));

                    download->add_StateChanged(
                        Callback<ICoreWebView2StateChangedEventHandler>(
                            [this, download](
                                ICoreWebView2DownloadOperation* sender,
                                IUnknown* args) noexcept -> HRESULT
                            {
                                COREWEBVIEW2_DOWNLOAD_STATE state;
                                CHECK_FAILURE(download->get_State(&state));

                                std::wstring state_string = L"";
                                switch (state)
                                {
                                case COREWEBVIEW2_DOWNLOAD_STATE_IN_PROGRESS:
                                    state_string = L"In progress";
                                    break;
                                case COREWEBVIEW2_DOWNLOAD_STATE_COMPLETED:
                                    state_string = L"Complete";
                                    download->remove_StateChanged(
                                        m_stateChangedToken);
                                    download->remove_BytesReceivedChanged(
                                        m_bytesReceivedChangedToken);
                                    download->remove_EstimatedEndTimeChanged(
                                        m_estimatedEndTimeChanged);
                                    break;
                                case COREWEBVIEW2_DOWNLOAD_STATE_INTERRUPTED:
                                    state_string = L"Interrupted";
                                    break;
                                }

                                COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON interrupt_reason;
                                CHECK_FAILURE(download->get_InterruptReason(&interrupt_reason));
                                std::wstring interrupt_reason_string =
                                    InterruptReasonToString(interrupt_reason);

                                std::wstring message = L"{ \"kind\": \"event\", \"name\": "
                                                      L"\"DownloadStateChanged\", \"args\": {";
                                message += L"\"state\": " + EncodeQuote(state_string) + L", " +
                                          L"\"interruptReason\": " +
                                          EncodeQuote(interrupt_reason_string) + L" ";

                                message +=
                                    L"}" +
                                    WebViewPropertiesToJsonString(m_webviewEventSource.get()) +
                                    L"}";
                                PostEventMessage(message);
                                return S_OK;
                            })
                            .Get(),
                        &m_stateChangedToken);

                    download->add_BytesReceivedChanged(
                        Callback<ICoreWebView2BytesReceivedChangedEventHandler>(
                            [this, download](
                                ICoreWebView2DownloadOperation* sender,
                                IUnknown* args) noexcept -> HRESULT
                            {
                                INT64 bytesReceived = 0;
                                CHECK_FAILURE(download->get_BytesReceived(
                                    &bytesReceived));

                                std::wstring message =
                                    L"{ \"kind\": \"event\", \"name\": "
                                    L"\"DownloadBytesReceivedChanged\", \"args\": {";
                                message += L"\"bytesReceived\": " +
                                          std::to_wstring(bytesReceived) + L" ";

                                message +=
                                    L"}" +
                                    WebViewPropertiesToJsonString(m_webviewEventSource.get()) +
                                    L"}";
                                PostEventMessage(message);
                                return S_OK;
                            })
                            .Get(),
                        &m_bytesReceivedChangedToken);

                    download->add_EstimatedEndTimeChanged(
                        Callback<ICoreWebView2EstimatedEndTimeChangedEventHandler>(
                            [this, download](
                                ICoreWebView2DownloadOperation* sender,
                                IUnknown* args) noexcept -> HRESULT
                            {
                                wil::unique_cotaskmem_string estimatedEndTime;
                                CHECK_FAILURE(download->get_EstimatedEndTime(&estimatedEndTime));

                                std::wstring message =
                                    L"{ \"kind\": \"event\", \"name\": "
                                    L"\"DownloadEstimatedEndTimeChanged\", \"args\": {";
                                message += L"\"estimatedEndTime\": " +
                                          EncodeQuote(estimatedEndTime.get()) + L" ";

                                message +=
                                    L"}" +
                                    WebViewPropertiesToJsonString(m_webviewEventSource.get()) +
                                    L"}";
                                PostEventMessage(message);
                                return S_OK;
                            })
                            .Get(),
                        &m_estimatedEndTimeChanged);

                    std::wstring message =
                        L"{ \"kind\": \"event\", \"name\": \"DownloadStarting\", \"args\": {";

                    message += L"\"cancel\": " + BoolToString(cancel) + L", " +
                              L"\"resultFilePath\": " + EncodeQuote(resultFilePath.get()) + L", " +
                              L"\"handled\": " + BoolToString(handled) + L", " +
                              L"\"uri\": " + EncodeQuote(uri.get()) + L", " + L"\"mimeType\": " +
                              EncodeQuote(mimeType.get()) + L", " + L"\"contentDisposition\": " +
                              EncodeQuote(contentDisposition.get()) + L", " +
                              L"\"totalBytesToReceive\": " +
                              std::to_wstring(totalBytesToReceive) + L" ";

                    message +=
                        L"}" + WebViewPropertiesToJsonString(m_webviewEventSource.get()) + L"}";
                    PostEventMessage(message);

                    return S_OK;
                })
                .Get(),
            &m_downloadStartingToken);

        m_webviewEventSource4->add_FrameCreated(
            Callback<ICoreWebView2FrameCreatedEventHandler>(
                [this](ICoreWebView2* sender, ICoreWebView2FrameCreatedEventArgs* args) noexcept
                -> HRESULT
                {
                    wil::com_ptr<ICoreWebView2Frame> webviewFrame;
                    CHECK_FAILURE(args->get_Frame(&webviewFrame));
                    InitializeFrameEventView(webviewFrame, first_level_iframe_depth + 1);
                    wil::unique_cotaskmem_string name;
                    CHECK_FAILURE(webviewFrame->get_Name(&name));

                    std::wstring message =
                        L"{ \"kind\": \"event\", \"name\": \"FrameCreated\", \"args\": {";
                    message += L"\"frame\": " + EncodeQuote(name.get());

                    auto webView2_20 = wil::try_com_query<ICoreWebView2_20>(sender);
                    if (webView2_20)
                    {
                        UINT32 frameId = 0;
                        CHECK_FAILURE(webView2_20->get_FrameId(&frameId));
                        message +=
                            L",\"sender main frame id\": " + std::to_wstring((int)frameId);
                    }
                    auto frame5 = webviewFrame.try_query<ICoreWebView2Frame5>();
                    if (frame5)
                    {
                        UINT32 frameId = 0;
                        CHECK_FAILURE(frame5->get_FrameId(&frameId));
                        message += L",\"frame id\": " + std::to_wstring((int)frameId);
                    }
                    message +=
                        L"}" + WebViewPropertiesToJsonString(m_webviewEventSource.get()) + L"}";
                    PostEventMessage(message);

                    return S_OK;
                })
                .Get(),
            &m_frameCreatedToken);
    }

    m_controllerEventSource->add_GotFocus(
        Callback<ICoreWebView2FocusChangedEventHandler>(
            [this](ICoreWebView2Controller* sender, IUnknown* args) noexcept -> HRESULT
            {
                std::wstring message =
                    L"{ \"kind\": \"event\", \"name\": \"GotFocus\", \"args\": {} }";
                PostEventMessage(message);
                return S_OK;
            })
            .Get(),
        &m_gotFocusToken);
    m_controllerEventSource->add_LostFocus(
        Callback<ICoreWebView2FocusChangedEventHandler>(
            [this](ICoreWebView2Controller* sender, IUnknown* args) noexcept -> HRESULT
            {
                std::wstring message =
                    L"{ \"kind\": \"event\", \"name\": \"LostFocus\", \"args\": {} }";
                PostEventMessage(message);
                return S_OK;
            })
            .Get(),
        &m_lostFocusToken);

    m_webViewEventSource9 = m_webviewEventSource.try_query<ICoreWebView2_9>();
    if (m_webViewEventSource9)
    {
        m_webViewEventSource9->add_IsDefaultDownloadDialogOpenChanged(
            Callback<ICoreWebView2IsDefaultDownloadDialogOpenChangedEventHandler>(
                [this](ICoreWebView2* sender, IUnknown* args) noexcept -> HRESULT
                {
                    std::wstring message =
                        L"{ \"kind\": \"event\", \"name\": "
                        L"\"IsDefaultDownloadDialogOpenChanged\", \"args\": {";
                    BOOL isOpen;
                    m_webViewEventSource9->get_IsDefaultDownloadDialogOpen(&isOpen);
                    message += L"\"isDefaultDownloadDialogOpen\": " + BoolToString(isOpen);
                    message +=
                        L"}" + WebViewPropertiesToJsonString(m_webviewEventSource.get()) + L"}";
                    PostEventMessage(message);
                    return S_OK;
                })
                .Get(),
            &m_isDefaultDownloadDialogOpenChangedToken);
    }

    m_webviewEventSource->add_PermissionRequested(
        Callback<ICoreWebView2PermissionRequestedEventHandler>(
            [this](
                ICoreWebView2* sender,
                ICoreWebView2PermissionRequestedEventArgs* args) noexcept -> HRESULT
            {
                wil::unique_cotaskmem_string uri;
                CHECK_FAILURE(args->get_Uri(&uri));
                COREWEBVIEW2_PERMISSION_KIND kind;
                CHECK_FAILURE(args->get_PermissionKind(&kind));
                COREWEBVIEW2_PERMISSION_STATE state;
                CHECK_FAILURE(args->get_State(&state));
                wil::com_ptr<ICoreWebView2PermissionRequestedEventArgs3> extended_args;
                CHECK_FAILURE(args->QueryInterface(IID_PPV_ARGS(&extended_args)));
                BOOL saves_in_profile = TRUE;
                CHECK_FAILURE(extended_args->get_SavesInProfile(&saves_in_profile));
                std::wstring message =
                    L"{ \"kind\": \"event\", \"name\": \"PermissionRequested\", \"args\": {"
                    L"\"uri\": " +
                    EncodeQuote(uri.get()) +
                    L", "
                    L"\"kind\": " +
                    EncodeQuote(PermissionKindToString(kind)) +
                    L", "
                    L"\"state\": " +
                    EncodeQuote(PermissionStateToString(state)) + L", \"SavesInProfile\": " +
                    BoolToString(saves_in_profile) + L"}" +
                    WebViewPropertiesToJsonString(m_webviewEventSource.get()) + L"}";
                PostEventMessage(message);
                return S_OK;
            })
            .Get(),
        &m_permissionRequestedToken);
}

void ScenarioWebViewEventMonitor::InitializeFrameEventView(
    wil::com_ptr<ICoreWebView2Frame> webviewFrame, int depth)
{
    webviewFrame->add_Destroyed(
        Callback<ICoreWebView2FrameDestroyedEventHandler>(
            [this](ICoreWebView2Frame* sender, IUnknown* args) noexcept -> HRESULT
            {
                wil::unique_cotaskmem_string name;
                CHECK_FAILURE(sender->get_Name(&name));
                std::wstring message = L"{ \"kind\": \"event\", \"name\": "
                                       L"\"CoreWebView2Frame::Destroyed\", \"args\": {";
                message += L"\"frame name\": " + EncodeQuote(name.get());
                message +=
                    L"}" + WebViewPropertiesToJsonString(m_webviewEventSource.get()) + L"}";
                PostEventMessage(message);
                return S_OK;
            })
            .Get(),
        NULL);

    wil::com_ptr<ICoreWebView2Frame2> frame2 = webviewFrame.try_query<ICoreWebView2Frame2>();
    if (frame2)
    {
        frame2->add_NavigationStarting(
            Callback<ICoreWebView2FrameNavigationStartingEventHandler>(
                [this](
                    ICoreWebView2Frame* sender,
                    ICoreWebView2NavigationStartingEventArgs* args) noexcept -> HRESULT
                {
                    std::wstring message = NavigationStartingArgsToJsonString(
                        m_webviewEventSource.get(), args,
                        L"CoreWebView2Frame::NavigationStarting");
                    PostEventMessage(message);

                    return S_OK;
                })
                .Get(),
            NULL);

        frame2->add_ContentLoading(
            Callback<ICoreWebView2FrameContentLoadingEventHandler>(
                [this](
                    ICoreWebView2Frame* sender,
                    ICoreWebView2ContentLoadingEventArgs* args) noexcept -> HRESULT
                {
                    std::wstring message = ContentLoadingArgsToJsonString(
                        m_webviewEventSource.get(), args, L"CoreWebView2Frame::ContentLoading");
                    PostEventMessage(message);

                    return S_OK;
                })
                .Get(),
            NULL);

        frame2->add_NavigationCompleted(
            Callback<ICoreWebView2FrameNavigationCompletedEventHandler>(
                [this](
                    ICoreWebView2Frame* sender,
                    ICoreWebView2NavigationCompletedEventArgs* args) noexcept -> HRESULT
                {
                    std::wstring message = NavigationCompletedArgsToJsonString(
                        m_webviewEventSource.get(), args,
                        L"CoreWebView2Frame::NavigationCompleted");
                    PostEventMessage(message);

                    return S_OK;
                })
                .Get(),
            NULL);

        frame2->add_DOMContentLoaded(
            Callback<ICoreWebView2FrameDOMContentLoadedEventHandler>(
                [this](
                    ICoreWebView2Frame* sender,
                    ICoreWebView2DOMContentLoadedEventArgs* args) noexcept -> HRESULT
                {
                    std::wstring message = DOMContentLoadedArgsToJsonString(
                        m_webviewEventSource.get(), args,
                        L"CoreWebView2Frame::DOMContentLoaded");
                    PostEventMessage(message);

                    return S_OK;
                })
                .Get(),
            NULL);
        frame2->add_WebMessageReceived(
            Callback<ICoreWebView2FrameWebMessageReceivedEventHandler>(
                [this](
                    ICoreWebView2Frame* sender,
                    ICoreWebView2WebMessageReceivedEventArgs* args) noexcept -> HRESULT
                {
                    wil::unique_cotaskmem_string source;
                    CHECK_FAILURE(args->get_Source(&source));
                    wil::unique_cotaskmem_string webMessageAsString;
                    if (SUCCEEDED(args->TryGetWebMessageAsString(&webMessageAsString)))
                    {
                        std::wstring message =
                            L"{ \"kind\": \"event\", \"name\": "
                            L"\"CoreWebView2Frame::WebMessageReceived\", \"args\": {";

                        wil::unique_cotaskmem_string uri;
                        CHECK_FAILURE(args->get_Source(&uri));
                        message += L"\"source\": " + EncodeQuote(uri.get()) + L", ";
                        message += L"\"webMessageAsString\": " +
                                   EncodeQuote(webMessageAsString.get()) + L" ";

                        UINT32 frameId = 0;
                        auto frame5 = wil::try_com_query<ICoreWebView2Frame5>(sender);
                        if (frame5)
                        {
                            CHECK_FAILURE(frame5->get_FrameId(&frameId));
                            message += L",\"sender webview frame id\": " +
                                       std::to_wstring((int)frameId);
                        }
                        message += L"}" +
                                   WebViewPropertiesToJsonString(m_webviewEventSource.get()) +
                                   L"}";
                        PostEventMessage(message);
                    }

                    return S_OK;
                })
                .Get(),
            NULL);
    }
    auto frame7 = webviewFrame.try_query<ICoreWebView2Frame7>();
    if (frame7)
    {
        //! [FrameCreated]
        frame7->add_FrameCreated(
            Callback<ICoreWebView2FrameChildFrameCreatedEventHandler>(
                [this, depth](
                    ICoreWebView2Frame* sender,
                    ICoreWebView2FrameCreatedEventArgs* args) noexcept -> HRESULT
                {
                    wil::com_ptr<ICoreWebView2Frame> webviewFrame;
                    CHECK_FAILURE(args->get_Frame(&webviewFrame));
                    wil::unique_cotaskmem_string name;
                    CHECK_FAILURE(webviewFrame->get_Name(&name));

                    InitializeFrameEventView(webviewFrame, depth + 1);

                    std::wstring message = L"{ \"kind\": \"event\", \"name\": "
                                           L"\"CoreWebView2Frame::FrameCreated\", \"args\": {";
                    message += L"\"frame name\": " + EncodeQuote(name.get());
                    message += L",\"depth\": " + std::to_wstring(depth);
                    auto frame5 = webviewFrame.try_query<ICoreWebView2Frame5>();
                    UINT32 frameId = 0;
                    if (frame5)
                    {
                        CHECK_FAILURE(frame5->get_FrameId(&frameId));
                        message += L",\"webview frame id\": " + std::to_wstring((int)frameId);
                    }
                    frame5 = wil::try_com_query<ICoreWebView2Frame5>(sender);
                    if (frame5)
                    {
                        CHECK_FAILURE(frame5->get_FrameId(&frameId));
                        message +=
                            L",\"sender webview frame id\": " + std::to_wstring((int)frameId);
                    }
                    message +=
                        L"}" + WebViewPropertiesToJsonString(m_webviewEventSource.get()) + L"}";
                    PostEventMessage(message);
                    return S_OK;
                })
                .Get(),
            &m_frameCreatedToken);
        //! [FrameCreated]
    }
}
void ScenarioWebViewEventMonitor::PostEventMessage(std::wstring message)
{
    HRESULT hr = m_webviewEventView->PostWebMessageAsJson(message.c_str());
    if (FAILED(hr))
    {
        ShowFailure(hr, L"PostWebMessageAsJson failed:\n" + message);
    }
}

std::wstring ScenarioWebViewEventMonitor::InterruptReasonToString(
    const COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON interrupt_reason)
{
    std::wstring interrupt_reason_string = L"";
    switch (interrupt_reason)
    {
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_NONE:
        interrupt_reason_string = L"None";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_FILE_FAILED:
        interrupt_reason_string = L"File failed";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_FILE_ACCESS_DENIED:
        interrupt_reason_string = L"File access denied";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_FILE_NO_SPACE:
        interrupt_reason_string = L"File no space";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_FILE_NAME_TOO_LONG:
        interrupt_reason_string = L"File name too long";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_FILE_TOO_LARGE:
        interrupt_reason_string = L"File too large";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_FILE_MALICIOUS:
        interrupt_reason_string = L"File malicious";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_FILE_TRANSIENT_ERROR:
        interrupt_reason_string = L"File transient error";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_FILE_BLOCKED_BY_POLICY:
        interrupt_reason_string = L"File blocked by policy";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_FILE_SECURITY_CHECK_FAILED:
        interrupt_reason_string = L"File security check failed";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_FILE_TOO_SHORT:
        interrupt_reason_string = L"File too short";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_FILE_HASH_MISMATCH:
        interrupt_reason_string = L"File hash mismatch";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_NETWORK_FAILED:
        interrupt_reason_string = L"Network failed";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_NETWORK_TIMEOUT:
        interrupt_reason_string = L"Network timeout";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_NETWORK_DISCONNECTED:
        interrupt_reason_string = L"Network disconnected";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_NETWORK_SERVER_DOWN:
        interrupt_reason_string = L"Network server down";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_NETWORK_INVALID_REQUEST:
        interrupt_reason_string = L"Network invalid request";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_SERVER_FAILED:
        interrupt_reason_string = L"Server failed";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_SERVER_NO_RANGE:
        interrupt_reason_string = L"Server no range";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_SERVER_BAD_CONTENT:
        interrupt_reason_string = L"Server bad content";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_SERVER_UNAUTHORIZED:
        interrupt_reason_string = L"Server unauthorized";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_SERVER_CERTIFICATE_PROBLEM:
        interrupt_reason_string = L"Server certificate problem";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_SERVER_FORBIDDEN:
        interrupt_reason_string = L"Server forbidden";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_SERVER_UNEXPECTED_RESPONSE:
        interrupt_reason_string = L"Server unexpected response";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_SERVER_CONTENT_LENGTH_MISMATCH:
        interrupt_reason_string = L"Server content length mismatch";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_SERVER_CROSS_ORIGIN_REDIRECT:
        interrupt_reason_string = L"Server cross origin redirect";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_USER_CANCELED:
        interrupt_reason_string = L"User canceled";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_USER_SHUTDOWN:
        interrupt_reason_string = L"User shutdown";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_USER_PAUSED:
        interrupt_reason_string = L"User paused";
        break;
    case COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON_DOWNLOAD_PROCESS_CRASHED:
        interrupt_reason_string = L"Download process crashed";
        break;
    }
    return interrupt_reason_string;
}
