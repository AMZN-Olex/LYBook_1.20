/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Rtti/ReflectContext.h>

namespace GradientSignal
{
    class ImageSettings
        : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(ImageSettings, "{B36FEB5C-41B6-4B58-A212-21EF5AEF523C}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(ImageSettings, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context);
        bool m_shouldProcess = true;
    };
}// namespace GradientSignal
