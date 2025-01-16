#include "stdafx.h"

#include "AppWindow.h"
#include "CheckFailure.h"
#include "ScenarioDefaultBackgroundColor.h"
#include <WebView2.h>
#include <d2d1helper.h>

using namespace Microsoft::WRL;
using namespace std;

static constexpr wchar_t c_samplePath[] = L"ScenarioDefaultBackgroundColor.html";

ScenarioDefaultBackgroundColor::ScenarioDefaultBackgroundColor(AppWindow* appWindow)
{
    std::wstring sampleUri = appWindow->GetLocalUri(c_samplePath);
    WebViewCreateOption options = appWindow->GetWebViewOption();

    options.bg_color = {255, 225, 0, 225};
    AppWindow* appWindowWco = new AppWindow(appWindow->GetCreationModeId(), options, sampleUri);
    // Delete this component when the window closes.
    appWindowWco->SetOnAppWindowClosing([this, appWindow]
                                        { appWindow->DeleteComponent(this); });
}

ScenarioDefaultBackgroundColor::~ScenarioDefaultBackgroundColor()
{
}