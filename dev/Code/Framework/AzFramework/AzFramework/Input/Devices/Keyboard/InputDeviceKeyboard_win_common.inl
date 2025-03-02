/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard_win_scancodes.inl>

#include <AzCore/std/string/conversions.h>

#include <Windows.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Class used to convert sequences of UTF-16 code units to UTF-8 code points
    class UTF16ToUTF8Converter
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Feed a UTF-16 code unit to the converter
        //!
        //! \param[in] codeUnitUTF16 The UTF-16 code unit to be converted. The code unit could be a
        //! standalone code point, in which case it is converted immediately. Or it could form part
        //! of a surrogate pair, in which case we rely on the lead and trailing surrogate being fed
        //! to this function in succession before the converted UTF-8 code point can be returned.
        //!
        //! \return If codeUnitUTF16 is a lead surrogate it is stored internally and an empty string
        //! returned. If codeUnitUTF16 is a trailing surrogate and forms a valid surrogate pair with
        //! the currently stored lead surrogate, the resulting UTF-16 code point is converted to the
        //! corresponding UTF-8 code point, and returned as a UTF-8 encoded string. If codeUnitUTF16
        //! is neither a lead or trailing surrogate, it is converted to the corresponding UTF-8 code
        //! point, and returned immediately as a UTF-8 encoded string. Any other case is an encoding
        //! error that will result in an empty string being returned.
        AZStd::string FeedCodeUnitUTF16(uint16_t codeUnitUTF16);

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! If a codeUnitUTF16 passed to FeedCodeUnitUTF16 is part of a surrogate pair we must store
        //! the 'lead surrogate' so the subsequent 'trailing surrogate' can be correctly interpreted.
        uint16_t m_leadSurrogate = 0;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline AZStd::string UTF16ToUTF8Converter::FeedCodeUnitUTF16(uint16_t codeUnitUTF16)
    {
        AZStd::string codePointUTF8;

        if (IS_HIGH_SURROGATE(codeUnitUTF16))
        {
            // Store the lead surrogate and wait for the trailing surrogate
            m_leadSurrogate = codeUnitUTF16;
        }
        else if (IS_LOW_SURROGATE(codeUnitUTF16))
        {
            if (m_leadSurrogate)
            {
                // Convert the valid UTF-16 surrogate pair to a UTF-8 code point
                const wchar_t codePointUTF16[2] = { m_leadSurrogate, codeUnitUTF16 };
                AZStd::to_string(codePointUTF8, codePointUTF16, 2);
                m_leadSurrogate = 0;
            }
            else
            {
                // Encoding error
                m_leadSurrogate = 0;
            }
        }
        else
        {
            // Convert the standalone UTF-16 code point to a UTF-8 code point
            const wchar_t codePointUTF16[1] = { codeUnitUTF16 };
            AZStd::to_string(codePointUTF8, codePointUTF16, 1);
            m_leadSurrogate = 0;
        }

        return codePointUTF8;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Get the input channel id that corresponds to a raw keyboard key event
    //! \param[in] scanCode The Windows scan code of the key that was pressed
    //! \param[in] virtualKeyCode The Windows virtual key code that was generated by the key press
    //! \param[in] hasExtendedKeyPrefix Does the Windows scan code have the extended key prefix set?
    //! \return The corresponding input channel id if found, nullptr otherwise
    inline const InputChannelId* GetInputChannelIdFromRawKeyEvent(AZ::u32 scanCode,
                                                                  AZ::u32 virtualKeyCode,
                                                                  bool hasExtendedKeyPrefix)
    {
        if (scanCode >= InputChannelIdByScanCodeTable.size() ||
            virtualKeyCode >= InputChannelIdByVirtualKeyCodeTable.size())
        {
            // Discard escaped sequences
            return nullptr;
        }

        // First look for the channel id using the scan code
        const InputChannelId* channelId = !hasExtendedKeyPrefix ?
            InputChannelIdByScanCodeTable[scanCode] :
            InputChannelIdByScanCodeWithExtendedPrefixTable[scanCode];

        // If we couldn't find the channelId from the scan code, try using the virtual key code.
        // Also, because the pause key generates the same scan code as the numlock key, we must
        // check the virtual key code in this case to distinguish between the two physical keys.
        if (!channelId || virtualKeyCode == VK_PAUSE)
        {
            channelId = InputChannelIdByVirtualKeyCodeTable[virtualKeyCode];
        }

        return channelId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Construct a map of Windows scan codes indexed by their corresponding input channel id
    //! \return A map of Windows scan codes indexed by their corresponding input channel id
    inline AZStd::unordered_map<InputChannelId, AZ::u32> ConstructScanCodeByInputChannelIdMap()
    {
        AZStd::unordered_map<InputChannelId, AZ::u32> scanCodesByInputChannelId;
        for (AZ::u32 scanCode = 0; scanCode < InputChannelIdByScanCodeTable.size(); ++scanCode)
        {
            const InputChannelId* inputChannelId = InputChannelIdByScanCodeTable[scanCode];
            if (inputChannelId != nullptr)
            {
                scanCodesByInputChannelId[*inputChannelId] = scanCode;
            }
        }
        return scanCodesByInputChannelId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Get the Windows scan code that corresponds to an input channel id
    //! \param[in] inputChannelId The input channel id whose corresponding scan code to return
    //! \return The corresponding Windows scan code id if found, 0 otherwise
    inline AZ::u32 GetScanCodeFromInputChannelId(const InputChannelId& inputChannelId)
    {
        for (AZ::u32 scanCode = 0; scanCode < InputChannelIdByScanCodeTable.size(); ++scanCode)
        {
            const InputChannelId* inputChannelIdOfScanCode = InputChannelIdByScanCodeTable[scanCode];
            if (inputChannelIdOfScanCode != nullptr && *inputChannelIdOfScanCode == inputChannelId)
            {
                return scanCode;
            }
        }
        return 0;
    }
} // namespace AzFramework
