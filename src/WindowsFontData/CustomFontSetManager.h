//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

// Removed unnecessary functions. Is a sample from:
// https://github.com/microsoft/Windows-classic-samples/tree/main

#pragma once

#include "stdafx.h"
#include "DXHelper.h"

namespace DWriteCustomFontSets
{
    class CustomFontSetManager
    {
    public:
        CustomFontSetManager();

        // Method for checking API availability
        bool IDWriteFactory5_IsAvailable();

        // Methods for creating font sets under the different scenarios.
        void CreateFontSetUsingLocalFontFiles(const std::vector<std::wstring>& selectedFilePathNames);

        void CreateFontCollectionFromFontSet();

        Microsoft::WRL::ComPtr<IDWriteFontSet>              m_customFontSet;
        Microsoft::WRL::ComPtr<IDWriteFactory3>             m_dwriteFactory3; // Available on all Windows 10 versions (build 10240 and later).
        Microsoft::WRL::ComPtr<IDWriteFactory5>             m_dwriteFactory5; // Available on Windows 10 Creators Update (preview build 15021 or later).
        Microsoft::WRL::ComPtr<IDWriteFontCollection1>       m_customFontCollection;
    }; // class CustomFontSetManager
} // namespace DWriteCustomFontSets
