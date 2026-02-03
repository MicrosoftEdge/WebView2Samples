// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"
#include "FoundryLocalClient.h"
#include <sstream>
#include <regex>
#include <algorithm>
#include <cctype>
#include <shellapi.h>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "wininet.lib")

FoundryLocalClient::FoundryLocalClient()
{
    // Initialize WinHTTP session
    m_hSession = WinHttpOpen(
        L"WebView2APISample/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    // Initialize model catalog and set default system prompt
    InitializeModelCatalog();
    m_systemPrompt = GetDefaultSystemPrompt(m_modelAlias);
}

FoundryLocalClient::~FoundryLocalClient()
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

    if (m_hSession)
    {
        WinHttpCloseHandle(m_hSession);
    }
}

std::string FoundryLocalClient::WideToUtf8(const std::wstring& wide)
{
    if (wide.empty()) return std::string();
    int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string utf8(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &utf8[0], size, nullptr, nullptr);
    return utf8;
}

std::wstring FoundryLocalClient::Utf8ToWide(const std::string& utf8)
{
    if (utf8.empty()) return std::wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring wide(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], size);
    return wide;
}

std::string FoundryLocalClient::UnescapeJsonString(const std::string& input)
{
    std::string result;
    result.reserve(input.length());
    
    for (size_t i = 0; i < input.length(); ++i)
    {
        if (input[i] == '\\' && i + 1 < input.length())
        {
            char next = input[i + 1];
            switch (next)
            {
                case 'n': result += '\n'; ++i; break;
                case 'r': result += '\r'; ++i; break;
                case 't': result += '\t'; ++i; break;
                case '"': result += '"'; ++i; break;
                case '\\': result += '\\'; ++i; break;
                case '/': result += '/'; ++i; break;
                case 'b': result += '\b'; ++i; break;
                case 'f': result += '\f'; ++i; break;
                case 'u':
                    // Unicode escape: \uXXXX
                    if (i + 5 < input.length())
                    {
                        std::string hex = input.substr(i + 2, 4);
                        bool validHex = true;
                        for (char c : hex)
                        {
                            if (!isxdigit(static_cast<unsigned char>(c)))
                            {
                                validHex = false;
                                break;
                            }
                        }
                        if (validHex)
                        {
                            unsigned int codepoint = std::stoul(hex, nullptr, 16);
                            // Convert Unicode codepoint to UTF-8
                            if (codepoint < 0x80)
                            {
                                result += static_cast<char>(codepoint);
                            }
                            else if (codepoint < 0x800)
                            {
                                result += static_cast<char>(0xC0 | (codepoint >> 6));
                                result += static_cast<char>(0x80 | (codepoint & 0x3F));
                            }
                            else
                            {
                                result += static_cast<char>(0xE0 | (codepoint >> 12));
                                result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                                result += static_cast<char>(0x80 | (codepoint & 0x3F));
                            }
                            i += 5; // Skip \uXXXX
                        }
                        else
                        {
                            result += input[i];
                        }
                    }
                    else
                    {
                        result += input[i];
                    }
                    break;
                default:
                    result += input[i];
                    break;
            }
        }
        else
        {
            result += input[i];
        }
    }
    return result;
}

std::string FoundryLocalClient::FilterThinkTags(const std::string& content)
{
    // DeepSeek R1 outputs thinking content followed by </think> then the actual response
    // The format is: [thinking...]</think>\n\n[actual response]
    // We need to discard everything up to and including </think>
    
    m_thinkBuffer += content;
    
    // Look for </think> tag
    size_t endThinkPos = m_thinkBuffer.find("</think>");
    if (endThinkPos != std::string::npos)
    {
        // Found </think> - discard everything up to and including it
        // Also skip any newlines after it
        size_t startPos = endThinkPos + 8; // Skip "</think>"
        while (startPos < m_thinkBuffer.length() && 
               (m_thinkBuffer[startPos] == '\n' || m_thinkBuffer[startPos] == '\r'))
        {
            startPos++;
        }
        std::string result = m_thinkBuffer.substr(startPos);
        m_thinkBuffer.clear();
        m_inThinkBlock = false; // We've passed the think block
        return result;
    }
    
    // Also check for <think> tag to start blocking output
    size_t startThinkPos = m_thinkBuffer.find("<think>");
    if (startThinkPos != std::string::npos)
    {
        // Output everything before <think>, then start blocking
        std::string result = m_thinkBuffer.substr(0, startThinkPos);
        m_thinkBuffer = m_thinkBuffer.substr(startThinkPos + 7);
        m_inThinkBlock = true;
        return result;
    }
    
    // If we're in a think block (saw <think> but not </think> yet), don't output
    if (m_inThinkBlock)
    {
        // Check for partial </think> at the end
        for (size_t i = 1; i < 8 && i <= m_thinkBuffer.length(); ++i)
        {
            if (m_thinkBuffer.substr(m_thinkBuffer.length() - i) == std::string("</think>").substr(0, i))
            {
                return ""; // Wait for more data
            }
        }
        m_thinkBuffer.clear(); // Discard think content
        return "";
    }
    
    // Not in think block and no tags found - check for partial tags at the end
    // Check for partial <think> or </think>
    for (size_t i = 1; i < 8 && i <= m_thinkBuffer.length(); ++i)
    {
        std::string suffix = m_thinkBuffer.substr(m_thinkBuffer.length() - i);
        if (suffix == std::string("<think>").substr(0, i) || 
            suffix == std::string("</think>").substr(0, i))
        {
            // Potential partial tag - output everything except the potential tag
            std::string result = m_thinkBuffer.substr(0, m_thinkBuffer.length() - i);
            m_thinkBuffer = suffix;
            return result;
        }
    }
    
    // No tags or partial tags - output everything
    std::string result = m_thinkBuffer;
    m_thinkBuffer.clear();
    return result;
}

bool FoundryLocalClient::IsOnline()
{
    DWORD flags;
    return InternetGetConnectedState(&flags, 0) == TRUE;
}

void FoundryLocalClient::InitializeModelCatalog()
{
    m_modelCatalog = {
        {
            L"", // id discovered dynamically
            L"phi-4-mini",
            L"Phi-4 Mini",
            L"Microsoft's compact yet powerful language model, great for general tasks",
            L"You are a helpful, friendly AI assistant powered by Microsoft Phi-4. Provide clear, concise, and accurate responses. Be conversational and engaging.",
            false
        },
        {
            L"",
            L"deepseek-r1-7b",
            L"DeepSeek R1 7B",
            L"Advanced reasoning model, excellent for complex problem-solving and analysis",
            L"You are DeepSeek R1, an AI assistant specialized in deep reasoning and analysis. When solving problems, think step-by-step, show your reasoning process, and provide thorough explanations. Excel at math, logic, coding, and complex analytical tasks.",
            false
        },
        {
            L"",
            L"mistral-7b-v0.2",
            L"Mistral 7B",
            L"Mistral AI's efficient and versatile model, great for diverse tasks",
            L"You are a helpful AI assistant powered by Mistral AI. You are knowledgeable, balanced, and aim to be helpful while being direct and honest. Provide informative responses and engage thoughtfully with questions.",
            false
        }
    };
}

std::wstring FoundryLocalClient::GetDefaultSystemPrompt(const std::wstring& alias) const
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

std::vector<ModelInfo> FoundryLocalClient::GetAvailableModels()
{
    // Get cache list once and check all models against it
    std::wstring command = L"foundry cache list";
    std::wstring cacheOutput = ExecuteCommand(command);
    std::string cacheOutputUtf8 = WideToUtf8(cacheOutput);
    std::transform(cacheOutputUtf8.begin(), cacheOutputUtf8.end(), cacheOutputUtf8.begin(), ::tolower);
    
    // Update cached status for each model
    for (auto& model : m_modelCatalog)
    {
        std::string aliasUtf8 = WideToUtf8(model.alias);
        std::transform(aliasUtf8.begin(), aliasUtf8.end(), aliasUtf8.begin(), ::tolower);
        model.isCached = cacheOutputUtf8.find(aliasUtf8) != std::string::npos;
    }
    return m_modelCatalog;
}

bool FoundryLocalClient::SetModelAlias(const std::wstring& alias)
{
    // Verify alias exists in catalog
    bool found = false;
    bool isCached = false;
    for (const auto& model : m_modelCatalog)
    {
        if (model.alias == alias)
        {
            found = true;
            isCached = model.isCached;
            break;
        }
    }
    
    if (!found) return false;
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_modelAlias = alias;
        m_modelId.clear(); // Clear so it's re-discovered for new model
        m_systemPrompt = GetDefaultSystemPrompt(alias);
    }
    
    // If model is cached, load it into the service asynchronously
    if (isCached)
    {
        std::thread([alias]() {
            std::wstring command = L"foundry model load " + alias;
            STARTUPINFOW si = {sizeof(STARTUPINFOW)};
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;
            PROCESS_INFORMATION pi = {};
            std::wstring cmdLine = L"cmd.exe /c " + command;
            std::vector<wchar_t> cmdBuf(cmdLine.begin(), cmdLine.end());
            cmdBuf.push_back(0);
            if (CreateProcessW(nullptr, cmdBuf.data(), nullptr, nullptr, FALSE,
                               CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
            {
                WaitForSingleObject(pi.hProcess, 30000); // Wait up to 30s
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
        }).detach();
    }
    
    return true;
}

std::wstring FoundryLocalClient::GetCurrentModelAlias() const
{
    return m_modelAlias;
}

std::wstring FoundryLocalClient::GetCurrentModelDisplayName() const
{
    for (const auto& model : m_modelCatalog)
    {
        if (model.alias == m_modelAlias)
        {
            return model.displayName;
        }
    }
    return m_modelAlias;
}

std::wstring FoundryLocalClient::ExecuteCommand(const std::wstring& command)
{
    std::wstring result;
    
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
    {
        return L"";
    }

    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si = {sizeof(STARTUPINFOW)};
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;

    PROCESS_INFORMATION pi = {};
    
    std::wstring cmdLine = L"cmd.exe /c " + command;
    std::vector<wchar_t> cmdBuf(cmdLine.begin(), cmdLine.end());
    cmdBuf.push_back(0);

    if (CreateProcessW(nullptr, cmdBuf.data(), nullptr, nullptr, TRUE,
                       CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
    {
        CloseHandle(hWritePipe);
        hWritePipe = nullptr;

        std::string output;
        char buffer[4096];
        DWORD bytesRead;

        while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0)
        {
            buffer[bytesRead] = 0;
            output += buffer;
        }

        WaitForSingleObject(pi.hProcess, 5000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        result = Utf8ToWide(output);
    }

    if (hWritePipe) CloseHandle(hWritePipe);
    CloseHandle(hReadPipe);

    return result;
}

bool FoundryLocalClient::RunElevatedCommand(const std::wstring& command, const std::wstring& args)
{
    SHELLEXECUTEINFOW sei = {sizeof(SHELLEXECUTEINFOW)};
    sei.lpVerb = L"runas";
    sei.lpFile = command.c_str();
    sei.lpParameters = args.c_str();
    sei.nShow = SW_SHOWNORMAL;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;

    if (ShellExecuteExW(&sei))
    {
        if (sei.hProcess)
        {
            WaitForSingleObject(sei.hProcess, INFINITE);
            DWORD exitCode;
            GetExitCodeProcess(sei.hProcess, &exitCode);
            CloseHandle(sei.hProcess);
            return exitCode == 0;
        }
        return true;
    }
    return false;
}

bool FoundryLocalClient::IsFoundryInstalled()
{
    // Run 'where foundry' to check if foundry.exe is in PATH
    std::wstring whereResult = ExecuteCommand(L"where foundry");
    if (whereResult.empty() || 
        whereResult.find(L"Could not find") != std::wstring::npos ||
        whereResult.find(L"INFO:") != std::wstring::npos)
    {
        return false;
    }

    // Double-check by running foundry --version
    std::wstring versionResult = ExecuteCommand(L"foundry --version");
    
    // Check for a version number pattern (digits and dots like "0.5.100")
    if (versionResult.empty())
    {
        return false;
    }
    
    // Look for version number pattern
    for (size_t i = 0; i < versionResult.length(); ++i)
    {
        if (iswdigit(versionResult[i]))
        {
            if (i + 1 < versionResult.length() && 
                (versionResult[i + 1] == L'.' || iswdigit(versionResult[i + 1])))
            {
                return true;
            }
        }
    }
    
    return false;
}

bool FoundryLocalClient::DiscoverServicePort()
{
    // Run 'foundry service status' and parse the port from output
    // Output looks like: "🟢 Model management service is running on http://127.0.0.1:62580/openai/status"
    std::wstring result = ExecuteCommand(L"foundry service status");
    
    if (result.empty())
    {
        return false;
    }

    // Look for the port in the URL pattern "127.0.0.1:XXXXX"
    size_t hostPos = result.find(L"127.0.0.1:");
    if (hostPos == std::wstring::npos)
    {
        // Try localhost
        hostPos = result.find(L"localhost:");
    }
    
    if (hostPos != std::wstring::npos)
    {
        // Find the colon and extract the port number
        size_t colonPos = result.find(L':', hostPos);
        if (colonPos != std::wstring::npos)
        {
            size_t portStart = colonPos + 1;
            size_t portEnd = portStart;
            
            // Find end of port number
            while (portEnd < result.length() && iswdigit(result[portEnd]))
            {
                portEnd++;
            }
            
            if (portEnd > portStart)
            {
                std::wstring portStr = result.substr(portStart, portEnd - portStart);
                m_servicePort = std::stoi(portStr);
                return true;
            }
        }
    }
    
    return false;
}

bool FoundryLocalClient::DiscoverModelId()
{
    if (!m_hSession || m_servicePort == 0) return false;

    HINTERNET hConnect = WinHttpConnect(
        m_hSession,
        m_serviceHost.c_str(),
        static_cast<INTERNET_PORT>(m_servicePort),
        0);

    if (!hConnect) return false;

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        L"/v1/models",
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0);

    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        return false;
    }

    bool found = false;
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                           WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
    {
        if (WinHttpReceiveResponse(hRequest, nullptr))
        {
            std::string response;
            char buffer[4096];
            DWORD bytesRead;

            while (WinHttpReadData(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0)
            {
                buffer[bytesRead] = 0;
                response += buffer;
            }

            // Parse response to find model ID containing our alias
            // Response looks like: {"data":[{"id":"Phi-4-mini-instruct-generic-cpu:5",...}]}
            // Look for "id":" pattern and extract the model ID
            std::string aliasLower = WideToUtf8(m_modelAlias);
            std::transform(aliasLower.begin(), aliasLower.end(), aliasLower.begin(), ::tolower);

            size_t pos = 0;
            while ((pos = response.find("\"id\":\"", pos)) != std::string::npos)
            {
                pos += 6; // Skip "id":"
                size_t endPos = response.find("\"", pos);
                if (endPos != std::string::npos)
                {
                    std::string modelId = response.substr(pos, endPos - pos);
                    std::string modelIdLower = modelId;
                    std::transform(modelIdLower.begin(), modelIdLower.end(), modelIdLower.begin(), ::tolower);
                    
                    // Match model ID based on alias patterns
                    // phi-4-mini -> matches "phi-4-mini" or "phi4mini" in model ID
                    // deepseek-r1-7b -> matches "deepseek" AND "7b" in model ID
                    // mistral-7b-v0.2 -> matches "mistral" AND "7b" in model ID
                    bool matches = false;
                    
                    if (aliasLower.find("phi-4-mini") != std::string::npos)
                    {
                        matches = modelIdLower.find("phi-4-mini") != std::string::npos ||
                                  modelIdLower.find("phi4mini") != std::string::npos;
                    }
                    else if (aliasLower.find("deepseek") != std::string::npos)
                    {
                        // DeepSeek R1 7B model ID: deepseek-r1-distill-qwen-7b-generic-cpu:3
                        matches = modelIdLower.find("deepseek") != std::string::npos &&
                                  modelIdLower.find("7b") != std::string::npos;
                    }
                    else if (aliasLower.find("mistral") != std::string::npos)
                    {
                        matches = modelIdLower.find("mistral") != std::string::npos;
                    }
                    else
                    {
                        // Generic fallback: check if all parts of alias appear in model ID
                        std::string aliasNorm = aliasLower;
                        aliasNorm.erase(std::remove(aliasNorm.begin(), aliasNorm.end(), '-'), aliasNorm.end());
                        std::string modelIdNorm = modelIdLower;
                        modelIdNorm.erase(std::remove(modelIdNorm.begin(), modelIdNorm.end(), '-'), modelIdNorm.end());
                        matches = modelIdNorm.find(aliasNorm) != std::string::npos;
                    }
                    
                    if (matches)
                    {
                        m_modelId = Utf8ToWide(modelId);
                        found = true;
                        break;
                    }
                    pos = endPos;
                }
            }
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    return found;
}

bool FoundryLocalClient::IsServiceRunning()
{
    if (!m_hSession) return false;

    // First, discover the port if not yet known
    if (m_servicePort == 0)
    {
        if (!DiscoverServicePort())
        {
            return false;
        }
    }

    // Try to connect to the service status endpoint
    HINTERNET hConnect = WinHttpConnect(
        m_hSession,
        m_serviceHost.c_str(),
        static_cast<INTERNET_PORT>(m_servicePort),
        0);

    if (!hConnect) return false;

    // Use /openai/status endpoint which is what Foundry Local exposes
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        L"/openai/status",
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0);

    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        return false;
    }

    bool running = false;
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                           WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
    {
        if (WinHttpReceiveResponse(hRequest, nullptr))
        {
            DWORD statusCode = 0;
            DWORD size = sizeof(statusCode);
            WinHttpQueryHeaders(hRequest,
                               WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                               WINHTTP_HEADER_NAME_BY_INDEX,
                               &statusCode, &size, WINHTTP_NO_HEADER_INDEX);
            running = (statusCode == 200);
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    return running;
}

bool FoundryLocalClient::StartService()
{
    // Reset port to force rediscovery
    m_servicePort = 0;
    
    // First check if already running
    if (DiscoverServicePort() && IsServiceRunning()) 
    {
        return true;
    }

    // Start the service via CLI
    std::wstring result = ExecuteCommand(L"foundry service start");
    
    // Wait for service to start and discover port
    for (int i = 0; i < 30; i++) // Try for 30 seconds
    {
        Sleep(1000);
        m_servicePort = 0; // Reset to force rediscovery
        if (DiscoverServicePort() && IsServiceRunning())
        {
            return true;
        }
    }
    
    return false;
}

bool FoundryLocalClient::InstallFoundryLocal()
{
    // Use winget to install Foundry Local with elevation
    return RunElevatedCommand(L"winget", L"install Microsoft.FoundryLocal --accept-source-agreements --accept-package-agreements");
}

bool FoundryLocalClient::IsModelCached(const std::wstring& alias)
{
    // Check catalog first (faster if recently updated)
    for (const auto& model : m_modelCatalog)
    {
        if (model.alias == alias)
        {
            return model.isCached;
        }
    }
    
    // Fallback: use CLI to check if model is in local cache
    std::wstring command = L"foundry cache list";
    std::wstring output = ExecuteCommand(command);
    
    if (output.empty()) return false;
    
    std::string outputUtf8 = WideToUtf8(output);
    std::string aliasUtf8 = WideToUtf8(alias);
    
    std::transform(outputUtf8.begin(), outputUtf8.end(), outputUtf8.begin(), ::tolower);
    std::transform(aliasUtf8.begin(), aliasUtf8.end(), aliasUtf8.begin(), ::tolower);
    
    return outputUtf8.find(aliasUtf8) != std::string::npos;
}

void FoundryLocalClient::DownloadModelAsync(
    const std::wstring& alias,
    ProgressCallback progressCallback,
    CompletionCallback completionCallback)
{
    if (m_downloadThread.joinable())
    {
        m_downloadThread.join();
    }

    m_downloadThread = std::thread([this, alias, progressCallback, completionCallback]()
    {
        // Use CLI to download the model (it shows progress)
        // We'll poll for completion since CLI doesn't have a progress API
        progressCallback(0.0);

        std::wstring command = L"foundry model download " + alias;
        
        // Run the download command
        STARTUPINFOW si = {sizeof(STARTUPINFOW)};
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        PROCESS_INFORMATION pi = {};
        
        std::wstring cmdLine = L"cmd.exe /c " + command;
        std::vector<wchar_t> cmdBuf(cmdLine.begin(), cmdLine.end());
        cmdBuf.push_back(0);

        if (CreateProcessW(nullptr, cmdBuf.data(), nullptr, nullptr, FALSE,
                           CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
        {
            // Simulate progress while waiting
            double progress = 0.0;
            while (WaitForSingleObject(pi.hProcess, 500) == WAIT_TIMEOUT)
            {
                if (progress < 95.0)
                {
                    progress += 2.0;
                    progressCallback(progress);
                }
            }

            DWORD exitCode;
            GetExitCodeProcess(pi.hProcess, &exitCode);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            if (exitCode == 0)
            {
                progressCallback(100.0);
                completionCallback(true, L"");
            }
            else
            {
                completionCallback(false, L"Download failed with exit code: " + std::to_wstring(exitCode));
            }
        }
        else
        {
            completionCallback(false, L"Failed to start download process");
        }
    });
}

std::string FoundryLocalClient::BuildChatRequestJson(const std::vector<ChatMessage>& messages)
{
    std::ostringstream json;
    json << "{";
    // Use discovered model ID, or fall back to alias if not discovered
    std::wstring modelToUse = m_modelId.empty() ? m_modelAlias : m_modelId;
    json << "\"model\":\"" << WideToUtf8(modelToUse) << "\",";
    json << "\"stream\":true,";
    json << "\"max_tokens\":4096,";
    json << "\"messages\":[";

    // Add system prompt first
    json << "{\"role\":\"system\",\"content\":\"" << WideToUtf8(m_systemPrompt) << "\"}";

    for (const auto& msg : messages)
    {
        json << ",{\"role\":\"" << WideToUtf8(msg.role) << "\",";
        
        // Escape special characters in content
        std::string content = WideToUtf8(msg.content);
        std::string escaped;
        for (char c : content)
        {
            switch (c)
            {
                case '"': escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default: escaped += c;
            }
        }
        json << "\"content\":\"" << escaped << "\"}";
    }

    json << "]}";
    return json.str();
}

void FoundryLocalClient::ParseSSEStream(const std::string& chunk, TokenCallback& tokenCallback)
{
    // Parse Server-Sent Events format
    std::istringstream stream(chunk);
    std::string line;

    while (std::getline(stream, line))
    {
        // Remove \r if present
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        // SSE data lines start with "data: "
        if (line.rfind("data: ", 0) == 0)
        {
            std::string data = line.substr(6);
            
            // Check for end of stream
            if (data == "[DONE]")
            {
                continue;
            }

            // Parse JSON to extract content
            // Look for "content":"..." with proper handling of escaped quotes
            size_t contentPos = data.find("\"content\":\"");
            if (contentPos != std::string::npos)
            {
                contentPos += 11; // Skip "content":"
                
                // Find the end of the JSON string, accounting for escaped quotes
                size_t endPos = contentPos;
                while (endPos < data.length())
                {
                    if (data[endPos] == '"')
                    {
                        // Check if this quote is escaped
                        size_t backslashCount = 0;
                        size_t checkPos = endPos;
                        while (checkPos > contentPos && data[checkPos - 1] == '\\')
                        {
                            backslashCount++;
                            checkPos--;
                        }
                        // If even number of backslashes, the quote is not escaped
                        if (backslashCount % 2 == 0)
                        {
                            break;
                        }
                    }
                    endPos++;
                }
                
                if (endPos < data.length())
                {
                    std::string content = data.substr(contentPos, endPos - contentPos);
                    std::string unescaped = UnescapeJsonString(content);

                    if (!unescaped.empty())
                    {
                        // Filter out DeepSeek R1 <think>...</think> blocks
                        std::string filtered = FilterThinkTags(unescaped);
                        if (!filtered.empty())
                        {
                            tokenCallback(Utf8ToWide(filtered));
                        }
                    }
                }
            }
            // Also check for delta.content format used by OpenAI-compatible APIs
            else if ((contentPos = data.find("\"delta\":{\"content\":\"")) != std::string::npos)
            {
                contentPos += 20; // Skip "delta":{"content":"
                
                // Find the end of the JSON string, accounting for escaped quotes
                size_t endPos = contentPos;
                while (endPos < data.length())
                {
                    if (data[endPos] == '"')
                    {
                        // Check if this quote is escaped
                        size_t backslashCount = 0;
                        size_t checkPos = endPos;
                        while (checkPos > contentPos && data[checkPos - 1] == '\\')
                        {
                            backslashCount++;
                            checkPos--;
                        }
                        // If even number of backslashes, the quote is not escaped
                        if (backslashCount % 2 == 0)
                        {
                            break;
                        }
                    }
                    endPos++;
                }
                
                if (endPos < data.length())
                {
                    std::string content = data.substr(contentPos, endPos - contentPos);
                    std::string unescaped = UnescapeJsonString(content);
                    if (!unescaped.empty())
                    {
                        // Filter out DeepSeek R1 <think>...</think> blocks
                        std::string filtered = FilterThinkTags(unescaped);
                        if (!filtered.empty())
                        {
                            tokenCallback(Utf8ToWide(filtered));
                        }
                    }
                }
            }
        }
    }
}

void FoundryLocalClient::ChatCompletionAsync(
    const std::vector<ChatMessage>& messages,
    TokenCallback tokenCallback,
    CompletionCallback completionCallback)
{
    if (m_inferenceThread.joinable())
    {
        m_inferenceThread.join();
    }

    m_cancelRequested = false;
    m_thinkBuffer.clear();
    
    // DeepSeek R1 models always start with thinking content, so start in think block mode
    std::wstring aliasLower = m_modelAlias;
    std::transform(aliasLower.begin(), aliasLower.end(), aliasLower.begin(), ::tolower);
    m_inThinkBlock = (aliasLower.find(L"deepseek") != std::wstring::npos);

    // Discover model ID if not yet known
    if (m_modelId.empty())
    {
        DiscoverModelId();
    }

    m_inferenceThread = std::thread([this, messages, tokenCallback, completionCallback]()
    {
        if (!m_hSession)
        {
            completionCallback(false, L"HTTP session not initialized");
            return;
        }

        HINTERNET hConnect = WinHttpConnect(
            m_hSession,
            m_serviceHost.c_str(),
            static_cast<INTERNET_PORT>(m_servicePort),
            0);

        if (!hConnect)
        {
            completionCallback(false, L"Failed to connect to Foundry Local service");
            return;
        }

        HINTERNET hRequest = WinHttpOpenRequest(
            hConnect,
            L"POST",
            L"/v1/chat/completions",
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            0);

        if (!hRequest)
        {
            WinHttpCloseHandle(hConnect);
            completionCallback(false, L"Failed to create HTTP request");
            return;
        }

        // Set timeouts for long inference operations (5 minutes)
        DWORD timeout = 300000; // 5 minutes in milliseconds
        WinHttpSetOption(hRequest, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
        WinHttpSetOption(hRequest, WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT, &timeout, sizeof(timeout));

        // Build request body
        std::string requestBody = BuildChatRequestJson(messages);

        // Set headers
        std::wstring headers = L"Content-Type: application/json\r\nAccept: text/event-stream";

        if (!WinHttpSendRequest(hRequest,
                                headers.c_str(),
                                static_cast<DWORD>(headers.length()),
                                const_cast<char*>(requestBody.c_str()),
                                static_cast<DWORD>(requestBody.length()),
                                static_cast<DWORD>(requestBody.length()),
                                0))
        {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            completionCallback(false, L"Failed to send HTTP request");
            return;
        }

        if (!WinHttpReceiveResponse(hRequest, nullptr))
        {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            completionCallback(false, L"Failed to receive HTTP response");
            return;
        }

        // Check status code
        DWORD statusCode = 0;
        DWORD size = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest,
                           WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                           WINHTTP_HEADER_NAME_BY_INDEX,
                           &statusCode, &size, WINHTTP_NO_HEADER_INDEX);

        if (statusCode != 200)
        {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            completionCallback(false, L"HTTP error: " + std::to_wstring(statusCode));
            return;
        }

        // Read streaming response
        char buffer[4096];
        DWORD bytesRead;
        TokenCallback callback = tokenCallback; // Copy for use in lambda
        std::string pendingData; // Buffer to accumulate partial SSE lines

        while (!m_cancelRequested)
        {
            if (!WinHttpReadData(hRequest, buffer, sizeof(buffer) - 1, &bytesRead))
            {
                break;
            }

            if (bytesRead == 0)
            {
                break;
            }

            buffer[bytesRead] = 0;
            pendingData += std::string(buffer, bytesRead);
            
            // Process complete lines only
            size_t pos = 0;
            size_t lineEnd;
            while ((lineEnd = pendingData.find('\n', pos)) != std::string::npos)
            {
                std::string line = pendingData.substr(pos, lineEnd - pos);
                // Remove \r if present
                if (!line.empty() && line.back() == '\r')
                {
                    line.pop_back();
                }
                
                // Process SSE data line
                if (line.rfind("data: ", 0) == 0)
                {
                    std::string data = line.substr(6);
                    
                    if (data != "[DONE]")
                    {
                        // Parse JSON to extract content
                        size_t contentPos = data.find("\"content\":\"");
                        if (contentPos != std::string::npos)
                        {
                            contentPos += 11; // Skip "content":"
                            
                            // Find the end of the JSON string, accounting for escaped quotes
                            size_t endPos = contentPos;
                            while (endPos < data.length())
                            {
                                if (data[endPos] == '"')
                                {
                                    size_t backslashCount = 0;
                                    size_t checkPos = endPos;
                                    while (checkPos > contentPos && data[checkPos - 1] == '\\')
                                    {
                                        backslashCount++;
                                        checkPos--;
                                    }
                                    if (backslashCount % 2 == 0)
                                    {
                                        break;
                                    }
                                }
                                endPos++;
                            }
                            
                            if (endPos < data.length())
                            {
                                std::string content = data.substr(contentPos, endPos - contentPos);
                                std::string unescaped = UnescapeJsonString(content);
                                if (!unescaped.empty())
                                {
                                    callback(Utf8ToWide(unescaped));
                                }
                            }
                        }
                        // Check delta.content format
                        else if ((contentPos = data.find("\"delta\":{\"content\":\"")) != std::string::npos)
                        {
                            contentPos += 20;
                            size_t endPos = contentPos;
                            while (endPos < data.length())
                            {
                                if (data[endPos] == '"')
                                {
                                    size_t backslashCount = 0;
                                    size_t checkPos = endPos;
                                    while (checkPos > contentPos && data[checkPos - 1] == '\\')
                                    {
                                        backslashCount++;
                                        checkPos--;
                                    }
                                    if (backslashCount % 2 == 0)
                                    {
                                        break;
                                    }
                                }
                                endPos++;
                            }
                            
                            if (endPos < data.length())
                            {
                                std::string content = data.substr(contentPos, endPos - contentPos);
                                std::string unescaped = UnescapeJsonString(content);
                                if (!unescaped.empty())
                                {
                                    callback(Utf8ToWide(unescaped));
                                }
                            }
                        }
                    }
                }
                
                pos = lineEnd + 1;
            }
            
            // Keep any incomplete line for next iteration
            if (pos < pendingData.length())
            {
                pendingData = pendingData.substr(pos);
            }
            else
            {
                pendingData.clear();
            }
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);

        if (m_cancelRequested)
        {
            completionCallback(false, L"Inference cancelled");
        }
        else
        {
            completionCallback(true, L"");
        }
    });
}

void FoundryLocalClient::CancelInference()
{
    m_cancelRequested = true;
}

void FoundryLocalClient::SetSystemPrompt(const std::wstring& prompt)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_systemPrompt = prompt;
}

std::wstring FoundryLocalClient::GetSystemPrompt() const
{
    return m_systemPrompt;
}

std::wstring FoundryLocalClient::GetServiceEndpoint()
{
    return L"http://" + m_serviceHost + L":" + std::to_wstring(m_servicePort);
}
