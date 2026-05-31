/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#ifdef __ANDROID__
#include <mutex>
#include "InputManager.hpp"

struct InputManager;

namespace AndroidJNICalls {
    extern InputManager* globalInputManager;

    void shareInternalFiles(const std::vector<std::string>& filePaths, const std::string& mimeType);
    void shareText(const std::string& str);
    void startTextInput(CustomEvents::InputTextBoxID newTextboxID, const std::shared_ptr<RichText::TextBox>& newTextbox, const std::shared_ptr<RichText::TextBox::Cursor>& newCursor, const std::shared_ptr<RichText::TextStyleModifier::ModifierMap>& modMap, int inputType);
    void startNetworkService();
    void stopNetworkService();
    void updateCursorPos(CustomEvents::InputTextBoxID tId, const std::shared_ptr<RichText::TextBox::Cursor>& newCursor);
    void updateModMap(CustomEvents::InputTextBoxID tId, const std::shared_ptr<RichText::TextStyleModifier::ModifierMap>& newModMap);
    void updateTextboxAndCursorPos(CustomEvents::InputTextBoxID tId, const std::shared_ptr<RichText::TextBox>& newTextbox, const std::shared_ptr<RichText::TextBox::Cursor>& newCursor);
    std::pair<bool, bool> getChangesToCommit(CustomEvents::InputTextBoxID tId);
    std::pair<bool, bool> commitAll(CustomEvents::InputTextBoxID tId, const std::shared_ptr<RichText::TextBox>& textBox, const std::shared_ptr<RichText::TextBox::Cursor>& cursor);
}

#endif
