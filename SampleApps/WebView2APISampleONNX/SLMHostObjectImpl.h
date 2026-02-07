// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"
#include "SLMHostObject_h.h"
#include "ONNXRuntimeClient.h"
#include <functional>
#include <vector>
#include <string>
#include <memory>

// Named SLMHostObjectImpl to avoid conflict with SLMHostObject coclass from IDL
class SLMHostObjectImpl : public Microsoft::WRL::RuntimeClass<
    Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
    ISLMHostObject, IDispatch>
{
public:
    SLMHostObjectImpl(std::function<void(std::function<void()>)> runCallbackAsync);
    ~SLMHostObjectImpl();

    // ISLMHostObject implementation
    STDMETHODIMP QueryStatus(BSTR* status) override;
    STDMETHODIMP SetupAsync(IDispatch* progressCallback, IDispatch* completionCallback) override;
    STDMETHODIMP InferAsync(BSTR message, IDispatch* tokenCallback, IDispatch* completionCallback) override;
    STDMETHODIMP CancelInference() override;
    STDMETHODIMP GetModels(BSTR* modelsJson) override;
    STDMETHODIMP SetModel(BSTR alias, VARIANT_BOOL* success) override;
    STDMETHODIMP GetCurrentModel(BSTR* alias) override;

    // IDispatch implementation (for JavaScript interop)
    STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) override;
    STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo) override;
    STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, 
                               LCID lcid, DISPID* rgDispId) override;
    STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
                        DISPPARAMS* pDispParams, VARIANT* pVarResult, 
                        EXCEPINFO* pExcepInfo, UINT* puArgErr) override;

private:
    enum class State
    {
        NotInitialized,
        Initializing,
        Downloading,
        Loading,
        Ready,
        Inferring,
        Error,
        Offline,
        ModelNotDownloaded
    };

    void InvokeCallback(IDispatch* callback, const std::vector<VARIANT>& args);
    void InvokeProgressCallback(IDispatch* callback, const std::wstring& stage, 
                                double progress, const std::wstring& message);
    void InvokeCompletionCallback(IDispatch* callback, bool success, const std::wstring& error);
    std::wstring StateToStatusString(State state) const;

    std::unique_ptr<ONNXRuntimeClient> m_client;
    std::function<void(std::function<void()>)> m_runCallbackAsync;
    State m_state = State::NotInitialized;
    
    // Conversation history for context
    std::vector<ChatMessage> m_conversationHistory;
    
    // Current model alias
    mutable std::wstring m_modelAlias = L"phi-3-mini";
    
    // Type library for IDispatch
    wil::com_ptr<ITypeLib> m_typeLib;
};
