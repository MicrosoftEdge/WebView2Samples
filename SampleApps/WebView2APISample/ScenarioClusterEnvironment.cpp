// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "ScenarioClusterEnvironment.h"

#include <sstream>
#include <string>
#include <utility>

#include "CheckFailure.h"
#include "TextInputDialog.h"
#include "resource.h"

// Generated SDK header for the cluster options object, included by bare name
// like the other generated WebView2 SDK headers.
#include "ClusterEnvironmentOptions.h"

using namespace Microsoft::WRL;

namespace
{
// Checkbox values identifying each boolean cluster option.
enum ClusterOption
{
    kAllowSingleSignOn = 1,
    kEnableTrackingPrevention,
    kAreBrowserExtensionsEnabled,
    kPerHostProfileIsolation,
};

constexpr wchar_t kClusterNameLabel[] = L"Cluster Name";
constexpr wchar_t kLanguageLabel[] = L"Language (blank for default)";
constexpr wchar_t kBrowserArgsLabel[] = L"Additional Browser Arguments";
constexpr wchar_t kOptionsGroupLabel[] = L"Environment options:";
constexpr wchar_t kChannelsGroupLabel[] = L"Release channels to search:";
constexpr wchar_t kSearchKindLabel[] = L"Channel search order";
constexpr wchar_t kDefaultClusterName[] = L"SampleCluster";

// Dropdown entries for kSearchKindLabel, ordered to match the values of
// COREWEBVIEW2_CHANNEL_SEARCH_KIND (MostStable is 0, LeastStable is 1).
constexpr wchar_t kMostStableLabel[] = L"MostStable";
constexpr wchar_t kLeastStableLabel[] = L"LeastStable";

// Reads a string property and returns it, freeing the COM allocation.
std::wstring GetOptionString(
    ICoreWebView2ExperimentalClusterEnvironmentOptions* options,
    HRESULT (STDMETHODCALLTYPE ICoreWebView2ExperimentalClusterEnvironmentOptions::*getter)(
        LPWSTR*))
{
    wil::unique_cotaskmem_string value;
    if (SUCCEEDED((options->*getter)(&value)) && value)
        return value.get();
    return std::wstring();
}

// Reads a bool property, returning false on failure.
bool GetOptionBool(
    ICoreWebView2ExperimentalClusterEnvironmentOptions* options,
    HRESULT (STDMETHODCALLTYPE ICoreWebView2ExperimentalClusterEnvironmentOptions::*getter)(
        BOOL*))
{
    BOOL value = FALSE;
    if (SUCCEEDED((options->*getter)(&value)))
        return value != FALSE;
    return false;
}

// Reads the release-channels mask, falling back to the documented default.
COREWEBVIEW2_RELEASE_CHANNELS GetOptionReleaseChannels(
    ICoreWebView2ExperimentalClusterEnvironmentOptions* options)
{
    COREWEBVIEW2_RELEASE_CHANNELS value = kAllReleaseChannels;
    if (SUCCEEDED(options->get_ReleaseChannels(&value)))
        return value;
    return kAllReleaseChannels;
}

// Reads the channel search order, falling back to the documented default.
COREWEBVIEW2_CHANNEL_SEARCH_KIND GetOptionChannelSearchKind(
    ICoreWebView2ExperimentalClusterEnvironmentOptions* options)
{
    COREWEBVIEW2_CHANNEL_SEARCH_KIND value = COREWEBVIEW2_CHANNEL_SEARCH_KIND_MOST_STABLE;
    if (SUCCEEDED(options->get_ChannelSearchKind(&value)))
        return value;
    return COREWEBVIEW2_CHANNEL_SEARCH_KIND_MOST_STABLE;
}

// Renders a release-channels mask as "Stable | Beta", or "None" when empty.
std::wstring FormatReleaseChannels(COREWEBVIEW2_RELEASE_CHANNELS channels)
{
    const std::pair<COREWEBVIEW2_RELEASE_CHANNELS, PCWSTR> kNames[] = {
        {COREWEBVIEW2_RELEASE_CHANNELS_STABLE, L"Stable"},
        {COREWEBVIEW2_RELEASE_CHANNELS_BETA, L"Beta"},
        {COREWEBVIEW2_RELEASE_CHANNELS_DEV, L"Dev"},
        {COREWEBVIEW2_RELEASE_CHANNELS_CANARY, L"Canary"},
    };

    std::wstring result;
    for (const auto& entry : kNames)
    {
        if ((channels & entry.first) == 0)
            continue;
        if (!result.empty())
            result += L" | ";
        result += entry.second;
    }
    return result.empty() ? L"None" : result;
}

// Renders a channel search order as its enum name.
PCWSTR FormatChannelSearchKind(COREWEBVIEW2_CHANNEL_SEARCH_KIND kind)
{
    return kind == COREWEBVIEW2_CHANNEL_SEARCH_KIND_LEAST_STABLE ? kLeastStableLabel
                                                                 : kMostStableLabel;
}

// Builds a cluster options object from |spec|.
ComPtr<ICoreWebView2ExperimentalClusterEnvironmentOptions> BuildClusterOptions(
    const ClusterEnvironmentSpec& spec)
{
    auto options = Make<embedded_browser_webview::CoreWebView2ClusterEnvironmentOptions>();
    CHECK_FAILURE(options->put_ClusterName(spec.clusterName.c_str()));
    if (!spec.language.empty())
        CHECK_FAILURE(options->put_Language(spec.language.c_str()));
    if (!spec.additionalBrowserArguments.empty())
        CHECK_FAILURE(
            options->put_AdditionalBrowserArguments(spec.additionalBrowserArguments.c_str()));
    CHECK_FAILURE(options->put_AllowSingleSignOnUsingOSPrimaryAccount(
        spec.allowSingleSignOn ? TRUE : FALSE));
    CHECK_FAILURE(
        options->put_EnableTrackingPrevention(spec.enableTrackingPrevention ? TRUE : FALSE));
    CHECK_FAILURE(options->put_AreBrowserExtensionsEnabled(
        spec.areBrowserExtensionsEnabled ? TRUE : FALSE));
    CHECK_FAILURE(
        options->put_PerHostProfileIsolation(spec.perHostProfileIsolation ? TRUE : FALSE));
    CHECK_FAILURE(options->put_ReleaseChannels(spec.releaseChannels));
    CHECK_FAILURE(options->put_ChannelSearchKind(spec.channelSearchKind));
    return options;
}

// Opens a new sample-app window whose WebView is hosted in the shared cluster
// environment. AppWindow deletes itself when its window is closed.
void OpenWindowInSharedEnvironment(ICoreWebView2Environment* environment, bool isMainWindow)
{
    WebViewCreateOption opt;
    new AppWindow(
        IDM_CREATION_MODE_WINDOWED, opt, /*initialUri=*/L"", /*userDataFolderParam=*/L"",
        isMainWindow, /*webviewCreatedCallback=*/nullptr, /*customWindowRect=*/false,
        /*windowRect=*/{0}, /*shouldHaveToolbar=*/true, /*isPopup=*/false,
        /*providedEnvironment=*/environment);
}

// Builds a human-readable, multi-line summary of a cluster options object.
std::wstring FormatClusterOptions(ICoreWebView2ExperimentalClusterEnvironmentOptions* options)
{
    std::wostringstream summary;
    summary << L"ClusterName: "
            << GetOptionString(
                   options,
                   &ICoreWebView2ExperimentalClusterEnvironmentOptions::get_ClusterName)
            << L"\r\nLanguage: "
            << GetOptionString(
                   options, &ICoreWebView2ExperimentalClusterEnvironmentOptions::get_Language)
            << L"\r\nAdditionalBrowserArguments: "
            << GetOptionString(
                   options, &ICoreWebView2ExperimentalClusterEnvironmentOptions::
                                get_AdditionalBrowserArguments)
            << L"\r\nAllowSingleSignOnUsingOSPrimaryAccount: "
            << (GetOptionBool(
                    options, &ICoreWebView2ExperimentalClusterEnvironmentOptions::
                                 get_AllowSingleSignOnUsingOSPrimaryAccount)
                    ? L"true"
                    : L"false")
            << L"\r\nEnableTrackingPrevention: "
            << (GetOptionBool(
                    options, &ICoreWebView2ExperimentalClusterEnvironmentOptions::
                                 get_EnableTrackingPrevention)
                    ? L"true"
                    : L"false")
            << L"\r\nAreBrowserExtensionsEnabled: "
            << (GetOptionBool(
                    options, &ICoreWebView2ExperimentalClusterEnvironmentOptions::
                                 get_AreBrowserExtensionsEnabled)
                    ? L"true"
                    : L"false")
            << L"\r\nPerHostProfileIsolation: "
            << (GetOptionBool(
                    options, &ICoreWebView2ExperimentalClusterEnvironmentOptions::
                                 get_PerHostProfileIsolation)
                    ? L"true"
                    : L"false")
            << L"\r\nReleaseChannels: "
            << FormatReleaseChannels(GetOptionReleaseChannels(options))
            << L"\r\nChannelSearchKind: "
            << FormatChannelSearchKind(GetOptionChannelSearchKind(options));
    return summary.str();
}

// Returns true if the requested |spec| matches the |pinned| option set on the
// scalar fields this sample exposes (the cluster name is the rendezvous key
// and is excluded). Mirrors the loader's strict, full-set equality closely
// enough for a pre-flight check before launching a second host process.
bool SpecMatchesPinned(
    const ClusterEnvironmentSpec& spec,
    ICoreWebView2ExperimentalClusterEnvironmentOptions* pinned)
{
    return spec.language ==
               GetOptionString(
                   pinned, &ICoreWebView2ExperimentalClusterEnvironmentOptions::get_Language) &&
           spec.additionalBrowserArguments ==
               GetOptionString(
                   pinned, &ICoreWebView2ExperimentalClusterEnvironmentOptions::
                               get_AdditionalBrowserArguments) &&
           spec.allowSingleSignOn ==
               GetOptionBool(
                   pinned, &ICoreWebView2ExperimentalClusterEnvironmentOptions::
                               get_AllowSingleSignOnUsingOSPrimaryAccount) &&
           spec.enableTrackingPrevention ==
               GetOptionBool(
                   pinned, &ICoreWebView2ExperimentalClusterEnvironmentOptions::
                               get_EnableTrackingPrevention) &&
           spec.areBrowserExtensionsEnabled ==
               GetOptionBool(
                   pinned, &ICoreWebView2ExperimentalClusterEnvironmentOptions::
                               get_AreBrowserExtensionsEnabled) &&
           spec.perHostProfileIsolation ==
               GetOptionBool(
                   pinned, &ICoreWebView2ExperimentalClusterEnvironmentOptions::
                               get_PerHostProfileIsolation) &&
           spec.releaseChannels == GetOptionReleaseChannels(pinned) &&
           spec.channelSearchKind == GetOptionChannelSearchKind(pinned);
}

// Shows a read-only, multi-line summary in a single scrollable text box, since
// the dialog's description field is fixed size and truncates.
void ShowClusterSummaryDialog(
    HWND parent, PCWSTR title, PCWSTR prompt, const std::wstring& heading,
    const std::wstring& summary)
{
    TextInputDialog::Builder(parent, title, prompt)
        .AddTextArea(
            heading, summary, /*readOnly=*/true, /*labelHeight=*/18,
            /*inputHeight=*/160)
        .Build();
}
// The loader rejects a cluster name with a trailing space or dot, and a
// leading space names a different cluster.
std::wstring Trimmed(const std::wstring& value)
{
    const size_t first = value.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos)
        return std::wstring();
    return value.substr(first, value.find_last_not_of(L" \t\r\n") - first + 1);
}
} // namespace

void CreateOrJoinClusterAndOpenWindow(
    const ClusterEnvironmentSpec& spec, bool isMainWindow, HWND parent)
{
    auto options = BuildClusterOptions(spec);
    std::wstring clusterName = spec.clusterName;

    HRESULT hr = CreateOrJoinCoreWebView2ClusterEnvironment(
        options.Get(),
        Callback<ICoreWebView2ExperimentalCreateOrJoinClusterEnvironmentCompletedHandler>(
            [isMainWindow, parent, clusterName](
                HRESULT errorCode,
                ICoreWebView2ExperimentalClusterEnvironmentCreateResult* result) -> HRESULT
            {
                if (FAILED(errorCode) || !result)
                {
                    ShowFailure(
                        errorCode, L"CreateOrJoinCoreWebView2ClusterEnvironment failed");
                    if (isMainWindow)
                        PostQuitMessage(1);
                    return S_OK;
                }

                COREWEBVIEW2_CLUSTER_ENVIRONMENT_STATUS status =
                    COREWEBVIEW2_CLUSTER_ENVIRONMENT_STATUS_SUCCEEDED;
                CHECK_FAILURE(result->get_Status(&status));
                if (status == COREWEBVIEW2_CLUSTER_ENVIRONMENT_STATUS_OPTIONS_MISMATCH)
                {
                    // A cluster already exists for this ClusterName with a
                    // different pinned option set, so this host cannot join it.
                    ShowPinnedClusterOptions(clusterName, parent);
                    if (isMainWindow)
                        PostQuitMessage(1);
                    return S_OK;
                }
                if (status == COREWEBVIEW2_CLUSTER_ENVIRONMENT_STATUS_NOT_SUPPORTED)
                {
                    // Reported as a status rather than a failure, so a real
                    // application would fall back to a private environment.
                    MessageBox(
                        parent,
                        L"Cluster environments are not supported in this host "
                        L"process. Use a private environment instead.",
                        L"Shared Cluster Environment", MB_OK);
                    if (isMainWindow)
                        PostQuitMessage(1);
                    return S_OK;
                }

                // Only SUCCEEDED carries an environment. Anything else, now or
                // once the enum grows, has none to hand out.
                if (status != COREWEBVIEW2_CLUSTER_ENVIRONMENT_STATUS_SUCCEEDED)
                {
                    ShowFailure(E_UNEXPECTED, L"CreateOrJoin reported an unrecognized status.");
                    if (isMainWindow)
                        PostQuitMessage(1);
                    return S_OK;
                }

                wil::com_ptr<ICoreWebView2Environment> environment;
                CHECK_FAILURE(result->get_Environment(&environment));
                OpenWindowInSharedEnvironment(environment.get(), isMainWindow);
                return S_OK;
            })
            .Get());
    if (FAILED(hr))
    {
        ShowFailure(hr, L"CreateOrJoinCoreWebView2ClusterEnvironment call failed");
        if (isMainWindow)
            PostQuitMessage(1);
    }
}

void ShowPinnedClusterOptions(const std::wstring& clusterName, HWND parent)
{
    wil::com_ptr<ICoreWebView2ExperimentalClusterEnvironmentOptions> pinned;
    HRESULT hr = GetCoreWebView2ClusterEnvironmentOptions(clusterName.c_str(), &pinned);
    if (SUCCEEDED(hr) && !pinned)
    {
        std::wstring message =
            L"No cluster is currently pinned for ClusterName \"" + clusterName + L"\".";
        MessageBox(parent, message.c_str(), L"Get Cluster Options", MB_OK);
        return;
    }
    if (FAILED(hr) || !pinned)
    {
        ShowFailure(hr, L"Failed to read the pinned cluster options.");
        return;
    }

    ShowClusterSummaryDialog(
        parent, L"Pinned Cluster Options", L"Options pinned for this cluster",
        L"Pinned options:", FormatClusterOptions(pinned.get()));
}

// Quotes `value` so CommandLineToArgvW in the child rebuilds it exactly. None
// of these fields forbid a space, and an unquoted one would reach the child as
// two arguments, leaving the two hosts asking for different clusters.
std::wstring QuoteForChildCommandLine(const std::wstring& value)
{
    std::wstring quoted = L"\"";
    for (auto it = value.begin();; ++it)
    {
        size_t backslashes = 0;
        while (it != value.end() && *it == L'\\')
        {
            ++it;
            ++backslashes;
        }
        if (it == value.end())
        {
            // These precede the closing quote, so they must not escape it.
            quoted.append(backslashes * 2, L'\\');
            break;
        }
        if (*it == L'"')
        {
            // Escape the run, then the quote itself.
            quoted.append(backslashes * 2 + 1, L'\\');
        }
        else
        {
            quoted.append(backslashes, L'\\');
        }
        quoted.push_back(*it);
    }
    quoted.push_back(L'"');
    return quoted;
}

void LaunchClusterHostProcess(const ClusterEnvironmentSpec& spec)
{
    wchar_t exePath[MAX_PATH] = {};
    const DWORD pathLength = GetModuleFileNameW(nullptr, exePath, ARRAYSIZE(exePath));
    // A path that did not fit returns the buffer size rather than zero, so the
    // truncated value has to be rejected too: launching it would either fail or
    // start something other than this sample.
    if (pathLength == 0 || pathLength == ARRAYSIZE(exePath))
    {
        ShowFailure(HRESULT_FROM_WIN32(GetLastError()), L"Failed to locate the sample app.");
        return;
    }

    // Pass every option explicitly so the second host reconstructs an identical
    // set and joins rather than mismatching.
    std::wostringstream cmd;
    cmd << L"\"" << exePath << L"\"" << L" --clustername="
        << QuoteForChildCommandLine(spec.clusterName) << L" --clusterlang="
        << QuoteForChildCommandLine(spec.language) << L" --clustersso="
        << (spec.allowSingleSignOn ? 1 : 0) << L" --clustertracking="
        << (spec.enableTrackingPrevention ? 1 : 0) << L" --clusterextensions="
        << (spec.areBrowserExtensionsEnabled ? 1 : 0) << L" --clusterisolation="
        << (spec.perHostProfileIsolation ? 1 : 0) << L" --clusterchannels="
        << static_cast<int>(spec.releaseChannels) << L" --clustersearchkind="
        << static_cast<int>(spec.channelSearchKind);
    if (!spec.additionalBrowserArguments.empty())
        cmd << L" --clusterargs=" << QuoteForChildCommandLine(spec.additionalBrowserArguments);
    std::wstring commandLine = cmd.str();

    STARTUPINFOW startupInfo = {sizeof(startupInfo)};
    PROCESS_INFORMATION processInfo = {};
    if (!CreateProcessW(
            exePath, commandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr,
            &startupInfo, &processInfo))
    {
        ShowFailure(
            HRESULT_FROM_WIN32(GetLastError()),
            L"Failed to launch a second host process for the cluster.");
        return;
    }
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
}

ScenarioClusterEnvironment::ScenarioClusterEnvironment(AppWindow* appWindow, Mode mode)
    : m_appWindow(appWindow)
{
    switch (mode)
    {
    case Mode::CreateOrJoin:
        PromptAndCreateOrJoin();
        break;
    case Mode::GetOptions:
        PromptAndGetOptions();
        break;
    }
}

ScenarioClusterEnvironment::~ScenarioClusterEnvironment()
{
}

bool ScenarioClusterEnvironment::PromptAndCreateOrJoin()
{
    TextInputDialog dialog =
        TextInputDialog::Builder(
            m_appWindow->GetMainWindow(), L"Shared Cluster Environment",
            L"Launch a new host process that joins this shared cluster")
            .AddTextArea(kClusterNameLabel, kDefaultClusterName, false, 20, 24)
            .AddTextArea(kLanguageLabel, L"", false, 20, 24)
            .AddTextArea(kBrowserArgsLabel, L"", false, 20, 24)
            .AddCheckBoxGroup(
                kOptionsGroupLabel,
                {{L"AllowSingleSignOnUsingOSPrimaryAccount", kAllowSingleSignOn, false},
                 {L"EnableTrackingPrevention", kEnableTrackingPrevention, true},
                 {L"AreBrowserExtensionsEnabled", kAreBrowserExtensionsEnabled, false},
                 {L"PerHostProfileIsolation", kPerHostProfileIsolation, true}})
            .AddCheckBoxGroup(
                kChannelsGroupLabel, {{L"Stable", COREWEBVIEW2_RELEASE_CHANNELS_STABLE, true},
                                      {L"Beta", COREWEBVIEW2_RELEASE_CHANNELS_BETA, true},
                                      {L"Dev", COREWEBVIEW2_RELEASE_CHANNELS_DEV, true},
                                      {L"Canary", COREWEBVIEW2_RELEASE_CHANNELS_CANARY, true}})
            .AddDropDown(
                kSearchKindLabel, {kMostStableLabel, kLeastStableLabel},
                COREWEBVIEW2_CHANNEL_SEARCH_KIND_LEAST_STABLE)
            .Build();

    if (!dialog.confirmed)
        return false;

    ClusterEnvironmentSpec spec;

    // Cluster name: fall back to the default when the user left it blank.
    spec.clusterName = kDefaultClusterName;
    auto nameIt = dialog.results.find(kClusterNameLabel);
    if (nameIt != dialog.results.end())
    {
        std::wstring input = Trimmed(std::get<TextArea>(nameIt->second).input);
        if (!input.empty())
            spec.clusterName = input;
    }

    auto langIt = dialog.results.find(kLanguageLabel);
    if (langIt != dialog.results.end())
        spec.language = Trimmed(std::get<TextArea>(langIt->second).input);

    auto argsIt = dialog.results.find(kBrowserArgsLabel);
    if (argsIt != dialog.results.end())
        spec.additionalBrowserArguments = Trimmed(std::get<TextArea>(argsIt->second).input);

    auto groupIt = dialog.results.find(kOptionsGroupLabel);
    if (groupIt != dialog.results.end())
    {
        auto& group = std::get<CheckBoxGroup>(groupIt->second);
        for (const auto& option : group.options)
        {
            switch (option.value)
            {
            case kAllowSingleSignOn:
                spec.allowSingleSignOn = option.isSelected;
                break;
            case kEnableTrackingPrevention:
                spec.enableTrackingPrevention = option.isSelected;
                break;
            case kAreBrowserExtensionsEnabled:
                spec.areBrowserExtensionsEnabled = option.isSelected;
                break;
            case kPerHostProfileIsolation:
                spec.perHostProfileIsolation = option.isSelected;
                break;
            }
        }
    }

    // Clearing every box yields NONE, which leaves no eligible channel and
    // makes creation fail. Passed through unchanged so that stays observable.
    auto channelsIt = dialog.results.find(kChannelsGroupLabel);
    if (channelsIt != dialog.results.end())
    {
        int mask = COREWEBVIEW2_RELEASE_CHANNELS_NONE;
        for (const auto& option : std::get<CheckBoxGroup>(channelsIt->second).options)
        {
            if (option.isSelected)
                mask |= option.value;
        }
        spec.releaseChannels = static_cast<COREWEBVIEW2_RELEASE_CHANNELS>(mask);
    }

    auto searchKindIt = dialog.results.find(kSearchKindLabel);
    if (searchKindIt != dialog.results.end())
    {
        spec.channelSearchKind = std::get<DropDown>(searchKindIt->second).selectedIndex ==
                                         COREWEBVIEW2_CHANNEL_SEARCH_KIND_LEAST_STABLE
                                     ? COREWEBVIEW2_CHANNEL_SEARCH_KIND_LEAST_STABLE
                                     : COREWEBVIEW2_CHANNEL_SEARCH_KIND_MOST_STABLE;
    }

    // Pre-flight the join in this process instead of launching a second host
    // that would only fail: if a cluster already exists for this ClusterName
    // with a different option set, report the mismatch now and do not spawn.
    wil::com_ptr<ICoreWebView2ExperimentalClusterEnvironmentOptions> pinned;
    HRESULT hr = GetCoreWebView2ClusterEnvironmentOptions(spec.clusterName.c_str(), &pinned);
    if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
    {
        // No cluster running is expected and reported as success with no
        // options. Anything else is a real failure worth surfacing here.
        ShowFailure(hr, L"Failed to read the pinned cluster options.");
        return true;
    }
    if (SUCCEEDED(hr) && pinned && !SpecMatchesPinned(spec, pinned.get()))
    {
        ShowClusterSummaryDialog(
            m_appWindow->GetMainWindow(), L"Cannot Join: Options Mismatch",
            L"The requested options do not match the running cluster",
            L"Options of the running cluster for ClusterName \"" + spec.clusterName + L"\":",
            FormatClusterOptions(pinned.get()));
        return true;
    }

    // Launch this twice, or alongside another instance, with the same
    // ClusterName and options to see several host processes share one browser.
    LaunchClusterHostProcess(spec);
    return true;
}

void ScenarioClusterEnvironment::PromptAndGetOptions()
{
    TextInputDialog dialog(
        m_appWindow->GetMainWindow(), L"Get Cluster Options",
        L"Read the options pinned for a cluster",
        L"Enter the cluster name to read its pinned options without spawning a browser.",
        kDefaultClusterName);
    if (!dialog.confirmed)
        return;

    std::wstring input = Trimmed(dialog.input);
    std::wstring clusterName = input.empty() ? kDefaultClusterName : input;
    ShowPinnedClusterOptions(clusterName, m_appWindow->GetMainWindow());
}
