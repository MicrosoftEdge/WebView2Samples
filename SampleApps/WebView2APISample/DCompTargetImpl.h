// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include "ViewComponent.h"

class DCompTargetImpl
    : public Microsoft::WRL::RuntimeClass<
    Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IDCompositionTarget>
{
public:
    DCompTargetImpl(ViewComponent* owner);
    void RemoveOwnerRef();

    // Inherited via IDCompositionTarget
    virtual HRESULT __stdcall SetRoot(IDCompositionVisual* visual) override;

private:
    ViewComponent* m_viewComponentOwner;
};
