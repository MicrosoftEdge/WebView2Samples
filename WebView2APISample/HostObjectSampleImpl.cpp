// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "HostObjectSampleImpl.h"

HostObjectSample::HostObjectSample(HostObjectSample::RunCallbackAsync runCallbackAsync)
    : m_propertyValue(L"Example Property String Value"), m_runCallbackAsync(runCallbackAsync)
{
}

STDMETHODIMP HostObjectSample::MethodWithParametersAndReturnValue(
    BSTR stringParameter, INT integerParameter, BSTR* stringResult)
{
    std::wstring result = L"MethodWithParametersAndReturnValue(";
    result += stringParameter;
    result += L", ";
    result += std::to_wstring(integerParameter);
    result += L") called.";
    *stringResult = SysAllocString(result.c_str());

    return S_OK;
}

STDMETHODIMP HostObjectSample::get_Property(BSTR* stringResult)
{
    *stringResult = SysAllocString(m_propertyValue.c_str());
    return S_OK;
}

STDMETHODIMP HostObjectSample::put_Property(BSTR stringValue)
{
    m_propertyValue = stringValue;
    return S_OK;
}

STDMETHODIMP HostObjectSample::CallCallbackAsynchronously(IDispatch* callbackParameter)
{
    wil::com_ptr<IDispatch> callbackParameterForCapture = callbackParameter;
    m_runCallbackAsync([callbackParameterForCapture]() -> void {
        callbackParameterForCapture->Invoke(
            DISPID_UNKNOWN, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, nullptr, nullptr,
            nullptr, nullptr);
    });

    return S_OK;
}

STDMETHODIMP HostObjectSample::GetTypeInfoCount(UINT* pctinfo)
{
    *pctinfo = 1;
    return S_OK;
}

STDMETHODIMP HostObjectSample::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo)
{
    if (0 != iTInfo)
    {
        return TYPE_E_ELEMENTNOTFOUND;
    }
    if (!m_typeLib)
    {
        RETURN_IF_FAILED(LoadTypeLib(L"WebView2APISample.tlb", &m_typeLib));
    }
    return m_typeLib->GetTypeInfoOfGuid(__uuidof(IHostObjectSample), ppTInfo);
}

STDMETHODIMP HostObjectSample::GetIDsOfNames(
    REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId)
{
    wil::com_ptr<ITypeInfo> typeInfo;
    RETURN_IF_FAILED(GetTypeInfo(0, lcid, &typeInfo));
    return typeInfo->GetIDsOfNames(rgszNames, cNames, rgDispId);
}

STDMETHODIMP HostObjectSample::Invoke(
    DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams,
    VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
    wil::com_ptr<ITypeInfo> typeInfo;
    RETURN_IF_FAILED(GetTypeInfo(0, lcid, &typeInfo));
    return typeInfo->Invoke(
        this, dispIdMember, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}
