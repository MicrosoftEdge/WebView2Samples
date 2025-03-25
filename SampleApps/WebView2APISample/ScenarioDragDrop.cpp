// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "stdafx.h"

#include "ScenarioDragDrop.h"

#include "AppWindow.h"
#include "CheckFailure.h"

#include <string>

using namespace Microsoft::WRL;

static constexpr WCHAR c_samplePath[] = L"ScenarioDragDrop.html";

ScenarioDragDrop::ScenarioDragDrop(AppWindow* appWindow) : m_appWindow(appWindow)
{
    m_webView = m_appWindow->GetWebView();

    //! [DroppedFilePath]
    CHECK_FAILURE(m_webView->add_WebMessageReceived(
        Callback<ICoreWebView2WebMessageReceivedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args)
            {
                wil::com_ptr<ICoreWebView2WebMessageReceivedEventArgs2> args2 =
                    wil::com_ptr<ICoreWebView2WebMessageReceivedEventArgs>(args)
                        .query<ICoreWebView2WebMessageReceivedEventArgs2>();
                wil::com_ptr<ICoreWebView2ObjectCollectionView> objectsCollection;
                args2->get_AdditionalObjects(&objectsCollection);
                unsigned int length;
                objectsCollection->get_Count(&length);

                // Array of file paths to be sent back to the webview as JSON
                std::wstring pathObjects = L"[";
                for (unsigned int i = 0; i < length; i++)
                {
                    wil::com_ptr<IUnknown> object;
                    objectsCollection->GetValueAtIndex(i, &object);

                    wil::com_ptr<ICoreWebView2File> file = object.query<ICoreWebView2File>();
                    if (file)
                    {
                        // Add the file to message to be sent back to webview
                        wil::unique_cotaskmem_string path;
                        file->get_Path(&path);
                        std::wstring pathObject =
                            L"{\"path\":\"" + std::wstring(path.get()) + L"\"}";
                        // Escape backslashes
                        std::wstring pathObjectEscaped;
                        for (const auto& c : pathObject)
                        {
                            if (c == L'\\')
                            {
                                pathObjectEscaped += L"\\\\";
                            }
                            else
                            {
                                pathObjectEscaped += c;
                            }
                        }
                        pathObjects += pathObjectEscaped;

                        if (i < length - 1)
                        {
                            pathObjects += L",";
                        }
                    }
                }
                pathObjects += L"]";

                // Post the message back to the webview so path is accessible to content
                m_webView->PostWebMessageAsJson(pathObjects.c_str());

                return S_OK;
            })
            .Get(),
        &m_webMessageReceivedToken));
    //! [DroppedFilePath]

    CHECK_FAILURE(m_webView->Navigate(m_appWindow->GetLocalUri(c_samplePath).c_str()));
}

ScenarioDragDrop::~ScenarioDragDrop()
{
    CHECK_FAILURE(m_webView->remove_WebMessageReceived(m_webMessageReceivedToken));
}