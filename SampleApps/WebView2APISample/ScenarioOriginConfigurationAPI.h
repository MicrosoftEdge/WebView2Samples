// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>
#include <vector>

#include "AppWindow.h"
#include "ComponentBase.h"

// Demonstrates configuring trusted origins via WebView2 profile.
class ScenarioOriginConfigurationAPI : public ComponentBase
{
public:
    ScenarioOriginConfigurationAPI(AppWindow* appWindow);
    ~ScenarioOriginConfigurationAPI() override;

    void SetFeatureForOrigins(
        const std::vector<std::wstring>& originPattern,
        const std::vector<
            std::pair<COREWEBVIEW2_ORIGIN_FEATURE, COREWEBVIEW2_ORIGIN_FEATURE_STATE>>&
            features);
    void GetOriginFeatures();
    void SetOriginFeatures();

private:
    static std::wstring FeatureToString(COREWEBVIEW2_ORIGIN_FEATURE feature);

    AppWindow* const m_appWindow; // Non-owning, guaranteed non-null.
    wil::com_ptr<ICoreWebView2Profile> m_webviewProfile;
};
