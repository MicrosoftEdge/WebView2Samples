// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include "stdafx.h"

#include "AppWindow.h"
#include "ComponentBase.h"

class ScenarioDragDropOverride : public ComponentBase
{
public:
    ScenarioDragDropOverride(AppWindow* appWindow);
    ~ScenarioDragDropOverride() override;

private:
    // Barebones IDropSource implementation to serve the example drag drop override.
    class ScenarioDragDropOverrideDropSource
        : public Microsoft::WRL::RuntimeClass<
              Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IDropSource>
    {
    public:
        // IDropSource implementation:
        STDMETHODIMP QueryContinueDrag(BOOL escapePressed, DWORD keyState) override
        {
            if (escapePressed)
            {
                return DRAGDROP_S_CANCEL;
            }
            if (!(keyState & (MK_LBUTTON | MK_RBUTTON)))
            {
                return DRAGDROP_S_DROP;
            }
            return S_OK;
        }

        STDMETHODIMP GiveFeedback(DWORD dwEffect) override
        {
            return DRAGDROP_S_USEDEFAULTCURSORS;
        }
    };

    enum class DragOverrideMode
    {
        DEFAULT = 0,
        OVERRIDE = 1,
        NOOP = 2,
    };

    EventRegistrationToken m_dragStartingToken = {};
    EventRegistrationToken m_webMessageReceivedToken = {};
    EventRegistrationToken m_contentLoadingToken = {};

    AppWindow* m_appWindow = nullptr;
    std::wstring m_sampleUri;
    wil::com_ptr<ICoreWebView2> m_webView = nullptr;
    wil::com_ptr<ICoreWebView2ExperimentalCompositionController6> m_compControllerExperimental6;
    wil::com_ptr<IDropSource> m_dropSource;
    DragOverrideMode m_dragOverrideMode = DragOverrideMode::DEFAULT;
};