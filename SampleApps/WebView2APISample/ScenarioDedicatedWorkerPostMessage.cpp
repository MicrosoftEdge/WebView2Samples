// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"
#include <sstream>

#include "CheckFailure.h"
#include "TextInputDialog.h"

#include "ScenarioDedicatedWorkerPostMessage.h"

using namespace Microsoft::WRL;

static constexpr WCHAR c_samplePath[] = L"ScenarioDedicatedWorkerPostMessage.html";

ScenarioDedicatedWorkerPostMessage::ScenarioDedicatedWorkerPostMessage(AppWindow* appWindow)
    : m_appWindow(appWindow)
{
    //! [DedicatedWorkerCreated]
    m_appWindow->GetWebView()->QueryInterface(IID_PPV_ARGS(&m_webView2Experimental_30));
    CHECK_FEATURE_RETURN_EMPTY(m_webView2Experimental_30);

    CHECK_FAILURE(m_webView2Experimental_30->add_DedicatedWorkerCreated(
        Callback<ICoreWebView2ExperimentalDedicatedWorkerCreatedEventHandler>(
            [this](
                ICoreWebView2* sender,
                ICoreWebView2ExperimentalDedicatedWorkerCreatedEventArgs* args)
            {
                wil::com_ptr<ICoreWebView2ExperimentalDedicatedWorker> dedicatedWorker;
                CHECK_FAILURE(args->get_Worker(&dedicatedWorker));

                wil::unique_cotaskmem_string scriptUri;
                CHECK_FAILURE(dedicatedWorker->get_ScriptUri(&scriptUri));

                std::wstring scriptUriStr(scriptUri.get());
                m_appWindow->AsyncMessageBox(scriptUriStr, L"Dedicated worker is created");

                SetupEventsOnDedicatedWorker(dedicatedWorker);
                ComputeWithDedicatedWorker(dedicatedWorker);

                return S_OK;
            })
            .Get(),
        &m_dedicatedWorkerCreatedToken));
    //! [DedicatedWorkerCreated]

    // Turn off this scenario if we navigate away from the sample page.
    CHECK_FAILURE(m_appWindow->GetWebView()->add_ContentLoading(
        Callback<ICoreWebView2ContentLoadingEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2ContentLoadingEventArgs* args) -> HRESULT
            {
                wil::unique_cotaskmem_string uri;
                sender->get_Source(&uri);
                if (uri.get() != m_sampleUri)
                {
                    m_appWindow->DeleteComponent(this);
                }
                return S_OK;
            })
            .Get(),
        &m_contentLoadingToken));

    m_sampleUri = m_appWindow->GetLocalUri(c_samplePath);
    CHECK_FAILURE(m_appWindow->GetWebView()->Navigate(m_sampleUri.c_str()));
}

void ScenarioDedicatedWorkerPostMessage::SetupEventsOnDedicatedWorker(
    wil::com_ptr<ICoreWebView2ExperimentalDedicatedWorker> dedicatedWorker)
{
    //! [WebMessageReceived]
    dedicatedWorker->add_WebMessageReceived(
        Callback<ICoreWebView2ExperimentalDedicatedWorkerWebMessageReceivedEventHandler>(
            [this](
                ICoreWebView2ExperimentalDedicatedWorker* sender,
                ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT
            {
                wil::unique_cotaskmem_string scriptUri;
                CHECK_FAILURE(args->get_Source(&scriptUri));

                wil::unique_cotaskmem_string messageRaw;
                CHECK_FAILURE(args->TryGetWebMessageAsString(&messageRaw));
                std::wstring messageFromWorker = messageRaw.get();

                std::wstringstream message{};
                message << L"Dedicated Worker: " << std::endl << scriptUri.get() << std::endl;
                message << std::endl;
                message << L"Message: " << std::endl << messageFromWorker << std::endl;
                m_appWindow->AsyncMessageBox(message.str(), L"Message from Dedicated Worker");

                return S_OK;
            })
            .Get(),
        nullptr);
    //! [WebMessageReceived]
}

void ScenarioDedicatedWorkerPostMessage::ComputeWithDedicatedWorker(
    wil::com_ptr<ICoreWebView2ExperimentalDedicatedWorker> dedicatedWorker)
{
    //! [PostWebMessageAsJson]
    // Do not block from event handler
    m_appWindow->RunAsync(
        [this, dedicatedWorker]
        {
            TextInputDialog dialog(
                m_appWindow->GetMainWindow(), L"Post Web Message JSON", L"Web message JSON",
                L"Enter the web message as JSON.",
                L"{\"command\":\"ADD\",\"first\":2,\"second\":3}");
            // Ex: {"command":"ADD","first":2,"second":3}
            if (dialog.confirmed)
            {
                dedicatedWorker->PostWebMessageAsJson(dialog.input.c_str());
            }
        });
    //! [PostWebMessageAsJson]
}

ScenarioDedicatedWorkerPostMessage::~ScenarioDedicatedWorkerPostMessage()
{
    m_webView2Experimental_30->remove_DedicatedWorkerCreated(m_dedicatedWorkerCreatedToken);
    m_appWindow->GetWebView()->remove_ContentLoading(m_contentLoadingToken);
}
