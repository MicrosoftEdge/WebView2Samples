// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"

class ScenarioFileTypePolicy : public ComponentBase
{
public:
    ScenarioFileTypePolicy(AppWindow* appWindow);
    ~ScenarioFileTypePolicy();

private:
    bool SuppressPolicyForExtension();

    AppWindow* m_appWindow;
    wil::com_ptr<ICoreWebView2> m_webView2;
    wil::com_ptr<ICoreWebView2_2> m_webView2_2;
    wil::com_ptr<ICoreWebView2Experimental27> m_webView2Experimental27;
    EventRegistrationToken m_saveFileSecurityCheckStartingToken = {};
    EventRegistrationToken m_DOMcontentLoadedToken = {};
    std::wstring m_sampleUri;
};
