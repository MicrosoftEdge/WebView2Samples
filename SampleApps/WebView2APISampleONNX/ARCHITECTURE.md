# WebView2APISampleONNX - Architecture Documentation

## Overview

WebView2APISampleONNX is a Win32 C++ application that demonstrates how to integrate a local Small Language Model (SLM) using ONNX Runtime GenAI with Microsoft WebView2. The application provides a Copilot-style chat interface running entirely on-device without requiring cloud connectivity.

---

## Requirements

### Functional Requirements

1. **Local AI Chat**: Provide a conversational AI experience using a locally-running language model
2. **Model Management**: Automatic download and caching of AI models from Hugging Face
3. **WebView2 Integration**: Use WebView2 for the chat UI with JavaScript-to-native interop
4. **Streaming Responses**: Display AI responses token-by-token as they're generated
5. **Offline Capable**: Once the model is downloaded, the app works without internet

### Non-Functional Requirements

1. **Performance**: Responsive UI during model inference
2. **Memory**: Efficient memory usage for large language models (~2.7GB model)
3. **Compatibility**: Support both GPU (DirectML) and CPU-only environments
4. **User Experience**: Modern Copilot-style UI with smooth animations

---

## Architecture Design

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    WebView2APISampleONNX                    │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────┐    ┌─────────────────────────────────┐│
│  │   Win32 Host    │    │         WebView2 Browser        ││
│  │                 │    │                                 ││
│  │ ┌─────────────┐ │    │  ┌───────────────────────────┐ ││
│  │ │ AppWindow   │ │    │  │    ScenarioSLM.html       │ ││
│  │ │             │ │    │  │                           │ ││
│  │ │ ScenarioSLM │◄├────┼──┤  • Chat UI (Copilot-style)│ ││
│  │ │             │ │    │  │  • Message rendering      │ ││
│  │ └─────────────┘ │    │  │  • Progress indicators    │ ││
│  │                 │    │  │                           │ ││
│  │ ┌─────────────┐ │    │  └───────────────────────────┘ ││
│  │ │SLMHostObject│◄├────┼──► chrome.webview.hostObjects  ││
│  │ │  (IDispatch)│ │    │                                 ││
│  │ └──────┬──────┘ │    └─────────────────────────────────┘│
│  │        │        │                                       │
│  │ ┌──────▼──────┐ │                                       │
│  │ │ONNXRuntime  │ │                                       │
│  │ │  Client     │ │                                       │
│  │ └──────┬──────┘ │                                       │
│  └────────┼────────┘                                       │
│           │                                                │
├───────────┼────────────────────────────────────────────────┤
│           ▼                                                │
│  ┌─────────────────────────────────────────────────────┐  │
│  │              ONNX Runtime GenAI                      │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │  │
│  │  │   Model     │  │  Tokenizer  │  │  Generator  │  │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  │  │
│  └─────────────────────────────────────────────────────┘  │
│                                                            │
│  ┌─────────────────────────────────────────────────────┐  │
│  │         Phi-3 Mini 4K Instruct (CPU/GPU)            │  │
│  │              %LOCALAPPDATA%\...\models\             │  │
│  └─────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────┘
```

### Component Breakdown

#### 1. AppWindow (AppWindow.cpp)
- Main Win32 window management
- WebView2 environment initialization
- Menu handling and scenario routing

#### 2. ScenarioSLM (ScenarioSLM.cpp)
- Creates the SLM chat scenario
- Registers the `SLMHostObject` as a host object in WebView2
- Navigates to the chat HTML page

#### 3. SLMHostObjectImpl (SLMHostObjectImpl.cpp)
- **IDispatch COM object** for JavaScript interop
- Exposes methods to JavaScript:
  - `QueryStatus()` - Check model download/load status
  - `SetupAsync(progressCallback, completionCallback)` - Download/load model
  - `InferAsync(prompt, streamCallback, completionCallback)` - Run inference
  - `StopInference()` - Cancel ongoing inference
- Uses type library for IDispatch implementation (required for WebView2)

#### 4. ONNXRuntimeClient (ONNXRuntimeClient.cpp)
- Manages ONNX Runtime GenAI lifecycle
- Handles model downloading from Hugging Face
- Creates and manages `OgaModel`, `OgaTokenizer`, `OgaGenerator`
- Implements streaming token generation

#### 5. ScenarioSLM.html
- Modern Copilot-style chat interface
- Markdown rendering with syntax highlighting
- Responsive design with animations
- Progress indicators for download/loading states

---

## Technology Decisions

### Decision 1: ONNX Runtime GenAI vs Other Runtimes

**Options Considered:**
- ONNX Runtime GenAI
- llama.cpp
- Foundry Local (Microsoft)
- DirectML directly

**Decision: ONNX Runtime GenAI**

**Rationale:**
- Native C++ API with clean abstractions
- Built-in support for Hugging Face model format
- DirectML acceleration on Windows
- CPU fallback when GPU unavailable
- Official Microsoft support and maintenance

### Decision 2: Phi-3 Mini vs Other Models

**Options Considered:**
- Phi-3 Mini 4K Instruct (~2.7GB)
- Llama 3.2 1B/3B
- Mistral 7B
- Gemma 2B

**Decision: Phi-3 Mini 4K Instruct**

**Rationale:**
- Publicly accessible on Hugging Face (no authentication required)
- Excellent quality-to-size ratio
- Optimized ONNX versions available (INT4 quantized)
- Microsoft model with good documentation
- 4K context window sufficient for chat scenarios

### Decision 3: CPU vs GPU Model Variant

**Options Considered:**
- DirectML (GPU) variant
- CPU INT4 variant

**Decision: CPU INT4 variant (cpu-int4-rtn-block-32-acc-level-4)**

**Rationale:**
- Works in all environments including VMs without GPU passthrough
- DirectML requires compatible GPU and drivers
- Hyper-V VMs typically don't expose GPU to guests
- CPU performance acceptable for demo purposes (~2-5 tokens/sec)
- Broader compatibility prioritized over raw speed

### Decision 4: IDispatch vs Custom Interface for JavaScript Interop

**Options Considered:**
- IDispatch with type library
- Custom interface with manual binding
- WinRT projection

**Decision: IDispatch with Type Library**

**Rationale:**
- WebView2's `AddHostObjectToScript` requires IDispatch
- Type library provides automatic parameter marshaling
- JavaScript can call methods naturally without manual serialization
- Callbacks work through IDispatch function invocation

### Decision 5: Model Storage Location

**Options Considered:**
- Application directory
- %LOCALAPPDATA%
- %APPDATA%
- User-specified location

**Decision: %LOCALAPPDATA%\WebView2APISampleONNX\models\**

**Rationale:**
- Standard Windows location for app-specific cached data
- No admin rights required
- Survives app reinstalls
- User-specific (multi-user support)
- Can be cleared without affecting app functionality

### Decision 6: Async Pattern for Long Operations

**Options Considered:**
- Blocking calls
- std::async with futures
- Callback-based async
- C++20 coroutines

**Decision: Callback-based async with background threads**

**Rationale:**
- Compatible with IDispatch JavaScript interop
- JavaScript callbacks work naturally
- UI remains responsive during model operations
- Simple threading model with `std::thread`

---

## Trade-offs

### 1. CPU Model vs GPU Model

| Aspect | CPU Model | GPU Model |
|--------|-----------|-----------|
| **Compatibility** | ✅ Works everywhere | ❌ Requires compatible GPU |
| **Performance** | ❌ Slower (2-5 tok/s) | ✅ Faster (20-50 tok/s) |
| **Memory** | Uses system RAM | Uses VRAM |
| **VM Support** | ✅ Full support | ❌ Requires GPU passthrough |

**Trade-off accepted:** Chose CPU for maximum compatibility at the cost of inference speed.

### 2. Pre-downloaded Model vs Runtime Download

| Aspect | Pre-downloaded | Runtime Download |
|--------|----------------|------------------|
| **App Size** | ❌ +2.7GB | ✅ Small installer |
| **First Run** | ✅ Instant | ❌ Long wait |
| **Updates** | ❌ Requires reinstall | ✅ Can update model independently |

**Trade-off accepted:** Runtime download keeps app small, accepts first-run delay.

### 3. Streaming vs Complete Response

| Aspect | Streaming | Complete |
|--------|-----------|----------|
| **User Experience** | ✅ Immediate feedback | ❌ Long wait |
| **Complexity** | ❌ More complex | ✅ Simpler |
| **Cancellation** | ✅ Can stop early | ❌ Must wait |

**Trade-off accepted:** Streaming improves UX, worth the added complexity.

### 4. Type Library vs Manual IDispatch

| Aspect | Type Library | Manual Implementation |
|--------|--------------|----------------------|
| **Setup Complexity** | ❌ IDL compilation required | ✅ No extra build steps |
| **Method Invocation** | ✅ Automatic | ❌ Manual DISPID handling |
| **Parameter Marshaling** | ✅ Automatic | ❌ Manual VARIANT handling |
| **Maintenance** | ✅ Add to IDL, done | ❌ Update multiple places |

**Trade-off accepted:** Type library simplifies development despite build complexity.

---

## File Structure

```
WebView2APISampleONNX/
├── assets/
│   └── ScenarioSLM.html      # Chat UI (Copilot-style)
├── Release/x64/
│   ├── WebView2APISampleONNX.exe
│   ├── onnxruntime.dll       # ONNX Runtime core
│   └── onnxruntime-genai.dll # ONNX Runtime GenAI
├── App.cpp                   # Application entry point
├── AppWindow.cpp             # Main window management
├── AppWindow.h
├── ONNXRuntimeClient.cpp     # ONNX Runtime wrapper
├── ONNXRuntimeClient.h
├── ScenarioSLM.cpp           # SLM scenario setup
├── ScenarioSLM.h
├── SLMHostObject.idl         # COM interface definition
├── SLMHostObjectImpl.cpp     # IDispatch implementation
├── SLMHostObjectImpl.h
├── packages.config           # NuGet packages
├── WebView2APISampleONNX.vcxproj
└── ARCHITECTURE.md           # This file
```

---

## Model Files

After download, the model is stored at:
```
%LOCALAPPDATA%\WebView2APISampleONNX\models\phi-3-mini\
├── phi3-mini-4k-instruct-cpu-int4-rtn-block-32-acc-level-4.onnx
├── genai_config.json
├── tokenizer.json
├── tokenizer_config.json
├── special_tokens_map.json
└── added_tokens.json
```

---

## NuGet Dependencies

| Package | Version | Purpose |
|---------|---------|---------|
| Microsoft.Web.WebView2 | 1.0.2903.40 | WebView2 browser control |
| Microsoft.Windows.ImplementationLibrary | 1.0.240803.1 | WIL helpers |
| Microsoft.ML.OnnxRuntimeGenAI.DirectML | 0.5.2 | GenAI high-level API |
| Microsoft.ML.OnnxRuntime.DirectML | 1.20.1 | ONNX Runtime core |

---

## User Flow

```
┌─────────────┐     ┌──────────────────┐     ┌─────────────────┐
│  App Start  │────►│  Check Model     │────►│ Model Exists?   │
└─────────────┘     └──────────────────┘     └────────┬────────┘
                                                      │
                    ┌─────────────────────────────────┼─────────────────────┐
                    │ No                              │ Yes                 │
                    ▼                                 ▼                     │
        ┌───────────────────────┐         ┌──────────────────────┐         │
        │ Show Download Button  │         │ Auto-load Model      │         │
        │ "Download Model"      │         │ "Loading model..."   │         │
        └───────────┬───────────┘         └──────────┬───────────┘         │
                    │                                │                     │
                    │ Click                          │                     │
                    ▼                                ▼                     │
        ┌───────────────────────┐         ┌──────────────────────┐         │
        │ Download with         │         │ Model Ready          │         │
        │ Progress Bar          │         │ "Start Chatting"     │         │
        └───────────┬───────────┘         └──────────┬───────────┘         │
                    │                                │                     │
                    │ Complete                       │ Click               │
                    ▼                                ▼                     │
        ┌───────────────────────┐         ┌──────────────────────┐         │
        │ Load Model            │────────►│ Chat Screen          │◄────────┘
        │ "Start Chatting"      │         │ Ready for input      │
        └───────────────────────┘         └──────────────────────┘
```

---

## Security Considerations

1. **Model Source**: Downloads only from official Hugging Face Microsoft repository
2. **Local Execution**: All inference runs locally, no data sent to cloud
3. **No Credentials**: Uses public model that doesn't require authentication
4. **File Validation**: Model files verified during ONNX Runtime loading

---

## Future Enhancements

1. **GPU Detection**: Auto-detect GPU capability and use DirectML when available
2. **Model Selection**: Allow users to choose from multiple model options
3. **Context Management**: Implement conversation history with proper context windowing
4. **Export/Import**: Save and load chat histories
5. **System Prompts**: Allow customization of AI persona
6. **Token Counting**: Display token usage and context window remaining

---

## Building the Project

### Prerequisites
- Visual Studio 2022 with C++ workload
- Windows SDK 10.0.26100.0 or later
- NuGet package restore

### Build Steps
```powershell
cd Q:\WebView2Samples\SampleApps\WebView2APISampleONNX
nuget restore
msbuild /t:Build /p:Configuration=Release /p:Platform=x64
```

### Running
```powershell
.\Release\x64\WebView2APISampleONNX.exe
```

Navigate to **Scenario** → **SLM Chat** to access the chat feature.

---

## Troubleshooting

| Issue | Cause | Solution |
|-------|-------|----------|
| "No adapter available for DML" | No GPU or running in VM | Using CPU model variant (already configured) |
| Model download fails | Network issues | Check internet connection, retry |
| App crashes on 2nd prompt | (Fixed) Dangling reference | Update to latest code |
| Slow inference | Using CPU model | Expected behavior (~2-5 tokens/sec) |

---

*Document Version: 1.0*  
*Last Updated: February 2026*
