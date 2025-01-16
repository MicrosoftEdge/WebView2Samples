// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include "ComponentBase.h"
#include <string>

class ScenarioDefaultBackgroundColor : public ComponentBase
{
public:
    ScenarioDefaultBackgroundColor(AppWindow* appWindow);
    ~ScenarioDefaultBackgroundColor() override;
};
