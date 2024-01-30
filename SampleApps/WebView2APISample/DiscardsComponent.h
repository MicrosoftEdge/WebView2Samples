// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include "AppWindow.h"
#include "ComponentBase.h"

class DiscardsComponent : public ComponentBase
{
public:
    DiscardsComponent(AppWindow* appWindow);
    ~DiscardsComponent() override;

private:
    AppWindow* m_appWindow = nullptr;

    // The AppWindow showing discards.
    AppWindow* m_appWindowDiscardsView;
};
