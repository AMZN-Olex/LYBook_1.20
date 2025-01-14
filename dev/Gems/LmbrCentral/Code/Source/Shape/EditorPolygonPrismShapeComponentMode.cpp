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
#include "EditorPolygonPrismShapeComponentMode.h"

#include <AzToolsFramework/Manipulators/LineHoverSelection.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

namespace LmbrCentral
{
    AZ_CLASS_ALLOCATOR_IMPL(EditorPolygonPrismShapeComponentMode, AZ::SystemAllocator, 0)

    /// Util to calculate central position of prism (to draw the height manipulator)
    static AZ::Vector3 CalculateHeightManipulatorPosition(const AZ::PolygonPrism& polygonPrism)
    {
        const AZ::VectorFloat height = polygonPrism.GetHeight();
        const AZ::VertexContainer<AZ::Vector2>& vertexContainer = polygonPrism.m_vertexContainer;

        AzToolsFramework::MidpointCalculator midpointCalculator;
        for (const AZ::Vector2& vertex : vertexContainer.GetVertices())
        {
            midpointCalculator.AddPosition(AZ::Vector2ToVector3(vertex, height));
        }

        return midpointCalculator.CalculateMidpoint();
    }

    EditorPolygonPrismShapeComponentMode::EditorPolygonPrismShapeComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid componentType)
        : EditorBaseComponentMode(entityComponentIdPair, componentType)
    {
        m_currentTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(
            m_currentTransform, entityComponentIdPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        AZ::TransformNotificationBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());
        PolygonPrismShapeComponentNotificationBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());
        ShapeComponentNotificationsBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());

        CreateManipulators();
    }

    EditorPolygonPrismShapeComponentMode::~EditorPolygonPrismShapeComponentMode()
    {
        ShapeComponentNotificationsBus::Handler::BusDisconnect();
        PolygonPrismShapeComponentNotificationBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();

        DestroyManipulators();
    }

    void EditorPolygonPrismShapeComponentMode::Refresh()
    {
        ContainerChanged();
    }

    AZStd::vector<AzToolsFramework::ActionOverride> EditorPolygonPrismShapeComponentMode::PopulateActionsImpl()
    {
        return m_vertexSelection.ActionOverrides();
    }

    bool EditorPolygonPrismShapeComponentMode::HandleMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        return m_vertexSelection.HandleMouse(mouseInteraction);
    }

    void EditorPolygonPrismShapeComponentMode::CreateManipulators()
    {
        using namespace AzToolsFramework;

        AZ::PolygonPrismPtr polygonPrism = nullptr;
        PolygonPrismShapeComponentRequestBus::EventResult(
            polygonPrism, GetEntityId(), &PolygonPrismShapeComponentRequests::GetPolygonPrism);

        // if we have no vertices, do not attempt to create any manipulators
        if (polygonPrism->m_vertexContainer.Empty())
        {
            return;
        }

        m_vertexSelection.Create(
            AZ::EntityComponentIdPair(GetEntityId(), GetComponentId()), g_mainManipulatorManagerId,
            AZStd::make_unique<LineSegmentHoverSelection<AZ::Vector2>>(GetEntityId(), g_mainManipulatorManagerId),
            TranslationManipulators::Dimensions::Two,
            ConfigureTranslationManipulatorAppearance2d);

        // callback after vertices in the selection have moved
        m_vertexSelection.SetVertexPositionsUpdatedCallback([this, polygonPrism]()
        {
            // ensure we refresh the height manipulator after vertices are moved to ensure it stays central to the prism
            m_heightManipulator->SetLocalTransform(
                AZ::Transform::CreateTranslation(CalculateHeightManipulatorPosition(*polygonPrism)));
            m_heightManipulator->SetBoundsDirty();
        });

        // initialize height manipulator
        m_heightManipulator = LinearManipulator::MakeShared(m_currentTransform);
        m_heightManipulator->AddEntityId(GetEntityId());
        m_heightManipulator->SetLocalTransform(
            AZ::Transform::CreateTranslation(CalculateHeightManipulatorPosition(*polygonPrism)));
        m_heightManipulator->SetAxis(AZ::Vector3::CreateAxisZ());

        const float lineLength = 0.5f;
        const float lineWidth = 0.05f;
        const float coneLength = 0.28f;
        const float coneRadius = 0.07f;
        ManipulatorViews views;
        views.emplace_back(CreateManipulatorViewLine(
            *m_heightManipulator, AZ::Color(0.0f, 0.0f, 1.0f, 1.0f),
            lineLength, lineWidth));
        views.emplace_back(CreateManipulatorViewCone(*m_heightManipulator,
            AZ::Color(0.0f, 0.0f, 1.0f, 1.0f), m_heightManipulator->GetAxis() *
            (lineLength - coneLength), coneLength, coneRadius));
        m_heightManipulator->SetViews(AZStd::move(views));

        // height manipulator callbacks
        m_heightManipulator->InstallMouseMoveCallback([this, polygonPrism](
            const LinearManipulator::Action& action)
        {
            polygonPrism->SetHeight(AZ::VectorFloat(
                action.LocalPosition().GetZ()).GetMax(AZ::VectorFloat::CreateZero()));
            m_heightManipulator->SetLocalTransform(
                AZ::Transform::CreateTranslation(Vector2ToVector3(Vector3ToVector2(
                    action.LocalPosition()), action.LocalPosition().GetZ().GetMax(AZ::VectorFloat::CreateZero()))));
            m_heightManipulator->SetBoundsDirty();

            // ensure property grid values are refreshed
            ToolsApplicationNotificationBus::Broadcast(
                &ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay,
                Refresh_Values);
        });

        m_heightManipulator->Register(g_mainManipulatorManagerId);
    }

    void EditorPolygonPrismShapeComponentMode::DestroyManipulators()
    {
        // clear all manipulators when deselected
        if (m_heightManipulator)
        {
            m_heightManipulator->Unregister();
            m_heightManipulator.reset();
        }

        m_vertexSelection.Destroy();
    }

    void EditorPolygonPrismShapeComponentMode::ContainerChanged()
    {
        DestroyManipulators();
        CreateManipulators();
    }

    void EditorPolygonPrismShapeComponentMode::RefreshManipulators()
    {
        m_vertexSelection.RefreshLocal();

        if (m_heightManipulator)
        {
            AZ::PolygonPrismPtr polygonPrism = nullptr;
            PolygonPrismShapeComponentRequestBus::EventResult(
                polygonPrism, GetEntityId(), &PolygonPrismShapeComponentRequests::GetPolygonPrism);

            m_heightManipulator->SetLocalTransform(
                AZ::Transform::CreateTranslation(CalculateHeightManipulatorPosition(*polygonPrism)));
            m_heightManipulator->SetBoundsDirty();
        }
    }

    void EditorPolygonPrismShapeComponentMode::OnTransformChanged(
        const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentTransform = world;

        // update the space manipulators are in after the entity has moved
        m_vertexSelection.RefreshSpace(world);

        if (m_heightManipulator)
        {
            m_heightManipulator->SetSpace(world);
        }
    }

    void EditorPolygonPrismShapeComponentMode::OnShapeChanged(ShapeChangeReasons /*changeReason*/)
    {
        RefreshManipulators();
    }

    void EditorPolygonPrismShapeComponentMode::OnVertexAdded(size_t index)
    {
        ContainerChanged();

        AZ::PolygonPrismPtr polygonPrism = nullptr;
        PolygonPrismShapeComponentRequestBus::EventResult(
            polygonPrism, GetEntityId(), &PolygonPrismShapeComponentRequests::GetPolygonPrism);

        m_vertexSelection.CreateTranslationManipulator(
            GetEntityId(), AzToolsFramework::g_mainManipulatorManagerId,
            polygonPrism->m_vertexContainer.GetVertices()[index], index);
    }

    void EditorPolygonPrismShapeComponentMode::OnVertexRemoved(size_t index)
    {
        ContainerChanged();
    }

    void EditorPolygonPrismShapeComponentMode::OnVerticesSet(const AZStd::vector<AZ::Vector2>& /*vertices*/)
    {
        ContainerChanged();
    }

    void EditorPolygonPrismShapeComponentMode::OnVerticesCleared()
    {
        ContainerChanged();
    }
} // namespace LmbrCentral