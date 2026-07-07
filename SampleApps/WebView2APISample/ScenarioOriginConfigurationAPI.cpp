// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include <string>
#include <vector>

#include "CheckFailure.h"
#include "TextInputDialog.h"
#include "Util.h"

#include "ScenarioOriginConfigurationAPI.h"

using namespace Microsoft::WRL;

static constexpr int kEnhancedSecurityModeValue = 1;
static constexpr int kSmartScreenValue = 2;

ScenarioOriginConfigurationAPI::ScenarioOriginConfigurationAPI(AppWindow* appWindow)
    : m_appWindow(appWindow)
{
    CHECK_FAILURE(m_appWindow ? S_OK : E_POINTER);
    wil::com_ptr<ICoreWebView2> webView = m_appWindow->GetWebView();
    CHECK_FAILURE(webView ? S_OK : E_POINTER);
    auto webView2_13 = webView.try_query<ICoreWebView2_13>();
    CHECK_FAILURE(webView2_13 ? S_OK : E_POINTER);
    CHECK_FAILURE(webView2_13->get_Profile(&m_webviewProfile));
}

ScenarioOriginConfigurationAPI::~ScenarioOriginConfigurationAPI() = default;

std::wstring ScenarioOriginConfigurationAPI::FeatureToString(
    COREWEBVIEW2_ORIGIN_FEATURE feature)
{
    switch (feature)
    {
    case COREWEBVIEW2_ORIGIN_FEATURE_ENHANCED_SECURITY_MODE:
        return L"EnhancedSecurityMode";
    case COREWEBVIEW2_ORIGIN_FEATURE_REPUTATION_CHECKING:
        return L"ReputationChecking";
    default:
        return L"Unknown";
    }
}

void ScenarioOriginConfigurationAPI::SetFeatureForOrigins(
    const std::vector<std::wstring>& originPatterns,
    const std::vector<
        std::pair<COREWEBVIEW2_ORIGIN_FEATURE, COREWEBVIEW2_ORIGIN_FEATURE_STATE>>& features)
{
    auto experimentalProfile16 =
        m_webviewProfile.try_query<ICoreWebView2ExperimentalProfile16>();
    CHECK_FEATURE_RETURN_EMPTY(experimentalProfile16);

    // featureSettings holds wil::com_ptr for COM lifetime management (keeps refcount > 0).
    // featureSettingsRaw holds raw pointers extracted from featureSettings to pass to the API.
    // Both are needed because the API requires a pointer array, but we need smart pointers to
    // prevent premature COM object destruction.
    std::vector<wil::com_ptr<ICoreWebView2ExperimentalOriginFeatureSetting>> featureSettings;
    std::vector<ICoreWebView2ExperimentalOriginFeatureSetting*> featureSettingsRaw;

    for (const auto& [featureKind, featureState] : features)
    {
        wil::com_ptr<ICoreWebView2ExperimentalOriginFeatureSetting> setting;
        CHECK_FAILURE(experimentalProfile16->CreateOriginFeatureSetting(
            featureKind, featureState, &setting));
        featureSettings.push_back(setting);
        featureSettingsRaw.push_back(setting.get());
    }

    std::vector<LPCWSTR> origins;
    for (const auto& pattern : originPatterns)
    {
        origins.push_back(pattern.c_str());
    }

    CHECK_FAILURE(experimentalProfile16->SetOriginFeatures(
        static_cast<UINT32>(origins.size()), origins.data(),
        static_cast<UINT32>(featureSettingsRaw.size()), featureSettingsRaw.data()));
}

void ScenarioOriginConfigurationAPI::GetOriginFeatures()
{
    auto experimentalProfile16 =
        m_webviewProfile.try_query<ICoreWebView2ExperimentalProfile16>();
    CHECK_FEATURE_RETURN_EMPTY(experimentalProfile16);

    TextInputDialog inputDialog(
        m_appWindow->GetMainWindow(), L"Get Trusted Origin Features",
        L"Enter the origin to retrieve feature settings for:", L"Origin:",
        std::wstring(L"https://www.microsoft.com"),
        false); // not read-only

    if (inputDialog.confirmed)
    {
        std::wstring origin = inputDialog.input;

        CHECK_FAILURE(experimentalProfile16->GetEffectiveFeaturesForOrigin(
            origin.c_str(),
            Callback<ICoreWebView2ExperimentalGetEffectiveFeaturesForOriginCompletedHandler>(
                [appWindow = m_appWindow, origin](
                    HRESULT errorCode,
                    ICoreWebView2ExperimentalOriginFeatureSettingCollectionView* result)
                    -> HRESULT
                {
                    if (SUCCEEDED(errorCode))
                    {
                        UINT32 count = 0;
                        CHECK_FAILURE(result->get_Count(&count));

                        std::wstring message = L"Features for origin: " + origin + L"\n";
                        for (UINT32 i = 0; i < count; i++)
                        {
                            wil::com_ptr<ICoreWebView2ExperimentalOriginFeatureSetting> setting;
                            CHECK_FAILURE(result->GetValueAtIndex(i, &setting));

                            COREWEBVIEW2_ORIGIN_FEATURE feature;
                            COREWEBVIEW2_ORIGIN_FEATURE_STATE featureState;
                            CHECK_FAILURE(setting->get_Feature(&feature));
                            CHECK_FAILURE(setting->get_State(&featureState));

                            message +=
                                L"Feature: " + FeatureToString(feature) + L", Enabled: " +
                                (featureState == COREWEBVIEW2_ORIGIN_FEATURE_STATE_ENABLED
                                     ? L"True"
                                     : L"False") +
                                L"\n";
                        }

                        MessageBoxW(
                            appWindow->GetMainWindow(), message.c_str(),
                            L"Trusted Origin Features", MB_OK);
                    }
                    else
                    {
                        ShowFailure(
                            errorCode,
                            L"Failed to get effective features for origin: " + origin);
                    }
                    return S_OK;
                })
                .Get()));
    }
}

void ScenarioOriginConfigurationAPI::SetOriginFeatures()
{
    static constexpr wchar_t kOriginPatternsLabel[] = L"Enter origin patterns separated with ;";
    static constexpr wchar_t kFeaturesGroupLabel[] = L"Select features to enable:";

    // Builder with text area for origin input and checkbox for feature selection.
    auto enhancedDialog =
        TextInputDialog::Builder(
            m_appWindow->GetMainWindow(), L"Configure Trusted Origin",
            L"Trusted Origin Configuration:")
            .AddTextArea(kOriginPatternsLabel, L"https://www.microsoft.com")
            .AddCheckBoxGroup(
                kFeaturesGroupLabel,
                {{L"Enable Enhanced Security Mode", kEnhancedSecurityModeValue, true},
                 {L"Disable Reputation Checking", kSmartScreenValue, false}})
            .Build();

    if (enhancedDialog.confirmed)
    {
        // Retrieve the origin text area. Skip if the control is missing.
        auto textAreaIt = enhancedDialog.results.find(kOriginPatternsLabel);
        if (textAreaIt == enhancedDialog.results.end())
            return;
        auto& textArea = std::get<TextArea>(textAreaIt->second);
        std::wstring input = textArea.input;

        // Retrieve the feature checkbox group. Skip if the control is missing.
        std::vector<std::pair<COREWEBVIEW2_ORIGIN_FEATURE, COREWEBVIEW2_ORIGIN_FEATURE_STATE>>
            features;
        auto groupIt = enhancedDialog.results.find(kFeaturesGroupLabel);
        if (groupIt == enhancedDialog.results.end())
            return;
        auto& group = std::get<CheckBoxGroup>(groupIt->second);
        for (const auto& option : group.options)
        {
            if (option.isSelected && option.value == kEnhancedSecurityModeValue)
            {
                features.push_back(
                    {COREWEBVIEW2_ORIGIN_FEATURE_ENHANCED_SECURITY_MODE,
                     COREWEBVIEW2_ORIGIN_FEATURE_STATE_ENABLED});
            }
            if (option.isSelected && option.value == kSmartScreenValue)
            {
                // Disabling SmartScreen for the origin = allow-listing it.
                features.push_back(
                    {COREWEBVIEW2_ORIGIN_FEATURE_REPUTATION_CHECKING,
                     COREWEBVIEW2_ORIGIN_FEATURE_STATE_DISABLED});
            }
        }

        std::vector<std::wstring> origins = Util::SplitString(input, L';');

        // Set features for all origins in a single call.
        if (!features.empty())
        {
            SetFeatureForOrigins(origins, features);
        }
    }
}
