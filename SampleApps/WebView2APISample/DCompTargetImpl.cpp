// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "DCompTargetImpl.h"

DCompTargetImpl::DCompTargetImpl(ViewComponent* owner)
{
    m_viewComponentOwner = owner;
}

void DCompTargetImpl::RemoveOwnerRef()
{
    m_viewComponentOwner = nullptr;
}

HRESULT __stdcall DCompTargetImpl::SetRoot(IDCompositionVisual* visual)
{
    HRESULT hr = S_OK;
    if (m_viewComponentOwner)
    {
        hr = m_viewComponentOwner->m_dcompWebViewVisual->RemoveAllVisuals();
        if (SUCCEEDED(hr) && visual)
        {
            hr = m_viewComponentOwner->m_dcompWebViewVisual->AddVisual(visual, FALSE, nullptr);
        }
    }

    return hr;
}