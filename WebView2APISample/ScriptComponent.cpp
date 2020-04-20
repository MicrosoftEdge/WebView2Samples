// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "ScriptComponent.h"

#include "CheckFailure.h"
#include "TextInputDialog.h"

using namespace Microsoft::WRL;

ScriptComponent::ScriptComponent(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{ }

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
        case IDM_ADD_REMOTE_OBJECT:
            AddComObject();
            return true;
        case IDM_OPEN_DEVTOOLS_WINDOW:
            m_webView->OpenDevToolsWindow();
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
                [](HRESULT error, PCWSTR result) -> HRESULT
        {
            if (error != S_OK) {
                ShowFailure(error, L"ExecuteScript failed");
            }
            MessageBox(nullptr, result, L"ExecuteScript Result", MB_OK);
            return S_OK;
        }).Get());
    }
}
//! [ExecuteScript]

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
            MessageBox(nullptr, id, L"AddScriptToExecuteOnDocumentCreated Id", MB_OK);
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
                [eventName](
                    ICoreWebView2* sender,
                    ICoreWebView2DevToolsProtocolEventReceivedEventArgs* args) -> HRESULT {
                    wil::unique_cotaskmem_string parameterObjectAsJson;
                    CHECK_FAILURE(args->get_ParameterObjectAsJson(&parameterObjectAsJson));
                    MessageBox(
                        nullptr, parameterObjectAsJson.get(),
                        (L"CDP Event Fired: " + eventName).c_str(), MB_OK);
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
                [](HRESULT error, PCWSTR resultJson) -> HRESULT
                {
                    MessageBox(nullptr, resultJson, L"CDP Method Result", MB_OK);
                    return S_OK;
                }).Get());
    }
}
//! [CallDevToolsProtocolMethod]

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


ScriptComponent::~ScriptComponent()
{
    for (auto& pair : m_devToolsProtocolEventReceivedTokenMap)
    {
        wil::com_ptr<ICoreWebView2DevToolsProtocolEventReceiver> receiver;
        CHECK_FAILURE(
            m_webView->GetDevToolsProtocolEventReceiver(pair.first.c_str(), &receiver));
        receiver->remove_DevToolsProtocolEventReceived(pair.second);
    }
}
