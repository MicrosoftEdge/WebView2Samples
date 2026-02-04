# SLM Chat Scenario - Technical Documentation

## Overview

The SLM Chat scenario demonstrates how to integrate on-device Small Language Models (SLMs) with WebView2 applications using Azure Foundry Local. This enables AI-powered chat functionality that runs entirely on the user's device without requiring cloud connectivity after initial setup.

## Table of Contents

1. [Requirements](#requirements)
2. [Architecture](#architecture)
3. [Design Decisions](#design-decisions)
4. [Trade-offs & Considerations](#trade-offs--considerations)
5. [Implementation Details](#implementation-details)
6. [File Structure](#file-structure)
7. [Supported Models](#supported-models)
8. [Usage Instructions](#usage-instructions)
9. [Future Enhancements](#future-enhancements)

---

## Requirements

### Functional Requirements

1. **On-device AI Chat**: Users should be able to chat with an AI model running locally on their device
2. **Model Selection**: Support multiple SLM models with easy switching
3. **Streaming Responses**: Display AI responses token-by-token as they're generated
4. **Offline Capability**: After initial model download, chat should work without internet
5. **Auto-setup**: Automatically install and configure Foundry Local if not present
6. **Native-Web Interop**: Allow both native C++ code and WebView2 JavaScript to access SLM capabilities

### Non-Functional Requirements

1. **Performance**: Responsive UI during model inference
2. **User Experience**: Modern, Copilot-style chat interface
3. **Reliability**: Graceful error handling and recovery
4. **Maintainability**: Clean separation of concerns between native and web layers

### Dependencies

- **Azure Foundry Local**: Runtime for on-device model inference
- **Windows 10/11**: Required for WebView2 and Foundry Local
- **WebView2 Runtime**: For rendering the chat UI
- **WinHTTP**: For REST API communication with Foundry Local service

---

## Architecture

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        WebView2APISample                         │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────────────┐     ┌─────────────────────────────────┐   │
│  │   AppWindow     │     │         ScenarioSLM             │   │
│  │  (Menu Handler) │────▶│    (Component Registration)     │   │
│  └─────────────────┘     └─────────────────────────────────┘   │
│                                       │                         │
│                                       ▼                         │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                   SLMHostObjectImpl                      │   │
│  │              (COM Host Object for JS)                    │   │
│  │  ┌─────────────────────────────────────────────────┐    │   │
│  │  │  - SetupAsync()      - GetModels()              │    │   │
│  │  │  - InferAsync()      - SetModel()               │    │   │
│  │  │  - QueryStatus()     - GetCurrentModel()        │    │   │
│  │  │  - CancelInference()                            │    │   │
│  │  └─────────────────────────────────────────────────┘    │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                       │                         │
│                                       ▼                         │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                  FoundryLocalClient                      │   │
│  │              (REST API Client for Foundry)               │   │
│  │  ┌─────────────────────────────────────────────────┐    │   │
│  │  │  - Service Discovery (dynamic port)             │    │   │
│  │  │  - Model Management (list, download, load)      │    │   │
│  │  │  - Chat Completions (streaming SSE)             │    │   │
│  │  │  - Auto-install via winget                      │    │   │
│  │  └─────────────────────────────────────────────────┘    │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                       │                         │
└───────────────────────────────────────│─────────────────────────┘
                                        │
                                        ▼
┌─────────────────────────────────────────────────────────────────┐
│                      Foundry Local Service                       │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │   /v1/models    │  │/v1/chat/complete│  │  /openai/status │ │
│  │  (Model List)   │  │   (Inference)   │  │ (Health Check)  │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

| Component | Responsibility |
|-----------|----------------|
| **AppWindow** | Handles menu selection, creates ScenarioSLM component |
| **ScenarioSLM** | Registers host object, navigates to chat HTML |
| **SLMHostObjectImpl** | Bridges JavaScript calls to native FoundryLocalClient |
| **FoundryLocalClient** | Manages all Foundry Local REST API interactions |
| **ScenarioSLM.html** | Chat UI with JavaScript for user interaction |

### Data Flow

1. **User sends message** → JavaScript calls `slm.InferAsync()`
2. **SLMHostObjectImpl** → Adds message to conversation history, calls FoundryLocalClient
3. **FoundryLocalClient** → Sends POST to `/v1/chat/completions` with SSE streaming
4. **Streaming response** → Parsed tokens sent back via callback chain
5. **JavaScript callback** → Updates UI with each token in real-time

---

## Design Decisions

### 1. COM Host Object Pattern

**Decision**: Use WebView2's AddHostObjectToScript to expose SLM functionality to JavaScript.

**Rationale**:
- Allows rich bidirectional communication between native and web
- JavaScript can call native methods asynchronously
- Native code can invoke JavaScript callbacks
- Clean separation between UI (HTML/JS) and business logic (C++)

**Alternative Considered**: PostWebMessageAsJson
- Rejected because it requires more boilerplate for complex async operations

### 2. REST API vs CLI

**Decision**: Use Foundry Local's REST API for inference, CLI for model management.

**Rationale**:
- REST API provides streaming (SSE) for real-time token display
- REST API is more performant for repeated calls
- CLI is simpler for one-time operations (download, load)
- CLI provides better progress output for downloads

### 3. Dynamic Port Discovery

**Decision**: Parse `foundry service status` output to discover the service port.

**Rationale**:
- Foundry Local uses dynamic ports (not fixed)
- Service status command reliably returns the current endpoint
- More robust than hardcoding or config files

**Implementation**:
```cpp
// Parse: "🟢 Model management service is running on http://127.0.0.1:62580/openai/status"
std::wregex portRegex(L"http://[^:]+:(\\d+)");
```

### 4. Model Catalog Approach

**Decision**: Maintain a curated catalog of 3 models with model-specific system prompts.

**Rationale**:
- Better user experience than showing all 20+ available models
- Allows tailored system prompts per model personality
- Simplifies UI and reduces decision fatigue

**Models Selected**:
| Model | Use Case | Size |
|-------|----------|------|
| Phi-4 Mini | General purpose, conversational | 3.7 GB |
| DeepSeek R1 7B | Reasoning, math, coding | 5.6 GB |
| Mistral 7B | Versatile, balanced | 4.1 GB |

### 5. JavaScript-Side Think Tag Filtering

**Decision**: Filter DeepSeek's `<think>...</think>` tags in JavaScript, not C++.

**Rationale**:
- Streaming chunks split tags unpredictably
- JavaScript can buffer the full response easily
- Enables showing a collapsible "Thinking" UI
- Keeps C++ layer model-agnostic

### 6. Async Model Loading

**Decision**: Load models asynchronously in a detached thread.

**Rationale**:
- Model loading takes 5-15 seconds
- UI must remain responsive during loading
- User can continue viewing conversation while model switches

---

## Trade-offs & Considerations

### Trade-off 1: Local vs Cloud

| Aspect | Local (Foundry) | Cloud |
|--------|-----------------|-------|
| Privacy | ✅ Data stays on device | ❌ Data sent to servers |
| Latency | ✅ No network round-trip | ❌ Network dependent |
| Cost | ✅ Free after download | ❌ Per-token pricing |
| Model Size | ❌ Limited by device | ✅ Large models available |
| Setup | ❌ Initial download required | ✅ Instant access |

**Our Choice**: Local-first with Foundry Local for privacy and offline capability.

### Trade-off 2: Streaming vs Complete Response

| Aspect | Streaming | Complete |
|--------|-----------|----------|
| User Experience | ✅ Immediate feedback | ❌ Wait for full response |
| Complexity | ❌ SSE parsing, chunking | ✅ Simple JSON response |
| Error Handling | ❌ Partial responses possible | ✅ All or nothing |

**Our Choice**: Streaming for better perceived performance.

### Trade-off 3: Curated vs Full Model List

| Aspect | Curated (3 models) | Full List (20+) |
|--------|-------------------|-----------------|
| User Experience | ✅ Simple choice | ❌ Overwhelming |
| Flexibility | ❌ Limited options | ✅ All models available |
| Maintenance | ✅ Easy to update | ❌ Must track all models |

**Our Choice**: Curated list for simplicity, can expand later.

### Consideration: Memory Usage

- Each model requires 4-12 GB RAM when loaded
- Only one model loaded at a time (Foundry manages this)
- Model unloads automatically after TTL (default 600s)

### Consideration: First-Run Experience

- First launch requires internet for model download
- Downloads can be 4-10 GB depending on model
- Progress indication essential for user trust

### Consideration: DeepSeek R1 Thinking

- DeepSeek outputs reasoning in `<think>` tags
- Raw output would show duplicated content
- Solution: Collapsible thinking UI shows process without clutter

---

## Implementation Details

### Foundry Local REST API

**Base URL**: `http://127.0.0.1:{dynamic_port}`

**Endpoints Used**:

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/openai/status` | GET | Health check |
| `/v1/models` | GET | List loaded models |
| `/v1/chat/completions` | POST | Chat inference (SSE) |

**Chat Request Format**:
```json
{
  "model": "Phi-4-mini-instruct-generic-cpu:5",
  "stream": true,
  "max_tokens": 4096,
  "messages": [
    {"role": "system", "content": "You are a helpful assistant..."},
    {"role": "user", "content": "Hello!"},
    {"role": "assistant", "content": "Hi there!"},
    {"role": "user", "content": "Who are you?"}
  ]
}
```

**SSE Response Format**:
```
data: {"choices":[{"delta":{"content":"I"}}]}
data: {"choices":[{"delta":{"content":" am"}}]}
data: {"choices":[{"delta":{"content":" Phi"}}]}
data: [DONE]
```

### Model ID Discovery

Foundry Local uses verbose model IDs like `Phi-4-mini-instruct-generic-cpu:5`. Our matching logic:

```cpp
// Alias: "phi-4-mini" must match ID: "Phi-4-mini-instruct-generic-cpu:5"
if (aliasLower.find("phi-4-mini") != std::string::npos) {
    matches = modelIdLower.find("phi-4-mini") != std::string::npos;
}
else if (aliasLower.find("deepseek") != std::string::npos) {
    // DeepSeek R1 7B ID: "deepseek-r1-distill-qwen-7b-generic-cpu:3"
    matches = modelIdLower.find("deepseek") != std::string::npos &&
              modelIdLower.find("7b") != std::string::npos;
}
```

### Conversation History Management

- History maintained in JavaScript `conversationHistory` array
- Sent with each inference request for context
- No persistent storage (cleared on page reload)

### Error Handling Strategy

| Error | Detection | Recovery |
|-------|-----------|----------|
| Foundry not installed | `where foundry` fails | Prompt winget install |
| Service not running | Connection refused | Auto-start service |
| Model not cached | `/v1/models` doesn't list it | Trigger download |
| Inference failure | HTTP 4xx/5xx | Show error message |
| Network offline | InternetGetConnectedState | Show offline screen |

---

## File Structure

```
WebView2APISample/
├── FoundryLocalClient.h          # REST client header
├── FoundryLocalClient.cpp        # REST client implementation
│   ├── Service discovery
│   ├── Model management
│   ├── Chat completions (SSE)
│   └── Auto-installation
│
├── SLMHostObject.idl             # COM interface definition
│   ├── QueryStatus()
│   ├── SetupAsync()
│   ├── InferAsync()
│   ├── GetModels() / SetModel()
│   └── CancelInference()
│
├── SLMHostObjectImpl.h           # Host object header
├── SLMHostObjectImpl.cpp         # Host object implementation
│   ├── State machine (NotReady → Ready)
│   ├── Conversation history
│   └── Callback dispatching
│
├── ScenarioSLM.h                 # Scenario header
├── ScenarioSLM.cpp               # Scenario registration
│   └── Host object injection
│
├── assets/
│   └── ScenarioSLM.html          # Chat UI
│       ├── Copilot-style design
│       ├── Model selector dropdown
│       ├── Streaming message display
│       ├── DeepSeek thinking UI
│       └── Model switch dividers
│
├── AppWindow.cpp                 # (Modified) Menu handler
├── resource.h                    # (Modified) Menu ID
├── WebView2APISample.rc          # (Modified) Menu entry
└── WebView2APISample.vcxproj     # (Modified) Project file
```

---

## Supported Models

### Phi-4 Mini
- **Alias**: `phi-4-mini`
- **Full ID**: `Phi-4-mini-instruct-generic-cpu:5`
- **Size**: ~3.7 GB (CPU), ~4.8 GB (GPU)
- **Strengths**: General tasks, conversation, tool calling
- **System Prompt**: Friendly, conversational assistant

### DeepSeek R1 7B
- **Alias**: `deepseek-r1-7b`
- **Full ID**: `deepseek-r1-distill-qwen-7b-generic-cpu:3`
- **Size**: ~5.6 GB (CPU), ~6.4 GB (GPU)
- **Strengths**: Mathematical reasoning, coding, analysis
- **System Prompt**: Step-by-step reasoning focus
- **Special**: Outputs `<think>...</think>` reasoning blocks

### Mistral 7B
- **Alias**: `mistral-7b-v0.2`
- **Full ID**: `mistralai-Mistral-7B-Instruct-v0-2-generic-cpu:2`
- **Size**: ~4.1 GB
- **Strengths**: Versatile, balanced performance
- **System Prompt**: Direct, informative assistant

---

## Usage Instructions

### Accessing the Feature

1. Build and run WebView2APISample
2. Go to **Scenario** menu → **SLM Chat**
3. First run will auto-install Foundry Local if needed

### First-Time Setup

1. Select a model from the dropdown
2. Click "Download [Model Name]" if not cached
3. Wait for download to complete (shows progress)
4. Click "Start Chatting" when ready

### Chatting

1. Type message in the input box
2. Press Enter or click Send
3. Response streams in real-time
4. For DeepSeek: Click "Thinking" to see reasoning process

### Switching Models

1. Click the model dropdown (top-left)
2. Select a different model
3. If cached: Divider appears, continue chatting
4. If not cached: Returns to setup screen for download

---

## Future Enhancements

### Potential Improvements

1. **Persistent Conversation History**
   - Save/load conversations to local storage
   - Export conversations as text/JSON

2. **More Models**
   - Add Qwen, Llama (when available in Foundry)
   - Allow custom model configuration

3. **Advanced Features**
   - System prompt customization
   - Temperature/token limit controls
   - Stop sequences

4. **UI Enhancements**
   - Dark mode support
   - Markdown rendering in responses
   - Code syntax highlighting
   - Copy message button

5. **Performance**
   - GPU acceleration toggle
   - Model preloading
   - Response caching

6. **Native Integration**
   - Expose SLM to other scenarios
   - Use SLM for in-app assistance

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | Feb 2026 | Initial implementation with Phi-4, DeepSeek R1, Mistral |

---

## References

- [Azure Foundry Local Documentation](https://learn.microsoft.com/en-us/azure/ai-foundry/)
- [WebView2 Host Objects](https://learn.microsoft.com/en-us/microsoft-edge/webview2/how-to/hostobject)
- [OpenAI Chat Completions API](https://platform.openai.com/docs/api-reference/chat)

---

*Document generated: February 2026*
*Branch: feature/slm-chat*
*Commit: ff72131*
