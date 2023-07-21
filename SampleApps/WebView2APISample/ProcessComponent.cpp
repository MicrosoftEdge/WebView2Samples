// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "psapi.h"

#include <sstream>

#include "ProcessComponent.h"
#include "CheckFailure.h"

using namespace Microsoft::WRL;

ProcessComponent::ProcessComponent(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    //! [ProcessFailed]
    // Register a handler for the ProcessFailed event.
    // This handler checks the failure kind and tries to:
    //   * Recreate the webview for browser failure and render unresponsive.
    //   * Reload the webview for render failure.
    //   * Reload the webview for frame-only render failure impacting app content.
    //   * Log information about the failure for other failures.
    CHECK_FAILURE(m_webView->add_ProcessFailed(
        Callback<ICoreWebView2ProcessFailedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2ProcessFailedEventArgs* argsRaw)
                -> HRESULT {
                wil::com_ptr<ICoreWebView2ProcessFailedEventArgs> args = argsRaw;
                COREWEBVIEW2_PROCESS_FAILED_KIND kind;
                CHECK_FAILURE(args->get_ProcessFailedKind(&kind));
                if (kind == COREWEBVIEW2_PROCESS_FAILED_KIND_BROWSER_PROCESS_EXITED)
                {
                    // Do not run a message loop from within the event handler
                    // as that could lead to reentrancy and leave the event
                    // handler in stack indefinitely. Instead, schedule the
                    // appropriate work to take place after completion of the
                    // event handler.
                    ScheduleReinitIfSelectedByUser(
                        L"Browser process exited unexpectedly.  Recreate webview?",
                        L"Browser process exited");
                }
                else if (kind == COREWEBVIEW2_PROCESS_FAILED_KIND_RENDER_PROCESS_UNRESPONSIVE)
                {
                    ScheduleReinitIfSelectedByUser(
                        L"Browser render process has stopped responding.  Recreate webview?",
                        L"Web page unresponsive");
                }
                else if (kind == COREWEBVIEW2_PROCESS_FAILED_KIND_RENDER_PROCESS_EXITED)
                {
                    // Reloading the page will start a new render process if
                    // needed.
                    ScheduleReloadIfSelectedByUser(
                        L"Browser render process exited unexpectedly. Reload page?",
                        L"Web page unresponsive");
                }
                // Check the runtime event args implements the newer interface.
                auto args2 = args.try_query<ICoreWebView2ProcessFailedEventArgs2>();
                if (!args2)
                {
                    return S_OK;
                }
                if (kind == COREWEBVIEW2_PROCESS_FAILED_KIND_FRAME_RENDER_PROCESS_EXITED)
                {
                    // A frame-only renderer has exited unexpectedly. Check if
                    // reload is needed.
                    wil::com_ptr<ICoreWebView2FrameInfoCollection> frameInfos;
                    wil::com_ptr<ICoreWebView2FrameInfoCollectionIterator> iterator;
                    CHECK_FAILURE(args2->get_FrameInfosForFailedProcess(&frameInfos));
                    CHECK_FAILURE(frameInfos->GetIterator(&iterator));

                    BOOL hasCurrent = FALSE;
                    while (SUCCEEDED(iterator->get_HasCurrent(&hasCurrent)) && hasCurrent)
                    {
                        wil::com_ptr<ICoreWebView2FrameInfo> frameInfo;
                        CHECK_FAILURE(iterator->GetCurrent(&frameInfo));

                        wil::unique_cotaskmem_string nameRaw;
                        wil::unique_cotaskmem_string sourceRaw;
                        CHECK_FAILURE(frameInfo->get_Name(&nameRaw));
                        CHECK_FAILURE(frameInfo->get_Source(&sourceRaw));
                        if (IsAppContentUri(sourceRaw.get()))
                        {
                            ScheduleReloadIfSelectedByUser(
                                L"Browser render process for app frame exited unexpectedly. "
                                L"Reload page?",
                                L"App content frame unresponsive");
                            break;
                        }

                        BOOL hasNext = FALSE;
                        CHECK_FAILURE(iterator->MoveNext(&hasNext));
                    }
                }
                else
                {
                    // Show the process failure details. Apps can collect info for their logging
                    // purposes.
                    COREWEBVIEW2_PROCESS_FAILED_REASON reason;
                    wil::unique_cotaskmem_string processDescription;
                    int exitCode;

                    CHECK_FAILURE(args2->get_Reason(&reason));
                    CHECK_FAILURE(args2->get_ProcessDescription(&processDescription));
                    CHECK_FAILURE(args2->get_ExitCode(&exitCode));

                    std::wstringstream message;
                    message << L"Kind: "                << ProcessFailedKindToString(kind) << L"\n"
                            << L"Reason: "              << ProcessFailedReasonToString(reason) << L"\n"
                            << L"Exit code: "           << std::to_wstring(exitCode) << L"\n"
                            << L"Process description: " << processDescription.get() << std::endl;
                    m_appWindow->AsyncMessageBox( std::move(message.str()), L"Child process failed");
                }
                return S_OK;
            })
            .Get(),
        &m_processFailedToken));
    //! [ProcessFailed]

    m_webViewEnvironment = appWindow->GetWebViewEnvironment();
    auto environment8 = m_webViewEnvironment.try_query<ICoreWebView2Environment8>();
    if (environment8)
    {
        CHECK_FAILURE(environment8->GetProcessInfos(&m_processCollection));
        // Register a handler for the ProcessInfosChanged event.
        //! [ProcessInfosChanged]
        CHECK_FAILURE(environment8->add_ProcessInfosChanged(
            Callback<ICoreWebView2ProcessInfosChangedEventHandler>(
                [this](ICoreWebView2Environment* sender, IUnknown* args) -> HRESULT {
                    wil::com_ptr<ICoreWebView2Environment8> webviewEnvironment;
                    sender->QueryInterface(IID_PPV_ARGS(&webviewEnvironment));
                    CHECK_FAILURE(
                        webviewEnvironment->GetProcessInfos(&m_processCollection));
                    return S_OK;
                })
                .Get(),
            &m_processInfosChangedToken));
        //! [ProcessInfosChanged]
    }
}

// static
bool ProcessComponent::IsAppContentUri(const std::wstring& source)
{
    wil::com_ptr<IUri> uri;
    CHECK_FAILURE(CreateUri(source.c_str(), Uri_CREATE_CANONICALIZE, 0, &uri));
    wil::unique_bstr domain;
    CHECK_FAILURE(uri->GetDomain(&domain));

    // Content from our app uses a mapped host name.
    const std::wstring mappedAppHostName = L"appassets.example";
    return domain.get() == mappedAppHostName;
}

bool ProcessComponent::HandleWindowMessage(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT* result)
{
    if (message == WM_COMMAND)
    {
        switch (LOWORD(wParam))
        {
        case IDM_PROCESS_INFO:
            ShowBrowserProcessInfo();
            return true;
        case IDM_CRASH_PROCESS:
            CrashBrowserProcess();
            return true;
        case IDM_CRASH_RENDER_PROCESS:
            CrashRenderProcess();
            return true;
        case IDM_PERFORMANCE_INFO:
            PerformanceInfo();
            return true;
        case IDM_PROCESS_FRAME_INFO:
            ShowProcessFrameInfo();
            return true;
        }
    }
    return false;
}

// Show the WebView's PID to the user.
void ProcessComponent::ShowBrowserProcessInfo()
{
    UINT32 processId;
    m_webView->get_BrowserProcessId(&processId);

    WCHAR buffer[4096] = L"";
    StringCchPrintf(buffer, ARRAYSIZE(buffer), L"Process ID: %u\n", processId);
    MessageBox(m_appWindow->GetMainWindow(), buffer, L"Process Info", MB_OK);
}

// Get a string for the frame kind enum value.
std::wstring ProcessComponent::FrameKindToString(const COREWEBVIEW2_FRAME_KIND kind)
{
    switch (kind)
    {
#define KIND_ENTRY(kindValue)                                                                  \
    case kindValue:                                                                            \
        return L#kindValue;

        KIND_ENTRY(COREWEBVIEW2_FRAME_KIND_MAIN_FRAME);
        KIND_ENTRY(COREWEBVIEW2_FRAME_KIND_IFRAME);
        KIND_ENTRY(COREWEBVIEW2_FRAME_KIND_OTHER);

#undef KIND_ENTRY
    }

    return std::to_wstring(static_cast<uint32_t>(kind));
}

void ProcessComponent::AppendFrameInfo(
    wil::com_ptr<ICoreWebView2FrameInfo> frameInfo, std::wstring& result)
{
    if (!frameInfo)
    {
        return;
    }

    wil::unique_cotaskmem_string nameRaw;
    CHECK_FAILURE(frameInfo->get_Name(&nameRaw));
    result.append(L"{frame name:");
    result.append(nameRaw.get());

    wil::com_ptr<ICoreWebView2ExperimentalFrameInfo> frameInfoExperimental;
    CHECK_FAILURE(frameInfo->QueryInterface(IID_PPV_ARGS(&frameInfoExperimental)));
    UINT32 frameId = 0;
    frameInfoExperimental->get_FrameId(&frameId);
    result.append(L" | frame Id:" + std::to_wstring(frameId));

    BOOL isMainFrameOrFirstLevelframeInfo = false;
    wil::com_ptr<ICoreWebView2FrameInfo> mainFrameInfo = GetAncestorMainFrameInfo(frameInfo);
    wil::com_ptr<ICoreWebView2FrameInfo> firstLevelFrameInfo =
        GetAncestorFirstLevelFrameInfo(frameInfo);
    // check if a frame is a main frame.
    if (mainFrameInfo == frameInfo)
    {
        result.append(L" | frame type: main frame");
        isMainFrameOrFirstLevelframeInfo = true;
    }
    // check if a frame is a first level frame.
    if (firstLevelFrameInfo == frameInfo)
    {
        result.append(L" | frame type: first level frame");
        isMainFrameOrFirstLevelframeInfo = true;
    }
    if (!isMainFrameOrFirstLevelframeInfo)
    {
        result.append(L" | frame type: other child frame");
    }

    COREWEBVIEW2_FRAME_KIND frameKind = COREWEBVIEW2_FRAME_KIND_OTHER;
    frameInfoExperimental->get_FrameKind(&frameKind);
    result.append(L"\n | frame kind:" + FrameKindToString(frameKind));

    wil::com_ptr<ICoreWebView2FrameInfo> parentFrameInfo;
    CHECK_FAILURE(frameInfoExperimental->get_ParentFrameInfo(&parentFrameInfo));
    if (parentFrameInfo)
    {
        CHECK_FAILURE(parentFrameInfo->QueryInterface(IID_PPV_ARGS(&frameInfoExperimental)));
        CHECK_FAILURE(frameInfoExperimental->get_FrameId(&frameId));
        result.append(L" | parent frame Id:" + std::to_wstring(frameId));
    }

    wil::unique_cotaskmem_string sourceRaw;
    CHECK_FAILURE(frameInfo->get_Source(&sourceRaw));
    result.append(L"\n | frame source:\n\"");
    result.append(sourceRaw.get());
    result.append(L"\"");
}

// Get the ancestor main frameInfo.
// Return itself if it's a main frame.
wil::com_ptr<ICoreWebView2FrameInfo> ProcessComponent::GetAncestorMainFrameInfo(
    wil::com_ptr<ICoreWebView2FrameInfo> frameInfo)
{
    wil::com_ptr<ICoreWebView2FrameInfo> mainFrameInfo;
    wil::com_ptr<ICoreWebView2ExperimentalFrameInfo> frameInfoExperimental;
    while (frameInfo)
    {
        mainFrameInfo = frameInfo;
        CHECK_FAILURE(frameInfo->QueryInterface(IID_PPV_ARGS(&frameInfoExperimental)));
        CHECK_FAILURE(frameInfoExperimental->get_ParentFrameInfo(&frameInfo));
    }
    return mainFrameInfo;
}

// Get the ancestor first level frameInfo.
// Return itself if it's a first level frameInfo.
wil::com_ptr<ICoreWebView2FrameInfo> ProcessComponent::GetAncestorFirstLevelFrameInfo(
    wil::com_ptr<ICoreWebView2FrameInfo> frameInfo)
{
    wil::com_ptr<ICoreWebView2FrameInfo> mainFrameInfo;
    wil::com_ptr<ICoreWebView2FrameInfo> firstLevelFrameInfo;
    wil::com_ptr<ICoreWebView2ExperimentalFrameInfo> frameInfoExperimental;
    while (frameInfo)
    {
        firstLevelFrameInfo = mainFrameInfo;
        mainFrameInfo = frameInfo;
        CHECK_FAILURE(frameInfo->QueryInterface(IID_PPV_ARGS(&frameInfoExperimental)));
        CHECK_FAILURE(frameInfoExperimental->get_ParentFrameInfo(&frameInfo));
    }
    return firstLevelFrameInfo;
}

// Append the frame Id of the ancestor first level frame and ancestor main frame.
void ProcessComponent::AppendAncestorFrameInfo(
    wil::com_ptr<ICoreWebView2FrameInfo> frameInfo, std::wstring& result)
{
    if (!frameInfo)
    {
        return;
    }

    wil::com_ptr<ICoreWebView2FrameInfo> mainFrameInfo = GetAncestorMainFrameInfo(frameInfo);
    wil::com_ptr<ICoreWebView2FrameInfo> firstLevelFrameInfo =
        GetAncestorFirstLevelFrameInfo(frameInfo);
    wil::com_ptr<ICoreWebView2ExperimentalFrameInfo> frameInfoExperimental;
    UINT32 frameId = 0;
    if (firstLevelFrameInfo)
    {
        CHECK_FAILURE(
            firstLevelFrameInfo->QueryInterface(IID_PPV_ARGS(&frameInfoExperimental)));
        CHECK_FAILURE(frameInfoExperimental->get_FrameId(&frameId));
        result.append(L"\n | ancestor first level frame Id:" + std::to_wstring(frameId));
    }
    if (mainFrameInfo)
    {
        CHECK_FAILURE(mainFrameInfo->QueryInterface(IID_PPV_ARGS(&frameInfoExperimental)));
        CHECK_FAILURE(frameInfoExperimental->get_FrameId(&frameId));
        result.append(L"\n | ancestor main frame Id:" + std::to_wstring(frameId));
    }
    result.append(L"},\n");
}

void ProcessComponent::ShowProcessFrameInfo()
{
    auto environmentExperimental11 =
        m_webViewEnvironment.try_query<ICoreWebView2ExperimentalEnvironment11>();
    if (environmentExperimental11)
    {
        //! [GetProcessInfosWithDetails]
        CHECK_FAILURE(environmentExperimental11->GetProcessInfosWithDetails(
            Callback<ICoreWebView2ExperimentalGetProcessInfosWithDetailsCompletedHandler>(
                [this](HRESULT error, ICoreWebView2ProcessInfoCollection* processCollection)
                    -> HRESULT
                {
                    UINT32 processCount = 0;
                    UINT32 rendererProcessCount = 0;
                    CHECK_FAILURE(processCollection->get_Count(&processCount));
                    std::wstring result;
                    std::wstring otherProcessResult;
                    for (UINT32 i = 0; i < processCount; i++)
                    {
                        Microsoft::WRL::ComPtr<ICoreWebView2ProcessInfo> processInfo;
                        CHECK_FAILURE(processCollection->GetValueAtIndex(i, &processInfo));
                        COREWEBVIEW2_PROCESS_KIND kind;
                        CHECK_FAILURE(processInfo->get_Kind(&kind));
                        INT32 processId = 0;
                        CHECK_FAILURE(processInfo->get_ProcessId(&processId));
                        if (kind == COREWEBVIEW2_PROCESS_KIND_RENDERER)
                        {
                            //! [AssociatedFrameInfos]
                            std::wstring rendererProcessResult;
                            wil::com_ptr<ICoreWebView2ExperimentalProcessInfo>
                                processInfoExperimental;
                            CHECK_FAILURE(processInfo->QueryInterface(
                                IID_PPV_ARGS(&processInfoExperimental)));
                            wil::com_ptr<ICoreWebView2FrameInfoCollection> frameInfoCollection;
                            CHECK_FAILURE(processInfoExperimental->get_AssociatedFrameInfos(
                                &frameInfoCollection));
                            wil::com_ptr<ICoreWebView2FrameInfoCollectionIterator> iterator;
                            CHECK_FAILURE(frameInfoCollection->GetIterator(&iterator));
                            BOOL hasCurrent = FALSE;
                            UINT32 frameInfoCount = 0;
                            while (SUCCEEDED(iterator->get_HasCurrent(&hasCurrent)) &&
                                   hasCurrent)
                            {
                                wil::com_ptr<ICoreWebView2FrameInfo> frameInfo;
                                CHECK_FAILURE(iterator->GetCurrent(&frameInfo));

                                AppendFrameInfo(frameInfo, rendererProcessResult);
                                AppendAncestorFrameInfo(frameInfo, rendererProcessResult);

                                BOOL hasNext = FALSE;
                                CHECK_FAILURE(iterator->MoveNext(&hasNext));
                                frameInfoCount++;
                            }
                            rendererProcessResult.insert(
                                0, std::to_wstring(frameInfoCount) +
                                       L" frameInfo(s) found in Renderer Process ID:" +
                                       std::to_wstring(processId) + L"\n");
                            result.append(rendererProcessResult + L"\n");
                            //! [AssociatedFrameInfos]
                            rendererProcessCount++;
                        }
                        else
                        {
                            otherProcessResult.append(
                                L"Process Id:" + std::to_wstring(processId) +
                                L" | Process Kind:" + ProcessKindToString(kind) + L"\n");
                        }
                    }
                    result.insert(
                        0, std::to_wstring(processCount) + L" process(es) found, from which " +
                               std::to_wstring(rendererProcessCount) +
                               L" renderer process(es) found\n\n");
                    otherProcessResult.insert(
                        0, L"\nRemaining " +
                               std::to_wstring(processCount - rendererProcessCount) +
                               L" Process(es) Infos:\n");
                    result.append(otherProcessResult);
                    MessageBox(
                        nullptr, result.c_str(), L"Process Info with Associated Frames", MB_OK);
                    return S_OK;
                })
                .Get()));
        //! [GetProcessInfosWithDetails]
    }
}

// Get a string for the failure kind enum value.
std::wstring ProcessComponent::ProcessFailedKindToString(
    const COREWEBVIEW2_PROCESS_FAILED_KIND kind)
{
    switch (kind)
    {
#define KIND_ENTRY(kindValue)                                                                  \
    case kindValue:                                                                            \
        return L#kindValue;

        KIND_ENTRY(COREWEBVIEW2_PROCESS_FAILED_KIND_BROWSER_PROCESS_EXITED);
        KIND_ENTRY(COREWEBVIEW2_PROCESS_FAILED_KIND_RENDER_PROCESS_EXITED);
        KIND_ENTRY(COREWEBVIEW2_PROCESS_FAILED_KIND_RENDER_PROCESS_UNRESPONSIVE);
        KIND_ENTRY(COREWEBVIEW2_PROCESS_FAILED_KIND_FRAME_RENDER_PROCESS_EXITED);
        KIND_ENTRY(COREWEBVIEW2_PROCESS_FAILED_KIND_UTILITY_PROCESS_EXITED);
        KIND_ENTRY(COREWEBVIEW2_PROCESS_FAILED_KIND_SANDBOX_HELPER_PROCESS_EXITED);
        KIND_ENTRY(COREWEBVIEW2_PROCESS_FAILED_KIND_GPU_PROCESS_EXITED);
        KIND_ENTRY(COREWEBVIEW2_PROCESS_FAILED_KIND_PPAPI_PLUGIN_PROCESS_EXITED);
        KIND_ENTRY(COREWEBVIEW2_PROCESS_FAILED_KIND_PPAPI_BROKER_PROCESS_EXITED);
        KIND_ENTRY(COREWEBVIEW2_PROCESS_FAILED_KIND_UNKNOWN_PROCESS_EXITED);

#undef KIND_ENTRY
    }

    return L"PROCESS FAILED: " + std::to_wstring(static_cast<uint32_t>(kind));
}

// Get a string for the failure reason enum value.
std::wstring ProcessComponent::ProcessFailedReasonToString(
    const COREWEBVIEW2_PROCESS_FAILED_REASON reason)
{
    switch (reason)
    {
#define REASON_ENTRY(reasonValue)                                                              \
    case reasonValue:                                                                          \
        return L#reasonValue;

        REASON_ENTRY(COREWEBVIEW2_PROCESS_FAILED_REASON_UNEXPECTED);
        REASON_ENTRY(COREWEBVIEW2_PROCESS_FAILED_REASON_UNRESPONSIVE);
        REASON_ENTRY(COREWEBVIEW2_PROCESS_FAILED_REASON_TERMINATED);
        REASON_ENTRY(COREWEBVIEW2_PROCESS_FAILED_REASON_CRASHED);
        REASON_ENTRY(COREWEBVIEW2_PROCESS_FAILED_REASON_LAUNCH_FAILED);
        REASON_ENTRY(COREWEBVIEW2_PROCESS_FAILED_REASON_OUT_OF_MEMORY);

#undef REASON_ENTRY
    }

    return L"REASON: " + std::to_wstring(static_cast<uint32_t>(reason));
}

// Get a string for the process kind enum value.
std::wstring ProcessComponent::ProcessKindToString(const COREWEBVIEW2_PROCESS_KIND kind)
{
    switch (kind)
    {
#define KIND_ENTRY(kindValue)                                                                  \
    case kindValue:                                                                            \
        return L#kindValue;

        KIND_ENTRY(COREWEBVIEW2_PROCESS_KIND_BROWSER);
        KIND_ENTRY(COREWEBVIEW2_PROCESS_KIND_RENDERER);
        KIND_ENTRY(COREWEBVIEW2_PROCESS_KIND_UTILITY);
        KIND_ENTRY(COREWEBVIEW2_PROCESS_KIND_SANDBOX_HELPER);
        KIND_ENTRY(COREWEBVIEW2_PROCESS_KIND_GPU);
        KIND_ENTRY(COREWEBVIEW2_PROCESS_KIND_PPAPI_PLUGIN);
        KIND_ENTRY(COREWEBVIEW2_PROCESS_KIND_PPAPI_BROKER);

#undef KIND_ENTRY
    }

    return L"PROCESS KIND: " + std::to_wstring(static_cast<uint32_t>(kind));
}

// Crash the browser's process on command, to test crash handlers.
void ProcessComponent::CrashBrowserProcess()
{
    m_webView->Navigate(L"edge://inducebrowsercrashforrealz");
}

// Crash the browser's render process on command, to test crash handlers.
void ProcessComponent::CrashRenderProcess()
{
    m_webView->Navigate(L"edge://kill");
}

//! [ProcessInfosChanged1]
void ProcessComponent::PerformanceInfo()
{
    std::wstring result;
    UINT processListCount;
    CHECK_FAILURE(m_processCollection->get_Count(&processListCount));

    if (processListCount == 0)
    {
        result += L"No process found.";
    }
    else
    {
        result += std::to_wstring(processListCount) + L" process(s) found";
        result += L"\n\n";
        for (UINT i = 0; i < processListCount; ++i)
        {
            wil::com_ptr<ICoreWebView2ProcessInfo> processInfo;
            CHECK_FAILURE(m_processCollection->GetValueAtIndex(i, &processInfo));

            INT32 processId = 0;
            COREWEBVIEW2_PROCESS_KIND kind;
            CHECK_FAILURE(processInfo->get_ProcessId(&processId));
            CHECK_FAILURE(processInfo->get_Kind(&kind));

            WCHAR id[4096] = L"";
            StringCchPrintf(id, ARRAYSIZE(id), L"Process ID: %u", processId);

            HANDLE processHandle =
                OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
            PROCESS_MEMORY_COUNTERS_EX pmc;
            GetProcessMemoryInfo(
                processHandle, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc));
            SIZE_T virtualMemUsed = pmc.PrivateUsage / 1024;
            WCHAR memory[4096] = L"";
            StringCchPrintf(memory, ARRAYSIZE(memory), L"Memory: %u", virtualMemUsed);
            CloseHandle(processHandle);

            result = result + id + L" | Process Kind: " + ProcessKindToString(kind) + L" | " +
                     memory + L" KB\n";
        }
    }
    MessageBox(nullptr, result.c_str(), L"Memory Usage", MB_OK);
}
//! [ProcessInfosChanged1]

/*static*/ void ProcessComponent::EnsureProcessIsClosed(UINT processId, int timeoutMs)
{
    UINT exitCode = 1;
    if (processId != 0)
    {
        HANDLE hBrowserProcess = ::OpenProcess(PROCESS_TERMINATE, false, processId);
        // Wait for the process to exit by itself
        DWORD waitResult = ::WaitForSingleObject(hBrowserProcess, timeoutMs);
        if (waitResult != WAIT_OBJECT_0)
        {
            // Force kill the process if it doesn't exit by itself
            BOOL result = ::TerminateProcess(hBrowserProcess, exitCode);
            ::CloseHandle(hBrowserProcess);
        }
    }
}

void ProcessComponent::ScheduleReinitIfSelectedByUser(
    const std::wstring& message, const std::wstring& caption)
{
    // Do not block from event handler
    m_appWindow->RunAsync([this, message, caption]() {
        int selection = MessageBox(
            m_appWindow->GetMainWindow(), message.c_str(), caption.c_str(), MB_YESNO);
        if (selection == IDYES)
        {
            m_appWindow->ReinitializeWebView();
        }
    });
}

void ProcessComponent::ScheduleReloadIfSelectedByUser(
    const std::wstring& message, const std::wstring& caption)
{
    // Do not block from event handler
    m_appWindow->RunAsync([this, message, caption]() {
        int selection = MessageBox(
            m_appWindow->GetMainWindow(), message.c_str(), caption.c_str(), MB_YESNO);
        if (selection == IDYES)
        {
            CHECK_FAILURE(m_webView->Reload());
        }
    });
}

ProcessComponent::~ProcessComponent()
{
    m_webView->remove_ProcessFailed(m_processFailedToken);
    auto environment8 = m_webViewEnvironment.try_query<ICoreWebView2Environment8>();
    if (environment8)
    {
        environment8->remove_ProcessInfosChanged(m_processInfosChangedToken);
    }
}
