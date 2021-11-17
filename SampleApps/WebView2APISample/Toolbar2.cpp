// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "Toolbar.h"

#include "AppWindow.h"
#include "resource.h"

void Toolbar::UpdateFont()
{
    static const WCHAR s_fontName[] = L"WebView2APISample Font";
    if (m_font)
    {
        DeleteObject(m_font);
    }
    LOGFONT logFont;
    GetObject(GetStockObject(SYSTEM_FONT), sizeof(LOGFONT), &logFont);
    double dpiScale = m_appWindow->GetDpiScale();
    double textScale = m_appWindow->GetTextScale();
    logFont.lfHeight *= LONG(dpiScale * textScale);
    logFont.lfWidth *= LONG(dpiScale * textScale);
    StringCchCopy(logFont.lfFaceName, ARRAYSIZE(logFont.lfFaceName), s_fontName);
    m_font = CreateFontIndirect(&logFont);
}
