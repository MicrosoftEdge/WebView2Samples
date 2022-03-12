// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include <algorithm>
#include <sstream>

#include "ProcessComponent.h"
#include "ScriptComponent.h"

#include "CheckFailure.h"
#include "TextInputDialog.h"

using namespace Microsoft::WRL;

//! [AdditionalAllowedFrameAncestors_1]
const std::wstring myTrustedSite = L"https://appassets.example";
const std::wstring siteToEmbed = L"https://www.microsoft.com";

// The trusted page is using <iframe name="my_site_embedding_frame">
// element to embed other sites.
const std::wstring siteEmbeddingFrameName = L"my_site_embedding_frame";

bool AreSitesSame(PCWSTR url1, PCWSTR url2)
{
    wil::com_ptr<IUri> uri1;
    CHECK_FAILURE(CreateUri(url1, Uri_CREATE_CANONICALIZE, 0, &uri1));
    DWORD scheme1 = -1;
    DWORD port1 = 0;
    wil::unique_bstr host1;
    CHECK_FAILURE(uri1->GetScheme(&scheme1));
    CHECK_FAILURE(uri1->GetHost(&host1));
    CHECK_FAILURE(uri1->GetPort(&port1));
    wil::com_ptr<IUri> uri2;
    CHECK_FAILURE(CreateUri(url2, Uri_CREATE_CANONICALIZE, 0, &uri2));
    DWORD scheme2 = -1;
    DWORD port2 = 0;
    wil::unique_bstr host2;
    CHECK_FAILURE(uri2->GetScheme(&scheme2));
    CHECK_FAILURE(uri2->GetHost(&host2));
    CHECK_FAILURE(uri2->GetPort(&port2));
    return (scheme1 == scheme2) && (port1 == port2) && (wcscmp(host1.get(), host2.get()) == 0);
}

// App specific logic to decide whether the page is fully trusted.
bool IsAppContentUri(PCWSTR pageUrl)
{
    return AreSitesSame(pageUrl, myTrustedSite.c_str());
}

// App specific logic to decide whether a site is the one it wants to embed.
bool IsTargetSite(PCWSTR siteUrl)
{
    return AreSitesSame(siteUrl, siteToEmbed.c_str());
}
//! [AdditionalAllowedFrameAncestors_1]

// Simple functions to retrieve fields from a JSON message.
// For production code, you should use a real JSON parser library.
const std::wstring GetJSONStringField(PCWSTR JsonMessage, PCWSTR fieldName)
{
    std::wstring message(JsonMessage);
    std::wstring startSubStr = L"\"";
    startSubStr.append(fieldName);
    startSubStr.append(L"\":\"");
    std::string::size_type start = message.find(startSubStr);
    if (start == std::wstring::npos)
        return std::wstring();
    start += startSubStr.length();
    std::string::size_type end = message.find(L'\"', start);
    if (end == std::wstring::npos)
        return std::wstring();
    return message.substr(start, end - start);
}
const int64_t GetJSONIntegerField(PCWSTR JsonMessage, PCWSTR fieldName)
{
    std::wstring message(JsonMessage);
    std::wstring startSubStr = L"\"";
    startSubStr.append(fieldName);
    startSubStr.append(L"\":");
    std::string::size_type start = message.find(startSubStr);
    if (start == std::wstring::npos)
        return 0;
    start += startSubStr.length();
    return _wtoi64(message.substr(start).c_str());
}

ScriptComponent::ScriptComponent(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    HandleIFrames();
    HandleCDPTargets();
}

bool ScriptComponent::HandleWindowMessage(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT* result)
{
    if (message == WM_COMMAND)
    {
        switch (LOWORD(wParam))
        {
        case IDM_INJECT_SCRIPT:
            InjectScript();
            return true;
        case ID_ADD_INITIALIZE_SCRIPT:
            AddInitializeScript();
            return true;
        case ID_REMOVE_INITIALIZE_SCRIPT:
            RemoveInitializeScript();
            return true;
        case IDM_POST_WEB_MESSAGE_STRING:
            SendStringWebMessage();
            return true;
        case IDM_POST_WEB_MESSAGE_JSON:
            SendJsonWebMessage();
            return true;
        case IDM_SUBSCRIBE_TO_CDP_EVENT:
            SubscribeToCdpEvent();
            return true;
        case IDM_CALL_CDP_METHOD:
            CallCdpMethod();
            return true;
        case IDM_CALL_CDP_METHOD_FOR_SESSION:
            CallCdpMethodForSession();
            return true;
        case IDM_COLLECT_HEAP_MEMORY_VIA_CDP:
            CollectHeapUsageViaCdp();
            return true;
        case IDM_ADD_HOST_OBJECT:
            AddComObject();
            return true;
        case IDM_INJECT_SITE_EMBEDDING_IFRAME:
            AddSiteEmbeddingIFrame();
            return true;
        case IDM_OPEN_DEVTOOLS_WINDOW:
            m_webView->OpenDevToolsWindow();
            return true;
        case IDM_OPEN_TASK_MANAGER_WINDOW:
            OpenTaskManagerWindow();
            return true;
        case IDM_INJECT_SCRIPT_FRAME:
            InjectScriptInIFrame();
            return true;
        case IDM_POST_WEB_MESSAGE_STRING_FRAME:
            SendStringWebMessageIFrame();
            return true;
        case IDM_POST_WEB_MESSAGE_JSON_FRAME:
            SendJsonWebMessageIFrame();
            return true;
        }
    }
    return false;
}

//! [ExecuteScript]
// Prompt the user for some script and then execute it.
void ScriptComponent::InjectScript()
{
    TextInputDialog dialog(
        m_appWindow->GetMainWindow(),
        L"Inject Script",
        L"Enter script code:",
        L"Enter the JavaScript code to run in the webview.",
        L"window.getComputedStyle(document.body).backgroundColor");
    if (dialog.confirmed)
    {
        m_webView->ExecuteScript(dialog.input.c_str(),
            Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                [appWindow = m_appWindow](HRESULT error, PCWSTR result) -> HRESULT
        {
            if (error != S_OK) {
                ShowFailure(error, L"ExecuteScript failed");
            }
            appWindow->AsyncMessageBox(result, L"ExecuteScript Result");
            return S_OK;
        }).Get());
    }
}
//! [ExecuteScript]
void ScriptComponent::InjectScriptInIFrame()
{
    std::wstring iframesData = IFramesToString();
    std::wstring iframesInfo =
        L"Enter iframe to run the JavaScript code in.\r\nAvailable iframes:" +
        (m_frames.size() > 0 ? iframesData : L"not available at this page.");
    TextInputDialog dialogIFrame(
        m_appWindow->GetMainWindow(), L"Inject Script Into IFrame", L"Enter iframe number:",
        iframesInfo.c_str(), L"0");
    if (dialogIFrame.confirmed)
    {
        int index = -1;
        try
        {
            index = std::stoi(dialogIFrame.input);
        }
        catch (std::exception)
        {
        }

        if (index < 0 || index >= static_cast<int>(m_frames.size()))
        {
            ShowFailure(S_OK, L"Can not read frame index or it is out of available range");
            return;
        }

        std::wstring iframesEnterCode =
            L"Enter the JavaScript code to run in the iframe " + dialogIFrame.input;
        TextInputDialog dialogScript(
            m_appWindow->GetMainWindow(), L"Inject Script Into IFrame", L"Enter script code:",
            iframesEnterCode.c_str(),
            L"window.getComputedStyle(document.body).backgroundColor");
        if (dialogScript.confirmed)
        {
            wil::com_ptr<ICoreWebView2Frame2> frame2 =
                m_frames[index].try_query<ICoreWebView2Frame2>();
            if (frame2)
            {
                frame2->ExecuteScript(
                    dialogScript.input.c_str(),
                    Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                        [this](HRESULT error, PCWSTR result) -> HRESULT {
                            m_appWindow->RunAsync([error, result = std::wstring(result)]
                            {
                                if (error != S_OK)
                                {
                                    ShowFailure(error, L"ExecuteScript failed");
                                }
                                else
                                {
                                    MessageBox(nullptr, result.c_str(), L"ExecuteScript Result", MB_OK);
                                }
                            });
                            return S_OK;
                        })
                        .Get());
            }
        }
    }
}

//! [AddScriptToExecuteOnDocumentCreated]
// Prompt the user for some script and register it to execute whenever a new page loads.
void ScriptComponent::AddInitializeScript()
{
    TextInputDialog dialog(
        m_appWindow->GetMainWindow(),
        L"Add Initialize Script",
        L"Initialization Script:",
        L"Enter the JavaScript code to run as the initialization script that "
            L"runs before any script in the HTML document.",
    // This example script stops child frames from opening new windows.  Because
    // the initialization script runs before any script in the HTML document, we
    // can trust the results of our checks on window.parent and window.top.
        L"if (window.parent !== window.top) {\r\n"
        L"    delete window.open;\r\n"
        L"}");
    if (dialog.confirmed)
    {
        m_webView->AddScriptToExecuteOnDocumentCreated(
            dialog.input.c_str(),
            Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(
                [this](HRESULT error, PCWSTR id) -> HRESULT
        {
            m_lastInitializeScriptId = id;
            m_appWindow->AsyncMessageBox(
                m_lastInitializeScriptId, L"AddScriptToExecuteOnDocumentCreated Id");
            return S_OK;
        }).Get());

    }
}
//! [AddScriptToExecuteOnDocumentCreated]

// Prompt the user for an initialization script ID and deregister that script.
void ScriptComponent::RemoveInitializeScript()
{
    TextInputDialog dialog(
        m_appWindow->GetMainWindow(),
        L"Remove Initialize Script",
        L"Script ID:",
        L"Enter the ID created from Add Initialize Script.",
        m_lastInitializeScriptId.c_str());
    if (dialog.confirmed)
    {
        m_webView->RemoveScriptToExecuteOnDocumentCreated(dialog.input.c_str());
    }
}


// Prompt the user for a string and then post it as a web message.
void ScriptComponent::SendStringWebMessage()
{
    TextInputDialog dialog(
        m_appWindow->GetMainWindow(),
        L"Post Web Message String",
        L"Web message string:",
        L"Enter the web message as a string.");
    if (dialog.confirmed)
    {
        m_webView->PostWebMessageAsString(dialog.input.c_str());
    }
}

// Prompt the user for some JSON and then post it as a web message.
void ScriptComponent::SendJsonWebMessage()
{
    TextInputDialog dialog(
        m_appWindow->GetMainWindow(),
        L"Post Web Message JSON",
        L"Web message JSON:",
        L"Enter the web message as JSON.",
        L"{\"SetColor\":\"blue\"}");
    if (dialog.confirmed)
    {
        m_webView->PostWebMessageAsJson(dialog.input.c_str());
    }
}

// Prompt the user for a string and then post it as a web message to the first iframe.
void ScriptComponent::SendStringWebMessageIFrame()
{
    TextInputDialog dialog(
        m_appWindow->GetMainWindow(), L"Post Web Message String IFrame", L"Web message string:",
        L"Enter the web message as a string.");
    if (dialog.confirmed)
    {
        if (!m_frames.empty())
        {
            wil::com_ptr<ICoreWebView2Frame2> frame2 =
                m_frames[0].try_query<ICoreWebView2Frame2>();
            if (frame2)
            {
                frame2->PostWebMessageAsString(dialog.input.c_str());
            }
        } else {
            ShowFailure(S_OK, L"No iframes found");
        }
    }
}

// Prompt the user for some JSON and then post it as a web message to the first iframe.
void ScriptComponent::SendJsonWebMessageIFrame()
{
    TextInputDialog dialog(
        m_appWindow->GetMainWindow(), L"Post Web Message JSON IFrame", L"Web message JSON:",
        L"Enter the web message as JSON.", L"{\"SetColor\":\"blue\"}");
    if (dialog.confirmed)
    {
        if (!m_frames.empty())
        {
            wil::com_ptr<ICoreWebView2Frame2> frame2 =
                m_frames[0].try_query<ICoreWebView2Frame2>();
            if (frame2)
            {
                frame2->PostWebMessageAsJson(dialog.input.c_str());
            }
        }
        else {
            ShowFailure(S_OK, L"No iframes found");
        }
    }
}

//! [DevToolsProtocolMethodMultiSession]
void ScriptComponent::HandleCDPTargets()
{
    wil::com_ptr<ICoreWebView2DevToolsProtocolEventReceiver> receiver;
    // Enable Runtime events to receive Runtime.consoleAPICalled events.
    m_webView->CallDevToolsProtocolMethod(L"Runtime.enable", L"{}", nullptr);
    CHECK_FAILURE(
        m_webView->GetDevToolsProtocolEventReceiver(L"Runtime.consoleAPICalled", &receiver));
    CHECK_FAILURE(receiver->add_DevToolsProtocolEventReceived(
        Callback<ICoreWebView2DevToolsProtocolEventReceivedEventHandler>(
            [this](
                ICoreWebView2* sender,
                ICoreWebView2DevToolsProtocolEventReceivedEventArgs* args) -> HRESULT
            {
                // Get console.log message details and which target it comes from.
                wil::unique_cotaskmem_string parameterObjectAsJson;
                CHECK_FAILURE(args->get_ParameterObjectAsJson(&parameterObjectAsJson));
                std::wstring eventSourceLabel;
                std::wstring eventDetails = parameterObjectAsJson.get();
                wil::com_ptr<ICoreWebView2DevToolsProtocolEventReceivedEventArgs2> args2;
                if (SUCCEEDED(args->QueryInterface(IID_PPV_ARGS(&args2))))
                {
                    wil::unique_cotaskmem_string sessionId;
                    CHECK_FAILURE(args2->get_SessionId(&sessionId));
                    if (sessionId.get() && *sessionId.get())
                    {
                        std::wstring targetId = m_devToolsSessionMap[sessionId.get()];
                        eventSourceLabel = m_devToolsTargetLabelMap[targetId];
                    }
                }
                // else, leave eventSourceLabel as empty string for the default target of top
                // page.

                // Log events to debug output, not using dialog as there could be a lot of
                // console.log events.
                std::wstring message = L"console.log Event: ";
                if (!eventSourceLabel.empty())
                {
                    message = message + L"(from " + eventSourceLabel + L")";
                }
                message += eventDetails + L"\n";
                OutputDebugString(message.c_str());
                return S_OK;
            })
            .Get(),
        &m_consoleAPICalledToken));
    receiver.reset();
    // Track Target and session info via CDP events.
    CHECK_FAILURE(
        m_webView->GetDevToolsProtocolEventReceiver(L"Target.attachedToTarget", &receiver));
    CHECK_FAILURE(receiver->add_DevToolsProtocolEventReceived(
        Callback<ICoreWebView2DevToolsProtocolEventReceivedEventHandler>(
            [this](
                ICoreWebView2* sender,
                ICoreWebView2DevToolsProtocolEventReceivedEventArgs* args) -> HRESULT
            {
                // A new target is attached, add its info to maps.
                wil::unique_cotaskmem_string jsonMessage;
                CHECK_FAILURE(args->get_ParameterObjectAsJson(&jsonMessage));
                std::wstring sessionId = GetJSONStringField(jsonMessage.get(), L"sessionId");
                std::wstring targetId = GetJSONStringField(jsonMessage.get(), L"targetId");
                m_devToolsSessionMap[sessionId] = targetId;
                std::wstring type = GetJSONStringField(jsonMessage.get(), L"type");
                std::wstring url = GetJSONStringField(jsonMessage.get(), L"url");
                m_devToolsTargetLabelMap.insert_or_assign(targetId, type + L"," + url);
                wil::com_ptr<ICoreWebView2_11> webview2 =
                    m_webView.try_query<ICoreWebView2_11>();
                if (webview2)
                {
                    // Auto-attach to targets further created from this target (identified by
                    // its session ID), like dedicated worker target created in the iframe.
                    webview2->CallDevToolsProtocolMethodForSession(
                        sessionId.c_str(), L"Target.setAutoAttach",
                        LR"({"autoAttach":true,"waitForDebuggerOnStart":false,"flatten":true})",
                        nullptr);
                    // Also enable Runtime events to receive Runtime.consoleAPICalled from the
                    // target.
                    webview2->CallDevToolsProtocolMethodForSession(
                        sessionId.c_str(), L"Runtime.enable", L"{}", nullptr);
                }
                return S_OK;
            })
            .Get(),
        &m_targetAttachedToken));
    receiver.reset();
    CHECK_FAILURE(
        m_webView->GetDevToolsProtocolEventReceiver(L"Target.detachedFromTarget", &receiver));
    CHECK_FAILURE(receiver->add_DevToolsProtocolEventReceived(
        Callback<ICoreWebView2DevToolsProtocolEventReceivedEventHandler>(
            [this](
                ICoreWebView2* sender,
                ICoreWebView2DevToolsProtocolEventReceivedEventArgs* args) -> HRESULT
            {
                // A target is detached, remove it from the maps.
                wil::unique_cotaskmem_string jsonMessage;
                CHECK_FAILURE(args->get_ParameterObjectAsJson(&jsonMessage));
                std::wstring sessionId = GetJSONStringField(jsonMessage.get(), L"sessionId");
                auto session = m_devToolsSessionMap.find(sessionId);
                if (session != m_devToolsSessionMap.end())
                {
                    m_devToolsTargetLabelMap.erase(session->second);
                    m_devToolsSessionMap.erase(session);
                }
                return S_OK;
            })
            .Get(),
        &m_targetDetachedToken));
    receiver.reset();
    CHECK_FAILURE(
        m_webView->GetDevToolsProtocolEventReceiver(L"Target.targetCreated", &receiver));
    CHECK_FAILURE(receiver->add_DevToolsProtocolEventReceived(
        Callback<ICoreWebView2DevToolsProtocolEventReceivedEventHandler>(
            [this](
                ICoreWebView2* sender,
                ICoreWebView2DevToolsProtocolEventReceivedEventArgs* args) -> HRESULT
            {
                // Shared worker targets are not auto attached. Have to attach it explicitly.
                wil::unique_cotaskmem_string jsonMessage;
                CHECK_FAILURE(args->get_ParameterObjectAsJson(&jsonMessage));
                std::wstring type = GetJSONStringField(jsonMessage.get(), L"type");
                if (type == L"shared_worker")
                {
                    std::wstring targetId = GetJSONStringField(jsonMessage.get(), L"targetId");
                    std::wstring parameters =
                        L"{\"targetId\":\"" + targetId + L"\",\"flatten\": true}";
                    // Call Target.attachToTarget and ignore returned value, let
                    // Target.attachedToTarget to handle the result.
                    m_webView->CallDevToolsProtocolMethod(
                        L"Target.attachToTarget", parameters.c_str(), nullptr);
                }
                return S_OK;
            })
            .Get(),
        &m_targetCreatedToken));
    receiver.reset();
    CHECK_FAILURE(
        m_webView->GetDevToolsProtocolEventReceiver(L"Target.targetInfoChanged", &receiver));
    CHECK_FAILURE(receiver->add_DevToolsProtocolEventReceived(
        Callback<ICoreWebView2DevToolsProtocolEventReceivedEventHandler>(
            [this](
                ICoreWebView2* sender,
                ICoreWebView2DevToolsProtocolEventReceivedEventArgs* args) -> HRESULT
            {
                // A target's info (such as its URL) has changed, so update its label in the
                // target label map.
                wil::unique_cotaskmem_string jsonMessage;
                CHECK_FAILURE(args->get_ParameterObjectAsJson(&jsonMessage));
                std::wstring targetId = GetJSONStringField(jsonMessage.get(), L"targetId");
                if (m_devToolsTargetLabelMap.find(targetId) !=
                    m_devToolsTargetLabelMap.end())
                {
                    // This is a target that we are interested in, update label.
                    std::wstring type = GetJSONStringField(jsonMessage.get(), L"type");
                    std::wstring url = GetJSONStringField(jsonMessage.get(), L"url");
                    m_devToolsTargetLabelMap[targetId] = type + L"," + url;
                }
                return S_OK;
            })
            .Get(),
        &m_targetInfoChangedToken));
    // Setup CDP targets operation mode.
    // Set auto attach to attach to dedicated worker target.
    m_webView->CallDevToolsProtocolMethod(
        L"Target.setAutoAttach",
        LR"({"autoAttach":true,"waitForDebuggerOnStart":false,"flatten":true})", nullptr);
    // Set setDiscoverTargets to get targetCreated event for shared worker target.
    m_webView->CallDevToolsProtocolMethod(
        L"Target.setDiscoverTargets", LR"({"discover":true})", nullptr);
}
//! [DevToolsProtocolMethodMultiSession]

void ScriptComponent::CollectHeapUsageViaCdp()
{
    if (m_pendingHeapUsageCollectionCount)
    {
        // Already collecting, return
        return;
    }
    wil::com_ptr<ICoreWebView2_11> webview2 = m_webView.try_query<ICoreWebView2_11>();
    CHECK_FEATURE_RETURN_EMPTY(webview2);
    m_pendingHeapUsageCollectionCount = 0;
    m_heapUsageResult.clear();
    m_heapUsageResult.str(L"Heap Usage (KB)");
    m_heapUsageResult << std::endl;
    // Collect main target
    ++m_pendingHeapUsageCollectionCount;
    std::wstring main_targetInfo = L"Main Page";
    m_webView->CallDevToolsProtocolMethod(
        L"Runtime.getHeapUsage", L"{}",
        Callback<ICoreWebView2CallDevToolsProtocolMethodCompletedHandler>(
            [this, main_targetInfo](HRESULT error, PCWSTR resultJson) -> HRESULT
            {
                HandleHeapUsageResult(main_targetInfo, resultJson);
                return S_OK;
            })
            .Get());
    // Collect heap usage for other targets.
    for (auto& target : m_devToolsSessionMap)
    {
        ++m_pendingHeapUsageCollectionCount;
        std::wstring targetLabel = m_devToolsTargetLabelMap[target.second];
        webview2->CallDevToolsProtocolMethodForSession(
            target.first.c_str(), L"Runtime.getHeapUsage", L"{}",
            Callback<ICoreWebView2CallDevToolsProtocolMethodCompletedHandler>(
                [this, targetLabel](HRESULT error, PCWSTR resultJson) -> HRESULT
                {
                    HandleHeapUsageResult(targetLabel, resultJson);
                    return S_OK;
                })
                .Get());
    }
}

void ScriptComponent::HandleHeapUsageResult(std::wstring targetInfo, PCWSTR resultJson)
{
    int64_t totalSize = GetJSONIntegerField(resultJson, L"totalSize");
    int64_t usedSize = GetJSONIntegerField(resultJson, L"usedSize");
    m_heapUsageResult << L"total:";
    m_heapUsageResult.width(8);
    m_heapUsageResult << (totalSize / 1024);
    m_heapUsageResult << L", used:";
    m_heapUsageResult.width(8);
    m_heapUsageResult << (usedSize / 1024);
    m_heapUsageResult << L", ";
    m_heapUsageResult << targetInfo;
    m_heapUsageResult << std::endl;
    if (--m_pendingHeapUsageCollectionCount == 0)
    {
        MessageBox(nullptr, m_heapUsageResult.str().c_str(), L"Heap Usage", MB_OK);
    }
}

    //! [DevToolsProtocolEventReceived]
// Prompt the user to name a CDP event, and then subscribe to that event.
void ScriptComponent::SubscribeToCdpEvent()
{
    TextInputDialog dialog(
        m_appWindow->GetMainWindow(),
        L"Subscribe to CDP Event",
        L"CDP event name:",
        L"Enter the name of the CDP event to subscribe to.\r\n"
            L"You may also have to call the \"enable\" method of the\r\n"
            L"event's domain to receive events (for example \"Log.enable\").\r\n",
        L"Log.entryAdded");
    if (dialog.confirmed)
    {
		std::wstring eventName = dialog.input;
        wil::com_ptr<ICoreWebView2DevToolsProtocolEventReceiver> receiver;
        CHECK_FAILURE(
            m_webView->GetDevToolsProtocolEventReceiver(eventName.c_str(), &receiver));

        // If we are already subscribed to this event, unsubscribe first.
        auto preexistingToken = m_devToolsProtocolEventReceivedTokenMap.find(eventName);
        if (preexistingToken != m_devToolsProtocolEventReceivedTokenMap.end())
        {
            CHECK_FAILURE(receiver->remove_DevToolsProtocolEventReceived(
                preexistingToken->second));
        }

        CHECK_FAILURE(receiver->add_DevToolsProtocolEventReceived(
            Callback<ICoreWebView2DevToolsProtocolEventReceivedEventHandler>(
                [this, eventName](
                    ICoreWebView2* sender,
                    ICoreWebView2DevToolsProtocolEventReceivedEventArgs* args) -> HRESULT 
                {
                    wil::unique_cotaskmem_string parameterObjectAsJson;
                    CHECK_FAILURE(args->get_ParameterObjectAsJson(&parameterObjectAsJson));
                    std::wstring title = eventName;
                    std::wstring details = parameterObjectAsJson.get();
                    //! [DevToolsProtocolEventReceivedSessionId]
                    wil::com_ptr<ICoreWebView2DevToolsProtocolEventReceivedEventArgs2> args2;
                    if (SUCCEEDED(args->QueryInterface(IID_PPV_ARGS(&args2))))
                    {
                        wil::unique_cotaskmem_string sessionId;
                        CHECK_FAILURE(args2->get_SessionId(&sessionId));
                        if (sessionId.get() && *sessionId.get())
                        {
                            title = eventName + L" (session:" + sessionId.get() + L")";
                            std::wstring targetId = m_devToolsSessionMap[sessionId.get()];
                            std::wstring targetLabel = m_devToolsTargetLabelMap[targetId];
                            details = L"From " + targetLabel + L" (session:" + sessionId.get() +
                                      L")\r\n" + details;
                        }
                    }
                    //! [DevToolsProtocolEventReceivedSessionId]
                    m_appWindow->AsyncMessageBox(details, L"CDP Event Fired: " + title);
                    return S_OK;
                })
                .Get(),
            &m_devToolsProtocolEventReceivedTokenMap[eventName]));
    }
}
//! [DevToolsProtocolEventReceived]

//! [CallDevToolsProtocolMethod]
// Prompt the user for the name and parameters of a CDP method, then call it.
void ScriptComponent::CallCdpMethod()
{
    TextInputDialog dialog(
        m_appWindow->GetMainWindow(),
        L"Call CDP Method",
        L"CDP method name:",
        L"Enter the CDP method name to call, followed by a space,\r\n"
            L"followed by the parameters in JSON format.",
        L"Runtime.evaluate {\"expression\":\"alert(\\\"test\\\")\"}");
    if (dialog.confirmed)
    {
        size_t delimiterPos = dialog.input.find(L' ');
        std::wstring methodName = dialog.input.substr(0, delimiterPos);
        std::wstring methodParams =
            (delimiterPos < dialog.input.size()
                ? dialog.input.substr(delimiterPos + 1)
                : L"{}");

        m_webView->CallDevToolsProtocolMethod(
            methodName.c_str(),
            methodParams.c_str(),
            Callback<ICoreWebView2CallDevToolsProtocolMethodCompletedHandler>(
                [this](HRESULT error, PCWSTR resultJson) -> HRESULT
                {
                    m_appWindow->AsyncMessageBox(resultJson, L"CDP method call result");
                    return S_OK;
                }).Get());
    }
}
//! [CallDevToolsProtocolMethod]

//! [CallDevToolsProtocolMethodForSession]
// Prompt the user for the sessionid, name and parameters of a CDP method, then call it.
void ScriptComponent::CallCdpMethodForSession()
{
    wil::com_ptr<ICoreWebView2_11> webview2 = m_webView.try_query<ICoreWebView2_11>();
    CHECK_FEATURE_RETURN_EMPTY(webview2);
    std::wstring sessionList = L"Sessions:";
    for (auto& target : m_devToolsSessionMap)
    {
        sessionList += L"\r\n";
        sessionList += target.first;
        sessionList += L":";
        sessionList += m_devToolsTargetLabelMap[target.second];
    }
    std::wstring description =
        L"Enter the sessionId, CDP method name to call, and parameters in JSON format, "
        L"separated by space,\r\n" +
        sessionList;
    TextInputDialog dialog(
        m_appWindow->GetMainWindow(), L"Call CDP Method For Session", L"Parameters:",
        description.c_str(),
        L"<sessionId> Runtime.getHeapUsage {}");
    if (dialog.confirmed)
    {
        size_t delimiter1Pos = dialog.input.find(L' ');
        std::wstring sessionId = dialog.input.substr(0, delimiter1Pos);
        size_t delimiter2Pos = dialog.input.find(L' ', delimiter1Pos+1);
        std::wstring methodName =
            dialog.input.substr(delimiter1Pos+1, delimiter2Pos - delimiter1Pos - 1);
        std::wstring methodParams =
            (delimiter2Pos < dialog.input.size() ? dialog.input.substr(delimiter2Pos + 1)
                                                : L"{}");

        webview2->CallDevToolsProtocolMethodForSession(
            sessionId.c_str(), methodName.c_str(), methodParams.c_str(),
            Callback<ICoreWebView2CallDevToolsProtocolMethodCompletedHandler>(
                [this](HRESULT error, PCWSTR resultJson) -> HRESULT
                {
                    m_appWindow->AsyncMessageBox(resultJson, L"CDP method call result");
                    return S_OK;
                })
                .Get());
    }
}
//! [CallDevToolsProtocolMethodForSession]


void ScriptComponent::AddComObject()
{
    TextInputDialog dialog(
        m_appWindow->GetMainWindow(),
        L"Add COM object",
        L"CLSID or ProgID of COM object:",
        L"Enter the CLSID (eg '{0002DF01-0000-0000-C000-000000000046}')\r\n"
            L"or ProgID (eg 'InternetExplorer.Application') of the COM object to create and\r\n"
            L"provide to the WebView as `window.chrome.remoteObjects.example`.",
        L"InternetExplorer.Application");
    if (dialog.confirmed)
    {
        CLSID classId = {};
        HRESULT hr = CLSIDFromProgID(dialog.input.c_str(), &classId);
        if (FAILED(hr))
        {
            hr = CLSIDFromString(dialog.input.c_str(), &classId);
        }

        if (SUCCEEDED(hr))
        {
            wil::com_ptr_nothrow<IDispatch> objectAsDispatch;
            hr = CoCreateInstance(
                classId,
                nullptr,
                CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                IID_PPV_ARGS(&objectAsDispatch));
            if (SUCCEEDED(hr))
            {
                wil::unique_variant objectAsVariant;
                objectAsVariant.vt = VT_DISPATCH;
                hr = objectAsDispatch.query_to(IID_PPV_ARGS(&objectAsVariant.pdispVal));
                if (SUCCEEDED(hr))
                {
                    hr = m_webView->AddHostObjectToScript(L"example", &objectAsVariant);
                    if (FAILED(hr))
                    {
                        ShowFailure(hr, L"AddHostObjectToScript failed");
                    }
                }
                else
                {
                    ShowFailure(hr, L"COM object doesn't support IDispatch");
                }
            }
            else
            {
                ShowFailure(hr, L"CoCreateInstance failed");
            }
        }
        else
        {
            ShowFailure(hr, L"Failed to convert string to CLSID or ProgID");
        }
    }
}

void ScriptComponent::AddSiteEmbeddingIFrame()
{
    // Prompt the user for which site to embed in the iframe.
    TextInputDialog dialog(
        m_appWindow->GetMainWindow(), L"Inject Site Embedding iframe", L"Enter iframe url:",
        L"Enter the url for the injected iframe.", siteToEmbed.c_str());
    if (dialog.confirmed)
    {
        std::wstring script =
            L"(() => { const iframe = document.createElement('iframe'); iframe.src = '";
        script += dialog.input;
        script +=
            L"'; iframe.name='my_site_embedding_frame'; document.body.appendChild(iframe); })()";
        m_webView->ExecuteScript(script.c_str(), nullptr);
    }
}


void ScriptComponent::HandleIFrames()
{
    wil::com_ptr<ICoreWebView2_4> webview2_4 = m_webView.try_query<ICoreWebView2_4>();
    if (webview2_4)
    {
        CHECK_FAILURE(webview2_4->add_FrameCreated(
            Callback<ICoreWebView2FrameCreatedEventHandler>(
                [this](
                    ICoreWebView2* sender, ICoreWebView2FrameCreatedEventArgs* args) -> HRESULT
                {
                    wil::com_ptr<ICoreWebView2Frame> webviewFrame;
                    CHECK_FAILURE(args->get_Frame(&webviewFrame));

                    m_frames.emplace_back(webviewFrame);

                    webviewFrame->add_Destroyed(
                        Callback<ICoreWebView2FrameDestroyedEventHandler>(
                            [this](ICoreWebView2Frame* sender, IUnknown* args) -> HRESULT
                            {
                                auto frame =
                                    std::find(m_frames.begin(), m_frames.end(), sender);
                                if (frame != m_frames.end())
                                {
                                    m_frames.erase(frame);
                                }
                                return S_OK;
                            })
                            .Get(),
                        NULL);
                    return S_OK;
                })
                .Get(),
            NULL));

        //! [AdditionalAllowedFrameAncestors_2]
        // Set up the event listeners to handle site embedding scenario. The code will take effect
        // when the site embedding page is navigated to and the embedding iframe navigates to the
        // site that we want to embed.

        // This part is trying to scope the API usage to the specific scenario where we are
        // embedding a site. The result is recorded in m_siteEmbeddingIFrameCount.
        CHECK_FAILURE(webview2_4->add_FrameCreated(
            Callback<ICoreWebView2FrameCreatedEventHandler>(
                [this](ICoreWebView2* sender, ICoreWebView2FrameCreatedEventArgs* args)
                    -> HRESULT 
                {
                    wil::com_ptr<ICoreWebView2Frame> webviewFrame;
                    CHECK_FAILURE(args->get_Frame(&webviewFrame));
                    wil::unique_cotaskmem_string page_url;
                    CHECK_FAILURE(m_webView->get_Source(&page_url));
                    // IsAppContentUri verifies that page_url belongs to  the app.
                    if (IsAppContentUri(page_url.get()))
                    {
                        // We are on trusted pages. Now check whether it is the iframe we plan
                        // to embed arbitrary sites.
                        wil::unique_cotaskmem_string frame_name;
                        CHECK_FAILURE(webviewFrame->get_Name(&frame_name));
                        if (siteEmbeddingFrameName == frame_name.get())
                        {
                            ++m_siteEmbeddingIFrameCount;
                            CHECK_FAILURE(webviewFrame->add_Destroyed(
                                Microsoft::WRL::Callback<
                                    ICoreWebView2FrameDestroyedEventHandler>(
                                    [this](
                                        ICoreWebView2Frame* sender, IUnknown* args) -> HRESULT
                                    {
                                        --m_siteEmbeddingIFrameCount;
                                        return S_OK;
                                    })
                                    .Get(),
                                nullptr));
                        }
                    }
                    return S_OK;
                })
                .Get(),
            nullptr));

        // Using FrameNavigationStarting event instead of NavigationStarting event of
        // CoreWebViewFrame to cover all possible nested iframes inside the embedded site as
        // CoreWebViewFrame object currently only support first level iframes in the top page.
        CHECK_FAILURE(m_webView->add_FrameNavigationStarting(
            Microsoft::WRL::Callback<ICoreWebView2NavigationStartingEventHandler>(
                [this](ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args)
                    -> HRESULT
                {
                    if (m_siteEmbeddingIFrameCount > 0)
                    {
                        wil::unique_cotaskmem_string navigationTargetUri;
                        CHECK_FAILURE(args->get_Uri(&navigationTargetUri));
                        if (IsTargetSite(navigationTargetUri.get()))
                        {
                            wil::com_ptr<ICoreWebView2NavigationStartingEventArgs2>
                                navigationStartArgs;
                            if (SUCCEEDED(args->QueryInterface(IID_PPV_ARGS(&navigationStartArgs))))
                            {
                                navigationStartArgs->put_AdditionalAllowedFrameAncestors(
                                    myTrustedSite.c_str());
                            }
                        }
                    }
                    return S_OK;
                })
                .Get(),
            nullptr));
    //! [AdditionalAllowedFrameAncestors_2]
    }
}

std::wstring ScriptComponent::IFramesToString()
{
    std::wstring data;
    for (size_t i = 0; i < m_frames.size(); i++)
    {
        wil::unique_cotaskmem_string name;
        CHECK_FAILURE(m_frames[i]->get_Name(&name));
        if (i > 0)
            data += L"; ";
        data += std::to_wstring(i) + L": " +
                (!std::wcslen(name.get()) ? L"<empty_name>" : name.get());
    }
    return data;
}

void ScriptComponent::OpenTaskManagerWindow()
{
    auto webView6 = m_webView.try_query<ICoreWebView2_6>();

    if (webView6)
    {
        webView6->OpenTaskManagerWindow();
    }
}

ScriptComponent::~ScriptComponent()
{
    for (auto& pair : m_devToolsProtocolEventReceivedTokenMap)
    {
        wil::com_ptr<ICoreWebView2DevToolsProtocolEventReceiver> receiver;
        CHECK_FAILURE(
            m_webView->GetDevToolsProtocolEventReceiver(pair.first.c_str(), &receiver));
        receiver->remove_DevToolsProtocolEventReceived(pair.second);
    }
    wil::com_ptr<ICoreWebView2DevToolsProtocolEventReceiver> receiver;
    // WebView could have been closed at this time, so only proceed if first
    // GetDevToolsProtocolEventReceiver succeeded.
    if (SUCCEEDED(
            m_webView->GetDevToolsProtocolEventReceiver(L"Target.attachedToTarget", &receiver)))
    {
        receiver->remove_DevToolsProtocolEventReceived(m_targetAttachedToken);
        receiver.reset();
        CHECK_FAILURE(m_webView->GetDevToolsProtocolEventReceiver(
            L"Target.detachedFromTarget", &receiver));
        receiver->remove_DevToolsProtocolEventReceived(m_targetDetachedToken);
        receiver.reset();
        CHECK_FAILURE(
            m_webView->GetDevToolsProtocolEventReceiver(L"Target.targetCreated", &receiver));
        receiver->remove_DevToolsProtocolEventReceived(m_targetCreatedToken);
        receiver.reset();
        CHECK_FAILURE(m_webView->GetDevToolsProtocolEventReceiver(
            L"Target.targetInfoChanged", &receiver));
        receiver->remove_DevToolsProtocolEventReceived(m_targetInfoChangedToken);
        receiver.reset();
        CHECK_FAILURE(m_webView->GetDevToolsProtocolEventReceiver(
            L"Runtime.consoleAPICalled", &receiver));
        receiver->remove_DevToolsProtocolEventReceived(m_consoleAPICalledToken);
    }
}
