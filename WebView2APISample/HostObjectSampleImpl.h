// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include <functional>
#include <string>
#include <wrl\client.h>

#include "HostObjectSample_h.h"

class HostObjectSample : public Microsoft::WRL::RuntimeClass<
                               Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
                               IHostObjectSample, IDispatch>
{
public:
    typedef std::function<void(void)> Callback;
    typedef std::function<void(Callback)> RunCallbackAsync;

    HostObjectSample(RunCallbackAsync runCallbackAsync);

    // IHostObjectSample implementation
    STDMETHODIMP MethodWithParametersAndReturnValue(
        BSTR stringParameter, INT integerParameter, BSTR* stringResult) override;

    // Demonstrate getting and setting a property.
    STDMETHODIMP get_Property(BSTR* stringResult) override;
    STDMETHODIMP put_Property(BSTR stringValue) override;

    // Demonstrate native calling back into JavaScript.
    STDMETHODIMP CallCallbackAsynchronously(IDispatch* callbackParameter) override;

    // IDispatch implementation
    STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) override;

    STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo) override;

    STDMETHODIMP GetIDsOfNames(
        REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId) override;

    STDMETHODIMP Invoke(
        DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams,
        VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) override;

private:
    std::wstring m_propertyValue;
    wil::com_ptr<IDispatch> m_callback;
    RunCallbackAsync m_runCallbackAsync;
    wil::com_ptr<ITypeLib> m_typeLib;
};
