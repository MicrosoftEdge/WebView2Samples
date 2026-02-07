// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <memory>

// Include ONNX Runtime GenAI headers (from NuGet package)
#include "ort_genai_c.h"

struct ChatMessage
{
    std::wstring role;
    std::wstring content;
};

struct ModelInfo
{
    std::wstring id;
    std::wstring alias;
    std::wstring displayName;
    std::wstring description;
    std::wstring systemPrompt;
    bool isDownloaded;
};

// Callback types
using TokenCallback = std::function<void(const std::wstring& token)>;
using ProgressCallback = std::function<void(double progress)>;
using CompletionCallback = std::function<void(bool success, const std::wstring& error)>;

class ONNXRuntimeClient
{
public:
    ONNXRuntimeClient();
    ~ONNXRuntimeClient();

    // Initialization
    bool Initialize();
    bool IsInitialized() const;

    // Network check
    bool IsOnline();

    // Model management
    std::vector<ModelInfo> GetAvailableModels();
    bool IsModelDownloaded(const std::wstring& alias);
    bool SetCurrentModel(const std::wstring& alias);
    std::wstring GetCurrentModelAlias() const;
    std::wstring GetCurrentModelDisplayName() const;
    
    // Model download
    void DownloadModelAsync(
        const std::wstring& alias,
        ProgressCallback progressCallback,
        CompletionCallback completionCallback);

    // Model loading
    bool LoadModel(const std::wstring& alias);
    bool IsModelLoaded() const;
    void UnloadModel();

    // Inference
    void ChatCompletionAsync(
        const std::vector<ChatMessage>& messages,
        TokenCallback tokenCallback,
        CompletionCallback completionCallback);

    void CancelInference();

    // System prompt management
    void SetSystemPrompt(const std::wstring& prompt);
    std::wstring GetSystemPrompt() const;

    // Paths
    std::wstring GetModelsDirectory() const;
    std::wstring GetModelPath(const std::wstring& alias) const;

private:
    // Internal helpers
    void InitializeModelCatalog();
    std::wstring GetDefaultSystemPrompt(const std::wstring& alias) const;
    std::string WideToUtf8(const std::wstring& wide);
    std::wstring Utf8ToWide(const std::string& utf8);
    std::wstring GetLocalAppDataPath() const;
    bool CreateDirectoryRecursive(const std::wstring& path);
    
    // Chat formatting
    std::string FormatChatPrompt(const std::vector<ChatMessage>& messages);
    std::string FilterThinkTags(const std::string& content);

    // Download helpers
    bool DownloadFile(const std::wstring& url, const std::wstring& localPath, 
                      ProgressCallback progressCallback);
    std::vector<std::pair<std::wstring, std::wstring>> GetModelFiles(const std::wstring& alias);

    // Member variables
    bool m_initialized = false;
    std::atomic<bool> m_cancelRequested{false};
    std::atomic<bool> m_modelLoaded{false};
    
    // Model state
    std::wstring m_currentModelAlias = L"phi-3-mini";
    std::wstring m_systemPrompt;
    std::vector<ModelInfo> m_modelCatalog;

    // ONNX Runtime GenAI objects - use raw pointers with manual cleanup
    // (unique_ptr with custom deleters defined in .cpp)
    OgaModel* m_model = nullptr;
    OgaTokenizer* m_tokenizer = nullptr;

    // Threading
    std::thread m_inferenceThread;
    std::thread m_downloadThread;
    std::mutex m_mutex;
    
    // Think tag filtering state
    bool m_inThinkBlock = false;
    std::string m_thinkBuffer;
};
