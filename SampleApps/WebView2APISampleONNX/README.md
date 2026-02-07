# WebView2APISampleONNX - On-Device SLM with ONNX Runtime

A WebView2 Win32 sample application demonstrating on-device Small Language Model (SLM) chat using **ONNX Runtime GenAI** for direct model inference.

## Overview

This sample shows how to integrate ONNX Runtime GenAI directly into a WebView2 application to run local AI inference without requiring external services like Azure Foundry Local. The model runs entirely on your device using DirectML for GPU acceleration.

### Key Features

- **100% Local Inference**: No cloud services required after model download
- **ONNX Runtime GenAI**: Direct integration with Microsoft's ONNX Runtime generative AI library
- **DirectML GPU Acceleration**: Hardware-accelerated inference on Windows
- **Model: Llama 3.2 1B**: Compact, efficient model optimized for ONNX
- **Streaming Responses**: Token-by-token response streaming for responsive UI
- **Auto Model Download**: Downloads model from HuggingFace on first run (~1.5GB)

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    WebView2APISampleONNX                    │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐    ┌──────────────────┐    ┌────────────┐ │
│  │   WebView2  │◄──►│ SLMHostObjectImpl│◄──►│ONNXRuntime │ │
│  │  (HTML/JS)  │    │   (COM Bridge)   │    │   Client   │ │
│  └─────────────┘    └──────────────────┘    └────────────┘ │
│                                                     │       │
│                                              ┌──────▼─────┐ │
│                                              │ONNX Runtime│ │
│                                              │  GenAI API │ │
│                                              └──────┬─────┘ │
│                                                     │       │
│                                              ┌──────▼─────┐ │
│                                              │  DirectML  │ │
│                                              │    GPU     │ │
│                                              └────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

## Model Information

| Model | Size | Quantization | Source |
|-------|------|--------------|--------|
| Llama 3.2 1B Instruct | ~1.5 GB | INT4 | [HuggingFace](https://huggingface.co/microsoft/Llama-3.2-1B-Instruct-onnx) |

## Prerequisites

1. **Windows 10/11** (x64 or ARM64)
2. **Visual Studio 2022** with C++ Desktop Development workload
3. **WebView2 Runtime** (usually pre-installed on Windows 10/11)
4. **DirectX 12 compatible GPU** (optional, but recommended)
5. **~2GB disk space** for model storage

## Building

### 1. Restore NuGet Packages

Open the solution in Visual Studio and restore NuGet packages:

```
Microsoft.Web.WebView2 (1.0.3796-prerelease)
Microsoft.Windows.ImplementationLibrary (1.0.220201.1)
Microsoft.ML.OnnxRuntimeGenAI.DirectML (0.5.2)
```

### 2. Build the Solution

- Select **Debug|x64** or **Release|x64** configuration
- Build → Build Solution (Ctrl+Shift+B)

### 3. Run

Press F5 to run. On first launch:
1. Click "Download Model (~1.5 GB)" 
2. Wait for model download from HuggingFace
3. Start chatting!

## Project Structure

```
WebView2APISampleONNX/
├── App.cpp/h                 # Application entry point
├── AppWindow.cpp/h           # Main window and WebView2 setup
├── ONNXRuntimeClient.cpp/h   # ONNX Runtime GenAI integration
├── SLMHostObjectImpl.cpp/h   # COM host object for JS interop
├── SLMHostObject.idl         # COM interface definition
├── ScenarioSLM.cpp/h         # Scenario that sets up SLM chat
├── assets/
│   └── ScenarioSLM.html      # Chat UI (Copilot-style)
├── packages.config           # NuGet packages
└── WebView2APISampleONNX.vcxproj
```

## Key Components

### ONNXRuntimeClient

Core class that wraps ONNX Runtime GenAI:

```cpp
class ONNXRuntimeClient {
    // Model management
    bool LoadModel(const std::wstring& alias);
    bool IsModelDownloaded(const std::wstring& alias);
    void DownloadModelAsync(...);
    
    // Inference
    void ChatCompletionAsync(
        const std::vector<ChatMessage>& messages,
        TokenCallback tokenCallback,
        CompletionCallback completionCallback);
};
```

### SLMHostObject

COM object exposed to JavaScript for host object bridging:

```javascript
// In ScenarioSLM.html
const slm = chrome.webview.hostObjects.slm;
await slm.QueryStatus();
await slm.SetupAsync(progressCallback, completionCallback);
await slm.InferAsync(prompt, tokenCallback, completionCallback);
```

## Model Storage

Models are stored in:
```
%LOCALAPPDATA%\WebView2APISampleONNX\models\llama-3.2-1b\
```

Required files:
- `model.onnx` - Main ONNX model
- `model.onnx.data` - Model weights
- `genai_config.json` - ONNX GenAI configuration
- `tokenizer.json` - Tokenizer vocabulary
- `tokenizer_config.json` - Tokenizer settings

## Comparison with Foundry Local Version

| Feature | WebView2APISample (Foundry) | WebView2APISampleONNX |
|---------|-----------------------------|-----------------------|
| Runtime | Azure Foundry Local | ONNX Runtime GenAI |
| API | REST API (HTTP) | Direct C++ API |
| Model Source | Foundry Catalog | HuggingFace |
| Dependencies | Foundry Local installed | NuGet packages only |
| Setup | Install Foundry + model | Auto-download model |
| Cross-platform | Windows only | Windows (extensible to macOS/Linux) |

## Extending

### Adding More Models

1. Add model info to `InitializeModelCatalog()` in `ONNXRuntimeClient.cpp`
2. Add download URLs to `GetModelFiles()` 
3. Update `FormatChatPrompt()` for the model's chat template

### Cross-Platform Support

ONNX Runtime GenAI supports multiple execution providers:
- **Windows**: DirectML (GPU), CPU
- **macOS**: CoreML, CPU  
- **Linux**: CUDA, CPU

To support macOS, create an Xcode project with CoreML execution provider.

## Troubleshooting

### Model fails to load
- Ensure all model files are present in the model directory
- Check `genai_config.json` matches the ONNX model version
- Verify you have a DirectX 12 compatible GPU

### Slow inference
- INT4 quantized model requires GPU for acceptable speed
- CPU-only inference may be slow (use smaller models)

### Download fails
- Check internet connectivity
- HuggingFace may rate-limit downloads
- Try manual download and copy to model directory

## License

This sample is provided under the Microsoft sample license. See the repository LICENSE file.

## Related Links

- [ONNX Runtime GenAI](https://github.com/microsoft/onnxruntime-genai)
- [Llama 3.2 ONNX Models](https://huggingface.co/microsoft/Llama-3.2-1B-Instruct-onnx)
- [WebView2 Documentation](https://docs.microsoft.com/microsoft-edge/webview2/)
- [DirectML](https://docs.microsoft.com/windows/ai/directml/dml)
