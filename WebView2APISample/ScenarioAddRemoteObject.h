// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"
#include "RemoteObjectSample_h.h"

class ScenarioAddRemoteObject : public ComponentBase
{
public:
    ScenarioAddRemoteObject(AppWindow* appWindow);
    ~ScenarioAddRemoteObject() override;

private:
    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<IWebView2WebView5> m_webView;
    wil::com_ptr<RemoteObjectSample> m_remoteObject;

    EventRegistrationToken m_navigationStartingToken = {};
};
