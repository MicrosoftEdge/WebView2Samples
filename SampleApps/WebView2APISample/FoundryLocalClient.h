// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <functional>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <windows.h>
#include <winhttp.h>
#include <wininet.h>

// Callback types for async operations
using TokenCallback = std::function<void(const std::wstring& token)>;
using ProgressCallback = std::function<void(double progress)>;
using CompletionCallback = std::function<void(bool success, const std::wstring& error)>;

// Chat message structure
struct ChatMessage
{
    std::wstring role;    // "system", "user", or "assistant"
    std::wstring content;
};

// Model info structure
struct ModelInfo
{
    std::wstring id;
    std::wstring alias;
    std::wstring displayName;
    std::wstring description;
    std::wstring systemPrompt;
    bool isCached;
};

// Foundry Local REST API client
class FoundryLocalClient
{
public:
    FoundryLocalClient();
    ~FoundryLocalClient();

    // Service management
    bool IsFoundryInstalled();
    bool IsServiceRunning();
    bool StartService();
    bool InstallFoundryLocal(); // Runs winget with elevation

    // Network status
    bool IsOnline();

    // Model management
    std::vector<ModelInfo> GetAvailableModels();
    bool SetModelAlias(const std::wstring& alias);
    std::wstring GetCurrentModelAlias() const;
    std::wstring GetCurrentModelDisplayName() const;
    bool IsModelCached(const std::wstring& alias);
    void DownloadModelAsync(
        const std::wstring& alias,
        ProgressCallback progressCallback,
        CompletionCallback completionCallback);

    // Inference
    void ChatCompletionAsync(
        const std::vector<ChatMessage>& messages,
        TokenCallback tokenCallback,
        CompletionCallback completionCallback);

    void CancelInference();

    // System prompt management
    void SetSystemPrompt(const std::wstring& prompt);
    std::wstring GetSystemPrompt() const;

    // Get service endpoint
    std::wstring GetServiceEndpoint();

private:
    // Internal helpers
    std::wstring ExecuteCommand(const std::wstring& command);
    bool RunElevatedCommand(const std::wstring& command, const std::wstring& args);
    bool DiscoverServicePort(); // Discover port from foundry service status
    bool DiscoverModelId(); // Discover actual model ID from /v1/models
    std::string WideToUtf8(const std::wstring& wide);
    std::wstring Utf8ToWide(const std::string& utf8);
    std::string UnescapeJsonString(const std::string& input);
    std::string FilterThinkTags(const std::string& content);
    std::string BuildChatRequestJson(const std::vector<ChatMessage>& messages);
    void ParseSSEStream(const std::string& chunk, TokenCallback& tokenCallback);
    void InitializeModelCatalog();
    std::wstring GetDefaultSystemPrompt(const std::wstring& alias) const;

    // Service endpoint (discovered dynamically)
    std::wstring m_serviceHost = L"127.0.0.1";
    int m_servicePort = 0; // Discovered dynamically from foundry service status
    std::wstring m_systemPrompt;
    std::wstring m_modelId; // Actual model ID discovered from API
    
    // DeepSeek R1 think tag filtering
    bool m_inThinkBlock = false;
    std::string m_thinkBuffer;

    // Threading
    std::atomic<bool> m_cancelRequested{false};
    std::thread m_inferenceThread;
    std::thread m_downloadThread;
    std::mutex m_mutex;

    // WinHTTP handles
    HINTERNET m_hSession = nullptr;

    // Model management
    std::wstring m_modelAlias = L"phi-4-mini";
    std::vector<ModelInfo> m_modelCatalog;
};
