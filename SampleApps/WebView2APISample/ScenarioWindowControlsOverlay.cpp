#include "stdafx.h"

#include "AppWindow.h"
#include "CheckFailure.h"
#include "ScenarioWindowControlsOverlay.h"
#include <WebView2.h>

using namespace Microsoft::WRL;
using namespace std;

static constexpr wchar_t c_samplePath[] = L"ScenarioWindowControlsOverlay.html";

ScenarioWindowControlsOverlay::ScenarioWindowControlsOverlay(AppWindow* appWindow)
{
    //! [WindowControlsOverlay]
    std::wstring sampleUri = appWindow->GetLocalUri(c_samplePath);
    WebViewCreateOption options = appWindow->GetWebViewOption();
    options.useWco = true;
    AppWindow* appWindowWco = new AppWindow(appWindow->GetCreationModeId(), options, sampleUri);
    //! [WindowControlsOverlay]
}

ScenarioWindowControlsOverlay::~ScenarioWindowControlsOverlay()
{
}
