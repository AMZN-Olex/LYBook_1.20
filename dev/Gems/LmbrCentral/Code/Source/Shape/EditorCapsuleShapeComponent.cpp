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

#include "LmbrCentral_precompiled.h"
#include "EditorCapsuleShapeComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include "CapsuleShapeComponent.h"
#include "EditorShapeComponentConverters.h"
#include "ShapeDisplay.h"
#include <LmbrCentral/Geometry/GeometrySystemComponentBus.h>

namespace LmbrCentral
{
    void EditorCapsuleShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Deprecate: EditorCapsuleColliderComponent -> EditorCapsuleShapeComponent
            serializeContext->ClassDeprecate(
                "EditorCapsuleColliderComponent",
                "{63247EE1-B081-40D9-8AE2-98E5C738EBD8}",
                &ClassConverters::DeprecateEditorCapsuleColliderComponent)
                ;

            serializeContext->Class<EditorCapsuleShapeComponent, EditorBaseShapeComponent>()
                ->Version(3, &ClassConverters::UpgradeEditorCapsuleShapeComponent)
                ->Field("CapsuleShape", &EditorCapsuleShapeComponent::m_capsuleShape)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorCapsuleShapeComponent>("Capsule Shape", "The Capsule Shape component creates a capsule around the associated entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Capsule_Shape.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Capsule_Shape.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-shapes.html")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorCapsuleShapeComponent::m_capsuleShape, "Capsule Shape", "Capsule Shape Configuration")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorCapsuleShapeComponent::ConfigurationChanged)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
            }
        }
    }

    void EditorCapsuleShapeComponent::Init()
    {
        EditorBaseShapeComponent::Init();

        SetShapeComponentConfig(&m_capsuleShape.ModifyCapsuleConfiguration());
    }

    void EditorCapsuleShapeComponent::Activate()
    {
        EditorBaseShapeComponent::Activate();
        m_capsuleShape.Activate(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());

        GenerateVertices();
    }

    void EditorCapsuleShapeComponent::Deactivate()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        m_capsuleShape.Deactivate();
        EditorBaseShapeComponent::Deactivate();
    }

    void EditorCapsuleShapeComponent::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        DisplayShape(
            debugDisplay, [this]() { return CanDraw(); },
            [this](AzFramework::DebugDisplayRequests& debugDisplay)
            {
                DrawShape(debugDisplay, { m_shapeColor, m_shapeWireColor, m_displayFilled }, m_capsuleShapeMesh);
            },
            m_capsuleShape.GetCurrentTransform());
    }

    void EditorCapsuleShapeComponent::ConfigurationChanged()
    {
        GenerateVertices();
        m_capsuleShape.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        ShapeComponentNotificationsBus::Event(
            GetEntityId(), &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    void EditorCapsuleShapeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (auto component = gameEntity->CreateComponent<CapsuleShapeComponent>())
        {
            component->SetConfiguration(m_capsuleShape.GetCapsuleConfiguration());
        }

        if (m_visibleInGameView)
        {
            if (auto component = gameEntity->CreateComponent<CapsuleShapeDebugDisplayComponent>())
            {
                component->SetConfiguration(m_capsuleShape.GetCapsuleConfiguration());
            }
        }
    }

    void EditorCapsuleShapeComponent::GenerateVertices()
    {
        CapsuleGeometrySystemRequestBus::Broadcast(
            &CapsuleGeometrySystemRequestBus::Events::GenerateCapsuleMesh,
            m_capsuleShape.GetCapsuleConfiguration().m_radius,
            m_capsuleShape.GetCapsuleConfiguration().m_height,
            g_capsuleDebugShapeSides,
            g_capsuleDebugShapeCapSegments,
            m_capsuleShapeMesh.m_vertexBuffer,
            m_capsuleShapeMesh.m_indexBuffer,
            m_capsuleShapeMesh.m_lineBuffer);
    }
} // namespace LmbrCentral