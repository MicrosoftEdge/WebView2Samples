// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"
#include "SLMHostObjectImpl.h"
#include <thread>

SLMHostObject::SLMHostObject(SLMHostObject::RunCallbackAsync runCallbackAsync)
    : m_runCallbackAsync(runCallbackAsync)
{
    m_client = std::make_unique<FoundryLocalClient>();
}

SLMHostObject::~SLMHostObject()
{
    if (m_client)
    {
        m_client->CancelInference();
    }
}

std::wstring SLMHostObject::GetCurrentStatus()
{
    // Check actual state
    if (!m_client->IsOnline() && !m_client->IsModelCached(m_modelAlias))
    {
        return L"offline";
    }

    if (!m_client->IsFoundryInstalled())
    {
        return L"not_installed";
    }

    if (!m_client->IsServiceRunning())
    {
        return L"service_stopped";
    }

    if (!m_client->IsModelCached(m_modelAlias))
    {
        return L"model_not_cached";
    }

    State currentState = m_state.load();
    switch (currentState)
    {
        case State::Installing:
            return L"installing";
        case State::StartingService:
            return L"starting_service";
        case State::Downloading:
            return L"downloading";
        case State::Inferring:
            return L"inferring";
        default:
            return L"ready";
    }
}

STDMETHODIMP SLMHostObject::QueryStatus(BSTR* status)
{
    std::wstring statusStr = GetCurrentStatus();
    *status = SysAllocString(statusStr.c_str());
    return S_OK;
}

void SLMHostObject::InvokeCallback(IDispatch* callback, const std::wstring& arg)
{
    if (!callback) return;

    VARIANT vArg;
    VariantInit(&vArg);
    vArg.vt = VT_BSTR;
    vArg.bstrVal = SysAllocString(arg.c_str());

    DISPPARAMS params = {};
    params.cArgs = 1;
    params.rgvarg = &vArg;

    callback->Invoke(
        DISPID_VALUE, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD,
        &params, nullptr, nullptr, nullptr);

    VariantClear(&vArg);
}

void SLMHostObject::InvokeProgressCallback(
    IDispatch* callback, const std::wstring& stage, double progress, const std::wstring& message)
{
    if (!callback) return;

    // Create a JSON-like string that JavaScript can parse
    std::wstring json = L"{\"stage\":\"" + stage + 
                        L"\",\"progress\":" + std::to_wstring(progress) + 
                        L",\"message\":\"" + message + L"\"}";

    InvokeCallback(callback, json);
}

void SLMHostObject::InvokeCompletionCallback(IDispatch* callback, bool success, const std::wstring& error)
{
    if (!callback) return;

    std::wstring json = L"{\"success\":" + std::wstring(success ? L"true" : L"false") + 
                        L",\"error\":\"" + error + L"\"}";

    InvokeCallback(callback, json);
}

STDMETHODIMP SLMHostObject::SetupAsync(
    IDispatch* progressCallback,
    IDispatch* completionCallback)
{
    wil::com_ptr<IDispatch> progressCb = progressCallback;
    wil::com_ptr<IDispatch> completionCb = completionCallback;

    // Run setup in background
    std::thread([this, progressCb, completionCb]() {
        try
        {
            // Step 1: Check if online (if not and model not cached, fail)
            if (!m_client->IsOnline() && !m_client->IsModelCached(m_modelAlias))
            {
                m_state = State::Offline;
                m_runCallbackAsync([this, completionCb]() {
                    InvokeCompletionCallback(completionCb.get(), false, 
                        L"No internet connection and model not cached. Please connect to the internet for first-time setup.");
                });
                return;
            }

            // Step 2: Check/Install Foundry Local
            if (!m_client->IsFoundryInstalled())
            {
                m_state = State::Installing;
                m_runCallbackAsync([this, progressCb]() {
                    InvokeProgressCallback(progressCb.get(), L"installing", 0, 
                        L"Installing Foundry Local (Administrator permission required)...");
                });

                if (!m_client->InstallFoundryLocal())
                {
                    m_state = State::NotInstalled;
                    m_runCallbackAsync([this, completionCb]() {
                        InvokeCompletionCallback(completionCb.get(), false, 
                            L"Failed to install Foundry Local. Please install manually using: winget install Microsoft.FoundryLocal");
                    });
                    return;
                }

                m_runCallbackAsync([this, progressCb]() {
                    InvokeProgressCallback(progressCb.get(), L"installing", 100, 
                        L"Foundry Local installed successfully");
                });
            }

            // Step 3: Start service if needed
            if (!m_client->IsServiceRunning())
            {
                m_state = State::StartingService;
                m_runCallbackAsync([this, progressCb]() {
                    InvokeProgressCallback(progressCb.get(), L"starting_service", 0, 
                        L"Starting Foundry Local service...");
                });

                if (!m_client->StartService())
                {
                    m_state = State::ServiceStopped;
                    m_runCallbackAsync([this, completionCb]() {
                        InvokeCompletionCallback(completionCb.get(), false, 
                            L"Failed to start Foundry Local service");
                    });
                    return;
                }

                m_runCallbackAsync([this, progressCb]() {
                    InvokeProgressCallback(progressCb.get(), L"starting_service", 100, 
                        L"Service started");
                });
            }

            // Step 4: Download model if needed
            if (!m_client->IsModelCached(m_modelAlias))
            {
                m_state = State::Downloading;
                std::wstring modelDisplayName = m_client->GetCurrentModelDisplayName();
                m_runCallbackAsync([this, progressCb, modelDisplayName]() {
                    InvokeProgressCallback(progressCb.get(), L"downloading", 0, 
                        L"Downloading " + modelDisplayName + L"...");
                });

                bool downloadComplete = false;
                bool downloadSuccess = false;
                std::wstring downloadError;

                m_client->DownloadModelAsync(
                    m_modelAlias,
                    [this, progressCb, modelDisplayName](double progress) {
                        m_runCallbackAsync([this, progressCb, progress, modelDisplayName]() {
                            InvokeProgressCallback(progressCb.get(), L"downloading", progress, 
                                L"Downloading " + modelDisplayName + L"... " + std::to_wstring(static_cast<int>(progress)) + L"%");
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
                    m_state = State::ModelNotCached;
                    m_runCallbackAsync([this, completionCb, downloadError]() {
                        InvokeCompletionCallback(completionCb.get(), false, 
                            L"Failed to download model: " + downloadError);
                    });
                    return;
                }
            }

            // All setup complete
            m_state = State::Ready;
            m_runCallbackAsync([this, completionCb]() {
                InvokeCompletionCallback(completionCb.get(), true, L"");
            });
        }
        catch (const std::exception& e)
        {
            std::string what = e.what();
            std::wstring error(what.begin(), what.end());
            m_runCallbackAsync([this, completionCb, error]() {
                InvokeCompletionCallback(completionCb.get(), false, L"Setup error: " + error);
            });
        }
    }).detach();

    return S_OK;
}

STDMETHODIMP SLMHostObject::InferAsync(
    BSTR prompt,
    IDispatch* tokenCallback,
    IDispatch* completionCallback)
{
    std::wstring promptStr(prompt, SysStringLen(prompt));
    wil::com_ptr<IDispatch> tokenCb = tokenCallback;
    wil::com_ptr<IDispatch> completionCb = completionCallback;

    m_state = State::Inferring;

    // Build messages
    std::vector<ChatMessage> messages;
    messages.push_back({L"user", promptStr});

    m_client->ChatCompletionAsync(
        messages,
        [this, tokenCb](const std::wstring& token) {
            m_runCallbackAsync([this, tokenCb, token]() {
                InvokeCallback(tokenCb.get(), token);
            });
        },
        [this, completionCb](bool success, const std::wstring& error) {
            m_state = State::Ready;
            m_runCallbackAsync([this, completionCb, success, error]() {
                InvokeCompletionCallback(completionCb.get(), success, error);
            });
        }
    );

    return S_OK;
}

STDMETHODIMP SLMHostObject::CancelInference()
{
    if (m_client)
    {
        m_client->CancelInference();
    }
    m_state = State::Ready;
    return S_OK;
}

STDMETHODIMP SLMHostObject::SetSystemPrompt(BSTR prompt)
{
    if (m_client)
    {
        std::wstring promptStr(prompt, SysStringLen(prompt));
        m_client->SetSystemPrompt(promptStr);
    }
    return S_OK;
}

STDMETHODIMP SLMHostObject::GetSystemPrompt(BSTR* prompt)
{
    if (m_client)
    {
        std::wstring currentPrompt = m_client->GetSystemPrompt();
        *prompt = SysAllocString(currentPrompt.c_str());
    }
    else
    {
        *prompt = SysAllocString(L"");
    }
    return S_OK;
}

STDMETHODIMP SLMHostObject::IsOnline(VARIANT_BOOL* online)
{
    *online = m_client && m_client->IsOnline() ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

STDMETHODIMP SLMHostObject::IsReady(VARIANT_BOOL* ready)
{
    if (!m_client)
    {
        *ready = VARIANT_FALSE;
        return S_OK;
    }

    bool isReady = m_client->IsFoundryInstalled() &&
                   m_client->IsServiceRunning() &&
                   m_client->IsModelCached(m_modelAlias);
    
    *ready = isReady ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

STDMETHODIMP SLMHostObject::GetModels(BSTR* modelsJson)
{
    if (!m_client)
    {
        *modelsJson = SysAllocString(L"[]");
        return S_OK;
    }

    auto models = m_client->GetAvailableModels();
    
    std::wstring json = L"[";
    bool first = true;
    for (const auto& model : models)
    {
        if (!first) json += L",";
        first = false;
        
        // Escape description for JSON
        std::wstring escapedDesc = model.description;
        
        json += L"{\"alias\":\"" + model.alias + L"\",";
        json += L"\"name\":\"" + model.displayName + L"\",";
        json += L"\"description\":\"" + escapedDesc + L"\",";
        json += L"\"cached\":" + std::wstring(model.isCached ? L"true" : L"false") + L"}";
    }
    json += L"]";
    
    *modelsJson = SysAllocString(json.c_str());
    return S_OK;
}

STDMETHODIMP SLMHostObject::SetModel(BSTR alias, VARIANT_BOOL* success)
{
    if (!m_client)
    {
        *success = VARIANT_FALSE;
        return S_OK;
    }

    std::wstring aliasStr(alias, SysStringLen(alias));
    bool result = m_client->SetModelAlias(aliasStr);
    
    if (result)
    {
        m_modelAlias = aliasStr;
    }
    
    *success = result ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

STDMETHODIMP SLMHostObject::GetCurrentModel(BSTR* alias)
{
    std::wstring currentAlias = m_client ? m_client->GetCurrentModelAlias() : m_modelAlias;
    *alias = SysAllocString(currentAlias.c_str());
    return S_OK;
}

// IDispatch implementation

STDMETHODIMP SLMHostObject::GetTypeInfoCount(UINT* pctinfo)
{
    *pctinfo = 1;
    return S_OK;
}

STDMETHODIMP SLMHostObject::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo)
{
    if (0 != iTInfo)
    {
        return TYPE_E_ELEMENTNOTFOUND;
    }
    if (!m_typeLib)
    {
        RETURN_IF_FAILED(LoadTypeLib(L"WebView2APISample.tlb", &m_typeLib));
    }
    return m_typeLib->GetTypeInfoOfGuid(__uuidof(ISLMHostObject), ppTInfo);
}

STDMETHODIMP SLMHostObject::GetIDsOfNames(
    REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId)
{
    wil::com_ptr<ITypeInfo> typeInfo;
    RETURN_IF_FAILED(GetTypeInfo(0, lcid, &typeInfo));
    return typeInfo->GetIDsOfNames(rgszNames, cNames, rgDispId);
}

STDMETHODIMP SLMHostObject::Invoke(
    DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams,
    VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
    wil::com_ptr<ITypeInfo> typeInfo;
    RETURN_IF_FAILED(GetTypeInfo(0, lcid, &typeInfo));
    return typeInfo->Invoke(
        this, dispIdMember, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}
