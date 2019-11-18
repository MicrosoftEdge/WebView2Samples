// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include <string>

// Constructing this struct will show a text input dialog and return when they user
// dismisses it.  If the user clicked the OK button, confirmed will be true and input will
// be set to the input they entered.
struct TextInputDialog
{
    TextInputDialog(
        HWND parent,
        PCWSTR title,
        PCWSTR prompt,
        PCWSTR description,
        const std::wstring& defaultInput = L"",
        bool readOnly = false);

    PCWSTR title;
    PCWSTR prompt;
    PCWSTR description;
    bool readOnly;

    bool confirmed;
    std::wstring input;
};
