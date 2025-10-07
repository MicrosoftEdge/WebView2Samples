// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "ScenarioSensitivityLabel.h"

#include "AppWindow.h"
#include "CheckFailure.h"
#include "TextInputDialog.h"
#include "Util.h"

using namespace Microsoft::WRL;

static constexpr WCHAR c_samplePath[] = L"ScenarioSensitivityLabelChanged.html";

ScenarioSensitivityLabel::ScenarioSensitivityLabel(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    m_sampleUri = m_appWindow->GetLocalUri(c_samplePath);
}

ScenarioSensitivityLabel::~ScenarioSensitivityLabel()
{
    UnregisterSensitivityLabelChange();
}

void ScenarioSensitivityLabel::SetPageRestrictionManagerAllowlist()
{
    std::vector<std::wstring> allowlist = ShowAllowlistInputDialog();

    if (allowlist.empty())
    {
        m_appWindow->AsyncMessageBox(
            L"No URLs entered. Allowlist not modified.", L"Sensitivity Label - Set Allowlist");
        return;
    }

    std::vector<LPCWSTR> items;
    items.reserve(allowlist.size());
    for (const auto& str : allowlist)
    {
        items.push_back(str.c_str());
    }

    HRESULT hr = SetAllowlistOnProfile(items);
    if (FAILED(hr))
    {
        m_appWindow->AsyncMessageBox(
            L"Failed to set PageInteractionRestrictionManager allowlist.",
            L"Sensitivity Label - Set Allowlist Error");
        return;
    }

    std::wstring message =
        L"PageInteractionRestrictionManager allowlist set successfully with " +
        std::to_wstring(allowlist.size()) + L" URLs:\n\n";
    for (const auto& url : allowlist)
    {
        message += L"- " + url + L"\n";
    }

    m_appWindow->AsyncMessageBox(message, L"Sensitivity Label - Allowlist Set");
}

void ScenarioSensitivityLabel::CheckPageRestrictionManagerAvailability()
{
    // Execute script to check if navigator.pageInteractionRestrictionManager exists
    std::wstring script =
        L"(() => {"
        L"  try {"
        L"    return typeof navigator.pageInteractionRestrictionManager !== 'undefined' && "
        L"           navigator.pageInteractionRestrictionManager !== null;"
        L"  } catch (e) {"
        L"    return false;"
        L"  }"
        L"})();";

    CHECK_FAILURE(m_webView->ExecuteScript(
        script.c_str(),
        Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
            [this](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT
            {
                if (FAILED(errorCode))
                {
                    m_appWindow->AsyncMessageBox(
                        L"Failed to execute script to check PageInteractionRestrictionManager "
                        L"availability.",
                        L"Sensitivity Label - Error");
                    return S_OK;
                }

                bool isAvailable =
                    (resultObjectAsJson && wcscmp(resultObjectAsJson, L"true") == 0);

                std::wstring message;
                if (isAvailable)
                {
                    message = L"PageInteractionRestrictionManager is AVAILABLE\n\n"
                              L"The navigator.pageInteractionRestrictionManager object "
                              L"exists and can be used for sensitivity labeling.";
                }
                else
                {
                    wil::unique_cotaskmem_string currentUri;
                    message = L"PageInteractionRestrictionManager is NOT AVAILABLE\n\n";

                    if (SUCCEEDED(m_webView->get_Source(&currentUri)) && currentUri.get())
                    {
                        message += L"Current URL: " + std::wstring(currentUri.get()) + L"\n\n";
                    }

                    message +=
                        L"To enable: Go to Scenario -> Sensitivity Label -> Set PIRM Allowlist";
                }

                m_appWindow->AsyncMessageBox(
                    message, L"Sensitivity Label - Availability Check");
                return S_OK;
            })
            .Get()));
}

void ScenarioSensitivityLabel::LaunchLabelDemoPage()
{
    RegisterForSensitivityLabelChange();

    CHECK_FAILURE(m_webView->Navigate(m_sampleUri.c_str()));
}

void ScenarioSensitivityLabel::ToggleEventListener()
{
    if (m_isEventListenerRegistered)
    {
        UnregisterSensitivityLabelChange();

        m_appWindow->AsyncMessageBox(
            L"Sensitivity label event listener has been turned OFF.\n\n"
            L"The application will no longer receive notifications when sensitivity labels "
            L"change.",
            L"Event Listener - OFF");
    }
    else
    {
        RegisterForSensitivityLabelChange();

        m_appWindow->AsyncMessageBox(
            L"Sensitivity label event listener has been turned ON.\n\n"
            L"The application will now receive notifications when sensitivity labels change.",
            L"Event Listener - ON");
    }
}

std::vector<std::wstring> ScenarioSensitivityLabel::ShowAllowlistInputDialog()
{
    std::vector<std::wstring> result;

    std::wstring prompt = L"Enter URLs for the PageInteractionRestrictionManager Allowlist "
                          L"separated by semicolons (;):";

    std::wstring description =
        L"Example: https://site1.com/;https://site2.com/*;https://*.site3.com/*";

    std::wstring defaultValue =
        L"https://*example.com*;https://microsoft.com*;https://github.com*";

    TextInputDialog dialog(
        m_appWindow->GetMainWindow(), L"PageInteractionRestrictionManager Allowlist",
        prompt.c_str(), description.c_str(), defaultValue);

    if (dialog.confirmed && !dialog.input.empty())
    {
        // Use the utility function to split the input string by semicolons and trim whitespace
        result = Util::SplitString(dialog.input, L';');
    }

    return result;
}

void ScenarioSensitivityLabel::RegisterForSensitivityLabelChange()
{
    // If event is already registered return
    if (m_sensitivityInfoChangedToken.value != 0)
    {
        return;
    }

    auto webView32 = m_webView.try_query<ICoreWebView2Experimental32>();
    CHECK_FEATURE_RETURN_EMPTY(webView32);
    //! [SensitivityInfoChanged]
    CHECK_FAILURE(webView32->add_SensitivityInfoChanged(
        Callback<ICoreWebView2ExperimentalSensitivityInfoChangedEventHandler>(
            [this](ICoreWebView2* sender, IUnknown* args) -> HRESULT
            {
                auto webView32 = this->m_webView.try_query<ICoreWebView2Experimental32>();
                Microsoft::WRL::ComPtr<ICoreWebView2ExperimentalSensitivityInfo>
                    sensitivityInfo;
                webView32->get_SensitivityInfo(&sensitivityInfo);

                OnSensitivityChanged(sensitivityInfo.Get());

                return S_OK;
            })
            .Get(),
        &m_sensitivityInfoChangedToken));
    //! [SensitivityInfoChanged]

    m_isEventListenerRegistered = true;
}

void ScenarioSensitivityLabel::UnregisterSensitivityLabelChange()
{
    if (m_sensitivityInfoChangedToken.value != 0)
    {
        auto webView32 = m_webView.try_query<ICoreWebView2Experimental32>();
        webView32->remove_SensitivityInfoChanged(m_sensitivityInfoChangedToken);
        m_sensitivityInfoChangedToken = {};
        m_isEventListenerRegistered = false;
    }
}

HRESULT ScenarioSensitivityLabel::SetAllowlistOnProfile(std::vector<LPCWSTR> allowlist)
{
    //! [SetAllowList]
    auto webView2_13 = m_webView.try_query<ICoreWebView2_13>();
    if (!webView2_13)
    {
        return E_NOINTERFACE;
    }

    wil::com_ptr<ICoreWebView2Profile> webView2Profile;
    HRESULT hr = webView2_13->get_Profile(&webView2Profile);
    if (FAILED(hr))
    {
        return hr;
    }

    auto experimentalProfile14 =
        webView2Profile.try_query<ICoreWebView2ExperimentalProfile14>();
    if (!experimentalProfile14)
    {
        return E_NOINTERFACE;
    }

    // allowlist consists of URL patterns that can access PageInteractionRestrictionManager API
    // Examples: "https://example.com/*", "https://*.trusted-domain.com/*", "*://site.com/*"
    return experimentalProfile14->SetPageInteractionRestrictionManagerAllowList(
        static_cast<UINT32>(allowlist.size()), allowlist.data());
    //! [SetAllowList]
}

std::wstring ScenarioSensitivityLabel::GetSensitivityLabelStrings(
    COREWEBVIEW2_SENSITIVITY_LABELS_STATE& sensitivityLabelsState)
{
    std::wstring labelsString;
    switch (sensitivityLabelsState)
    {
    case COREWEBVIEW2_SENSITIVITY_LABELS_STATE_NOT_APPLICABLE:
        labelsString = L"No allowlisted pages loaded";
        break;
    case COREWEBVIEW2_SENSITIVITY_LABELS_STATE_PENDING:
        labelsString = L"Labels undetermined - allowlisted page loaded but no "
                       L"labels detected yet";
        break;
    case COREWEBVIEW2_SENSITIVITY_LABELS_STATE_AVAILABLE:
        labelsString = L"Labels determined";
        break;
    }
    return labelsString;
}

void ScenarioSensitivityLabel::OnSensitivityChanged(
    ICoreWebView2ExperimentalSensitivityInfo* sensitivityInfo)
{
    if (!sensitivityInfo)
    {
        return;
    }

    //! [SensitivityInfo]
    COREWEBVIEW2_SENSITIVITY_LABELS_STATE sensitivityLabelsState;
    CHECK_FAILURE(sensitivityInfo->get_SensitivityLabelsState(&sensitivityLabelsState));
    Microsoft::WRL::ComPtr<ICoreWebView2ExperimentalSensitivityLabelCollectionView>
        sensitivityLabelsCollection;
    if (sensitivityLabelsState == COREWEBVIEW2_SENSITIVITY_LABELS_STATE_AVAILABLE)
    {
        CHECK_FAILURE(sensitivityInfo->get_SensitivityLabels(&sensitivityLabelsCollection));
    }
    //! [SensitivityInfo]

    std::wstring labelsString = GetSensitivityLabelStrings(sensitivityLabelsState);

    if (sensitivityLabelsCollection)
    {
        // Get the count of labels
        UINT32 labelCount = 0;
        CHECK_FAILURE(sensitivityLabelsCollection->get_Count(&labelCount));

        if (labelCount == 0)
        {
            labelsString += L"\n\nNo labels present";
        }
        else
        {
            // Extract label information into a vector of pairs
            std::vector<std::pair<std::wstring, std::wstring>> labelInfos =
                ExtractSensitivityLabelInfo(sensitivityLabelsCollection.Get());

            // Generate the formatted string from the label information
            labelsString += L"\n\nDetected " + std::to_wstring(labelCount) + L" label(s):\n";
            labelsString += GenerateLabelInfoString(labelInfos);
        }
    }

    // Show the sensitivity labels in a popup dialog
    // Using a sync message box here to ensure the user sees the label change
    // in the right order if multiple changes occur quickly
    MessageBoxW(
        m_appWindow->GetMainWindow(), labelsString.c_str(), L"Sensitivity Label Changed",
        MB_OK | MB_ICONINFORMATION);
}

//! [SensitivityLabels]
std::vector<std::pair<std::wstring, std::wstring>> ScenarioSensitivityLabel::
    ExtractSensitivityLabelInfo(
        ICoreWebView2ExperimentalSensitivityLabelCollectionView* labelCollection)
{
    std::vector<std::pair<std::wstring, std::wstring>> labelInfos;

    if (!labelCollection)
    {
        return labelInfos;
    }

    UINT32 labelCount = 0;
    CHECK_FAILURE(labelCollection->get_Count(&labelCount));

    for (UINT32 i = 0; i < labelCount; ++i)
    {
        Microsoft::WRL::ComPtr<ICoreWebView2ExperimentalSensitivityLabel> sensitivityLabel;
        CHECK_FAILURE(labelCollection->GetValueAtIndex(i, &sensitivityLabel));

        COREWEBVIEW2_SENSITIVITY_LABEL_KIND labelKind;
        CHECK_FAILURE(sensitivityLabel->get_LabelKind(&labelKind));
        if (labelKind == COREWEBVIEW2_SENSITIVITY_LABEL_KIND_MIP)
        {
            //! [MipSensitivityLabels]
            Microsoft::WRL::ComPtr<ICoreWebView2ExperimentalMipSensitivityLabel> mipLabel;
            if (SUCCEEDED(sensitivityLabel.As(&mipLabel)))
            {
                wil::unique_cotaskmem_string labelId;
                wil::unique_cotaskmem_string organizationId;
                CHECK_FAILURE(mipLabel->get_LabelId(&labelId));
                CHECK_FAILURE(mipLabel->get_OrganizationId(&organizationId));

                // Store the label id and organization id as a pair. Can be used
                // to query Purview for the allowed rights on the document
                labelInfos.emplace_back(
                    std::wstring(labelId.get()), std::wstring(organizationId.get()));
            }
            //! [MipSensitivityLabels]
        }
    }
    return labelInfos;
}
//! [SensitivityLabels]

std::wstring ScenarioSensitivityLabel::GenerateLabelInfoString(
    const std::vector<std::pair<std::wstring, std::wstring>>& labelInfos)
{
    std::wstring result;

    if (labelInfos.empty())
    {
        result += L"\n\nNo MIP labels present";
        return result;
    }

    for (size_t i = 0; i < labelInfos.size(); ++i)
    {
        const auto& labelInfo = labelInfos[i];

        result += L"\n" + std::to_wstring(i + 1) + L". ";
        result +=
            L"MIP Label\n   ID: " + labelInfo.first + L"\n   Organization: " + labelInfo.second;
    }

    return result;
}
