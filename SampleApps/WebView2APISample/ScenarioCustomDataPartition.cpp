// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "ScenarioCustomDataPartition.h"

#include "AppWindow.h"
#include "CheckFailure.h"

using namespace Microsoft::WRL;

static constexpr WCHAR c_samplePath[] = L"ScenarioCustomDataPartition.html";
static constexpr int c_minNewWindowSize = 100;
static constexpr WCHAR c_childWindowClassName[] = L"ScenarioCustomDataPartitionChildHostWindow";

static RECT GetPopupWindowRectFromUri(const std::wstring& uri)
{
    int left = 100;
    int top = 100;
    int width = 800;
    int height = 600;

    if (uri.find(L"ScenarioCustomDataPartitionChild.html") != std::wstring::npos)
    {
        width = 600;
        height = 800;
    }

    if (width < c_minNewWindowSize)
    {
        width = c_minNewWindowSize;
    }
    if (height < c_minNewWindowSize)
    {
        height = c_minNewWindowSize;
    }

    RECT rect = {left, top, left + width, top + height};
    return rect;
}

bool ScenarioCustomDataPartition::EnsureChildWindowClassRegistered()
{
    static bool s_registered = false;
    if (s_registered)
    {
        return true;
    }

    WNDCLASSEXW windowClass = {};
    windowClass.cbSize = sizeof(WNDCLASSEXW);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = ScenarioCustomDataPartition::ChildWndProcStatic;
    windowClass.hInstance = GetModuleHandle(nullptr);
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = c_childWindowClassName;

    if (!RegisterClassExW(&windowClass))
    {
        DWORD error = GetLastError();
        if (error != ERROR_CLASS_ALREADY_EXISTS)
        {
            return false;
        }
    }

    s_registered = true;
    return true;
}

LRESULT CALLBACK ScenarioCustomDataPartition::ChildWndProcStatic(
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_NCCREATE)
    {
        auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto* self = reinterpret_cast<ScenarioCustomDataPartition*>(createStruct->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }

    auto* self = reinterpret_cast<ScenarioCustomDataPartition*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (self)
    {
        return self->ChildWndProc(hWnd, message, wParam, lParam);
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT ScenarioCustomDataPartition::ChildWndProc(
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
        CloseChildWindow();
        return 0;
    case WM_SIZE:
        ResizeChildWebView();
        break;
    case WM_DESTROY:
        if (hWnd == m_childWindow)
        {
            m_childWindow = nullptr;
            m_childController = nullptr;
            m_childWebView = nullptr;
        }
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

void ScenarioCustomDataPartition::ResizeChildWebView()
{
    if (!m_childWindow || !m_childController)
    {
        return;
    }

    RECT bounds = {};
    if (GetClientRect(m_childWindow, &bounds))
    {
        CHECK_FAILURE(m_childController->put_Bounds(bounds));
    }
}

void ScenarioCustomDataPartition::CloseChildWindow()
{
    if (m_childWebView && m_childWebMessageReceivedToken.value != 0)
    {
        m_childWebView->remove_WebMessageReceived(m_childWebMessageReceivedToken);
        m_childWebMessageReceivedToken = {};
    }

    if (m_childController)
    {
        m_childController->Close();
        m_childController = nullptr;
    }
    m_childWebView = nullptr;

    if (m_childWindow)
    {
        HWND childWindow = m_childWindow;
        m_childWindow = nullptr;
        DestroyWindow(childWindow);
    }
}

ScenarioCustomDataPartition::ScenarioCustomDataPartition(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    m_sampleUri = m_appWindow->GetLocalUri(c_samplePath);

    // Disable host-level popup handling so this scenario can fully own NewWindowRequested.
    m_appWindow->EnableHandlingNewWindowRequest(false);

    // Register event handlers
    RegisterEventHandlers();

    // Navigate to the sample page
    CHECK_FAILURE(m_webView->Navigate(m_sampleUri.c_str()));
}

void ScenarioCustomDataPartition::RegisterEventHandlers()
{
    // Turn off this scenario if we navigate away from the sample page
    CHECK_FAILURE(m_webView->add_ContentLoading(
        Callback<ICoreWebView2ContentLoadingEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2ContentLoadingEventArgs* args) -> HRESULT
            {
                wil::unique_cotaskmem_string uri;
                sender->get_Source(&uri);
                if (uri.get() != m_sampleUri)
                {
                    m_appWindow->DeleteComponent(this);
                }
                return S_OK;
            })
            .Get(),
        &m_contentLoadingToken));

    // Handle messages from the HTML UI
    CHECK_FAILURE(m_webView->add_WebMessageReceived(
        Callback<ICoreWebView2WebMessageReceivedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT
            {
                wil::unique_cotaskmem_string message;
                CHECK_FAILURE(args->TryGetWebMessageAsString(&message));
                HandleWebMessage(message.get());
                return S_OK;
            })
            .Get(),
        &m_webMessageReceivedToken));

    //! [CustomDataPartitionNewWindowRequested]
    // Register a handler for the NewWindowRequested event.
    // This handler captures child windows by creating them and setting partitions.
    CHECK_FAILURE(m_webView->add_NewWindowRequested(
        Callback<ICoreWebView2NewWindowRequestedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT
            {
                // Get a deferral - we'll complete this asynchronously
                wil::com_ptr<ICoreWebView2Deferral> deferral;
                CHECK_FAILURE(args->GetDeferral(&deferral));

                wil::com_ptr<ICoreWebView2NewWindowRequestedEventArgs> argsAsComPtr = args;

                if (m_childWindow || m_childController)
                {
                    CloseChildWindow();
                }

                wil::unique_cotaskmem_string uri;
                CHECK_FAILURE(args->get_Uri(&uri));
                std::wstring targetUri = uri.get() ? uri.get() : L"";

                RECT popupRect = GetPopupWindowRectFromUri(targetUri);

                if (!EnsureChildWindowClassRegistered())
                {
                    SendStatusToUI(L"Error: Failed to register child window class.");
                    CHECK_FAILURE(argsAsComPtr->put_Handled(FALSE));
                    CHECK_FAILURE(deferral->Complete());
                    return S_OK;
                }

                m_childWindow = CreateWindowExW(
                    WS_EX_CONTROLPARENT, c_childWindowClassName,
                    L"Custom Data Partition Child Window",
                    WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, popupRect.left,
                    popupRect.top, popupRect.right - popupRect.left,
                    popupRect.bottom - popupRect.top, nullptr, nullptr,
                    GetModuleHandle(nullptr), this);

                if (!m_childWindow)
                {
                    SendStatusToUI(L"Error: Failed to create child native window.");
                    CHECK_FAILURE(argsAsComPtr->put_Handled(FALSE));
                    CHECK_FAILURE(deferral->Complete());
                    return S_OK;
                }

                ShowWindow(m_childWindow, SW_SHOWNORMAL);
                UpdateWindow(m_childWindow);

                auto environment = m_appWindow->GetWebViewEnvironment();
                if (!environment)
                {
                    SendStatusToUI(L"Error: WebView2 environment not available for child window.");
                    CloseChildWindow();
                    CHECK_FAILURE(argsAsComPtr->put_Handled(FALSE));
                    CHECK_FAILURE(deferral->Complete());
                    return S_OK;
                }

                CHECK_FAILURE(environment->CreateCoreWebView2Controller(
                    m_childWindow,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [this, argsAsComPtr, deferral, targetUri](
                            HRESULT result, ICoreWebView2Controller* controller) -> HRESULT
                        {
                            if (result == S_OK && controller)
                            {
                                m_childController = controller;
                                CHECK_FAILURE(m_childController->get_CoreWebView2(&m_childWebView));
                                SetPartitionOnLastChild(L"DefaultPartitionForChild");
                                CHECK_FAILURE(m_childWebView->add_WebMessageReceived(
                                    Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                        [this](
                                            ICoreWebView2* sender,
                                            ICoreWebView2WebMessageReceivedEventArgs* args)
                                            -> HRESULT
                                        {
                                            wil::unique_cotaskmem_string message;
                                            CHECK_FAILURE(args->TryGetWebMessageAsString(&message));

                                            if (std::wstring(message.get()) == L"getCustomPartition")
                                            {
                                                wil::com_ptr<ICoreWebView2Experimental20>
                                                    webViewExperimental;
                                                webViewExperimental =
                                                    m_childWebView.try_query<
                                                        ICoreWebView2Experimental20>();

                                                if (!webViewExperimental)
                                                {
                                                    SendStatusToChildUI(
                                                        L"Error: Custom partition API unavailable.");
                                                    return S_OK;
                                                }

                                                wil::unique_cotaskmem_string partitionId;
                                                HRESULT partitionHr =
                                                    webViewExperimental
                                                        ->get_CustomDataPartitionId(&partitionId);

                                                if (SUCCEEDED(partitionHr))
                                                {
                                                    if (partitionId.get() == nullptr ||
                                                        wcslen(partitionId.get()) == 0)
                                                    {
                                                        SendStatusToChildUI(
                                                            L"Child partition: (default/none)");
                                                    }
                                                    else
                                                    {
                                                        std::wstring status =
                                                            L"Child partition: '" +
                                                            std::wstring(partitionId.get()) +
                                                            L"'";
                                                        SendStatusToChildUI(status);
                                                    }
                                                }
                                                else
                                                {
                                                    SendStatusToChildUI(
                                                        L"Error: Failed to read child partition.");
                                                }
                                            }

                                            return S_OK;
                                        })
                                        .Get(),
                                    &m_childWebMessageReceivedToken));

                                auto childWebView3 = m_childWebView.try_query<ICoreWebView2_3>();
                                if (childWebView3)
                                {
                                    CHECK_FAILURE(childWebView3->SetVirtualHostNameToFolderMapping(
                                        L"appassets.example", L"assets",
                                        COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY_CORS));
                                }

                                ResizeChildWebView();
                                CHECK_FAILURE(argsAsComPtr->put_NewWindow(m_childWebView.get()));
                                CHECK_FAILURE(argsAsComPtr->put_Handled(TRUE));
                                if (!targetUri.empty())
                                {
                                    CHECK_FAILURE(m_childWebView->Navigate(targetUri.c_str()));
                                }
                                SendStatusToUI(
                                    L"Child window created and captured! Ready to set partition.");
                            }
                            else
                            {
                                CloseChildWindow();
                                SendStatusToUI(
                                    L"Error: Failed to create child WebView controller.");
                                CHECK_FAILURE(argsAsComPtr->put_Handled(FALSE));
                            }

                            CHECK_FAILURE(deferral->Complete());
                            return S_OK;
                        })
                        .Get()));

                return S_OK;
            })
            .Get(),
        &m_newWindowRequestedToken));
    //! [CustomDataPartitionNewWindowRequested]
}

void ScenarioCustomDataPartition::HandleWebMessage(const std::wstring& message)
{
    // Parse simple command messages from JavaScript
    if (message.find(L"setPartition:") == 0)
    {
        // Extract partition ID from message
        std::wstring partitionId = message.substr(13); // Skip "setPartition:"
        SetPartitionOnLastChild(partitionId);
    }
    else if (message == L"checkPartition")
    {
        CheckCurrentPartition();
    }
}

void ScenarioCustomDataPartition::SetPartitionOnLastChild(const std::wstring& partitionId)
{
    if (!m_childWebView)
    {
        SendStatusToUI(L"Error: No child window available. Create a child window first.");
        return;
    }

    //! [CustomDataPartitionSet]
    // Try to set custom data partition using the experimental API
    wil::com_ptr<ICoreWebView2Experimental20> webViewExperimental;
    webViewExperimental = m_childWebView.try_query<ICoreWebView2Experimental20>();

    if (!webViewExperimental)
    {
        SendStatusToUI(L"Error: Custom data partition API (ICoreWebView2Experimental20) not available in this WebView2 version.");
        return;
    }

    // Set the partition ID on the child window
    HRESULT hr = webViewExperimental->put_CustomDataPartitionId(partitionId.c_str());
    //! [CustomDataPartitionSet]

    if (SUCCEEDED(hr))
    {
        std::wstring status = L"✓ Success! Partition '" + partitionId +
                             L"' set on child window.";
        SendStatusToUI(status);
    }
    else
    {
        SendStatusToUI(L"Error: Failed to set custom data partition.");
    }
}

void ScenarioCustomDataPartition::CheckCurrentPartition()
{
    //! [CustomDataPartitionGet]
    // Check the partition of the parent window (should be empty/default)
    wil::com_ptr<ICoreWebView2Experimental20> webViewExperimental;
    webViewExperimental = m_webView.try_query<ICoreWebView2Experimental20>();

    if (!webViewExperimental)
    {
        SendStatusToUI(L"Error: Custom data partition API not available.");
        return;
    }

    wil::unique_cotaskmem_string partitionId;
    HRESULT hr = webViewExperimental->get_CustomDataPartitionId(&partitionId);
    //! [CustomDataPartitionGet]

    if (SUCCEEDED(hr))
    {
        if (partitionId.get() == nullptr || wcslen(partitionId.get()) == 0)
        {
            SendStatusToUI(L"✓ Parent window partition: (default/none) - Correct! Parent has no custom partition.");
        }
        else
        {
            std::wstring status = L"⚠ Parent window partition: '" + std::wstring(partitionId.get()) +
                                 L"' (Note: Parent should typically remain in default partition)";
            SendStatusToUI(status);
        }
    }
    else
    {
        SendStatusToUI(L"Error: Failed to get partition ID.");
    }
}

void ScenarioCustomDataPartition::SendStatusToUI(const std::wstring& status)
{
    // Send status message back to the HTML UI
    // Escape single quotes in the status message
    std::wstring escapedStatus = status;
    size_t pos = 0;
    while ((pos = escapedStatus.find(L"'", pos)) != std::wstring::npos)
    {
        escapedStatus.replace(pos, 1, L"\\'");
        pos += 2;
    }

    std::wstring script = L"updateStatus('" + escapedStatus + L"');";
    CHECK_FAILURE(m_webView->ExecuteScript(script.c_str(), nullptr));
}

void ScenarioCustomDataPartition::SendStatusToChildUI(const std::wstring& status)
{
    if (!m_childWebView)
    {
        return;
    }

    std::wstring escapedStatus = status;
    size_t pos = 0;
    while ((pos = escapedStatus.find(L"'", pos)) != std::wstring::npos)
    {
        escapedStatus.replace(pos, 1, L"\\'");
        pos += 2;
    }

    std::wstring script = L"updateChildPartitionStatus('" + escapedStatus + L"');";
    CHECK_FAILURE(m_childWebView->ExecuteScript(script.c_str(), nullptr));
}

ScenarioCustomDataPartition::~ScenarioCustomDataPartition()
{
    // Restore default host-level popup handling.
    if (m_appWindow)
    {
        m_appWindow->EnableHandlingNewWindowRequest(true);
    }

    CloseChildWindow();

    m_webView->remove_ContentLoading(m_contentLoadingToken);
    m_webView->remove_WebMessageReceived(m_webMessageReceivedToken);
    m_webView->remove_NewWindowRequested(m_newWindowRequestedToken);
}
