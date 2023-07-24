// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <ctime>

#include "stdafx.h"

class Util
{
public:
    static std::wstring UnixEpochToDateTime(double value);
};
