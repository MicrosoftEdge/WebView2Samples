// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include "stdafx.h"

#include <string>

#include "AppWindow.h"
#include "ComponentBase.h"

// Every release channel. Matches the default of the cluster options object, so
// a spec that is left untouched requests exactly the documented default.
inline constexpr COREWEBVIEW2_RELEASE_CHANNELS kAllReleaseChannels =
    static_cast<COREWEBVIEW2_RELEASE_CHANNELS>(
        COREWEBVIEW2_RELEASE_CHANNELS_STABLE | COREWEBVIEW2_RELEASE_CHANNELS_BETA |
        COREWEBVIEW2_RELEASE_CHANNELS_DEV | COREWEBVIEW2_RELEASE_CHANNELS_CANARY);

// Describes the shared cluster environment a host wants. Used both by the
// scenario dialog and by the command-line launch path (--clustername=...), so
// a second host process can reconstruct an identical option set and join.
struct ClusterEnvironmentSpec
{
    std::wstring clusterName;
    std::wstring language;
    std::wstring additionalBrowserArguments;
    bool allowSingleSignOn = false;
    bool enableTrackingPrevention = true;
    bool areBrowserExtensionsEnabled = false;
    bool perHostProfileIsolation = true;
    // Mask of the release channels shared environment creation searches. Part
    // of the pinned set, so it selects the channel of the shared browser.
    COREWEBVIEW2_RELEASE_CHANNELS releaseChannels = kAllReleaseChannels;
    // The order in which those channels are searched. LeastStable is the
    // default so the loader prefers a Canary/Dev channel install over the
    // Evergreen stable runtime.
    COREWEBVIEW2_CHANNEL_SEARCH_KIND channelSearchKind =
        COREWEBVIEW2_CHANNEL_SEARCH_KIND_LEAST_STABLE;
};

// Calls CreateOrJoinCoreWebView2ClusterEnvironment for |spec| and, on success,
// opens a sample-app window hosted in the returned shared environment. On an
// options mismatch it shows the pinned options. |isMainWindow| marks the window
// as the process's main one and asks the process to quit if the create fails.
// |parent| parents any dialogs and may be nullptr.
void CreateOrJoinClusterAndOpenWindow(
    const ClusterEnvironmentSpec& spec, bool isMainWindow, HWND parent);

// Reads and displays (read-only) the options pinned for cluster |clusterName|,
// or a "not pinned" message when no cluster exists for it. |parent| may be
// nullptr.
void ShowPinnedClusterOptions(const std::wstring& clusterName, HWND parent);

// Launches a second instance of this sample app as a separate host process that
// joins the cluster described by |spec|. This demonstrates that multiple host
// processes share a single browser process tree.
void LaunchClusterHostProcess(const ClusterEnvironmentSpec& spec);

// Demonstrates the "Shared WebView2 Cluster Environment" API. "Create or Join"
// checks the requested options against any running cluster and then launches a
// second host process, which is what actually creates or joins and hosts its
// window in the shared environment. "Get Options" reads the options pinned for
// a cluster name.
class ScenarioClusterEnvironment : public ComponentBase
{
public:
    // Selects which part of the cluster API this scenario exercises.
    enum class Mode
    {
        // Prompt for a cluster name and options, then CreateOrJoin the cluster.
        CreateOrJoin,
        // Prompt for a cluster name, then read and display its pinned options.
        GetOptions,
    };

    ScenarioClusterEnvironment(AppWindow* appWindow, Mode mode = Mode::CreateOrJoin);
    ~ScenarioClusterEnvironment() override;

private:
    // Shows the input dialog, builds the spec, reports a mismatch against a
    // cluster that is already running, and otherwise spawns the host process
    // that creates or joins. Returns false if the user cancelled.
    bool PromptAndCreateOrJoin();

    // Prompts for a cluster name and displays the options pinned for it.
    void PromptAndGetOptions();

    AppWindow* m_appWindow = nullptr;
};
