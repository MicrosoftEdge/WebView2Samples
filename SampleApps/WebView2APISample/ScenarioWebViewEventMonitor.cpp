// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING 1

#include "AppWindow.h"
#include "CheckFailure.h"
#include "ScenarioWebViewEventMonitor.h"
#include <WebView2.h>
#include <regex>
#include <string>
#include <codecvt>
#include <locale>

using namespace Microsoft::WRL;
using namespace std;

static constexpr wchar_t c_samplePath[] = L"ScenarioWebViewEventMonitor.html";

ScenarioWebViewEventMonitor::ScenarioWebViewEventMonitor(AppWindow* appWindowEventSource)
    : m_appWindowEventSource(appWindowEventSource),
      m_webviewEventSource(appWindowEventSource->GetWebView())
{
    m_sampleUri = m_appWindowEventSource->GetLocalUri(c_samplePath);
    m_appWindowEventView = new AppWindow(
        IDM_CREATION_MODE_WINDOWED,
        m_sampleUri, appWindowEventSource->GetUserDataFolder(),
        false,
        [this]() -> void {
            InitializeEventView(m_appWindowEventView->GetWebView());
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
    m_webviewEventSource->remove_NavigationCompleted(m_navigationCompletedToken);
    m_webviewEventSource->remove_DocumentTitleChanged(m_documentTitleChangedToken);
    m_webviewEventSource->remove_WebMessageReceived(m_webMessageReceivedToken);
    m_webviewEventSource->remove_NewWindowRequested(m_newWindowRequestedToken);
    m_webviewEventSource2->remove_DOMContentLoaded(m_DOMContentLoadedToken);
    m_webviewEventSourceExperimental2->remove_DownloadStarting(m_downloadStartingToken);
    EnableWebResourceRequestedEvent(false);
    EnableWebResourceResponseReceivedEvent(false);

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

    std::wstring result = L", \"webview\": {"
        L"\"documentTitle\": " + EncodeQuote(documentTitle.get()) + L", "
        + L"\"source\": " + EncodeQuote(source.get()) + L" "
        + L"}";

    return result;
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
                [this](ICoreWebView2* webview, ICoreWebView2WebResourceResponseReceivedEventArgs* args)
                    -> HRESULT {
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
                    WebViewPropertiesToJsonString(m_webviewEventSource.get()) + L"}";

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
        &m_sourceChangedToken);

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
        &m_contentLoadingToken);

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
        &m_historyChangedToken);

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
        &m_navigationCompletedToken);

    m_webviewEventSource2->add_DOMContentLoaded(
        Callback<ICoreWebView2DOMContentLoadedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2DOMContentLoadedEventArgs* args)
                -> HRESULT {
                UINT64 navigationId = 0;
                CHECK_FAILURE(args->get_NavigationId(&navigationId));

                std::wstring message =
                    L"{ \"kind\": \"event\", \"name\": \"DOMContentLoaded\", \"args\": {";

                message += L"\"navigationId\": " + std::to_wstring(navigationId);

                message +=
                    L"}" + WebViewPropertiesToJsonString(m_webviewEventSource.get()) + L"}";
                PostEventMessage(message);

                return S_OK;
            })
            .Get(),
        &m_DOMContentLoadedToken);

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
        &m_documentTitleChangedToken);

    m_webviewEventSourceExperimental2 = m_webviewEventSource.try_query<ICoreWebView2Experimental2>();
    if (m_webviewEventSourceExperimental2) {
        m_webviewEventSourceExperimental2->add_DownloadStarting(
            Callback<ICoreWebView2ExperimentalDownloadStartingEventHandler>(
                [this](ICoreWebView2* sender, ICoreWebView2ExperimentalDownloadStartingEventArgs* args)
                    -> HRESULT {
                    wil::com_ptr<ICoreWebView2ExperimentalDownloadOperation> download;
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
                        Callback<ICoreWebView2ExperimentalStateChangedEventHandler>(
                            [this, download](
                                ICoreWebView2ExperimentalDownloadOperation* sender,
                                IUnknown* args)
                                -> HRESULT {
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
                        Callback<
                            ICoreWebView2ExperimentalBytesReceivedChangedEventHandler>(
                            [this, download](
                                ICoreWebView2ExperimentalDownloadOperation* sender, IUnknown* args) -> HRESULT {
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
                        Callback<ICoreWebView2ExperimentalEstimatedEndTimeChangedEventHandler>(
                            [this, download](
                                ICoreWebView2ExperimentalDownloadOperation* sender, IUnknown* args) -> HRESULT {
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
