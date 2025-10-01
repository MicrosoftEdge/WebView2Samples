// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <ctime>
#include <vector>

#include "stdafx.h"

class Util
{
public:
    static std::wstring UnixEpochToDateTime(double value);
    static std::vector<std::wstring> SplitString(const std::wstring& input, wchar_t delimiter);
    static std::wstring TrimWhitespace(const std::wstring& text);
};
