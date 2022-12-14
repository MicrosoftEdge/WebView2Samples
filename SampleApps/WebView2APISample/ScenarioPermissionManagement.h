// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include "stdafx.h"

#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"

std::wstring PermissionKindToString(COREWEBVIEW2_PERMISSION_KIND type);

std::wstring PermissionStateToString(COREWEBVIEW2_PERMISSION_STATE state);
