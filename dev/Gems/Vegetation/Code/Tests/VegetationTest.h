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
#pragma once

#include <Vegetation/Ebuses/AreaRequestBus.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <Tests/TestTypes.h>
#include <AzCore/std/functional.h>

/*
* testing fixtures for vegetation testing
*/
namespace UnitTest
{
    class VegetationComponentTests
        : public ::testing::Test
    {
    protected:
        AZ::ComponentApplication m_app;

        virtual void RegisterComponentDescriptors() {}

        void SetUp() override
        {
            AZ::ComponentApplication::Descriptor appDesc;
            appDesc.m_memoryBlocksByteSize = 20 * 1024 * 1024;
            appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_NO_RECORDS;
            appDesc.m_stackRecordLevels = 20;

            m_app.Create(appDesc);
            RegisterComponentDescriptors();
        }

        void TearDown() override
        {
            m_app.Destroy();
        }

        int m_createdCallbackCount = 0;
        int m_existedCallbackCount = 0;
        bool m_existedCallbackOutput = false;

        template <const AZStd::size_t CountX = 1, const AZStd::size_t CountY = 1>
        Vegetation::ClaimContext CreateContext(AZ::Vector3 startValue, float stepValue = 1.0f)
        {
            Vegetation::ClaimContext claimContext;

            for (auto x = 0; x < CountX; ++x)
            {
                for (auto y = 0; y < CountY; ++y)
                {
                    AZ::Vector3 value = startValue + AZ::Vector3(x * stepValue, y * stepValue, 0.0f);

                    size_t hash = 0;
                    AZStd::hash_combine<float>(hash, value.GetX());
                    AZStd::hash_combine<float>(hash, value.GetY());

                    Vegetation::ClaimPoint pt;
                    pt.m_position = value;
                    pt.m_handle = hash;
                    claimContext.m_availablePoints.push_back(pt);
                }
            }

            claimContext.m_createdCallback = [this](const Vegetation::ClaimPoint&, const Vegetation::InstanceData&) 
            {
                ++m_createdCallbackCount;
            };

            claimContext.m_existedCallback = [this](const Vegetation::ClaimPoint&, const Vegetation::InstanceData&) 
            { 
                m_existedCallbackCount;
                return m_existedCallbackOutput;
            };

            return claimContext;
        }

        template <typename Component, typename Configuration>
        AZStd::unique_ptr<AZ::Entity> CreateEntity(const Configuration& config, Component** ppComponent)
        {
            m_app.RegisterComponentDescriptor(Component::CreateDescriptor());

            auto entity = AZStd::make_unique<AZ::Entity>();

            if (ppComponent)
            {
                *ppComponent = entity->CreateComponent<Component>(config);
            }
            else
            {
                entity->CreateComponent<Component>(config);
            }

            entity->Init();
            EXPECT_EQ(AZ::Entity::ES_INIT, entity->GetState());

            entity->Activate();
            EXPECT_EQ(AZ::Entity::ES_ACTIVE, entity->GetState());

            return entity;
        }

        using CreateAdditionalComponents = AZStd::function<void(AZ::Entity*)>;

        template <typename Component, typename Configuration>
        AZStd::unique_ptr<AZ::Entity> CreateEntity(const Configuration& config, Component** ppComponent, CreateAdditionalComponents fnCreateAdditionalComponents)
        {
            m_app.RegisterComponentDescriptor(Component::CreateDescriptor());

            auto entity = AZStd::make_unique<AZ::Entity>();

            if (ppComponent)
            {
                *ppComponent = entity->CreateComponent<Component>(config);
            }
            else
            {
                entity->CreateComponent<Component>(config);
            }

            fnCreateAdditionalComponents(entity.get());

            entity->Init();
            EXPECT_EQ(AZ::Entity::ES_INIT, entity->GetState());

            entity->Activate();
            EXPECT_EQ(AZ::Entity::ES_ACTIVE, entity->GetState());

            return entity;
        }
    };
}