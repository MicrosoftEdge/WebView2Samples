// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "CheckFailure.h"
#include "Util.h"

std::wstring Util::UnixEpochToDateTime(double value)
{
    WCHAR rawResult[32] = {};
    std::time_t rawTime = std::time_t(value / 1000);
    struct tm timeStruct = {};
    gmtime_s(&timeStruct, &rawTime);
    _wasctime_s(rawResult, 32, &timeStruct);
    std::wstring result(rawResult);
    return result;
}