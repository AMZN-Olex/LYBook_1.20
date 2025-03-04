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

#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/SceneBus.h>

#include <Editor/GraphCanvas/Components/NodeDescriptors/NodeDescriptorComponent.h>

#include <ScriptCanvas/Variable/VariableBus.h>
#include <Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>

namespace ScriptCanvasEditor
{
    class VariableNodeDescriptorComponent
        : public NodeDescriptorComponent
        , public GraphCanvas::NodeNotificationBus::Handler
        , public GraphCanvas::SceneMemberNotificationBus::Handler
        , public ScriptCanvas::VariableNotificationBus::Handler
        , public ScriptCanvas::VariableNodeNotificationBus::Handler
        , public VariableNodeDescriptorRequestBus::Handler
        , public VariableGraphMemberRefCountRequestBus::Handler
    {
    public:
        AZ_COMPONENT(VariableNodeDescriptorComponent, "{80CB9400-E40D-4DC7-B185-412F766C8565}", NodeDescriptorComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);

        VariableNodeDescriptorComponent();
        ~VariableNodeDescriptorComponent() = default;

        // Component
        void Activate() override;
        void Deactivate() override;
        ////

        // VariableNotificationBus
        void OnVariableRenamed(AZStd::string_view variableName) override;
        void OnVariableRemoved() override;
        ////

        // VariableNodeNotificationBus
        void OnVariableIdChanged(const ScriptCanvas::VariableId& oldVariableId, const ScriptCanvas::VariableId& newVariableId) override;
        ////

        // NodeNotificationBus
        void OnAddedToScene(const AZ::EntityId& sceneId) override;        
        ////

        // SceneMemberNotifications
        void OnSceneMemberAboutToSerialize(GraphCanvas::GraphSerialization& graphSerialization) override;
        void OnSceneMemberDeserialized(const AZ::EntityId& graphId, const GraphCanvas::GraphSerialization& graphSerialization) override;
        ////

        // VariableNodeDescriptorBus
        ScriptCanvas::VariableId GetVariableId() const;
        ////

        // VariableGraphMemberRefCountRequestBus
        AZ::EntityId GetGraphMemberId() const override;
        ////
        
    protected:
    
        virtual void UpdateTitle(AZStd::string_view variableName);

    private:
        
        void SetVariableId(const ScriptCanvas::VariableId& variableId);
    };
}