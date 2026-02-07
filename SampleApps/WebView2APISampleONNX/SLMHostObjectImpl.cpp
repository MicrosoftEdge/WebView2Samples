// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"
#include "SLMHostObjectImpl.h"
#include <sstream>
#include <thread>

SLMHostObjectImpl::SLMHostObjectImpl(std::function<void(std::function<void()>)> runCallbackAsync)
    : m_runCallbackAsync(runCallbackAsync)
{
    m_client = std::make_unique<ONNXRuntimeClient>();
    m_client->Initialize();
}

SLMHostObjectImpl::~SLMHostObjectImpl()
{
}

std::wstring SLMHostObjectImpl::StateToStatusString(State state) const
{
    switch (state)
    {
        case State::NotInitialized: return L"not_initialized";
        case State::Initializing: return L"initializing";
        case State::Downloading: return L"downloading";
        case State::Loading: return L"loading";
        case State::Ready: return L"ready";
        case State::Inferring: return L"inferring";
        case State::Error: return L"error";
        case State::Offline: return L"offline";
        case State::ModelNotDownloaded: return L"model_not_downloaded";
        default: return L"unknown";
    }
}

STDMETHODIMP SLMHostObjectImpl::QueryStatus(BSTR* status)
{
    if (!status) return E_POINTER;
    
    // Check network
    if (!m_client->IsOnline())
    {
        *status = SysAllocString(L"offline");
        return S_OK;
    }
    
    // Check if model is downloaded
    if (!m_client->IsModelDownloaded(m_modelAlias))
    {
        *status = SysAllocString(L"model_not_downloaded");
        return S_OK;
    }
    
    // Check if model is loaded and ready
    if (m_client->IsModelLoaded())
    {
        *status = SysAllocString(L"ready");
        return S_OK;
    }
    
    *status = SysAllocString(StateToStatusString(m_state).c_str());
    return S_OK;
}

void SLMHostObjectImpl::InvokeCallback(IDispatch* callback, const std::vector<VARIANT>& args)
{
    if (!callback) return;
    
    DISPPARAMS params = {};
    params.cArgs = static_cast<UINT>(args.size());
    
    if (!args.empty())
    {
        // Args need to be in reverse order for IDispatch::Invoke
        std::vector<VARIANT> reversedArgs(args.rbegin(), args.rend());
        params.rgvarg = reversedArgs.data();
        callback->Invoke(DISPID_VALUE, IID_NULL, LOCALE_USER_DEFAULT, 
                         DISPATCH_METHOD, &params, nullptr, nullptr, nullptr);
    }
    else
    {
        callback->Invoke(DISPID_VALUE, IID_NULL, LOCALE_USER_DEFAULT, 
                         DISPATCH_METHOD, &params, nullptr, nullptr, nullptr);
    }
}

void SLMHostObjectImpl::InvokeProgressCallback(IDispatch* callback, const std::wstring& stage,
                                               double progress, const std::wstring& message)
{
    if (!callback) return;
    
    // Create JSON progress object
    std::wostringstream json;
    json << L"{\"stage\":\"" << stage << L"\","
         << L"\"progress\":" << progress << L","
         << L"\"message\":\"" << message << L"\"}";
    
    VARIANT arg;
    VariantInit(&arg);
    arg.vt = VT_BSTR;
    arg.bstrVal = SysAllocString(json.str().c_str());
    
    InvokeCallback(callback, {arg});
    
    VariantClear(&arg);
}

void SLMHostObjectImpl::InvokeCompletionCallback(IDispatch* callback, bool success, 
                                                  const std::wstring& error)
{
    if (!callback) return;
    
    std::wostringstream json;
    json << L"{\"success\":" << (success ? L"true" : L"false");
    if (!success && !error.empty())
    {
        // Escape special characters in error message
        std::wstring escapedError;
        for (wchar_t c : error)
        {
            if (c == L'"') escapedError += L"\\\"";
            else if (c == L'\\') escapedError += L"\\\\";
            else if (c == L'\n') escapedError += L"\\n";
            else if (c == L'\r') escapedError += L"\\r";
            else escapedError += c;
        }
        json << L",\"error\":\"" << escapedError << L"\"";
    }
    json << L"}";
    
    VARIANT arg;
    VariantInit(&arg);
    arg.vt = VT_BSTR;
    arg.bstrVal = SysAllocString(json.str().c_str());
    
    InvokeCallback(callback, {arg});
    
    VariantClear(&arg);
}

STDMETHODIMP SLMHostObjectImpl::SetupAsync(IDispatch* progressCallback, IDispatch* completionCallback)
{
    // AddRef the callbacks to prevent them from being freed
    Microsoft::WRL::ComPtr<IDispatch> progressCb(progressCallback);
    Microsoft::WRL::ComPtr<IDispatch> completionCb(completionCallback);
    
    std::thread([this, progressCb, completionCb]()
    {
        // Step 1: Check network
        if (!m_client->IsOnline())
        {
            m_state = State::Offline;
            m_runCallbackAsync([this, completionCb]() {
                InvokeCompletionCallback(completionCb.Get(), false, 
                    L"No internet connection. Required for first-time model download.");
            });
            return;
        }
        
        m_runCallbackAsync([this, progressCb]() {
            InvokeProgressCallback(progressCb.Get(), L"checking", 10, L"Checking model status...");
        });
        
        // Step 2: Download model if needed
        if (!m_client->IsModelDownloaded(m_modelAlias))
        {
            m_state = State::Downloading;
            m_runCallbackAsync([this, progressCb]() {
                InvokeProgressCallback(progressCb.Get(), L"downloading", 0, 
                    L"Downloading " + m_client->GetCurrentModelDisplayName() + L"...");
            });
            
            bool downloadComplete = false;
            bool downloadSuccess = false;
            std::wstring downloadError;
            
            m_client->DownloadModelAsync(
                m_modelAlias,
                [this, progressCb](double progress) {
                    m_runCallbackAsync([this, progressCb, progress]() {
                        std::wstring msg = L"Downloading " + m_client->GetCurrentModelDisplayName() + 
                                          L"... " + std::to_wstring(static_cast<int>(progress)) + L"%";
                        InvokeProgressCallback(progressCb.Get(), L"downloading", progress, msg);
                    });
                },
                [&downloadComplete, &downloadSuccess, &downloadError](bool success, const std::wstring& error) {
                    downloadSuccess = success;
                    downloadError = error;
                    downloadComplete = true;
                }
            );
            
            // Wait for download to complete
            while (!downloadComplete)
            {
                Sleep(100);
            }
            
            if (!downloadSuccess)
            {
                m_state = State::ModelNotDownloaded;
                m_runCallbackAsync([this, completionCb, downloadError]() {
                    InvokeCompletionCallback(completionCb.Get(), false, 
                        L"Failed to download model: " + downloadError);
                });
                return;
            }
        }
        
        // Step 3: Load model
        m_state = State::Loading;
        m_runCallbackAsync([this, progressCb]() {
            InvokeProgressCallback(progressCb.Get(), L"loading", 80, 
                L"Loading " + m_client->GetCurrentModelDisplayName() + L"...");
        });
        
        if (!m_client->LoadModel(m_modelAlias))
        {
            m_state = State::Error;
            m_runCallbackAsync([this, completionCb]() {
                InvokeCompletionCallback(completionCb.Get(), false, L"Failed to load model");
            });
            return;
        }
        
        // All setup complete
        m_state = State::Ready;
        m_runCallbackAsync([this, progressCb, completionCb]() {
            InvokeProgressCallback(progressCb.Get(), L"ready", 100, L"Ready");
            InvokeCompletionCallback(completionCb.Get(), true, L"");
        });
        
    }).detach();
    
    return S_OK;
}

STDMETHODIMP SLMHostObjectImpl::InferAsync(BSTR message, IDispatch* tokenCallback, 
                                           IDispatch* completionCallback)
{
    if (!message || !tokenCallback || !completionCallback)
        return E_POINTER;
    
    std::wstring userMessage(message);
    
    // Add user message to conversation history
    m_conversationHistory.push_back({L"user", userMessage});
    
    Microsoft::WRL::ComPtr<IDispatch> tokenCb(tokenCallback);
    Microsoft::WRL::ComPtr<IDispatch> completionCb(completionCallback);
    
    m_state = State::Inferring;
    
    // Track assistant response using shared_ptr to ensure it lives through the async operation
    auto assistantResponse = std::make_shared<std::wstring>();
    
    m_client->ChatCompletionAsync(
        m_conversationHistory,
        [this, tokenCb, assistantResponse](const std::wstring& token) {
            *assistantResponse += token;
            m_runCallbackAsync([this, tokenCb, token]() {
                VARIANT arg;
                VariantInit(&arg);
                arg.vt = VT_BSTR;
                arg.bstrVal = SysAllocString(token.c_str());
                InvokeCallback(tokenCb.Get(), {arg});
                VariantClear(&arg);
            });
        },
        [this, completionCb, assistantResponse](bool success, const std::wstring& error) {
            if (success)
            {
                // Add assistant response to conversation history
                m_conversationHistory.push_back({L"assistant", *assistantResponse});
            }
            m_state = State::Ready;
            m_runCallbackAsync([this, completionCb, success, error]() {
                InvokeCompletionCallback(completionCb.Get(), success, error);
            });
        }
    );
    
    return S_OK;
}

STDMETHODIMP SLMHostObjectImpl::CancelInference()
{
    m_client->CancelInference();
    m_state = State::Ready;
    return S_OK;
}

STDMETHODIMP SLMHostObjectImpl::GetModels(BSTR* modelsJson)
{
    if (!modelsJson) return E_POINTER;
    
    auto models = m_client->GetAvailableModels();
    
    std::wostringstream json;
    json << L"[";
    
    bool first = true;
    for (const auto& model : models)
    {
        if (!first) json << L",";
        first = false;
        
        json << L"{"
             << L"\"alias\":\"" << model.alias << L"\","
             << L"\"name\":\"" << model.displayName << L"\","
             << L"\"description\":\"" << model.description << L"\","
             << L"\"cached\":" << (model.isDownloaded ? L"true" : L"false")
             << L"}";
    }
    
    json << L"]";
    
    *modelsJson = SysAllocString(json.str().c_str());
    return S_OK;
}

STDMETHODIMP SLMHostObjectImpl::SetModel(BSTR alias, VARIANT_BOOL* success)
{
    if (!alias || !success) return E_POINTER;
    
    std::wstring aliasStr(alias);
    bool result = m_client->SetCurrentModel(aliasStr);
    
    if (result)
    {
        m_modelAlias = aliasStr;
        // Clear conversation history when switching models
        m_conversationHistory.clear();
    }
    
    *success = result ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

STDMETHODIMP SLMHostObjectImpl::GetCurrentModel(BSTR* alias)
{
    if (!alias) return E_POINTER;
    
    *alias = SysAllocString(m_client->GetCurrentModelAlias().c_str());
    return S_OK;
}

// IDispatch implementation
STDMETHODIMP SLMHostObjectImpl::GetTypeInfoCount(UINT* pctinfo)
{
    *pctinfo = 1;
    return S_OK;
}

STDMETHODIMP SLMHostObjectImpl::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo)
{
    if (0 != iTInfo)
    {
        return TYPE_E_ELEMENTNOTFOUND;
    }
    if (!m_typeLib)
    {
        RETURN_IF_FAILED(LoadTypeLib(L"WebView2APISampleONNX.tlb", &m_typeLib));
    }
    return m_typeLib->GetTypeInfoOfGuid(__uuidof(ISLMHostObject), ppTInfo);
}

STDMETHODIMP SLMHostObjectImpl::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, 
                                               LCID lcid, DISPID* rgDispId)
{
    wil::com_ptr<ITypeInfo> typeInfo;
    RETURN_IF_FAILED(GetTypeInfo(0, lcid, &typeInfo));
    return typeInfo->GetIDsOfNames(rgszNames, cNames, rgDispId);
}

STDMETHODIMP SLMHostObjectImpl::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
                                        DISPPARAMS* pDispParams, VARIANT* pVarResult, 
                                        EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
    wil::com_ptr<ITypeInfo> typeInfo;
    RETURN_IF_FAILED(GetTypeInfo(0, lcid, &typeInfo));
    return typeInfo->Invoke(
        this, dispIdMember, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}
