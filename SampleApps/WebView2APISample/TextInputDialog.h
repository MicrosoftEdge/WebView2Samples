// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include <map>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

// Per-type structs for dynamically created dialog controls.

// Default heights (in dialog units) for dynamic controls.
inline constexpr int kLabelHeight = 100;
inline constexpr int kTextAreaHeight = 120;

// A single checkbox option within a group. Holds a display label, an integer
// value returned to the caller when selected, and a flag indicating whether
// the checkbox should be pre-checked when the dialog is shown.
struct CheckBoxOption
{
    // The text displayed next to the checkbox.
    std::wstring label;
    // Application-defined integer identifying the feature this checkbox represents.
    int value;
    // Pre-checked on show if true; reflects user's final check state after OK.
    bool isSelected;

    CheckBoxOption(const std::wstring& label, int value, bool isSelected = false)
        : label(label), value(value), isSelected(isSelected)
    {
    }
};

// A group of related checkboxes rendered together under an optional label.
// Use this to present a set of feature toggles that the user can independently check or
// uncheck. Each option gets its own Win32 BUTTON control with a unique control ID.
struct CheckBoxGroup
{
    // Heading text above the checkboxes; if empty, no label is rendered.
    std::wstring groupLabel;
    // Checkbox options rendered top-to-bottom in vector order.
    std::vector<CheckBoxOption> options;

    CheckBoxGroup(const std::wstring& groupLabel, std::vector<CheckBoxOption> options)
        : groupLabel(groupLabel), options(std::move(options))
    {
    }
};

// A multi-line text input area with a descriptive label above it.
// The label is rendered as a read-only EDIT (grey background, bordered) and
// the input area below it is a writable EDIT where the user can type.
// Set |readOnly| to true to make the input area non-editable (useful for
// displaying results). After the dialog is dismissed, |input| holds the
// text entered by the user.
struct TextArea
{
    // Descriptive text shown in a read-only EDIT above the input area.
    std::wstring label;
    // If true, the input area is non-editable (display-only).
    bool readOnly;
    // Pre-filled with default text; holds the user-entered text after OK.
    std::wstring input;
    // Height in pixels for the label and input areas.
    int labelHeight;
    int inputHeight;

    TextArea(
        const std::wstring& label, const std::wstring& input = L"", bool readOnly = false,
        int labelHeight = kLabelHeight, int inputHeight = kTextAreaHeight)
        : label(label), readOnly(readOnly), input(input), labelHeight(labelHeight),
          inputHeight(inputHeight)
    {
    }
};

// A dropdown (combo box) control with a label above it.
// After the dialog is dismissed, |selectedIndex| holds the index of the
// user's selection.
struct DropDown
{
    // Descriptive text shown above the combo box.
    std::wstring label;
    // The list of options to display in the dropdown.
    std::vector<std::wstring> options;
    // The index of the initially selected option. Updated to the user's
    // selection after the dialog is dismissed.
    int selectedIndex;

    DropDown(
        const std::wstring& label, std::vector<std::wstring> options, int selectedIndex = 0)
        : label(label), options(std::move(options)), selectedIndex(selectedIndex)
    {
    }
};

// Ordered collection of dynamic controls. Add new types to this variant.
using DialogControl = std::variant<CheckBoxGroup, TextArea, DropDown>;

// Constructing this struct will show a text input dialog and return when the user
// dismisses it.  If the user clicked the OK button, confirmed will be true and input will
// be set to the input they entered.
// Use the Builder class for extended configuration (e.g., adding checkboxes, text areas).
struct TextInputDialog
{
    // Builder for constructing a TextInputDialog with dynamic controls.
    // When Builder is used, the built-in description and input controls are
    // hidden; use AddTextArea and AddCheckBoxGroup to compose the dialog.
    class Builder
    {
    public:
        Builder(HWND parent, PCWSTR title, PCWSTR prompt);
        Builder& AddCheckBoxGroup(
            const std::wstring& groupLabel, std::vector<CheckBoxOption> options);
        Builder& AddTextArea(
            const std::wstring& label, const std::wstring& input = L"", bool readOnly = false,
            int labelHeight = kLabelHeight, int inputHeight = kTextAreaHeight);
        Builder& AddDropDown(
            const std::wstring& label, std::vector<std::wstring> options,
            int selectedIndex = 0);
        TextInputDialog Build();

    private:
        HWND m_parent;
        PCWSTR m_title;
        PCWSTR m_prompt;
        std::vector<DialogControl> m_controls;
    };

    TextInputDialog(
        HWND parent,
        PCWSTR title,
        PCWSTR prompt,
        PCWSTR description,
        const std::wstring& defaultInput = L"",
        bool readOnly = false);

    // The title displayed in the dialog's title bar.
    PCWSTR title;
    // The prompt shown as the group box title in the dialog.
    PCWSTR prompt;
    // TODO(task.ms/61534504): Use Builder method for creating TextInputDialog in win32 sample
    // app
    // Additional descriptive text shown in the dialog.
    PCWSTR description;
    // Whether the text input field is read-only.
    bool readOnly;

    // True if the user clicked OK to dismiss the dialog.
    bool confirmed;
    // The text entered by the user in the input field.
    std::wstring input;

    // Extended fields populated via Builder.
    // The dynamic controls (checkboxes, text areas) added via Builder.
    std::vector<DialogControl> controls;
    // Unified result map populated on OK, keyed by control label (group label
    // for checkbox groups, text area label for text areas). Each value is the
    // DialogControl variant with its state updated:
    //   - CheckBoxGroup: each option's isSelected reflects the user's
    //     choice. Iterate options to find checked values.
    //   - TextArea: |input| holds the user-entered text.
    // Use std::get<CheckBoxGroup> or std::get<TextArea> to access.
    std::map<std::wstring, DialogControl> results;

private:
    TextInputDialog(
        HWND parent, PCWSTR title, PCWSTR prompt, PCWSTR description,
        const std::wstring& defaultInput, bool readOnly, std::vector<DialogControl> controls);

    // Creates all dynamic controls and resizes the dialog to fit them.
    void CreateDynamicControls(HWND hDlg);
    // Collects user-entered values from all dynamic controls.
    void CollectControlValues(HWND hDlg);
    // Handles a BN_CLICKED notification for a dynamic checkbox control.
    void HandleCheckBoxClick(HWND hDlg, int controlId);

    friend INT_PTR CALLBACK DlgProcStatic(HWND, UINT, WPARAM, LPARAM);
};
