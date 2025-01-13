// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "stdafx.h"

#include "CheckFailure.h"

#include "ScenarioDragDropOverride.h"
using namespace Microsoft::WRL;

static constexpr WCHAR c_samplePath[] = L"ScenarioDragDropOverride.html";
static constexpr WCHAR c_webmessageDefault[] = L"default";
static constexpr WCHAR c_webmessageOverride[] = L"override";
static constexpr WCHAR c_webmessageNoop[] = L"noop";

ScenarioDragDropOverride::ScenarioDragDropOverride(AppWindow* appWindow)
    : m_appWindow(appWindow)
{
    m_sampleUri = m_appWindow->GetLocalUri(c_samplePath);
    m_webView = m_appWindow->GetWebView();
    wil::com_ptr<ICoreWebView2Controller> controller = appWindow->GetWebViewController();
    wil::com_ptr<ICoreWebView2CompositionController> compController =
        controller.try_query<ICoreWebView2CompositionController>();
    if (!compController)
    {
        return;
    }

    m_compControllerExperimental6 =
        compController.try_query<ICoreWebView2ExperimentalCompositionController6>();
    if (!m_compControllerExperimental6)
    {
        return;
    }

    CHECK_FAILURE(m_webView->add_WebMessageReceived(
        Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args)
            {
                wil::unique_cotaskmem_string uri;
                CHECK_FAILURE(args->get_Source(&uri));

                // Always validate that the origin of the message is what you expect.
                if (uri.get() != m_sampleUri)
                {
                    return S_OK;
                }
                wil::unique_cotaskmem_string messageRaw;
                CHECK_FAILURE(args->TryGetWebMessageAsString(&messageRaw));
                std::wstring message = messageRaw.get();

                if (message == c_webmessageDefault)
                {
                    m_dragOverrideMode = DragOverrideMode::DEFAULT;
                }
                else if (message == c_webmessageOverride)
                {
                    m_dragOverrideMode = DragOverrideMode::OVERRIDE;
                }
                else if (message == c_webmessageNoop)
                {
                    m_dragOverrideMode = DragOverrideMode::NOOP;
                }

                return S_OK;
            })
            .Get(),
        &m_webMessageReceivedToken));

    //! [DragStarting]
    // Using DragStarting to simply make a synchronous DoDragDrop call instead of
    // having WebView2 do it.
    CHECK_FAILURE(m_compControllerExperimental6->add_DragStarting(
        Callback<ICoreWebView2ExperimentalDragStartingEventHandler>(
            [this](
                ICoreWebView2CompositionController* sender,
                ICoreWebView2ExperimentalDragStartingEventArgs* args)
            {
                if (m_dragOverrideMode != DragOverrideMode::OVERRIDE)
                {
                    // If the event is marked handled, WebView2 will not execute its drag logic.
                    args->put_Handled(m_dragOverrideMode == DragOverrideMode::NOOP);
                    return S_OK;
                }

                wil::com_ptr<IDataObject> dragData;
                DWORD okEffects = DROPEFFECT_NONE;
                CHECK_FAILURE(args->get_AllowedDropEffects(&okEffects));
                CHECK_FAILURE(args->get_Data(&dragData));

                // This member refers to an implementation of IDropSource. It is an
                // OLE interface that is necessary to initiate drag in an application.
                // https://learn.microsoft.com/en-us/windows/win32/api/oleidl/nn-oleidl-idropsource
                if (!m_dropSource)
                {
                    m_dropSource = Make<ScenarioDragDropOverrideDropSource>();
                }

                DWORD effect = DROPEFFECT_NONE;
                HRESULT hr = DoDragDrop(dragData.get(), m_dropSource.get(), okEffects, &effect);
                args->put_Handled(TRUE);

                return hr;
            })
            .Get(),
        &m_dragStartingToken));
    //! [DragStarting]

    CHECK_FAILURE(m_webView->add_ContentLoading(
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

    CHECK_FAILURE(m_webView->Navigate(m_sampleUri.c_str()));
}

ScenarioDragDropOverride::~ScenarioDragDropOverride()
{
    if (m_compControllerExperimental6)
    {
        CHECK_FAILURE(m_compControllerExperimental6->remove_DragStarting(m_dragStartingToken));
    }
    CHECK_FAILURE(m_webView->remove_WebMessageReceived(m_webMessageReceivedToken));
    CHECK_FAILURE(m_webView->remove_ContentLoading(m_contentLoadingToken));
}
