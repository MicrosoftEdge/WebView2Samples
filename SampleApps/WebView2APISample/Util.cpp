// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "CheckFailure.h"
#include "Util.h"

std::wstring Util::UnixEpochToDateTime(double value)
{
    WCHAR rawResult[32] = {};
    std::time_t rawTime = std::time_t(value / 1000);
    struct tm timeStruct = {};
    gmtime_s(&timeStruct, &rawTime);
    _wasctime_s(rawResult, 32, &timeStruct);
    std::wstring result(rawResult);
    return result;
}

std::wstring Util::TrimWhitespace(const std::wstring& text)
{
    if (text.empty())
    {
        return text;
    }

    std::wstring trimmedText = text;
    trimmedText.erase(0, trimmedText.find_first_not_of(L" \t\r\n"));
    trimmedText.erase(trimmedText.find_last_not_of(L" \t\r\n") + 1);

    return trimmedText;
}

std::vector<std::wstring> Util::SplitString(const std::wstring& input, wchar_t delimiter)
{
    std::vector<std::wstring> result;

    if (input.empty())
    {
        return result;
    }

    std::wstring::size_type start = 0;
    std::wstring::size_type end = 0;

    end = input.find(delimiter, start);
    while (end != std::wstring::npos)
    {
        std::wstring token = input.substr(start, end - start);
        token = TrimWhitespace(token);

        if (!token.empty())
        {
            result.push_back(token);
        }

        start = end + 1;
        end = input.find(delimiter, start);
    }

    // Handle the last token after the final delimiter (or the only token if there's no
    // delimiter)
    if (start < input.length())
    {
        std::wstring token = input.substr(start);
        token = TrimWhitespace(token);

        if (!token.empty())
        {
            result.push_back(token);
        }
    }

    return result;
}