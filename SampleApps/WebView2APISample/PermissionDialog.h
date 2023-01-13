// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

// Constructing this struct will show a client certificate selection dialog and return when
// the user dismisses it. If the user clicks the OK button, confirmed will be true with the
// selected certificate.
struct PermissionDialog
{
    PermissionDialog(
        HWND parent, std::vector<COREWEBVIEW2_PERMISSION_KIND> kinds,
        std::vector<COREWEBVIEW2_PERMISSION_STATE> states);

    std::vector<COREWEBVIEW2_PERMISSION_KIND> permissionKinds;
    std::vector<COREWEBVIEW2_PERMISSION_STATE> permissionStates;

    std::wstring origin;
    COREWEBVIEW2_PERMISSION_KIND kind = COREWEBVIEW2_PERMISSION_KIND_UNKNOWN_PERMISSION;
    COREWEBVIEW2_PERMISSION_STATE state = COREWEBVIEW2_PERMISSION_STATE_DEFAULT;
    bool confirmed = false;
};
