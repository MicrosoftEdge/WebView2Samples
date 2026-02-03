// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include <functional>
#include <string>
#include <atomic>
#include <wrl\client.h>

#include "SLMHostObject_h.h"
#include "FoundryLocalClient.h"

class SLMHostObject : public Microsoft::WRL::RuntimeClass<
                            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
                            ISLMHostObject, IDispatch>
{
public:
    typedef std::function<void(void)> Callback;
    typedef std::function<void(Callback)> RunCallbackAsync;

    SLMHostObject(RunCallbackAsync runCallbackAsync);
    ~SLMHostObject();

    // ISLMHostObject implementation
    STDMETHODIMP QueryStatus(BSTR* status) override;

    STDMETHODIMP SetupAsync(
        IDispatch* progressCallback,
        IDispatch* completionCallback) override;

    STDMETHODIMP InferAsync(
        BSTR prompt,
        IDispatch* tokenCallback,
        IDispatch* completionCallback) override;

    STDMETHODIMP CancelInference() override;

    STDMETHODIMP SetSystemPrompt(BSTR prompt) override;

    STDMETHODIMP GetSystemPrompt(BSTR* prompt) override;

    STDMETHODIMP IsOnline(VARIANT_BOOL* online) override;

    STDMETHODIMP IsReady(VARIANT_BOOL* ready) override;

    STDMETHODIMP GetModels(BSTR* modelsJson) override;

    STDMETHODIMP SetModel(BSTR alias, VARIANT_BOOL* success) override;

    STDMETHODIMP GetCurrentModel(BSTR* alias) override;

    // IDispatch implementation
    STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) override;

    STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo) override;

    STDMETHODIMP GetIDsOfNames(
        REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId) override;

    STDMETHODIMP Invoke(
        DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams,
        VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) override;

private:
    // Helper to invoke JavaScript callback with a string argument
    void InvokeCallback(IDispatch* callback, const std::wstring& arg);
    
    // Helper to invoke JavaScript callback with progress info
    void InvokeProgressCallback(IDispatch* callback, const std::wstring& stage, double progress, const std::wstring& message);
    
    // Helper to invoke JavaScript completion callback
    void InvokeCompletionCallback(IDispatch* callback, bool success, const std::wstring& error);

    // Get current status string based on state
    std::wstring GetCurrentStatus();

    RunCallbackAsync m_runCallbackAsync;
    wil::com_ptr<ITypeLib> m_typeLib;
    
    // Foundry Local client
    std::unique_ptr<FoundryLocalClient> m_client;
    
    // Current state
    enum class State
    {
        Unknown,
        NotInstalled,
        Installing,
        ServiceStopped,
        StartingService,
        ModelNotCached,
        Downloading,
        Ready,
        Offline,
        Inferring
    };
    std::atomic<State> m_state{State::Unknown};

    // Model alias (synced with client)
    std::wstring m_modelAlias = L"phi-4-mini";
};
