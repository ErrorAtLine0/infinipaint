#pragma once
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

#include "CustomFontSetManager.h"

using Microsoft::WRL::ComPtr;

namespace DWriteCustomFontSets
{

    //**********************************************************************
    //
    //   Constructors, destructors
    //
    //**********************************************************************

    CustomFontSetManager::CustomFontSetManager()
    {
        HRESULT hr;

        // IDWriteFactory3 supports APIs available in any Windows 10 version (build 10240 or later).
        DX::ThrowIfFailed(
            DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory3), &m_dwriteFactory3)
        );

#ifndef FORCE_TH1_IMPLEMENTATION
        // IDWriteFactory5 supports APIs available in Windows 10 Creators Update (preview build 15021 or later).
        hr = m_dwriteFactory3.As(&m_dwriteFactory5);
        if (hr == E_NOINTERFACE)
        {
            // Let this go. Later, if we might use the interface, we'll branch gracefully.
        }
        else
        {
            DX::ThrowIfFailed(hr);
        }
#endif // !FORCE_TH1_IMPLEMENTATION

    } // end CustomFontSetManager::CustomFontSetManager()


      //**********************************************************************
      //
      //   Method for checking API availability.
      //
      //**********************************************************************

    bool CustomFontSetManager::IDWriteFactory5_IsAvailable()
    {
        return m_dwriteFactory5 != nullptr;
    }


    void CustomFontSetManager::CreateFontCollectionFromFontSet() {
        if(IDWriteFactory5_IsAvailable())
            m_dwriteFactory5->CreateFontCollectionFromFontSet(m_customFontSet.Get(), m_customFontCollection.GetAddressOf());
        else
            m_dwriteFactory3->CreateFontCollectionFromFontSet(m_customFontSet.Get(), m_customFontCollection.GetAddressOf());
    }

    //**********************************************************************
    //
    //   Methods for the creating a font set under the various scenarios.
    //
    //**********************************************************************

    void CustomFontSetManager::CreateFontSetUsingLocalFontFiles(const std::vector<std::wstring>& selectedFilePathNames)
    {
        // Requires any version of Windows 10.

        // Creates a custom font set for font files at paths in local storage. If a file is
        // an OpenType collection file, which contains multiple fonts, all of the fonts will
        // be added to the collection.
        //
        // If running on Windows 10 Creators Update (preview build 15021 or later), the 
        // IDWriteFontSetBuilder1::AddFontFile method will be used. This method handles all of
        // the fonts in an OpenType collection file in a single call, and it also supports
        // OpenType variable fonts, which can be realized as many different font faces -- all
        // named instances in the variable font will be added in a single call. This method is
        // recommended when available.
        //
        // If running on earlier Windows 10 versions, the method used will be 
        // IDWriteFontSetBuilder::AddFontFaceReference. This does not support OpenType variable
        // fonts, and also requires that a font file first be analyzed to determine whether it
        // is an OpenType collection file, in which case each font must be handled in a separate
        // call.
        //
        // If one of the input path names is not a font file, it will be ignored.
        //
        // When creating a custom font set with font files that are not assumed to be known by
        // the app, DWrite will need to extract some basic font properties, such as names,
        // directly from the font files. This will result in a little extra I/O overhead.

        // Check if IDWriteFontSetBuilder1 will be available (we're running on preview build 15021 or later)
        if (m_dwriteFactory5 != nullptr)
        {
            // We'll need an IDWriteFontFile for each font file to be added to the font set.
            // We won't assume every file is a font file in a supported format; if not, we'll
            // ignore the file.

            // Get the font set builder -- IDWriteFontSetBuilder1.
            ComPtr<IDWriteFontSetBuilder1> fontSetBuilder;
            DX::ThrowIfFailed(
                m_dwriteFactory5->CreateFontSetBuilder(&fontSetBuilder)
            );

            // Loop over the file paths.
            for (auto& filePath : selectedFilePathNames)
            {
                ComPtr<IDWriteFontFile> fontFile;

                DX::ThrowIfFailed(
                    m_dwriteFactory5->CreateFontFileReference(filePath.c_str(), /* filetime */ nullptr, &fontFile)
                );

                // Add to the font set builder. If the file is a collection, all of the fonts will
                // get added. If the file is not a supported font file, the call will fail; we'll
                // check for that and ignore.
                HRESULT hr = fontSetBuilder->AddFontFile(fontFile.Get());
                if ((hr != DWRITE_E_FILEFORMAT) && (hr != DWRITE_E_FILENOTFOUND) && (hr != DWRITE_E_FILEACCESS))
                {
                    // Ignore file format or access errors.
                    DX::ThrowIfFailed(hr);
                }
            } // for loop

            // Now create the custom font set.
            DX::ThrowIfFailed(
                fontSetBuilder->CreateFontSet(&m_customFontSet)
            );
        }
        else
        {
            // We're limited to APIs and functionality available on earlier Windows 10 versions, prior
            // to the Windows 10 Creators Update (preview build 15021).
            //
            // Also, we'll need an IDWriteFontFaceReference for each font to be added to the font set.
            // If a file is an OpenType collection, it may contain multiple fonts. For that reason, 
            // we'll need to analyze the file to get the count of fonts, and then provide an index 
            // when creating each font face reference.

            // Get the font set builder - IDWriteFontSetBuilder.
            ComPtr<IDWriteFontSetBuilder> fontSetBuilder;
            DX::ThrowIfFailed(
                m_dwriteFactory3->CreateFontSetBuilder(&fontSetBuilder)
            );

            // Loop over the file paths.
            for (auto& filePath : selectedFilePathNames)
            {
                ComPtr<IDWriteFontFile> fontFile;
                DX::ThrowIfFailed(
                    m_dwriteFactory3->CreateFontFileReference(filePath.c_str(), /* filetime */ nullptr, &fontFile)
                );

                // Confirm the file is a supported font file and get the collection face count.
                BOOL isSupported;
                DWRITE_FONT_FILE_TYPE fileType;
                UINT32 numberOfFonts;
                DX::ThrowIfFailed(
                    fontFile->Analyze(&isSupported, &fileType, /* face type */ nullptr, &numberOfFonts)
                );
                if (!isSupported)
                    continue;

                // For each font within the font file, get a font face reference and add to the builder.
                for (UINT32 fontIndex = 0; fontIndex < numberOfFonts; fontIndex++)
                {
                    ComPtr<IDWriteFontFaceReference> fontFaceReference;
                    DX::ThrowIfFailed(
                        m_dwriteFactory3->CreateFontFaceReference(fontFile.Get(), fontIndex, DWRITE_FONT_SIMULATIONS_NONE, &fontFaceReference)
                    );

                    // If fonts were assumed known, we could set custom properties, and would do that here.
                    // But these are not assumed known, so we'll leave it to DirectWrite to read properties
                    // directly out of the font files.

                    DX::ThrowIfFailed(
                        fontSetBuilder->AddFontFaceReference(fontFaceReference.Get())
                    );
                } // for loop -- over fonts with font file
            } // for loop -- over font files

            // Now create the custom font set
            DX::ThrowIfFailed(
                fontSetBuilder->CreateFontSet(&m_customFontSet)
            );

        } // end if (IDWriteFactory5_IsAvailable())
    } // end CustomFontSetManager::CreateFontSetUsingLocalFontFiles()
} // namespace DWriteCustomFontSets
