// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ComponentBase.h"
#include "stdafx.h"

#include <string>
#include <utility>
#include <vector>

class AppWindow;
struct ICoreWebView2ExperimentalSensitivityInfo;

class ScenarioSensitivityLabel : public ComponentBase
{
public:
    ScenarioSensitivityLabel(AppWindow* appWindow);
    ~ScenarioSensitivityLabel() override;

    // PageInteractionRestrictionManager Allowlist functionality
    void SetPageRestrictionManagerAllowlist();
    void CheckPageRestrictionManagerAvailability();
    void LaunchLabelDemoPage();
    void ToggleEventListener();

private:
    AppWindow* m_appWindow = nullptr;
    wil::com_ptr<ICoreWebView2> m_webView;
    EventRegistrationToken m_sensitivityInfoChangedToken = {};
    std::wstring m_sampleUri;
    bool m_isEventListenerRegistered = false;

    // UI method for collecting user input
    std::vector<std::wstring> ShowAllowlistInputDialog();

    // Sensitivity label event handling
    void RegisterForSensitivityLabelChange();
    void UnregisterSensitivityLabelChange();
    std::wstring GetSensitivityLabelStrings(
        COREWEBVIEW2_SENSITIVITY_LABELS_STATE& sensitivityLabelsState);
    void OnSensitivityChanged(ICoreWebView2ExperimentalSensitivityInfo* sensitivityInfo);

    // Label processing helpers
    std::vector<std::pair<std::wstring, std::wstring>> ExtractSensitivityLabelInfo(
        ICoreWebView2ExperimentalSensitivityLabelCollectionView* labelCollection);
    std::wstring GenerateLabelInfoString(
        const std::vector<std::pair<std::wstring, std::wstring>>& labelInfos);

    // Helper methods
    HRESULT SetAllowlistOnProfile(std::vector<LPCWSTR> allowlist);
};
