// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

#include <wil/com.h>
#include <wil/resource.h>
#include <wil/result.h>
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <wrl.h>

#include "WebView2Experimental.h"
#include "WebView2ExperimentalEnvironmentOptions.h"
