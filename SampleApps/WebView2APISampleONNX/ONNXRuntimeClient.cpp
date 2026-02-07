// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"
#include "ONNXRuntimeClient.h"

#include <windows.h>
#include <winhttp.h>
#include <wininet.h>
#include <shlobj.h>
#include <filesystem>
#include <fstream>
#include <sstream>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "wininet.lib")

namespace fs = std::filesystem;

// Custom deleters for ONNX GenAI types
void SafeDestroyModel(OgaModel* p) { if (p) OgaDestroyModel(p); }
void SafeDestroyTokenizer(OgaTokenizer* p) { if (p) OgaDestroyTokenizer(p); }

ONNXRuntimeClient::ONNXRuntimeClient()
{
    InitializeModelCatalog();
}

ONNXRuntimeClient::~ONNXRuntimeClient()
{
    CancelInference();
    
    if (m_inferenceThread.joinable())
    {
        m_inferenceThread.join();
    }
    if (m_downloadThread.joinable())
    {
        m_downloadThread.join();
    }
    
    UnloadModel();
}

bool ONNXRuntimeClient::Initialize()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Create models directory if it doesn't exist
    std::wstring modelsDir = GetModelsDirectory();
    CreateDirectoryRecursive(modelsDir);
    
    m_initialized = true;
    return true;
}

bool ONNXRuntimeClient::IsInitialized() const
{
    return m_initialized;
}

void ONNXRuntimeClient::InitializeModelCatalog()
{
    m_modelCatalog.clear();
    
    // Phi-3 Mini - ONNX optimized version (publicly accessible)
    ModelInfo phi3mini;
    phi3mini.id = L"microsoft/Phi-3-mini-4k-instruct-onnx";
    phi3mini.alias = L"phi-3-mini";
    phi3mini.displayName = L"Phi-3 Mini 4K";
    phi3mini.description = L"Compact and fast model for general chat";
    phi3mini.systemPrompt = L"You are a helpful AI assistant. Be concise and accurate.";
    phi3mini.isDownloaded = false;
    m_modelCatalog.push_back(phi3mini);
    
    // Check which models are downloaded
    for (auto& model : m_modelCatalog)
    {
        model.isDownloaded = IsModelDownloaded(model.alias);
    }
}

std::vector<ModelInfo> ONNXRuntimeClient::GetAvailableModels()
{
    // Refresh download status
    for (auto& model : m_modelCatalog)
    {
        model.isDownloaded = IsModelDownloaded(model.alias);
    }
    return m_modelCatalog;
}

bool ONNXRuntimeClient::IsOnline()
{
    DWORD flags;
    return InternetGetConnectedState(&flags, 0) != FALSE;
}

bool ONNXRuntimeClient::IsModelDownloaded(const std::wstring& alias)
{
    std::wstring modelPath = GetModelPath(alias);
    
    // Check if the model directory exists and contains required files
    if (!fs::exists(modelPath))
    {
        return false;
    }
    
    // Check for essential ONNX model files (CPU model has different filename)
    bool hasModel = fs::exists(modelPath + L"\\model.onnx") || 
                    fs::exists(modelPath + L"\\model.onnx.data") ||
                    fs::exists(modelPath + L"\\phi3-mini-4k-instruct-cpu-int4-rtn-block-32-acc-level-4.onnx");
    bool hasConfig = fs::exists(modelPath + L"\\genai_config.json");
    bool hasTokenizer = fs::exists(modelPath + L"\\tokenizer.json") ||
                        fs::exists(modelPath + L"\\tokenizer.model");
    
    return hasModel && hasConfig && hasTokenizer;
}

bool ONNXRuntimeClient::SetCurrentModel(const std::wstring& alias)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Verify the model exists in our catalog
    bool found = false;
    for (const auto& model : m_modelCatalog)
    {
        if (model.alias == alias)
        {
            found = true;
            m_systemPrompt = model.systemPrompt;
            break;
        }
    }
    
    if (!found)
    {
        return false;
    }
    
    // If different model, unload current
    if (m_currentModelAlias != alias && m_modelLoaded)
    {
        UnloadModel();
    }
    
    m_currentModelAlias = alias;
    return true;
}

std::wstring ONNXRuntimeClient::GetCurrentModelAlias() const
{
    return m_currentModelAlias;
}

std::wstring ONNXRuntimeClient::GetCurrentModelDisplayName() const
{
    for (const auto& model : m_modelCatalog)
    {
        if (model.alias == m_currentModelAlias)
        {
            return model.displayName;
        }
    }
    return m_currentModelAlias;  // Return alias as fallback
}

void ONNXRuntimeClient::DownloadModelAsync(
    const std::wstring& alias,
    ProgressCallback progressCallback,
    CompletionCallback completionCallback)
{
    // If download thread is running, wait for it
    if (m_downloadThread.joinable())
    {
        m_downloadThread.join();
    }
    
    m_downloadThread = std::thread([this, alias, progressCallback, completionCallback]()
    {
        try
        {
            // Model files to download from HuggingFace
            auto files = GetModelFiles(alias);
            if (files.empty())
            {
                completionCallback(false, L"Unknown model: " + alias);
                return;
            }
            
            std::wstring modelPath = GetModelPath(alias);
            CreateDirectoryRecursive(modelPath);
            
            size_t totalFiles = files.size();
            size_t downloadedFiles = 0;
            
            for (const auto& [url, localName] : files)
            {
                std::wstring localPath = modelPath + L"\\" + localName;
                
                // Skip if file already exists
                if (fs::exists(localPath))
                {
                    downloadedFiles++;
                    double progress = (double)downloadedFiles / totalFiles * 100.0;
                    progressCallback(progress);
                    continue;
                }
                
                // Download file with progress
                auto fileProgress = [&](double filePercent)
                {
                    double overallProgress = ((double)downloadedFiles + filePercent / 100.0) / totalFiles * 100.0;
                    progressCallback(overallProgress);
                };
                
                if (!DownloadFile(url, localPath, fileProgress))
                {
                    completionCallback(false, L"Failed to download: " + localName);
                    return;
                }
                
                downloadedFiles++;
            }
            
            progressCallback(100.0);
            completionCallback(true, L"");
        }
        catch (const std::exception& e)
        {
            completionCallback(false, Utf8ToWide(e.what()));
        }
    });
}

bool ONNXRuntimeClient::DownloadFile(
    const std::wstring& url,
    const std::wstring& localPath,
    ProgressCallback progressCallback)
{
    // Parse URL
    URL_COMPONENTS urlComp = {0};
    urlComp.dwStructSize = sizeof(urlComp);
    
    wchar_t hostName[256] = {0};
    wchar_t urlPath[2048] = {0};
    
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = 256;
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = 2048;
    
    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &urlComp))
    {
        return false;
    }
    
    // Open WinHTTP session
    HINTERNET hSession = WinHttpOpen(
        L"WebView2APISampleONNX/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    
    if (!hSession) return false;
    
    // Connect to server
    HINTERNET hConnect = WinHttpConnect(
        hSession,
        hostName,
        urlComp.nPort,
        0);
    
    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return false;
    }
    
    // Open request
    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        urlPath,
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags);
    
    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }
    
    // Send request
    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }
    
    // Receive response
    if (!WinHttpReceiveResponse(hRequest, nullptr))
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }
    
    // Get content length
    DWORD contentLength = 0;
    DWORD size = sizeof(contentLength);
    WinHttpQueryHeaders(
        hRequest,
        WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &contentLength,
        &size,
        WINHTTP_NO_HEADER_INDEX);
    
    // Open output file
    std::ofstream outFile(localPath, std::ios::binary);
    if (!outFile)
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }
    
    // Read data
    DWORD totalRead = 0;
    char buffer[8192];
    DWORD bytesRead = 0;
    
    while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
    {
        outFile.write(buffer, bytesRead);
        totalRead += bytesRead;
        
        if (contentLength > 0 && progressCallback)
        {
            double progress = (double)totalRead / contentLength * 100.0;
            progressCallback(progress);
        }
    }
    
    outFile.close();
    
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    return true;
}

std::vector<std::pair<std::wstring, std::wstring>> ONNXRuntimeClient::GetModelFiles(const std::wstring& alias)
{
    std::vector<std::pair<std::wstring, std::wstring>> files;
    
    if (alias == L"phi-3-mini")
    {
        // HuggingFace URL base for Phi-3-mini ONNX models - using CPU version for compatibility
        // (DirectML version requires a DirectX 12 compatible GPU)
        std::wstring baseUrl = L"https://huggingface.co/microsoft/Phi-3-mini-4k-instruct-onnx/resolve/main/cpu_and_mobile/cpu-int4-rtn-block-32-acc-level-4/";
        
        // CPU model uses different filename than DirectML version
        std::wstring modelName = L"phi3-mini-4k-instruct-cpu-int4-rtn-block-32-acc-level-4";
        
        files.push_back({baseUrl + L"genai_config.json", L"genai_config.json"});
        files.push_back({baseUrl + modelName + L".onnx", modelName + L".onnx"});
        files.push_back({baseUrl + modelName + L".onnx.data", modelName + L".onnx.data"});
        files.push_back({baseUrl + L"tokenizer.json", L"tokenizer.json"});
        files.push_back({baseUrl + L"tokenizer.model", L"tokenizer.model"});
        files.push_back({baseUrl + L"tokenizer_config.json", L"tokenizer_config.json"});
        files.push_back({baseUrl + L"special_tokens_map.json", L"special_tokens_map.json"});
        files.push_back({baseUrl + L"added_tokens.json", L"added_tokens.json"});
    }
    
    return files;
}

bool ONNXRuntimeClient::LoadModel(const std::wstring& alias)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_modelLoaded && m_currentModelAlias == alias)
    {
        return true;  // Already loaded
    }
    
    UnloadModel();
    
    std::wstring modelPath = GetModelPath(alias);
    if (!IsModelDownloaded(alias))
    {
        return false;
    }
    
    try
    {
        std::string modelPathUtf8 = WideToUtf8(modelPath);
        OutputDebugStringA(("Loading model from: " + modelPathUtf8 + "\n").c_str());
        
        // Create model using ONNX Runtime GenAI
        OgaModel* model = nullptr;
        OgaResult* result = OgaCreateModel(modelPathUtf8.c_str(), &model);
        if (result)
        {
            const char* errorMsg = OgaResultGetError(result);
            std::string errStr = errorMsg ? errorMsg : "unknown error";
            OutputDebugStringA(("OgaCreateModel failed: " + errStr + "\n").c_str());
            MessageBoxA(nullptr, errStr.c_str(), "Model Load Error", MB_OK | MB_ICONERROR);
            OgaDestroyResult(result);
            return false;
        }
        m_model = model;
        
        // Create tokenizer
        OgaTokenizer* tokenizer = nullptr;
        result = OgaCreateTokenizer(model, &tokenizer);
        if (result)
        {
            const char* errorMsg = OgaResultGetError(result);
            OutputDebugStringA(("OgaCreateTokenizer failed: " + std::string(errorMsg ? errorMsg : "unknown") + "\n").c_str());
            OgaDestroyResult(result);
            SafeDestroyModel(m_model);
            m_model = nullptr;
            return false;
        }
        m_tokenizer = tokenizer;
        
        m_modelLoaded = true;
        m_currentModelAlias = alias;
        return true;
    }
    catch (const std::exception& e)
    {
        OutputDebugStringA(("Model load failed: " + std::string(e.what()) + "\n").c_str());
        return false;
    }
}

bool ONNXRuntimeClient::IsModelLoaded() const
{
    return m_modelLoaded;
}

void ONNXRuntimeClient::UnloadModel()
{
    SafeDestroyTokenizer(m_tokenizer);
    m_tokenizer = nullptr;
    SafeDestroyModel(m_model);
    m_model = nullptr;
    m_modelLoaded = false;
}

void ONNXRuntimeClient::ChatCompletionAsync(
    const std::vector<ChatMessage>& messages,
    TokenCallback tokenCallback,
    CompletionCallback completionCallback)
{
    // Wait for any previous inference to complete
    if (m_inferenceThread.joinable())
    {
        m_inferenceThread.join();
    }
    
    m_cancelRequested = false;
    
    m_inferenceThread = std::thread([this, messages, tokenCallback, completionCallback]()
    {
        try
        {
            if (!m_modelLoaded || !m_model || !m_tokenizer)
            {
                completionCallback(false, L"Model not loaded");
                return;
            }
            
            // Format the chat prompt using Llama 3 format
            std::string prompt = FormatChatPrompt(messages);
            
            // Create sequences object first
            OgaSequences* inputSequences = nullptr;
            OgaResult* result = OgaCreateSequences(&inputSequences);
            if (result)
            {
                const char* errorMsg = OgaResultGetError(result);
                OgaDestroyResult(result);
                completionCallback(false, L"Failed to create sequences");
                return;
            }
            
            // Encode the prompt into sequences
            result = OgaTokenizerEncode(m_tokenizer, prompt.c_str(), inputSequences);
            if (result)
            {
                const char* errorMsg = OgaResultGetError(result);
                OgaDestroyResult(result);
                OgaDestroySequences(inputSequences);
                completionCallback(false, L"Failed to encode prompt");
                return;
            }
            
            // Create generator params
            OgaGeneratorParams* params = nullptr;
            result = OgaCreateGeneratorParams(m_model, &params);
            if (result)
            {
                OgaDestroySequences(inputSequences);
                const char* errorMsg = OgaResultGetError(result);
                OgaDestroyResult(result);
                completionCallback(false, L"Failed to create generator params");
                return;
            }
            
            // Set generation parameters
            OgaGeneratorParamsSetSearchNumber(params, "max_length", 2048);
            OgaGeneratorParamsSetSearchNumber(params, "top_p", 0.9);
            OgaGeneratorParamsSetSearchNumber(params, "temperature", 0.7);
            OgaGeneratorParamsSetSearchBool(params, "do_sample", true);
            
            // Set input sequences
            result = OgaGeneratorParamsSetInputSequences(params, inputSequences);
            if (result)
            {
                OgaDestroyGeneratorParams(params);
                OgaDestroySequences(inputSequences);
                const char* errorMsg = OgaResultGetError(result);
                OgaDestroyResult(result);
                completionCallback(false, L"Failed to set input sequences");
                return;
            }
            
            // Create generator
            OgaGenerator* generator = nullptr;
            result = OgaCreateGenerator(m_model, params, &generator);
            if (result)
            {
                OgaDestroyGeneratorParams(params);
                OgaDestroySequences(inputSequences);
                const char* errorMsg = OgaResultGetError(result);
                OgaDestroyResult(result);
                completionCallback(false, L"Failed to create generator");
                return;
            }
            
            // Stream tokens
            std::string fullResponse;
            OgaTokenizerStream* stream = nullptr;
            result = OgaCreateTokenizerStream(m_tokenizer, &stream);
            if (result)
            {
                OgaDestroyGenerator(generator);
                OgaDestroyGeneratorParams(params);
                OgaDestroySequences(inputSequences);
                const char* errorMsg = OgaResultGetError(result);
                OgaDestroyResult(result);
                completionCallback(false, L"Failed to create tokenizer stream");
                return;
            }
            
            while (!OgaGenerator_IsDone(generator) && !m_cancelRequested)
            {
                result = OgaGenerator_ComputeLogits(generator);
                if (result)
                {
                    break;
                }
                
                result = OgaGenerator_GenerateNextToken(generator);
                if (result)
                {
                    break;
                }
                
                // Get the new token
                size_t numTokens = OgaGenerator_GetSequenceCount(generator, 0);
                if (numTokens > 0)
                {
                    int32_t newToken = OgaGenerator_GetSequenceData(generator, 0)[numTokens - 1];
                    
                    // Decode token to string
                    const char* tokenStr = nullptr;
                    result = OgaTokenizerStreamDecode(stream, newToken, &tokenStr);
                    if (!result && tokenStr)
                    {
                        std::string token(tokenStr);
                        
                        // Filter thinking tags
                        std::string filtered = FilterThinkTags(token);
                        if (!filtered.empty())
                        {
                            fullResponse += filtered;
                            tokenCallback(Utf8ToWide(filtered));
                        }
                    }
                }
            }
            
            // Cleanup
            OgaDestroyTokenizerStream(stream);
            OgaDestroyGenerator(generator);
            OgaDestroyGeneratorParams(params);
            OgaDestroySequences(inputSequences);
            
            if (m_cancelRequested)
            {
                completionCallback(false, L"Cancelled");
            }
            else
            {
                completionCallback(true, L"");
            }
        }
        catch (const std::exception& e)
        {
            completionCallback(false, Utf8ToWide(e.what()));
        }
    });
}

void ONNXRuntimeClient::CancelInference()
{
    m_cancelRequested = true;
}

void ONNXRuntimeClient::SetSystemPrompt(const std::wstring& prompt)
{
    m_systemPrompt = prompt;
}

std::wstring ONNXRuntimeClient::GetSystemPrompt() const
{
    return m_systemPrompt;
}

std::wstring ONNXRuntimeClient::GetModelsDirectory() const
{
    return GetLocalAppDataPath() + L"\\WebView2APISampleONNX\\models";
}

std::wstring ONNXRuntimeClient::GetModelPath(const std::wstring& alias) const
{
    return GetModelsDirectory() + L"\\" + alias;
}

std::string ONNXRuntimeClient::FormatChatPrompt(const std::vector<ChatMessage>& messages)
{
    // Phi-3 chat format
    std::ostringstream prompt;
    
    // Add system prompt if set
    if (!m_systemPrompt.empty())
    {
        prompt << "<|system|>\n" << WideToUtf8(m_systemPrompt) << "<|end|>\n";
    }
    
    // Add conversation messages
    for (const auto& msg : messages)
    {
        std::string role = WideToUtf8(msg.role);
        std::string content = WideToUtf8(msg.content);
        
        prompt << "<|" << role << "|>\n" << content << "<|end|>\n";
    }
    
    // Add assistant header for response
    prompt << "<|assistant|>\n";
    
    return prompt.str();
}

std::string ONNXRuntimeClient::FilterThinkTags(const std::string& content)
{
    std::string result;
    result.reserve(content.size());
    
    for (size_t i = 0; i < content.size(); ++i)
    {
        char c = content[i];
        
        if (m_inThinkBlock)
        {
            m_thinkBuffer += c;
            
            // Check for </think> end tag
            if (m_thinkBuffer.size() >= 8 &&
                m_thinkBuffer.substr(m_thinkBuffer.size() - 8) == "</think>")
            {
                m_inThinkBlock = false;
                m_thinkBuffer.clear();
            }
        }
        else
        {
            m_thinkBuffer += c;
            
            // Check for <think> start tag
            if (m_thinkBuffer.size() >= 7 &&
                m_thinkBuffer.substr(m_thinkBuffer.size() - 7) == "<think>")
            {
                m_inThinkBlock = true;
                // Remove <think> from result
                if (result.size() >= 6)
                {
                    result = result.substr(0, result.size() - 6);
                }
            }
            else
            {
                result += c;
            }
            
            // Keep buffer small when not in think block
            if (m_thinkBuffer.size() > 10)
            {
                m_thinkBuffer = m_thinkBuffer.substr(m_thinkBuffer.size() - 10);
            }
        }
    }
    
    return result;
}

std::wstring ONNXRuntimeClient::GetDefaultSystemPrompt(const std::wstring& alias) const
{
    for (const auto& model : m_modelCatalog)
    {
        if (model.alias == alias)
        {
            return model.systemPrompt;
        }
    }
    return L"You are a helpful AI assistant.";
}

std::string ONNXRuntimeClient::WideToUtf8(const std::wstring& wide)
{
    if (wide.empty()) return std::string();
    
    int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), nullptr, 0, nullptr, nullptr);
    std::string utf8(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), &utf8[0], size, nullptr, nullptr);
    return utf8;
}

std::wstring ONNXRuntimeClient::Utf8ToWide(const std::string& utf8)
{
    if (utf8.empty()) return std::wstring();
    
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), nullptr, 0);
    std::wstring wide(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), &wide[0], size);
    return wide;
}

std::wstring ONNXRuntimeClient::GetLocalAppDataPath() const
{
    wchar_t* path = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path)))
    {
        std::wstring result(path);
        CoTaskMemFree(path);
        return result;
    }
    return L"";
}

bool ONNXRuntimeClient::CreateDirectoryRecursive(const std::wstring& path)
{
    try
    {
        fs::create_directories(path);
        return true;
    }
    catch (...)
    {
        return false;
    }
}
