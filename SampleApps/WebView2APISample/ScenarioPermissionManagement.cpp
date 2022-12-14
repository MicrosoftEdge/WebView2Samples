// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "ScenarioPermissionManagement.h"

#include "App.h"
#include "CheckFailure.h"
#include "PermissionDialog.h"
#include "resource.h"

using namespace Microsoft::WRL;

std::wstring PermissionStateToString(COREWEBVIEW2_PERMISSION_STATE state)
{
    switch (state)
    {
    case COREWEBVIEW2_PERMISSION_STATE_ALLOW:
        return L"allow";
    case COREWEBVIEW2_PERMISSION_STATE_DENY:
        return L"deny";
    default:
        return L"default";
    }
}
std::wstring PermissionKindToString(COREWEBVIEW2_PERMISSION_KIND type)
{
    switch (type)
    {
    case COREWEBVIEW2_PERMISSION_KIND_CAMERA:
        return L"camera";
    case COREWEBVIEW2_PERMISSION_KIND_CLIPBOARD_READ:
        return L"clipboard read";
    case COREWEBVIEW2_PERMISSION_KIND_GEOLOCATION:
        return L"geolocation";
    case COREWEBVIEW2_PERMISSION_KIND_MICROPHONE:
        return L"mic";
    case COREWEBVIEW2_PERMISSION_KIND_NOTIFICATIONS:
        return L"notifications";
    case COREWEBVIEW2_PERMISSION_KIND_OTHER_SENSORS:
        return L"other sensors";
    default:
        return L"unknown";
    }
}
