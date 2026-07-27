// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "TextInputDialog.h"

#include <cassert>

#include "App.h"
#include "resource.h"

// Maximum number of options per control group. Used to space control IDs so
// each group gets a non-overlapping range (e.g., group 0 = 5000-5009,
// group 1 = 5010-5019).
static constexpr int kMaxOptionsPerGroup = 10;

// Heights for dynamic controls.
static constexpr int kCheckBoxHeight = 20;
static constexpr int kCheckBoxSpacing = 4;
static constexpr int kGroupLabelHeight = 18;
static constexpr int kControlSpacing = 16;
static constexpr int kLabelToInputSpacing = 6;

namespace
{

// Returns the position of a child control in dialog-client coordinates.
RECT GetControlRect(HWND hDlg, HWND control)
{
    RECT rect;
    GetWindowRect(control, &rect);
    MapWindowPoints(NULL, hDlg, reinterpret_cast<POINT*>(&rect), 2);
    return rect;
}

// Shifts a button control by the given vertical offset.
void ShiftButton(HWND hDlg, int buttonId, int yOffset)
{
    HWND button = GetDlgItem(hDlg, buttonId);
    RECT rect = GetControlRect(hDlg, button);
    SetWindowPos(button, NULL, rect.left, rect.top + yOffset, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

// Applies the dialog font to a child control, if a font handle is available.
void SetControlFont(HWND ctrl, HFONT dialogFont)
{
    if (dialogFont)
    {
        SendMessage(ctrl, WM_SETFONT, reinterpret_cast<WPARAM>(dialogFont), TRUE);
    }
}

// Creates a single BS_AUTOCHECKBOX BUTTON as a child of |hDlg|, assigns it
// the given |hmenu| ID, applies the dialog font, and pre-checks it if
// |isSelected| is true. Returns the checkbox HWND.
HWND CreateCheckBox(
    HWND hDlg, PCWSTR label, HMENU hmenu, int x, int y, int width, HFONT dialogFont,
    bool isSelected)
{
    HWND checkBox = CreateWindow(
        L"BUTTON", label, WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, x, y, width, kCheckBoxHeight,
        hDlg, hmenu, GetModuleHandle(NULL), NULL);
    SetControlFont(checkBox, dialogFont);
    if (isSelected)
    {
        SendMessage(checkBox, BM_SETCHECK, BST_CHECKED, 0);
    }
    return checkBox;
}

// Creates a checkbox group: an optional STATIC label followed by one
// BS_AUTOCHECKBOX BUTTON per option. Each checkbox gets a unique control ID
// starting at |controlId| (incremented by 1 per option) so BN_CLICKED
// notifications can be mapped back to the correct option. Pre-checks any
// option whose isSelected is true. Returns total pixel height consumed.
int CreateCheckBoxGroup(
    HWND hDlg, CheckBoxGroup& group, int controlId, int x, int y, int width, HFONT dialogFont)
{
    int totalHeight = 0;

    // Optional group label rendered as a STATIC text control.
    if (!group.groupLabel.empty())
    {
        // Create a plain text label above the checkboxes to describe the group.
        HWND label = CreateWindow(
            L"STATIC", group.groupLabel.c_str(), WS_VISIBLE | WS_CHILD, x, y, width,
            kGroupLabelHeight, hDlg, NULL, GetModuleHandle(NULL), NULL);
        SetControlFont(label, dialogFont);
        totalHeight += kGroupLabelHeight + kCheckBoxSpacing;
    }

    // Assert that the group fits within the kMaxOptionsPerGroup ID range.
    assert(group.options.size() <= static_cast<size_t>(kMaxOptionsPerGroup));
    for (size_t i = 0; i < group.options.size(); ++i)
    {
        HMENU hmenu =
            reinterpret_cast<HMENU>(static_cast<UINT_PTR>(controlId + static_cast<int>(i)));
        CreateCheckBox(
            hDlg, group.options[i].label.c_str(), hmenu, x, y + totalHeight, width, dialogFont,
            group.options[i].isSelected);
        totalHeight += kCheckBoxHeight + kCheckBoxSpacing;
    }
    return totalHeight > 0 ? totalHeight - kCheckBoxSpacing : 0;
}

// Creates a text area control: a read-only EDIT as a descriptive label (grey
// background, bordered, scrollable) followed by a writable EDIT below it for
// user input. The writable EDIT is assigned |controlId| so its text can be
// retrieved later via GetDlgItem. If |readOnly| is set, the input EDIT is
// made non-editable. Returns total pixel height consumed.
int CreateTextArea(
    HWND hDlg, TextArea& textArea, int controlId, int x, int y, int width, HFONT dialogFont)
{
    HMENU hmenu = reinterpret_cast<HMENU>(static_cast<UINT_PTR>(controlId));
    int labelH = textArea.labelHeight;
    int inputH = textArea.inputHeight;

    // Read-only label EDIT — renders with a border and grey background.
    HWND label = CreateWindowEx(
        WS_EX_CLIENTEDGE, L"EDIT", textArea.label.c_str(),
        WS_VISIBLE | WS_CHILD | ES_READONLY | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL, x, y,
        width, labelH, hDlg, NULL, GetModuleHandle(NULL), NULL);
    SetControlFont(label, dialogFont);

    // Writable input EDIT — placed below the label for user text entry.
    HWND inputArea = CreateWindowEx(
        WS_EX_CLIENTEDGE, L"EDIT", textArea.input.c_str(),
        WS_VISIBLE | WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL, x,
        y + labelH + kLabelToInputSpacing, width, inputH, hDlg, hmenu, GetModuleHandle(NULL),
        NULL);
    SetControlFont(inputArea, dialogFont);
    if (textArea.readOnly)
    {
        SendMessage(inputArea, EM_SETREADONLY, TRUE, 0);
    }
    return labelH + kLabelToInputSpacing + inputH;
}

// Creates a dropdown (combo box) control: a STATIC label above a COMBOBOX.
// The combo box is assigned |controlId| so its selection can be retrieved
// later via GetDlgItem. Returns total pixel height consumed.
static constexpr int kDropDownLabelHeight = 16;
static constexpr int kDropDownHeight = 200; // drop-down list height (expanded)
static constexpr int kDropDownControlHeight = 22;

int CreateDropDown(
    HWND hDlg, DropDown& dropDown, int controlId, int x, int y, int width, HFONT dialogFont)
{
    HMENU hmenu = reinterpret_cast<HMENU>(static_cast<UINT_PTR>(controlId));

    // Static label above the combo box.
    HWND label = CreateWindowEx(
        0, L"STATIC", dropDown.label.c_str(), WS_VISIBLE | WS_CHILD, x, y, width,
        kDropDownLabelHeight, hDlg, NULL, GetModuleHandle(NULL), NULL);
    SetControlFont(label, dialogFont);

    // Combo box — CBS_DROPDOWNLIST prevents typing, user must pick from list.
    HWND comboBox = CreateWindowEx(
        0, L"COMBOBOX", NULL, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, x,
        y + kDropDownLabelHeight + kLabelToInputSpacing, width, kDropDownHeight, hDlg, hmenu,
        GetModuleHandle(NULL), NULL);
    SetControlFont(comboBox, dialogFont);

    for (const auto& option : dropDown.options)
    {
        SendMessage(comboBox, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(option.c_str()));
    }
    SendMessage(comboBox, CB_SETCURSEL, dropDown.selectedIndex, 0);

    return kDropDownLabelHeight + kLabelToInputSpacing + kDropDownControlHeight;
}

// Dispatches to the appropriate creation helper based on the DialogControl
// variant type. Returns the pixel height consumed by the created control.
int CreateDynamicControl(
    HWND hDlg, DialogControl& control, int controlId, int x, int y, int width, HFONT dialogFont)
{
    return std::visit(
        [&](auto& ctrl) -> int
        {
            using T = std::decay_t<decltype(ctrl)>;
            if constexpr (std::is_same_v<T, CheckBoxGroup>)
            {
                return CreateCheckBoxGroup(hDlg, ctrl, controlId, x, y, width, dialogFont);
            }
            else if constexpr (std::is_same_v<T, TextArea>)
            {
                return CreateTextArea(hDlg, ctrl, controlId, x, y, width, dialogFont);
            }
            else if constexpr (std::is_same_v<T, DropDown>)
            {
                return CreateDropDown(hDlg, ctrl, controlId, x, y, width, dialogFont);
            }
        },
        control);
}

// Reads the check state of each checkbox in a group from its Win32 HWND and
// updates the corresponding option's isSelected flag.
void CollectCheckBoxGroupValues(HWND hDlg, CheckBoxGroup& group, int controlId)
{
    for (size_t j = 0; j < group.options.size(); ++j)
    {
        HWND ctrl = GetDlgItem(hDlg, controlId + static_cast<int>(j));
        if (!ctrl)
            continue;
        group.options[j].isSelected = (SendMessage(ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED);
    }
}

// Reads the user-entered text from a text area's writable EDIT control
// and stores it in the TextArea's |input| field.
void CollectTextAreaValue(HWND hDlg, TextArea& textArea, int controlId)
{
    HWND ctrl = GetDlgItem(hDlg, controlId);
    if (!ctrl)
        return;
    int len = GetWindowTextLength(ctrl);
    textArea.input.resize(len);
    GetWindowText(ctrl, textArea.input.data(), len + 1);
}

// Reads the selected index from a combo box and updates the DropDown.
// Leaves the existing |selectedIndex| unchanged if the combo box has no
// current selection (CB_GETCURSEL returns CB_ERR).
void CollectDropDownValue(HWND hDlg, DropDown& dropDown, int controlId)
{
    HWND ctrl = GetDlgItem(hDlg, controlId);
    if (!ctrl)
        return;
    const LRESULT sel = SendMessage(ctrl, CB_GETCURSEL, 0, 0);
    if (sel != CB_ERR)
    {
        dropDown.selectedIndex = static_cast<int>(sel);
    }
}

} // namespace

static INT_PTR CALLBACK DlgProcStatic(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    auto* self = (TextInputDialog*)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        self = (TextInputDialog*)lParam;
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)self);

        SetWindowText(hDlg, self->title);
        SetDlgItemText(hDlg, IDC_STATIC_LABEL, self->prompt);
        SetDlgItemText(hDlg, IDC_EDIT_DESCRIPTION, self->description);
        SetDlgItemText(hDlg, IDC_EDIT_INPUT, self->input.data());
        if (self->readOnly)
        {
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_INPUT), false);
        }

        self->CreateDynamicControls(hDlg);
        return (INT_PTR)TRUE;
    }
    // TODO: don't close dialog if enter is pressed in edit control
    case WM_COMMAND:
        // Only handle button-click notifications (BN_CLICKED == 0). This also
        // matches Enter key and dialog-manager-generated IDOK/IDCANCEL
        // commands, which send HIWORD(wParam) == 0. Filtering here prevents
        // spurious notifications like BN_SETFOCUS or BN_DOUBLECLICKED from
        // triggering input collection or closing the dialog.
        if (HIWORD(wParam) == BN_CLICKED)
        {
            if (self)
            {
                // Handle checkbox clicks within checkbox groups.
                if (!self->controls.empty())
                {
                    self->HandleCheckBoxClick(hDlg, LOWORD(wParam));
                }

                if (LOWORD(wParam) == IDOK)
                {
                    int length = GetWindowTextLength(GetDlgItem(hDlg, IDC_EDIT_INPUT));
                    self->input.resize(length);
                    PWSTR data = const_cast<PWSTR>(self->input.data());
                    GetDlgItemText(hDlg, IDC_EDIT_INPUT, data, length + 1);

                    self->CollectControlValues(hDlg);
                    self->confirmed = true;
                }
            }

            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                SetWindowLongPtr(hDlg, GWLP_USERDATA, NULL);
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
        }
        break;
    case WM_NCDESTROY:
        SetWindowLongPtr(hDlg, GWLP_USERDATA, NULL);
        return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}

// --- TextInputDialog member methods ---

// Existing public constructor - delegates to private extended constructor.
TextInputDialog::TextInputDialog(
    HWND parent, PCWSTR title, PCWSTR prompt, PCWSTR description,
    const std::wstring& defaultInput, bool readOnly)
    : TextInputDialog(parent, title, prompt, description, defaultInput, readOnly, {})
{
}

// Private extended constructor used by Builder and the public constructor.
TextInputDialog::TextInputDialog(
    HWND parent, PCWSTR title, PCWSTR prompt, PCWSTR description,
    const std::wstring& defaultInput, bool readOnly, std::vector<DialogControl> controls)
    : title(title), prompt(prompt), description(description), readOnly(readOnly),
      confirmed(false), input(defaultInput), controls(std::move(controls))
{
    DialogBoxParam(
        g_hInstance, MAKEINTRESOURCE(IDD_DIALOG_INPUT),
        parent, DlgProcStatic, (LPARAM)this);
}

// Creates all dynamic controls and resizes the dialog to fit them.
// When the Builder path is used, the dialog template's built-in description
// and input edit controls are hidden (they are unused — all input comes from
// dynamic controls instead). Their vertical space is reclaimed so the dialog
// doesn't have a blank gap.
// Then iterates over the |controls| vector — each entry is a DialogControl
// variant (either a CheckBoxGroup or a TextArea). For each entry,
// computes a base control ID spaced by kMaxOptionsPerGroup so ID ranges don't
// overlap, then delegates to CreateDynamicControl which creates the Win32
// child windows. After all controls are placed, expands the group box and
// dialog to fit, and shifts the OK/Cancel buttons down accordingly.
void TextInputDialog::CreateDynamicControls(HWND hDlg)
{
    if (controls.empty())
        return;

    // When dynamic controls are present (Builder path), hide the built-in
    // description and input controls since all inputs are managed dynamically.
    HWND descriptionEdit = GetDlgItem(hDlg, IDC_EDIT_DESCRIPTION);
    HWND inputEdit = GetDlgItem(hDlg, IDC_EDIT_INPUT);

    // The prompt label is a group box. Place dynamic controls inside it.
    HWND promptLabel = GetDlgItem(hDlg, IDC_STATIC_LABEL);
    RECT promptRect = GetControlRect(hDlg, promptLabel);

    // Capture the original content height (group box top to input bottom)
    // before hiding the built-in controls.
    RECT inputRect = GetControlRect(hDlg, inputEdit);
    int originalContentHeight = inputRect.bottom - promptRect.top;

    ShowWindow(descriptionEdit, SW_HIDE);
    ShowWindow(inputEdit, SW_HIDE);

    // Inset controls inside the group box with padding.
    static constexpr int kGroupBoxPadding = 10;
    static constexpr int kGroupBoxTitleHeight = 20;
    int controlX = promptRect.left + kGroupBoxPadding;
    int controlWidth = (promptRect.right - promptRect.left) - 2 * kGroupBoxPadding;
    int currentY = promptRect.top + kGroupBoxTitleHeight;

    HFONT dialogFont = reinterpret_cast<HFONT>(SendMessage(hDlg, WM_GETFONT, 0, 0));

    for (size_t i = 0; i < controls.size(); ++i)
    {
        int controlId = IDC_DYNAMIC_CONTROL_BASE + static_cast<int>(i) * kMaxOptionsPerGroup;
        int height = CreateDynamicControl(
            hDlg, controls[i], controlId, controlX, currentY, controlWidth, dialogFont);
        currentY += height + kControlSpacing;
    }

    // Expand the group box to contain all dynamic controls.
    int newGroupBoxHeight = currentY - promptRect.top + kGroupBoxPadding;
    SetWindowPos(
        promptLabel, NULL, 0, 0, promptRect.right - promptRect.left, newGroupBoxHeight,
        SWP_NOMOVE | SWP_NOZORDER);

    // Height delta: new group box height vs. original content area
    // (group box + description + input edit that were replaced).
    int additionalHeight = newGroupBoxHeight - originalContentHeight;

    // Grow or shrink the dialog by the computed delta.
    // Uses screen-coordinate rect directly since we only need width/height.
    RECT dlgRect;
    GetWindowRect(hDlg, &dlgRect);
    SetWindowPos(
        hDlg, NULL, 0, 0, dlgRect.right - dlgRect.left,
        dlgRect.bottom - dlgRect.top + additionalHeight, SWP_NOMOVE | SWP_NOZORDER);

    // Shift OK and Cancel buttons down by the same delta.
    ShiftButton(hDlg, IDOK, additionalHeight);
    ShiftButton(hDlg, IDCANCEL, additionalHeight);
}

// Collects user-entered values from all dynamic controls into the |results|
// map, keyed by each control's label. Called when the user clicks OK.
// Iterates over |controls| using the same ID scheme as CreateDynamicControls
// (base + index * kMaxOptionsPerGroup). Dispatches to CollectCheckBoxGroupValues
// or CollectTextAreaValue for each control type, then stores the updated
// control in results via insert_or_assign.
void TextInputDialog::CollectControlValues(HWND hDlg)
{
    results.clear();
    for (size_t i = 0; i < controls.size(); ++i)
    {
        int controlId = IDC_DYNAMIC_CONTROL_BASE + static_cast<int>(i) * kMaxOptionsPerGroup;

        std::visit(
            [&](auto& control)
            {
                using T = std::decay_t<decltype(control)>;
                if constexpr (std::is_same_v<T, CheckBoxGroup>)
                {
                    CollectCheckBoxGroupValues(hDlg, control, controlId);
                    results.insert_or_assign(control.groupLabel, control);
                }
                else if constexpr (std::is_same_v<T, TextArea>)
                {
                    CollectTextAreaValue(hDlg, control, controlId);
                    results.insert_or_assign(control.label, control);
                }
                else if constexpr (std::is_same_v<T, DropDown>)
                {
                    CollectDropDownValue(hDlg, control, controlId);
                    results.insert_or_assign(control.label, control);
                }
            },
            controls[i]);
    }
}

// Handles a BN_CLICKED notification for a dynamic checkbox. Decodes the
// control ID to determine which group and option was clicked, then updates
// that option's isSelected flag from the checkbox's current check state.
// Uses early returns to keep the logic flat instead of deeply nested.
void TextInputDialog::HandleCheckBoxClick(HWND hDlg, int controlId)
{
    if (controlId < IDC_DYNAMIC_CONTROL_BASE)
        return;

    int groupIndex = (controlId - IDC_DYNAMIC_CONTROL_BASE) / kMaxOptionsPerGroup;
    int optionIndex = (controlId - IDC_DYNAMIC_CONTROL_BASE) % kMaxOptionsPerGroup;

    if (groupIndex < 0 || groupIndex >= static_cast<int>(controls.size()))
        return;

    auto* group = std::get_if<CheckBoxGroup>(&controls[groupIndex]);
    if (!group)
        return;

    if (optionIndex < 0 || optionIndex >= static_cast<int>(group->options.size()))
        return;

    HWND checkBox = GetDlgItem(hDlg, controlId);
    if (!checkBox)
        return;

    group->options[optionIndex].isSelected =
        (SendMessage(checkBox, BM_GETCHECK, 0, 0) == BST_CHECKED);
}

// --- Builder implementation ---

TextInputDialog::Builder::Builder(HWND parent, PCWSTR title, PCWSTR prompt)
    : m_parent(parent), m_title(title), m_prompt(prompt)
{
}

TextInputDialog::Builder& TextInputDialog::Builder::AddCheckBoxGroup(
    const std::wstring& groupLabel, std::vector<CheckBoxOption> options)
{
    m_controls.emplace_back(CheckBoxGroup(groupLabel, std::move(options)));
    return *this;
}

TextInputDialog::Builder& TextInputDialog::Builder::AddTextArea(
    const std::wstring& label, const std::wstring& input, bool readOnly, int labelHeight,
    int inputHeight)
{
    m_controls.emplace_back(TextArea(label, input, readOnly, labelHeight, inputHeight));
    return *this;
}

TextInputDialog::Builder& TextInputDialog::Builder::AddDropDown(
    const std::wstring& label, std::vector<std::wstring> options, int selectedIndex)
{
    m_controls.emplace_back(DropDown(label, std::move(options), selectedIndex));
    return *this;
}

TextInputDialog TextInputDialog::Builder::Build()
{
    return TextInputDialog(m_parent, m_title, m_prompt, L"", L"", false, std::move(m_controls));
}
