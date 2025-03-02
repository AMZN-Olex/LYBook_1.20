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

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <EMotionFX/Source/AnimGraphBus.h>
#include "EMotionFXConfig.h"
#include "AnimGraphNode.h"
#include "AnimGraphInstance.h"
#include "ActorInstance.h"
#include "EventManager.h"
#include "AnimGraphStateMachine.h"
#include "AnimGraphNodeGroup.h"
#include "AnimGraphTransitionCondition.h"
#include "AnimGraphTriggerAction.h"
#include "AnimGraphStateTransition.h"
#include "AnimGraphObjectData.h"
#include "AnimGraphEventBuffer.h"
#include "AnimGraphManager.h"
#include "AnimGraphSyncTrack.h"
#include "AnimGraph.h"
#include "Recorder.h"

#include "AnimGraphMotionNode.h"
#include "ActorManager.h"
#include "EMotionFXManager.h"

#include <MCore/Source/StringIdPool.h>
#include <MCore/Source/IDGenerator.h>
#include <MCore/Source/Attribute.h>
#include <MCore/Source/AttributeFactory.h>
#include <MCore/Source/Stream.h>
#include <MCore/Source/Compare.h>
#include <MCore/Source/Random.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphNode, AnimGraphAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphNode::Port, AnimGraphAllocator, 0)

    AnimGraphNode::AnimGraphNode()
        : AnimGraphObject(nullptr)
        , m_id(AnimGraphNodeId::Create())
        , mNodeIndex(MCORE_INVALIDINDEX32)
        , mDisabled(false)
        , mParentNode(nullptr)
        , mCustomData(nullptr)
        , mVisEnabled(false)
        , mIsCollapsed(false)
        , mPosX(0)
        , mPosY(0)
    {
        const AZ::u32 col = MCore::GenerateColor();
        mVisualizeColor = AZ::Color(
            MCore::ExtractRed(col)/255.0f,
            MCore::ExtractGreen(col)/255.0f,
            MCore::ExtractBlue(col)/255.0f,
            1.0f);
    }


    AnimGraphNode::AnimGraphNode(AnimGraph* animGraph, const char* name)
        : AnimGraphNode()
    {
        SetName(name);
        InitAfterLoading(animGraph);
    }


    AnimGraphNode::~AnimGraphNode()
    {
        RemoveAllConnections();
        RemoveAllChildNodes();

        if (mAnimGraph)
        {
            mAnimGraph->RemoveObject(this);
        }
    }


    void AnimGraphNode::RecursiveReinit()
    {
        for (BlendTreeConnection* connection : mConnections)
        {
            connection->Reinit();
        }

        for (AnimGraphNode* childNode : mChildNodes)
        {
            childNode->RecursiveReinit();
        }
    }


    bool AnimGraphNode::InitAfterLoading(AnimGraph* animGraph)
    {
        bool result = true;
        SetAnimGraph(animGraph);

        if (animGraph)
        {
            animGraph->AddObject(this);
        }

        // Initialize connections.
        for (BlendTreeConnection* connection : mConnections)
        {
            connection->InitAfterLoading(animGraph);
        }

        // Initialize child nodes.
        for (AnimGraphNode* childNode : mChildNodes)
        {
            // Sync the child node's parent.
            childNode->SetParentNode(this);

            if (!childNode->InitAfterLoading(animGraph))
            {
                result = false;
            }
        }

        // Initialize trigger actions
        for (AnimGraphTriggerAction* action : m_actionSetup.GetActions())
        {
            action->InitAfterLoading(animGraph);
        }

        return result;
    }


    // copy base settings to the other node
    void AnimGraphNode::CopyBaseNodeTo(AnimGraphNode* node) const
    {
        //CopyBaseObjectTo( node );

        // now copy the node related things
        // the parent
        //if (mParentNode)
        //node->mParentNode = node->GetAnimGraph()->RecursiveFindNodeByID( mParentNode->GetID() );

        // copy the easy values
        node->m_name            = m_name;
        node->m_id              = m_id;
        node->mNodeInfo         = mNodeInfo;
        node->mCustomData       = mCustomData;
        node->mDisabled         = mDisabled;
        node->mPosX             = mPosX;
        node->mPosY             = mPosY;
        node->mVisualizeColor   = mVisualizeColor;
        node->mVisEnabled       = mVisEnabled;
        node->mIsCollapsed      = mIsCollapsed;
    }


    void AnimGraphNode::RemoveAllConnections()
    {
        for (BlendTreeConnection* connection : mConnections)
        {
            delete connection;
        }

        mConnections.clear();
    }


    // add a connection
    BlendTreeConnection* AnimGraphNode::AddConnection(AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort)
    {
        // make sure the source and target ports are in range
        if (targetPort < mInputPorts.size() && sourcePort < sourceNode->mOutputPorts.size())
        {
            BlendTreeConnection* connection = aznew BlendTreeConnection(sourceNode, sourcePort, targetPort);
            mConnections.push_back(connection);
            mInputPorts[targetPort].mConnection = connection;
            sourceNode->mOutputPorts[sourcePort].mConnection = connection;
            return connection;
        }
        return nullptr;
    }


    BlendTreeConnection* AnimGraphNode::AddUnitializedConnection(AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort)
    {
        BlendTreeConnection* connection = aznew BlendTreeConnection(sourceNode, sourcePort, targetPort);
        mConnections.push_back(connection);
        return connection;
    }


    // validate the connections
    bool AnimGraphNode::ValidateConnections() const
    {
        for (const BlendTreeConnection* connection : mConnections)
        {
            if (!connection->GetIsValid())
            {
                return false;
            }
        }

        return true;
    }


    // check if the given input port is connected
    bool AnimGraphNode::CheckIfIsInputPortConnected(uint16 inputPort) const
    {
        for (const BlendTreeConnection* connection : mConnections)
        {
            if (connection->GetTargetPort() == inputPort)
            {
                return true;
            }
        }

        // failure, no connected plugged into the given port
        return false;
    }


    // remove all child nodes
    void AnimGraphNode::RemoveAllChildNodes(bool delFromMem)
    {
        if (delFromMem)
        {
            for (AnimGraphNode* childNode : mChildNodes)
            {
                delete childNode;
            }
        }

        mChildNodes.clear();

        // trigger that we removed nodes
        GetEventManager().OnRemovedChildNode(mAnimGraph, this);

        // TODO: remove the nodes from the node groups of the anim graph as well here
    }


    // remove a given node
    void AnimGraphNode::RemoveChildNode(uint32 index, bool delFromMem)
    {
        // remove the node from its node group
        AnimGraphNodeGroup* nodeGroup = mAnimGraph->FindNodeGroupForNode(mChildNodes[index]);
        if (nodeGroup)
        {
            nodeGroup->RemoveNodeById(mChildNodes[index]->GetId());
        }

        // delete the node from memory
        if (delFromMem)
        {
            delete mChildNodes[index];
        }

        // delete the node from the child array
        mChildNodes.erase(mChildNodes.begin() + index);

        // trigger callbacks
        GetEventManager().OnRemovedChildNode(mAnimGraph, this);
    }


    // remove a node by pointer
    void AnimGraphNode::RemoveChildNodeByPointer(AnimGraphNode* node, bool delFromMem)
    {
        // find the index of the given node in the child node array and remove it in case the index is valid
        const auto iterator = AZStd::find(mChildNodes.begin(), mChildNodes.end(), node);

        if (iterator != mChildNodes.end())
        {
            const uint32 index = static_cast<uint32>(iterator - mChildNodes.begin());
            RemoveChildNode(index, delFromMem);
        }
    }


    AnimGraphNode* AnimGraphNode::RecursiveFindNodeByName(const char* nodeName) const
    {
        if (AzFramework::StringFunc::Equal(m_name.c_str(), nodeName, true /* case sensitive */))
        {
            return const_cast<AnimGraphNode*>(this);
        }

        for (const AnimGraphNode* childNode : mChildNodes)
        {
            AnimGraphNode* result = childNode->RecursiveFindNodeByName(nodeName);
            if (result)
            {
                return result;
            }
        }

        return nullptr;
    }


    bool AnimGraphNode::RecursiveIsNodeNameUnique(const AZStd::string& newNameCandidate, const AnimGraphNode* forNode) const
    {
        if (forNode != this && m_name == newNameCandidate)
        {
            return false;
        }

        for (const AnimGraphNode* childNode : mChildNodes)
        {
            if (!childNode->RecursiveIsNodeNameUnique(newNameCandidate, forNode))
            {
                return false;
            }
        }

        return true;
    }


    AnimGraphNode* AnimGraphNode::RecursiveFindNodeById(AnimGraphNodeId nodeId) const
    {
        if (m_id == nodeId)
        {
            return const_cast<AnimGraphNode*>(this);
        }

        for (const AnimGraphNode* childNode : mChildNodes)
        {
            AnimGraphNode* result = childNode->RecursiveFindNodeById(nodeId);
            if (result)
            {
                return result;
            }
        }

        return nullptr;
    }


    // find a child node by name
    AnimGraphNode* AnimGraphNode::FindChildNode(const char* name) const
    {
        for (AnimGraphNode* childNode : mChildNodes)
        {
            // compare the node name with the parameter and return a pointer to the node in case they are equal
            if (AzFramework::StringFunc::Equal(childNode->GetName(), name, true /* case sensitive */))
            {
                return childNode;
            }
        }

        // failure, return nullptr pointer
        return nullptr;
    }


    AnimGraphNode* AnimGraphNode::FindChildNodeById(AnimGraphNodeId childId) const
    {
        for (AnimGraphNode* childNode : mChildNodes)
        {
            if (childNode->GetId() == childId)
            {
                return childNode;
            }
        }

        return nullptr;
    }


    // find a child node index by name
    uint32 AnimGraphNode::FindChildNodeIndex(const char* name) const
    {
        const size_t numChildNodes = mChildNodes.size();
        for (size_t i = 0; i < numChildNodes; ++i)
        {
            // compare the node name with the parameter and return the relative child node index in case they are equal
            if (AzFramework::StringFunc::Equal(mChildNodes[i]->GetNameString().c_str(), name, true /* case sensitive */))
            {
                return static_cast<uint32>(i);
            }
        }

        // failure, return invalid index
        return MCORE_INVALIDINDEX32;
    }


    // find a child node index
    uint32 AnimGraphNode::FindChildNodeIndex(AnimGraphNode* node) const
    {
        const auto iterator = AZStd::find(mChildNodes.begin(), mChildNodes.end(), node);
        if (iterator == mChildNodes.end())
        {
            return MCORE_INVALIDINDEX32;
        }

        const size_t index = iterator - mChildNodes.begin();
        return static_cast<uint32>(index);
    }


    AnimGraphNode* AnimGraphNode::FindFirstChildNodeOfType(const AZ::TypeId& nodeType) const
    {
        for (AnimGraphNode* childNode : mChildNodes)
        {
            if (azrtti_typeid(childNode) == nodeType)
            {
                return childNode;
            }
        }

        return nullptr;
    }


    bool AnimGraphNode::HasChildNodeOfType(const AZ::TypeId& nodeType) const
    {
        for (const AnimGraphNode* childNode : mChildNodes)
        {
            if (azrtti_typeid(childNode) == nodeType)
            {
                return true;
            }
        }

        return false;
    }


    // does this node has a specific incoming connection?
    bool AnimGraphNode::GetHasConnection(AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort) const
    {
        for (const BlendTreeConnection* connection : mConnections)
        {
            if (connection->GetSourceNode() == sourceNode && connection->GetSourcePort() == sourcePort && connection->GetTargetPort() == targetPort)
            {
                return true;
            }
        }

        return false;
    }

    // remove a given connection
    void AnimGraphNode::RemoveConnection(BlendTreeConnection* connection, bool delFromMem)
    {
        mInputPorts[connection->GetTargetPort()].mConnection = nullptr;

        AnimGraphNode* sourceNode = connection->GetSourceNode();
        if (sourceNode)
        {
            sourceNode->mOutputPorts[connection->GetSourcePort()].mConnection = nullptr;
        }

        // Remove object by value.
        mConnections.erase(AZStd::remove(mConnections.begin(), mConnections.end(), connection), mConnections.end());
        if (delFromMem)
        {
            delete connection;
        }
    }


    // remove a given connection
    void AnimGraphNode::RemoveConnection(AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort)
    {
        // for all input connections
        for (BlendTreeConnection* connection : mConnections)
        {
            if (connection->GetSourceNode() == sourceNode && connection->GetSourcePort() == sourcePort && connection->GetTargetPort() == targetPort)
            {
                RemoveConnection(connection, true);
                return;
            }
        }
    }


    bool AnimGraphNode::RemoveConnectionById(AnimGraphConnectionId connectionId, bool delFromMem)
    {
        const size_t numConnections = mConnections.size();
        for (size_t i = 0; i < numConnections; ++i)
        {
            if (mConnections[i]->GetId() == connectionId)
            {
                mInputPorts[mConnections[i]->GetTargetPort()].mConnection = nullptr;

                AnimGraphNode* sourceNode = mConnections[i]->GetSourceNode();
                if (sourceNode)
                {
                    sourceNode->mOutputPorts[mConnections[i]->GetSourcePort()].mConnection = nullptr;
                }

                if (delFromMem)
                {
                    delete mConnections[i];
                }

                mConnections.erase(mConnections.begin() + i);
            }
        }

        return false;
    }


    // find a given connection
    BlendTreeConnection* AnimGraphNode::FindConnection(const AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort) const
    {
        for (BlendTreeConnection* connection : mConnections)
        {
            if (connection->GetSourceNode() == sourceNode && connection->GetSourcePort() == sourcePort && connection->GetTargetPort() == targetPort)
            {
                return connection;
            }
        }

        return nullptr;
    }



    // initialize the input ports
    void AnimGraphNode::InitInputPorts(uint32 numPorts)
    {
        mInputPorts.resize(numPorts);
    }


    // initialize the output ports
    void AnimGraphNode::InitOutputPorts(uint32 numPorts)
    {
        mOutputPorts.resize(numPorts);
    }


    // find a given output port number
    uint32 AnimGraphNode::FindOutputPortIndex(const AZStd::string& name) const
    {
        const size_t numPorts = mOutputPorts.size();
        for (size_t i = 0; i < numPorts; ++i)
        {
            // if the port name is equal to the name we are searching for, return the index
            if (mOutputPorts[i].GetNameString() == name)
            {
                return static_cast<uint32>(i);
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find a given input port number
    uint32 AnimGraphNode::FindInputPortIndex(const AZStd::string& name) const
    {
        const size_t numPorts = mInputPorts.size();
        for (size_t i = 0; i < numPorts; ++i)
        {
            // if the port name is equal to the name we are searching for, return the index
            if (mInputPorts[i].GetNameString() == name)
            {
                return static_cast<uint32>(i);
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // add an output port and return its index
    uint32 AnimGraphNode::AddOutputPort()
    {
        const size_t currentSize = mOutputPorts.size();
        mOutputPorts.emplace_back();
        return static_cast<uint32>(currentSize);
    }


    // add an input port, and return its index
    uint32 AnimGraphNode::AddInputPort()
    {
        const size_t currentSize = mInputPorts.size();
        mInputPorts.emplace_back();
        return static_cast<uint32>(currentSize);
    }


    // setup a port name
    void AnimGraphNode::SetInputPortName(uint32 portIndex, const char* name)
    {
        MCORE_ASSERT(portIndex < mInputPorts.size());
        mInputPorts[portIndex].mNameID = MCore::GetStringIdPool().GenerateIdForString(name);
    }


    // setup a port name
    void AnimGraphNode::SetOutputPortName(uint32 portIndex, const char* name)
    {
        MCORE_ASSERT(portIndex < mOutputPorts.size());
        mOutputPorts[portIndex].mNameID = MCore::GetStringIdPool().GenerateIdForString(name);
    }


    // get the total number of children
    uint32 AnimGraphNode::RecursiveCalcNumNodes() const
    {
        uint32 result = 0;
        for (const AnimGraphNode* childNode : mChildNodes)
        {
            childNode->RecursiveCountChildNodes(result);
        }

        return result;
    }


    // recursively count the number of nodes down the hierarchy
    void AnimGraphNode::RecursiveCountChildNodes(uint32& numNodes) const
    {
        // increase the counter
        numNodes++;

        for (const AnimGraphNode* childNode : mChildNodes)
        {
            childNode->RecursiveCountChildNodes(numNodes);
        }
    }


    // recursively calculate the number of node connections
    uint32 AnimGraphNode::RecursiveCalcNumNodeConnections() const
    {
        uint32 result = 0;
        RecursiveCountNodeConnections(result);
        return result;
    }


    // recursively calculate the number of node connections
    void AnimGraphNode::RecursiveCountNodeConnections(uint32& numConnections) const
    {
        // add the connections to our counter
        numConnections += GetNumConnections();

        for (const AnimGraphNode* childNode : mChildNodes)
        {
            childNode->RecursiveCountNodeConnections(numConnections);
        }
    }


    // setup an output port to output a given local pose
    void AnimGraphNode::SetupOutputPortAsPose(const char* name, uint32 outputPortNr, uint32 portID)
    {
        // check if we already registered this port ID
        const uint32 duplicatePort = FindOutputPortByID(portID);
        if (duplicatePort != MCORE_INVALIDINDEX32)
        {
            MCore::LogError("EMotionFX::AnimGraphNode::SetOutputPortAsPose() - There is already a port with the same ID (portID=%d existingPort='%s' newPort='%s' node='%s')", portID, mOutputPorts[duplicatePort].GetName(), name, RTTI_GetTypeName());
        }

        SetOutputPortName(outputPortNr, name);
        mOutputPorts[outputPortNr].Clear();
        mOutputPorts[outputPortNr].mCompatibleTypes[0] = AttributePose::TYPE_ID;    // setup the compatible types of this port
        mOutputPorts[outputPortNr].mPortID = portID;
    }


    // setup an output port to output a given motion instance
    void AnimGraphNode::SetupOutputPortAsMotionInstance(const char* name, uint32 outputPortNr, uint32 portID)
    {
        // check if we already registered this port ID
        const uint32 duplicatePort = FindOutputPortByID(portID);
        if (duplicatePort != MCORE_INVALIDINDEX32)
        {
            MCore::LogError("EMotionFX::AnimGraphNode::SetOutputPortAsMotionInstance() - There is already a port with the same ID (portID=%d existingPort='%s' newPort='%s' node='%s')", portID, mOutputPorts[duplicatePort].GetName(), name, RTTI_GetTypeName());
        }

        SetOutputPortName(outputPortNr, name);
        mOutputPorts[outputPortNr].Clear();
        mOutputPorts[outputPortNr].mCompatibleTypes[0] = AttributeMotionInstance::TYPE_ID;  // setup the compatible types of this port
        mOutputPorts[outputPortNr].mPortID = portID;
    }


    // setup an output port
    void AnimGraphNode::SetupOutputPort(const char* name, uint32 outputPortNr, uint32 attributeTypeID, uint32 portID)
    {
        // check if we already registered this port ID
        const uint32 duplicatePort = FindOutputPortByID(portID);
        if (duplicatePort != MCORE_INVALIDINDEX32)
        {
            MCore::LogError("EMotionFX::AnimGraphNode::SetOutputPort() - There is already a port with the same ID (portID=%d existingPort='%s' newPort='%s' name='%s')", portID, mOutputPorts[duplicatePort].GetName(), name, RTTI_GetTypeName());
        }

        SetOutputPortName(outputPortNr, name);
        mOutputPorts[outputPortNr].Clear();
        mOutputPorts[outputPortNr].mCompatibleTypes[0] = attributeTypeID;
        mOutputPorts[outputPortNr].mPortID = portID;
    }

    void AnimGraphNode::SetupInputPortAsVector3(const char* name, uint32 inputPortNr, uint32 portID)
    {
        SetupInputPort(name, inputPortNr, AZStd::vector<uint32>{MCore::AttributeVector3::TYPE_ID, MCore::AttributeVector2::TYPE_ID, MCore::AttributeVector4::TYPE_ID}, portID);
    }

    void AnimGraphNode::SetupInputPortAsVector2(const char* name, uint32 inputPortNr, uint32 portID)
    {
        SetupInputPort(name, inputPortNr, AZStd::vector<uint32>{MCore::AttributeVector2::TYPE_ID, MCore::AttributeVector3::TYPE_ID}, portID);
    }

    void AnimGraphNode::SetupInputPortAsVector4(const char* name, uint32 inputPortNr, uint32 portID)
    {
        SetupInputPort(name, inputPortNr, AZStd::vector<uint32>{MCore::AttributeVector4::TYPE_ID, MCore::AttributeVector3::TYPE_ID}, portID);
    }

    void AnimGraphNode::SetupInputPort(const char* name, uint32 inputPortNr, const AZStd::vector<uint32>& attributeTypeIDs, uint32 portID)
    {
        // Check if we already registered this port ID
        const uint32 duplicatePort = FindInputPortByID(portID);
        if (duplicatePort != MCORE_INVALIDINDEX32)
        {
            MCore::LogError("EMotionFX::AnimGraphNode::SetInputPortAsNumber() - There is already a port with the same ID (portID=%d existingPort='%s' newPort='%s' node='%s')", portID, MCore::GetStringIdPool().GetName(mInputPorts[duplicatePort].mNameID).c_str(), name, RTTI_GetTypeName());
        }

        SetInputPortName(inputPortNr, name);
        mInputPorts[inputPortNr].Clear();
        mInputPorts[inputPortNr].mPortID = portID;
        mInputPorts[inputPortNr].SetCompatibleTypes(attributeTypeIDs);
    }

    // setup an input port as a number (float/int/bool)
    void AnimGraphNode::SetupInputPortAsNumber(const char* name, uint32 inputPortNr, uint32 portID)
    {
        // check if we already registered this port ID
        const uint32 duplicatePort = FindInputPortByID(portID);
        if (duplicatePort != MCORE_INVALIDINDEX32)
        {
            MCore::LogError("EMotionFX::AnimGraphNode::SetInputPortAsNumber() - There is already a port with the same ID (portID=%d existingPort='%s' newPort='%s' node='%s')", portID, mInputPorts[duplicatePort].GetName(), name, RTTI_GetTypeName());
        }

        SetInputPortName(inputPortNr, name);
        mInputPorts[inputPortNr].Clear();
        mInputPorts[inputPortNr].mCompatibleTypes[0] = MCore::AttributeFloat::TYPE_ID;
        //mInputPorts[inputPortNr].mCompatibleTypes[1] = MCore::AttributeInt32::TYPE_ID;
        //mInputPorts[inputPortNr].mCompatibleTypes[2] = MCore::AttributeBool::TYPE_ID;;
        mInputPorts[inputPortNr].mPortID = portID;
    }

    void AnimGraphNode::SetupInputPortAsBool(const char* name, uint32 inputPortNr, uint32 portID)
    {
        // check if we already registered this port ID
        const uint32 duplicatePort = FindInputPortByID(portID);
        if (duplicatePort != MCORE_INVALIDINDEX32)
        {
            MCore::LogError("EMotionFX::AnimGraphNode::SetInputPortAsBool() - There is already a port with the same ID (portID=%d existingPort='%s' newPort='%s' node='%s')", portID, mInputPorts[duplicatePort].GetName(), name, RTTI_GetTypeName());
        }

        SetInputPortName(inputPortNr, name);
        mInputPorts[inputPortNr].Clear();
        mInputPorts[inputPortNr].mCompatibleTypes[0] = MCore::AttributeBool::TYPE_ID;
        mInputPorts[inputPortNr].mCompatibleTypes[1] = MCore::AttributeFloat::TYPE_ID;;
        mInputPorts[inputPortNr].mCompatibleTypes[2] = MCore::AttributeInt32::TYPE_ID;
        mInputPorts[inputPortNr].mPortID = portID;
    }

    // setup a given input port in a generic way
    void AnimGraphNode::SetupInputPort(const char* name, uint32 inputPortNr, uint32 attributeTypeID, uint32 portID)
    {
        // check if we already registered this port ID
        const uint32 duplicatePort = FindInputPortByID(portID);
        if (duplicatePort != MCORE_INVALIDINDEX32)
        {
            MCore::LogError("EMotionFX::AnimGraphNode::SetInputPort() - There is already a port with the same ID (portID=%d existingPort='%s' newPort='%s' node='%s')", portID, mInputPorts[duplicatePort].GetName(), name, RTTI_GetTypeName());
        }

        SetInputPortName(inputPortNr, name);
        mInputPorts[inputPortNr].Clear();
        mInputPorts[inputPortNr].mCompatibleTypes[0] = attributeTypeID;
        mInputPorts[inputPortNr].mPortID = portID;

        // make sure we were able to create the attribute
        //MCORE_ASSERT( mInputPorts[inputPortNr].mValue );
    }


    void AnimGraphNode::RecursiveResetUniqueData(AnimGraphInstance* animGraphInstance)
    {
        ResetUniqueData(animGraphInstance);

        for (AnimGraphNode* childNode : mChildNodes)
        {
            childNode->RecursiveResetUniqueData(animGraphInstance);
        }
    }


    void AnimGraphNode::RecursiveOnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // some attributes have changed
        OnUpdateUniqueData(animGraphInstance);

        for (AnimGraphNode* childNode : mChildNodes)
        {
            childNode->RecursiveOnUpdateUniqueData(animGraphInstance);
        }
    }


    // get the input value for a given port
    const MCore::Attribute* AnimGraphNode::GetInputValue(AnimGraphInstance* animGraphInstance, uint32 inputPort) const
    {
        MCORE_UNUSED(animGraphInstance);

        // get the connection that is plugged into the port
        BlendTreeConnection* connection = mInputPorts[inputPort].mConnection;
        MCORE_ASSERT(connection); // make sure there is a connection plugged in, otherwise we can't read the value

        // get the value from the output port of the source node
        return connection->GetSourceNode()->GetOutputValue(animGraphInstance, connection->GetSourcePort());
    }


    // recursively reset the output is ready flag
    void AnimGraphNode::RecursiveResetFlags(AnimGraphInstance* animGraphInstance, uint32 flagsToReset)
    {
        // reset the flag in this node
        animGraphInstance->DisableObjectFlags(mObjectIndex, flagsToReset);

        if (GetEMotionFX().GetIsInEditorMode())
        {
            // reset all connections
            for (BlendTreeConnection* connection : mConnections)
            {
                connection->SetIsVisited(false);
            }
        }
        for (AnimGraphNode* childNode : mChildNodes)
        {
            childNode->RecursiveResetFlags(animGraphInstance, flagsToReset);
        }
    }


    // sync the current time with another node
    void AnimGraphNode::SyncPlayTime(AnimGraphInstance* animGraphInstance, AnimGraphNode* masterNode)
    {
        const float masterDuration = masterNode->GetDuration(animGraphInstance);
        const float normalizedTime = (masterDuration > MCore::Math::epsilon) ? masterNode->GetCurrentPlayTime(animGraphInstance) / masterDuration : 0.0f;
        SetCurrentPlayTimeNormalized(animGraphInstance, normalizedTime);
    }

    void AnimGraphNode::AutoSync(AnimGraphInstance* animGraphInstance, AnimGraphNode* masterNode, float weight, ESyncMode syncMode, bool resync)
    {
        // exit if we don't want to sync or we have no master node to sync to
        if (syncMode == SYNCMODE_DISABLED || masterNode == nullptr)
        {
            return;
        }

        // if one of the tracks is empty, sync the full clip
        if (syncMode == SYNCMODE_TRACKBASED)
        {
            // get the sync tracks
            const AnimGraphSyncTrack* syncTrackA = masterNode->FindUniqueNodeData(animGraphInstance)->GetSyncTrack();
            const AnimGraphSyncTrack* syncTrackB = FindUniqueNodeData(animGraphInstance)->GetSyncTrack();

            // if we have sync keys in both nodes, do the track based sync
            if (syncTrackA && syncTrackB && syncTrackA->GetNumEvents() > 0 && syncTrackB->GetNumEvents() > 0)
            {
                SyncUsingSyncTracks(animGraphInstance, masterNode, syncTrackA, syncTrackB, weight, resync, /*modifyMasterSpeed*/false);
                return;
            }
        }

        // we either have no evens inside the sync tracks in both nodes, or we just want to sync based on full clips
        SyncFullNode(animGraphInstance, masterNode, weight, /*modifyMasterSpeed*/false);
    }


    void AnimGraphNode::SyncFullNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* masterNode, float weight, bool modifyMasterSpeed)
    {
        SyncPlaySpeeds(animGraphInstance, masterNode, weight, modifyMasterSpeed);
        SyncPlayTime(animGraphInstance, masterNode);
    }


    // set the normalized time
    void AnimGraphNode::SetCurrentPlayTimeNormalized(AnimGraphInstance* animGraphInstance, float normalizedTime)
    {
        const float duration = GetDuration(animGraphInstance);
        SetCurrentPlayTime(animGraphInstance, normalizedTime * duration);
    }


    // sync blend the play speed of two nodes
    void AnimGraphNode::SyncPlaySpeeds(AnimGraphInstance* animGraphInstance, AnimGraphNode* masterNode, float weight, bool modifyMasterSpeed)
    {
        AnimGraphNodeData* uniqueDataA = masterNode->FindUniqueNodeData(animGraphInstance);
        AnimGraphNodeData* uniqueDataB = FindUniqueNodeData(animGraphInstance);

        const float durationA   = uniqueDataA->GetDuration();
        const float durationB   = uniqueDataB->GetDuration();
        const float timeRatio   = (durationB > MCore::Math::epsilon) ? durationA / durationB : 0.0f;
        const float timeRatio2  = (durationA > MCore::Math::epsilon) ? durationB / durationA : 0.0f;
        const float factorA     = MCore::LinearInterpolate<float>(1.0f, timeRatio, weight);
        const float factorB     = MCore::LinearInterpolate<float>(timeRatio2, 1.0f, weight);
        const float interpolatedSpeed   = MCore::LinearInterpolate<float>(uniqueDataA->GetPlaySpeed(), uniqueDataB->GetPlaySpeed(), weight);

        if (modifyMasterSpeed)
        {
            uniqueDataA->SetPlaySpeed(interpolatedSpeed * factorA);
        }

        uniqueDataB->SetPlaySpeed(interpolatedSpeed * factorB);
    }

    void AnimGraphNode::CalcSyncFactors(const AnimGraphInstance* animGraphInstance, const AnimGraphNode* masterNode, const AnimGraphNode* servantNode, ESyncMode syncMode, float weight, float* outMasterFactor, float* outServantFactor, float* outPlaySpeed)
    {
        const AnimGraphNodeData* masterUniqueData = masterNode->FindUniqueNodeData(animGraphInstance);
        const AnimGraphNodeData* servantUniqueData = servantNode->FindUniqueNodeData(animGraphInstance);

        CalcSyncFactors(masterUniqueData->GetPlaySpeed(), masterUniqueData->GetSyncTrack(), masterUniqueData->GetSyncIndex(), masterUniqueData->GetDuration(),
            servantUniqueData->GetPlaySpeed(), servantUniqueData->GetSyncTrack(), servantUniqueData->GetSyncIndex(), servantUniqueData->GetDuration(),
            syncMode, weight, outMasterFactor, outServantFactor, outPlaySpeed);
    }

    void AnimGraphNode::CalcSyncFactors(float masterPlaySpeed, const AnimGraphSyncTrack* masterSyncTrack, uint32 masterSyncTrackIndex, float masterDuration,
        float servantPlaySpeed, const AnimGraphSyncTrack* servantSyncTrack, uint32 servantSyncTrackIndex, float servantDuration,
        ESyncMode syncMode, float weight, float* outMasterFactor, float* outServantFactor, float* outPlaySpeed)
    {
        *outPlaySpeed = AZ::Lerp(masterPlaySpeed, servantPlaySpeed, weight);

        // exit if we don't want to sync or we have no master node to sync to
        if (syncMode == SYNCMODE_DISABLED)
        {
            *outMasterFactor = 1.0f;
            *outServantFactor = 1.0f;
            return;
        }

        // if one of the tracks is empty, sync the full clip
        if (syncMode == SYNCMODE_TRACKBASED)
        {
            // if we have sync keys in both nodes, do the track based sync
            if (masterSyncTrack && servantSyncTrack && masterSyncTrack->GetNumEvents() > 0 && servantSyncTrack->GetNumEvents() > 0)
            {
                // if the sync indices are invalid, act like no syncing
                if (masterSyncTrackIndex == MCORE_INVALIDINDEX32 || servantSyncTrackIndex == MCORE_INVALIDINDEX32)
                {
                    *outMasterFactor = 1.0f;
                    *outServantFactor = 1.0f;
                    return;
                }

                // get the segment lengths
                // TODO: handle motion clip start and end
                uint32 masterSyncIndexNext = masterSyncTrackIndex + 1;
                if (masterSyncIndexNext >= masterSyncTrack->GetNumEvents())
                {
                    masterSyncIndexNext = 0;
                }

                uint32 servantSyncIndexNext = servantSyncTrackIndex + 1;
                if (servantSyncIndexNext >= servantSyncTrack->GetNumEvents())
                {
                    servantSyncIndexNext = 0;
                }

                const float durationA = masterSyncTrack->CalcSegmentLength(masterSyncTrackIndex, masterSyncIndexNext);
                const float durationB = servantSyncTrack->CalcSegmentLength(servantSyncTrackIndex, servantSyncIndexNext);
                const float timeRatio = (durationB > MCore::Math::epsilon) ? durationA / durationB : 0.0f;
                const float timeRatio2 = (durationA > MCore::Math::epsilon) ? durationB / durationA : 0.0f;
                *outMasterFactor = AZ::Lerp(1.0f, timeRatio, weight);
                *outServantFactor = AZ::Lerp(timeRatio2, 1.0f, weight);
                return;
            }
        }

        // calculate the factor based on full clip sync
        const float timeRatio = (servantDuration > MCore::Math::epsilon) ? masterDuration / servantDuration : 0.0f;
        const float timeRatio2 = (masterDuration > MCore::Math::epsilon) ? servantDuration / masterDuration : 0.0f;
        *outMasterFactor = AZ::Lerp(1.0f, timeRatio, weight);
        *outServantFactor = AZ::Lerp(timeRatio2, 1.0f, weight);
    }


    // recursively call the on change motion set callback function
    void AnimGraphNode::RecursiveOnChangeMotionSet(AnimGraphInstance* animGraphInstance, MotionSet* newMotionSet)
    {
        // callback call
        OnChangeMotionSet(animGraphInstance, newMotionSet);

        // get the number of child nodes, iterate through them and recursively call this function
        const uint32 numChildNodes = GetNumChildNodes();
        for (uint32 i = 0; i < numChildNodes; ++i)
        {
            mChildNodes[i]->RecursiveOnChangeMotionSet(animGraphInstance, newMotionSet);
        }
    }


    // perform syncing using the sync tracks
    void AnimGraphNode::SyncUsingSyncTracks(AnimGraphInstance* animGraphInstance, AnimGraphNode* syncWithNode, const AnimGraphSyncTrack* syncTrackA, const AnimGraphSyncTrack* syncTrackB, float weight, bool resync, bool modifyMasterSpeed)
    {
        AnimGraphNode* nodeA = syncWithNode;
        AnimGraphNode* nodeB = this;

        AnimGraphNodeData* uniqueDataA = nodeA->FindUniqueNodeData(animGraphInstance);
        AnimGraphNodeData* uniqueDataB = nodeB->FindUniqueNodeData(animGraphInstance);

        // get the time of motion A
        const float currentTime = uniqueDataA->GetCurrentPlayTime();
        const bool forward = !uniqueDataA->GetIsBackwardPlaying();

        // get the event indices
        size_t firstIndexA;
        size_t firstIndexB;
        if (syncTrackA->FindEventIndices(currentTime, &firstIndexA, &firstIndexB) == false)
        {
            //MCORE_ASSERT(false);
            //MCore::LogInfo("FAILED FindEventIndices at time %f", currentTime);
            return;
        }

        // if the main motion changed event, we must make sure we also change it
        if (uniqueDataA->GetSyncIndex() != firstIndexA)
        {
            animGraphInstance->EnableObjectFlags(nodeA->GetObjectIndex(), AnimGraphInstance::OBJECTFLAGS_SYNCINDEX_CHANGED);
        }

        size_t startEventIndex = uniqueDataB->GetSyncIndex();
        if (animGraphInstance->GetIsObjectFlagEnabled(nodeA->GetObjectIndex(), AnimGraphInstance::OBJECTFLAGS_SYNCINDEX_CHANGED))
        {
            if (forward)
            {
                startEventIndex++;
            }
            else
            {
                startEventIndex--;
            }

            if (startEventIndex >= syncTrackB->GetNumEvents())
            {
                startEventIndex = 0;
            }

            if (startEventIndex == MCORE_INVALIDINDEX32)
            {
                startEventIndex = syncTrackB->GetNumEvents() - 1;
            }

            animGraphInstance->EnableObjectFlags(nodeB->GetObjectIndex(), AnimGraphInstance::OBJECTFLAGS_SYNCINDEX_CHANGED);
        }

        // find the matching indices in the second track
        size_t secondIndexA;
        size_t secondIndexB;
        if (resync == false)
        {
            if (syncTrackB->FindMatchingEvents(startEventIndex, syncTrackA->GetEvent(firstIndexA).HashForSyncing(uniqueDataA->GetIsMirrorMotion()), syncTrackA->GetEvent(firstIndexB).HashForSyncing(uniqueDataA->GetIsMirrorMotion()), &secondIndexA, &secondIndexB, forward, uniqueDataB->GetIsMirrorMotion()) == false)
            {
                //MCORE_ASSERT(false);
                //MCore::LogInfo("FAILED FindMatchingEvents");
                return;
            }
        }
        else // resyncing is required
        {
            const size_t occurrence = syncTrackA->CalcOccurrence(firstIndexA, firstIndexB, uniqueDataA->GetIsMirrorMotion());
            if (syncTrackB->ExtractOccurrence(occurrence, syncTrackA->GetEvent(firstIndexA).HashForSyncing(uniqueDataA->GetIsMirrorMotion()), syncTrackA->GetEvent(firstIndexB).HashForSyncing(uniqueDataA->GetIsMirrorMotion()), &secondIndexA, &secondIndexB, uniqueDataB->GetIsMirrorMotion()) == false)
            {
                //MCORE_ASSERT(false);
                //MCore::LogInfo("FAILED ExtractOccurrence");
                return;
            }
        }

        // update the sync indices
        uniqueDataA->SetSyncIndex(static_cast<uint32>(firstIndexA));
        uniqueDataB->SetSyncIndex(static_cast<uint32>(secondIndexA));

        // calculate the segment lengths
        const float firstSegmentLength  = syncTrackA->CalcSegmentLength(firstIndexA, firstIndexB);
        const float secondSegmentLength = syncTrackB->CalcSegmentLength(secondIndexA, secondIndexB);

        //MCore::LogInfo("t=%f firstA=%d firstB=%d occ=%d secA=%d secB=%d", currentTime, firstIndexA, firstIndexB, occurrence, secondIndexA, secondIndexB);

        // calculate the normalized offset inside the segment
        float normalizedOffset;
        if (firstIndexA < firstIndexB) // normal case
        {
            normalizedOffset = (firstSegmentLength > MCore::Math::epsilon) ? (currentTime - syncTrackA->GetEvent(firstIndexA).GetStartTime()) / firstSegmentLength : 0.0f;
        }
        else // looping case
        {
            float timeOffset;
            if (currentTime > syncTrackA->GetEvent(0).GetStartTime())
            {
                timeOffset = currentTime - syncTrackA->GetEvent(firstIndexA).GetStartTime();
            }
            else
            {
                timeOffset = (uniqueDataA->GetDuration() - syncTrackA->GetEvent(firstIndexA).GetStartTime()) + currentTime;
            }

            normalizedOffset = (firstSegmentLength > MCore::Math::epsilon) ? timeOffset / firstSegmentLength : 0.0f;
        }

        // get the durations of both nodes for later on
        const float durationA   = firstSegmentLength;
        const float durationB   = secondSegmentLength;

        // calculate the new time in the motion
        //  bool looped = false;
        float newTimeB;
        if (secondIndexA < secondIndexB) // if the second segment is a non-wrapping one, so a regular non-looping case
        {
            newTimeB = syncTrackB->GetEvent(secondIndexA).GetStartTime() + secondSegmentLength * normalizedOffset;
        }
        else // looping case
        {
            // calculate the new play time
            const float unwrappedTime = syncTrackB->GetEvent(secondIndexA).GetStartTime() + secondSegmentLength * normalizedOffset;

            // if it is past the motion duration, we need to wrap around
            if (unwrappedTime > uniqueDataB->GetDuration())
            {
                // the new wrapped time
                newTimeB = MCore::Math::SafeFMod(unwrappedTime, uniqueDataB->GetDuration()); // TODO: doesn't take into account the motion start and end clip times
            }
            else
            {
                newTimeB = unwrappedTime;
            }
        }

        // adjust the play speeds
        nodeB->SetCurrentPlayTime(animGraphInstance, newTimeB);
        const float timeRatio       = (durationB > MCore::Math::epsilon) ? durationA / durationB : 0.0f;
        const float timeRatio2      = (durationA > MCore::Math::epsilon) ? durationB / durationA : 0.0f;
        const float factorA         = MCore::LinearInterpolate<float>(1.0f, timeRatio, weight);
        const float factorB         = MCore::LinearInterpolate<float>(timeRatio2, 1.0f, weight);
        const float interpolatedSpeed   = MCore::LinearInterpolate<float>(uniqueDataA->GetPlaySpeed(), uniqueDataB->GetPlaySpeed(), weight);

        if (modifyMasterSpeed)
        {
            uniqueDataA->SetPlaySpeed(interpolatedSpeed * factorA);
        }

        uniqueDataB->SetPlaySpeed(interpolatedSpeed * factorB);
    }


    // check if the given node is the parent or the parent of the parent etc. of the node
    bool AnimGraphNode::RecursiveIsParentNode(AnimGraphNode* node) const
    {
        // if we're dealing with a root node we can directly return failure
        if (!mParentNode)
        {
            return false;
        }

        // check if the parent is the node and return success in that case
        if (mParentNode == node)
        {
            return true;
        }

        // check if the parent's parent is the node we're searching for
        return mParentNode->RecursiveIsParentNode(node);
    }


    // check if the given node is a child or a child of a chid etc. of the node
    bool AnimGraphNode::RecursiveIsChildNode(AnimGraphNode* node) const
    {
        // check if the given node is a child node of the current node
        if (FindChildNodeIndex(node) != MCORE_INVALIDINDEX32)
        {
            return true;
        }

        // get the number of child nodes, iterate through them and compare if the node is a child of the child nodes of this node
        for (const AnimGraphNode* childNode : mChildNodes)
        {
            if (childNode->RecursiveIsChildNode(node))
            {
                return true;
            }
        }

        // failure, the node isn't a child or a child of a child node
        return false;
    }


    void AnimGraphNode::SetHasError(AnimGraphInstance* animGraphInstance, bool hasError)
    {
        // check if the anim graph instance is valid
        //if (animGraphInstance == nullptr)
        //return;

        // nothing to change, return directly, only update when something changed
        if (GetHasErrorFlag(animGraphInstance) == hasError)
        {
            return;
        }

        // update the flag
        SetHasErrorFlag(animGraphInstance, hasError);

        // sync the current node
        SyncVisualObject();

        // in case the parent node is valid check the error status of the parent by checking all children recursively and set that value
        if (mParentNode)
        {
            mParentNode->SetHasError(animGraphInstance, mParentNode->HierarchicalHasError(animGraphInstance, true));
        }
    }


    bool AnimGraphNode::HierarchicalHasError(AnimGraphInstance* animGraphInstance, bool onlyCheckChildNodes) const
    {
        // check if the anim graph instance is valid
        //if (animGraphInstance == nullptr)
        //return true;

        if (onlyCheckChildNodes == false && GetHasErrorFlag(animGraphInstance))
        {
            return true;
        }

        for (const AnimGraphNode* childNode : mChildNodes)
        {
            if (childNode->GetHasErrorFlag(animGraphInstance))
            {
                return true;
            }
        }

        // no erroneous node found
        return false;
    }


    // collect child nodes of the given type
    void AnimGraphNode::CollectChildNodesOfType(const AZ::TypeId& nodeType, MCore::Array<AnimGraphNode*>* outNodes) const
    {
        for (AnimGraphNode* childNode : mChildNodes)
        {
            // check the current node type and add it to the output array in case they are the same
            if (azrtti_typeid(childNode) == nodeType)
            {
                outNodes->Add(childNode);
            }
        }
    }

    void AnimGraphNode::CollectChildNodesOfType(const AZ::TypeId& nodeType, AZStd::vector<AnimGraphNode*>& outNodes) const
    {
        for (AnimGraphNode* childNode : mChildNodes)
        {
            if (azrtti_typeid(childNode) == nodeType)
            {
                outNodes.push_back(childNode);
            }
        }
    }

    void AnimGraphNode::RecursiveCollectNodesOfType(const AZ::TypeId& nodeType, AZStd::vector<AnimGraphNode*>* outNodes) const
    {
        // check the current node type
        if (nodeType == azrtti_typeid(this))
        {
            outNodes->emplace_back(const_cast<AnimGraphNode*>(this));
        }

        for (const AnimGraphNode* childNode : mChildNodes)
        {
            childNode->RecursiveCollectNodesOfType(nodeType, outNodes);
        }
    }

    void AnimGraphNode::RecursiveCollectTransitionConditionsOfType(const AZ::TypeId& conditionType, MCore::Array<AnimGraphTransitionCondition*>* outConditions) const
    {
        // check if the current node is a state machine
        if (azrtti_typeid(this) == azrtti_typeid<AnimGraphStateMachine>())
        {
            // type cast the node to a state machine
            const AnimGraphStateMachine* stateMachine = static_cast<AnimGraphStateMachine*>(const_cast<AnimGraphNode*>(this));

            // get the number of transitions and iterate through them
            const size_t numTransitions = stateMachine->GetNumTransitions();
            for (size_t i = 0; i < numTransitions; ++i)
            {
                const AnimGraphStateTransition* transition = stateMachine->GetTransition(i);

                // get the number of conditions and iterate through them
                const size_t numConditions = transition->GetNumConditions();
                for (size_t j = 0; j < numConditions; ++j)
                {
                    // check if the given condition is of the given type and add it to the output array in this case
                    AnimGraphTransitionCondition* condition = transition->GetCondition(j);
                    if (azrtti_typeid(condition) == conditionType)
                    {
                        outConditions->Add(condition);
                    }
                }
            }
        }

        for (const AnimGraphNode* childNode : mChildNodes)
        {
            childNode->RecursiveCollectTransitionConditionsOfType(conditionType, outConditions);
        }
    }


    void AnimGraphNode::RecursiveCollectObjectsOfType(const AZ::TypeId& objectType, AZStd::vector<AnimGraphObject*>& outObjects) const
    {
        if (azrtti_istypeof(objectType, this))
        {
            outObjects.emplace_back(const_cast<AnimGraphNode*>(this));
        }

        for (const AnimGraphNode* childNode : mChildNodes)
        {
            childNode->RecursiveCollectObjectsOfType(objectType, outObjects);
        }
    }
    
    void AnimGraphNode::RecursiveCollectObjectsAffectedBy(AnimGraph* animGraph, AZStd::vector<AnimGraphObject*>& outObjects) const 
    {
        for (const AnimGraphNode* childNode : mChildNodes)
        {
            childNode->RecursiveCollectObjectsAffectedBy(animGraph, outObjects);
        }
    }

    void AnimGraphNode::OnStateEnter(AnimGraphInstance* animGraphInstance, AnimGraphNode* previousState, AnimGraphStateTransition* usedTransition)
    {
        MCORE_UNUSED(animGraphInstance);
        MCORE_UNUSED(previousState);
        MCORE_UNUSED(usedTransition);

        // Note: The enter action will only trigger when NOT entering from same state (because then you are not actually entering the state).
        if (this != previousState)
        {
            // Trigger the action at the enter of state.
            for (AnimGraphTriggerAction* action : m_actionSetup.GetActions())
            {
                if (action->GetTriggerMode() == AnimGraphTriggerAction::MODE_TRIGGERONENTER)
                {
                    action->TriggerAction(animGraphInstance);
                }
            }
        }
    }


    void AnimGraphNode::OnStateEnd(AnimGraphInstance* animGraphInstance, AnimGraphNode* newState, AnimGraphStateTransition* usedTransition)
    {
        MCORE_UNUSED(animGraphInstance);
        MCORE_UNUSED(newState);
        MCORE_UNUSED(usedTransition);

        // Note: The end of state action will only trigger when NOT entering the same state (because then you are not actually exiting the state).
        if (this != newState)
        {
            // Trigger the action when entering the state.
            for (AnimGraphTriggerAction* action : m_actionSetup.GetActions())
            {
                if (action->GetTriggerMode() == AnimGraphTriggerAction::MODE_TRIGGERONEXIT)
                {
                    action->TriggerAction(animGraphInstance);
                }
            }
        }
    }


    // find the input port, based on the port name
    AnimGraphNode::Port* AnimGraphNode::FindInputPortByName(const AZStd::string& portName)
    {
        for (Port& port : mInputPorts)
        {
            if (port.GetNameString() == portName)
            {
                return &port;
            }
        }
        return nullptr;
    }


    // find the output port, based on the port name
    AnimGraphNode::Port* AnimGraphNode::FindOutputPortByName(const AZStd::string& portName)
    {
        for (Port& port : mOutputPorts)
        {
            if (port.GetNameString() == portName)
            {
                return &port;
            }
        }
        return nullptr;
    }


    // find the input port index, based on the port id
    uint32 AnimGraphNode::FindInputPortByID(uint32 portID) const
    {
        const size_t numPorts = mInputPorts.size();
        for (size_t i = 0; i < numPorts; ++i)
        {
            if (mInputPorts[i].mPortID == portID)
            {
                return static_cast<uint32>(i);
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find the output port index, based on the port id
    uint32 AnimGraphNode::FindOutputPortByID(uint32 portID) const
    {
        const size_t numPorts = mOutputPorts.size();
        for (size_t i = 0; i < numPorts; ++i)
        {
            if (mOutputPorts[i].mPortID == portID)
            {
                return static_cast<uint32>(i);
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // this function will get called to rewind motion nodes as well as states etc. to reset several settings when a state gets exited
    void AnimGraphNode::Rewind(AnimGraphInstance* animGraphInstance)
    {
        // on default only reset the current time back to the start
        SetCurrentPlayTimeNormalized(animGraphInstance, 0.0f);
    }


    // collect all outgoing connections
    void AnimGraphNode::CollectOutgoingConnections(AZStd::vector<AZStd::pair<BlendTreeConnection*, AnimGraphNode*> >& outConnections) const
    {
        outConnections.clear();

        // if we don't have a parent node we cannot proceed
        if (!mParentNode)
        {
            return;
        }

        for (AnimGraphNode* childNode : mParentNode->GetChildNodes())
        {
            // Skip this child if the child is this node
            if (childNode == this)
            {
                continue;
            }

            for (BlendTreeConnection* connection : childNode->GetConnections())
            {
                // check if the connection comes from this node, if so add it to our output array
                if (connection->GetSourceNode() == this)
                {
                    outConnections.emplace_back(connection, childNode);
                }
            }
        }
    }


    void AnimGraphNode::CollectOutgoingConnections(AZStd::vector<AZStd::pair<BlendTreeConnection*, AnimGraphNode*> >& outConnections, const uint32 portIndex) const
    {
        outConnections.clear();

        if (!mParentNode)
        {
            return;
        }

        for (AnimGraphNode* childNode : mParentNode->GetChildNodes())
        {
            // Skip this child if the child is this node
            if (childNode == this)
            {
                continue;
            }

            for (BlendTreeConnection* connection : childNode->GetConnections())
            {
                if (connection->GetSourceNode() == this && connection->GetSourcePort() == portIndex)
                {
                    outConnections.emplace_back(connection, childNode);
                }
            }
        }
    }


    // find and return the connection connected to the given port
    BlendTreeConnection* AnimGraphNode::FindConnection(uint16 port) const
    {
        // get the number of connections and iterate through them
        const uint32 numConnections = GetNumConnections();
        for (uint32 i = 0; i < numConnections; ++i)
        {
            // get the current connection and check if the connection is connected to the given port
            BlendTreeConnection* connection = GetConnection(i);
            if (connection->GetTargetPort() == port)
            {
                return connection;
            }
        }

        // failure, there is no connection connected to the given port
        return nullptr;
    }


    BlendTreeConnection* AnimGraphNode::FindConnectionById(AnimGraphConnectionId connectionId) const
    {
        for (BlendTreeConnection* connection : mConnections)
        {
            if (connection->GetId() == connectionId)
            {
                return connection;
            }
        }

        return nullptr;
    }


    bool AnimGraphNode::HasConnectionAtInputPort(AZ::u32 inputPortNr) const
    {
        const Port& inputPort = mInputPorts[inputPortNr];
        return inputPort.mConnection != nullptr;
    }


    // callback that gets called before a node gets removed
    void AnimGraphNode::OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove)
    {
        for (AnimGraphNode* childNode : mChildNodes)
        {
            childNode->OnRemoveNode(animGraph, nodeToRemove);
        }
    }


    // collect internal objects
    void AnimGraphNode::RecursiveCollectObjects(MCore::Array<AnimGraphObject*>& outObjects) const
    {
        outObjects.Add(const_cast<AnimGraphNode*>(this));

        for (const AnimGraphNode* childNode : mChildNodes)
        {
            childNode->RecursiveCollectObjects(outObjects);
        }
    }


    // topdown update
    void AnimGraphNode::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        //if (mDisabled)
        //return;

        // get the unique data
        AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
        HierarchicalSyncAllInputNodes(animGraphInstance, uniqueData);

        // top down update all incoming connections
        for (BlendTreeConnection* connection : mConnections)
        {
            connection->GetSourceNode()->TopDownUpdate(animGraphInstance, timePassedInSeconds);
        }
    }

    // default update implementation
    void AnimGraphNode::Output(AnimGraphInstance* animGraphInstance)
    {
        OutputAllIncomingNodes(animGraphInstance);
    }


    // default update implementation
    void AnimGraphNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // get the unique data
        AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);

        // iterate over all incoming connections
        bool syncTrackFound = false;
        size_t connectionIndex = MCORE_INVALIDINDEX32;
        const size_t numConnections = mConnections.size();
        for (size_t i = 0; i < numConnections; ++i)
        {
            const BlendTreeConnection* connection = mConnections[i];
            AnimGraphNode* sourceNode = connection->GetSourceNode();

            // update the node
            UpdateIncomingNode(animGraphInstance, sourceNode, timePassedInSeconds);

            // use the sync track of the first input port of this node
            if (connection->GetTargetPort() == 0 && sourceNode->GetHasOutputPose())
            {
                syncTrackFound = true;
                connectionIndex = i;
            }
        }

        if (connectionIndex != MCORE_INVALIDINDEX32)
        {
            uniqueData->Init(animGraphInstance, mConnections[connectionIndex]->GetSourceNode());
        }

        // set the current sync track to the first input connection
        if (!syncTrackFound && numConnections > 0 && mConnections[0]->GetSourceNode()->GetHasOutputPose()) // just pick the first connection's sync track
        {
            uniqueData->Init(animGraphInstance, mConnections[0]->GetSourceNode());
        }
    }


    // output all incoming nodes
    void AnimGraphNode::OutputAllIncomingNodes(AnimGraphInstance* animGraphInstance)
    {
        for (const BlendTreeConnection* connection : mConnections)
        {
            OutputIncomingNode(animGraphInstance, connection->GetSourceNode());
        }
    }


    // update a specific node
    void AnimGraphNode::UpdateIncomingNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* node, float timePassedInSeconds)
    {
        if (!node)
        {
            return;
        }
        node->PerformUpdate(animGraphInstance, timePassedInSeconds);
    }


    // update all incoming nodes
    void AnimGraphNode::UpdateAllIncomingNodes(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        for (const BlendTreeConnection* connection : mConnections)
        {
            AnimGraphNode* sourceNode = connection->GetSourceNode();
            sourceNode->PerformUpdate(animGraphInstance, timePassedInSeconds);
        }
    }


    // mark the connections going to a given node as visited
    void AnimGraphNode::MarkConnectionVisited(AnimGraphNode* sourceNode)
    {
        if (GetEMotionFX().GetIsInEditorMode())
        {
            for (BlendTreeConnection* connection : mConnections)
            {
                if (connection->GetSourceNode() == sourceNode)
                {
                    connection->SetIsVisited(true);
                }
            }
        }
    }


    // output a node
    void AnimGraphNode::OutputIncomingNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* nodeToOutput)
    {
        if (nodeToOutput == nullptr)
        {
            return;
        }

        // output the node
        nodeToOutput->PerformOutput(animGraphInstance);

        // mark any connection originating from this node as visited
        if (GetEMotionFX().GetIsInEditorMode())
        {
            for (BlendTreeConnection* connection : mConnections)
            {
                if (connection->GetSourceNode() == nodeToOutput)
                {
                    connection->SetIsVisited(true);
                }
            }
        }
    }



    // Process events and motion extraction delta.
    void AnimGraphNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // Post process all incoming nodes.
        bool poseFound = false;
        size_t connectionIndex = MCORE_INVALIDINDEX32;
        AZ::u16 minTargetPortIndex = MCORE_INVALIDINDEX16;
        const size_t numConnections = mConnections.size();
        for (size_t i = 0; i < numConnections; ++i)
        {
            const BlendTreeConnection* connection = mConnections[i];
            AnimGraphNode* sourceNode = connection->GetSourceNode();

            // update the node
            sourceNode->PerformPostUpdate(animGraphInstance, timePassedInSeconds);

            // If the input node has no pose, we can skip to the next connection.
            if (!sourceNode->GetHasOutputPose())
            {
                continue;
            }

            // Find the first connection that plugs into a port that can take a pose.
            const AZ::u16 targetPortIndex = connection->GetTargetPort();
            if (mInputPorts[targetPortIndex].mCompatibleTypes[0] == AttributePose::TYPE_ID)
            {
                poseFound = true;
                if (targetPortIndex < minTargetPortIndex)
                {
                    connectionIndex = i;
                    minTargetPortIndex = targetPortIndex;
                }
            }
        }

        // request the anim graph reference counted data objects
        RequestRefDatas(animGraphInstance);

        AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
        if (poseFound && connectionIndex != MCORE_INVALIDINDEX32)
        {
            const BlendTreeConnection* connection = mConnections[connectionIndex];
            AnimGraphNode* sourceNode = connection->GetSourceNode();

            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            AnimGraphRefCountedData* sourceData = sourceNode->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();

            if (sourceData)
            {
                data->SetEventBuffer(sourceData->GetEventBuffer());
                data->SetTrajectoryDelta(sourceData->GetTrajectoryDelta());
                data->SetTrajectoryDeltaMirrored(sourceData->GetTrajectoryDeltaMirrored());
            }
        }
        else
        if (poseFound == false && numConnections > 0 && mConnections[0]->GetSourceNode()->GetHasOutputPose())
        {
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            AnimGraphNode* sourceNode = mConnections[0]->GetSourceNode();
            AnimGraphRefCountedData* sourceData = sourceNode->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();
            data->SetEventBuffer(sourceData->GetEventBuffer());
            data->SetTrajectoryDelta(sourceData->GetTrajectoryDelta());
            data->SetTrajectoryDeltaMirrored(sourceData->GetTrajectoryDeltaMirrored());
        }
        else
        {
            if (poseFound == false)
            {
                AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
                data->ClearEventBuffer();
                data->ZeroTrajectoryDelta();
            }
        }
    }



    // recursively set object data flag
    void AnimGraphNode::RecursiveSetUniqueDataFlag(AnimGraphInstance* animGraphInstance, uint32 flag, bool enabled)
    {
        // set the flag
        animGraphInstance->SetObjectFlags(mObjectIndex, flag, enabled);

        // recurse downwards
        for (BlendTreeConnection* connection : mConnections)
        {
            connection->GetSourceNode()->RecursiveSetUniqueDataFlag(animGraphInstance, flag, enabled);
        }
    }

    void AnimGraphNode::FilterEvents(AnimGraphInstance* animGraphInstance, EEventMode eventMode, AnimGraphNode* nodeA, AnimGraphNode* nodeB, float localWeight, AnimGraphRefCountedData* refData)
    {
        AnimGraphRefCountedData* refDataA = nullptr;
        if (nodeA)
        {
            refDataA = nodeA->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();
        }

        FilterEvents(animGraphInstance, eventMode, refDataA, nodeB, localWeight, refData);
    }

    // filter the event based on a given event mode
    void AnimGraphNode::FilterEvents(AnimGraphInstance* animGraphInstance, EEventMode eventMode, AnimGraphRefCountedData* refDataNodeA, AnimGraphNode* nodeB, float localWeight, AnimGraphRefCountedData* refData)
    {
        switch (eventMode)
        {
        // Output nothing, so clear the output buffer.
        case EVENTMODE_NONE:
        {
            if (refData)
            {
                refData->GetEventBuffer().Clear();
            }
        }
        break;

        // only the master
        case EVENTMODE_MASTERONLY:
        {
            if (refDataNodeA)
            {
                refData->SetEventBuffer(refDataNodeA->GetEventBuffer());
            }
        }
        break;

        // only the slave
        case EVENTMODE_SLAVEONLY:
        {
            if (nodeB)
            {
                AnimGraphRefCountedData* refDataNodeB = nodeB->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();
                if (refDataNodeB)
                {
                    refData->SetEventBuffer(refDataNodeB->GetEventBuffer());
                }
            }
            else if (refDataNodeA)
            {
                refData->SetEventBuffer(refDataNodeA->GetEventBuffer()); // master is also slave
            }
        }
        break;

        // both nodes
        case EVENTMODE_BOTHNODES:
        {
            AnimGraphRefCountedData* refDataNodeB = nodeB ? nodeB->FindUniqueNodeData(animGraphInstance)->GetRefCountedData() : nullptr;

            const uint32 numEventsA = refDataNodeA ? refDataNodeA->GetEventBuffer().GetNumEvents() : 0;
            const uint32 numEventsB = refDataNodeB ? refDataNodeB->GetEventBuffer().GetNumEvents() : 0;

            // resize to the right number of events already
            AnimGraphEventBuffer& eventBuffer = refData->GetEventBuffer();
            eventBuffer.Resize(numEventsA + numEventsB);

            // add the first node's events
            if (refDataNodeA)
            {
                const AnimGraphEventBuffer& eventBufferA = refDataNodeA->GetEventBuffer();
                for (uint32 i = 0; i < numEventsA; ++i)
                {
                    eventBuffer.SetEvent(i, eventBufferA.GetEvent(i));
                }
            }

            if (refDataNodeB)
            {
                const AnimGraphEventBuffer& eventBufferB = refDataNodeB->GetEventBuffer();
                for (uint32 i = 0; i < numEventsB; ++i)
                {
                    eventBuffer.SetEvent(numEventsA + i, eventBufferB.GetEvent(i));
                }
            }
        }
        break;

        // most active node's events
        case EVENTMODE_MOSTACTIVE:
        {
            // if the weight is lower than half, use the first node's events
            if (localWeight <= 0.5f)
            {
                if (refDataNodeA)
                {
                    refData->SetEventBuffer(refDataNodeA->GetEventBuffer());
                }
            }
            else // try to use the second node's events
            {
                if (nodeB)
                {
                    AnimGraphRefCountedData* refDataNodeB = nodeB->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();
                    if (refDataNodeB)
                    {
                        refData->SetEventBuffer(refDataNodeB->GetEventBuffer());
                    }
                }
                else if (refDataNodeA)
                {
                       refData->SetEventBuffer(refDataNodeA->GetEventBuffer()); // master is also slave
                }
            }
        }
        break;

        default:
            AZ_Assert(false, "Unknown event filter mode used!");
        };
    }


    // hierarchically sync input a given input node
    void AnimGraphNode::HierarchicalSyncInputNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* inputNode, AnimGraphNodeData* uniqueDataOfThisNode)
    {
        AnimGraphNodeData* inputUniqueData = inputNode->FindUniqueNodeData(animGraphInstance);

        if (animGraphInstance->GetIsSynced(inputNode->GetObjectIndex()))
        {
            inputNode->AutoSync(animGraphInstance, this, 0.0f, SYNCMODE_TRACKBASED, false);
        }
        else
        {
            inputUniqueData->SetPlaySpeed(uniqueDataOfThisNode->GetPlaySpeed()); // default child node speed propagation in case it is not synced
        }
        // pass the global weight along to the child nodes
        inputUniqueData->SetGlobalWeight(uniqueDataOfThisNode->GetGlobalWeight());
        inputUniqueData->SetLocalWeight(1.0f);
    }


    // hierarchically sync all input nodes
    void AnimGraphNode::HierarchicalSyncAllInputNodes(AnimGraphInstance* animGraphInstance, AnimGraphNodeData* uniqueDataOfThisNode)
    {
        // for all connections
        for (const BlendTreeConnection* connection : mConnections)
        {
            AnimGraphNode* inputNode = connection->GetSourceNode();
            HierarchicalSyncInputNode(animGraphInstance, inputNode, uniqueDataOfThisNode);
        }
    }


    // on default create a base class object
    void AnimGraphNode::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // try to find existing data
        AnimGraphNodeData* data = animGraphInstance->FindUniqueNodeData(this);
        if (data == nullptr) // doesn't exist
        {
            AnimGraphNodeData* newData = aznew AnimGraphNodeData(this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(newData);
        }

        OnUpdateTriggerActionsUniqueData(animGraphInstance);
    }

    void AnimGraphNode::OnUpdateTriggerActionsUniqueData(AnimGraphInstance* animGraphInstance)
    {
        const size_t numActions = m_actionSetup.GetNumActions();
        for (size_t a = 0; a < numActions; ++a)
        {
            AnimGraphTriggerAction* action = m_actionSetup.GetAction(a);
            action->OnUpdateUniqueData(animGraphInstance);
        }
    }


    // recursively collect active animgraph nodes
    void AnimGraphNode::RecursiveCollectActiveNodes(AnimGraphInstance* animGraphInstance, MCore::Array<AnimGraphNode*>* outNodes, const AZ::TypeId& nodeType) const
    {
        // check and add this node
        if (azrtti_typeid(this) == nodeType || nodeType.IsNull())
        {
            if (animGraphInstance->GetIsOutputReady(mObjectIndex)) // if we processed this node
            {
                outNodes->Add(const_cast<AnimGraphNode*>(this));
            }
        }

        // process all child nodes (but only active ones)
        for (const AnimGraphNode* childNode : mChildNodes)
        {
            if (animGraphInstance->GetIsOutputReady(childNode->GetObjectIndex()))
            {
                childNode->RecursiveCollectActiveNodes(animGraphInstance, outNodes, nodeType);
            }
        }
    }


    void AnimGraphNode::RecursiveCollectActiveNetTimeSyncNodes(AnimGraphInstance * animGraphInstance, AZStd::vector<AnimGraphNode*>* outNodes) const
    {
        // Check and add this node
        if (GetNeedsNetTimeSync())
        {
            if (animGraphInstance->GetIsOutputReady(mObjectIndex)) // if we processed this node
            {
                outNodes->emplace_back(const_cast<AnimGraphNode*>(this));
            }
        }

        // Process all child nodes (but only active ones)
        for (const AnimGraphNode* childNode : mChildNodes)
        {
            if (animGraphInstance->GetIsOutputReady(childNode->GetObjectIndex()))
            {
                childNode->RecursiveCollectActiveNetTimeSyncNodes(animGraphInstance, outNodes);
            }
        }
    }


    bool AnimGraphNode::RecursiveDetectCycles(AZStd::unordered_set<const AnimGraphNode*>& nodes) const
    {
        for (const AnimGraphNode* childNode : mChildNodes)
        {
            if (nodes.find(childNode) != nodes.end())
            {
                return true;
            }

            if (childNode->RecursiveDetectCycles(nodes))
            {
                return true;
            }

            nodes.emplace(childNode);
        }

        return false;
    }


    // decrease the reference count
    void AnimGraphNode::DecreaseRef(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
        if (uniqueData->GetPoseRefCount() == 0)
        {
            return;
        }

        uniqueData->DecreasePoseRefCount();
        if (uniqueData->GetPoseRefCount() > 0 || !GetHasOutputPose())
        {
            return;
        }

        //AnimGraphNodeData* uniqueData = animGraphInstance->FindUniqueNodeData(this);
        const uint32 threadIndex = animGraphInstance->GetActorInstance()->GetThreadIndex();
        AnimGraphPosePool& posePool = GetEMotionFX().GetThreadData(threadIndex)->GetPosePool();
        const size_t numOutputs = mOutputPorts.size();
        for (size_t i = 0; i < numOutputs; ++i)
        {
            if (mOutputPorts[i].mCompatibleTypes[0] == AttributePose::TYPE_ID)
            {
                MCore::Attribute* attribute = GetOutputAttribute(animGraphInstance, static_cast<uint32>(i));
                MCORE_ASSERT(attribute->GetType() == AttributePose::TYPE_ID);

                AttributePose* poseAttribute = static_cast<AttributePose*>(attribute);
                AnimGraphPose* pose = poseAttribute->GetValue();
                if (pose)
                {
                    posePool.FreePose(pose);
                }
                poseAttribute->SetValue(nullptr);
            }
        }
    }


    // request poses from the pose cache for all output poses
    void AnimGraphNode::RequestPoses(AnimGraphInstance* animGraphInstance)
    {
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        const uint32 threadIndex = actorInstance->GetThreadIndex();

        AnimGraphPosePool& posePool = GetEMotionFX().GetThreadData(threadIndex)->GetPosePool();

        const size_t numOutputs = mOutputPorts.size();
        for (size_t i = 0; i < numOutputs; ++i)
        {
            if (mOutputPorts[i].mCompatibleTypes[0] == AttributePose::TYPE_ID)
            {
                MCore::Attribute* attribute = GetOutputAttribute(animGraphInstance, static_cast<uint32>(i));
                MCORE_ASSERT(attribute->GetType() == AttributePose::TYPE_ID);

                AnimGraphPose* pose = posePool.RequestPose(actorInstance);
                AttributePose* poseAttribute = static_cast<AttributePose*>(attribute);
                poseAttribute->SetValue(pose);
            }
        }
    }


    // free all poses from all incoming nodes
    void AnimGraphNode::FreeIncomingPoses(AnimGraphInstance* animGraphInstance)
    {
        for (Port& inputPort : mInputPorts)
        {
            BlendTreeConnection* connection = inputPort.mConnection;
            if (connection)
            {
                AnimGraphNode* sourceNode = connection->GetSourceNode();
                sourceNode->DecreaseRef(animGraphInstance);
            }
        }
    }


    // free all poses from all incoming nodes
    void AnimGraphNode::FreeIncomingRefDatas(AnimGraphInstance* animGraphInstance)
    {
        for (Port& port : mInputPorts)
        {
            BlendTreeConnection* connection = port.mConnection;
            if (connection)
            {
                AnimGraphNode* sourceNode = connection->GetSourceNode();
                sourceNode->DecreaseRefDataRef(animGraphInstance);
            }
        }
    }


    // request poses from the pose cache for all output poses
    void AnimGraphNode::RequestRefDatas(AnimGraphInstance* animGraphInstance)
    {
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        const uint32 threadIndex = actorInstance->GetThreadIndex();

        AnimGraphRefCountedDataPool& pool = GetEMotionFX().GetThreadData(threadIndex)->GetRefCountedDataPool();
        AnimGraphRefCountedData* newData = pool.RequestNew();

        FindUniqueNodeData(animGraphInstance)->SetRefCountedData(newData);
    }


    // decrease the reference count
    void AnimGraphNode::DecreaseRefDataRef(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
        if (uniqueData->GetRefDataRefCount() == 0)
        {
            return;
        }

        uniqueData->DecreaseRefDataRefCount();
        if (uniqueData->GetRefDataRefCount() > 0)
        {
            return;
        }

        // free it
        const uint32 threadIndex = animGraphInstance->GetActorInstance()->GetThreadIndex();
        AnimGraphRefCountedDataPool& pool = GetEMotionFX().GetThreadData(threadIndex)->GetRefCountedDataPool();
        if (uniqueData->GetRefCountedData())
        {
            pool.Free(uniqueData->GetRefCountedData());
            uniqueData->SetRefCountedData(nullptr);
        }
    }


    // perform top down update
    void AnimGraphNode::PerformTopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // check if we already did update
        if (animGraphInstance->GetIsTopDownUpdateReady(mObjectIndex))
        {
            return;
        }

        TopDownUpdate(animGraphInstance, timePassedInSeconds);

        // mark as done
        animGraphInstance->EnableObjectFlags(mObjectIndex, AnimGraphInstance::OBJECTFLAGS_TOPDOWNUPDATE_READY);
    }


    // perform post update
    void AnimGraphNode::PerformPostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // check if we already did update
        if (animGraphInstance->GetIsPostUpdateReady(mObjectIndex))
        {
            return;
        }

        // perform the actual post update
        PostUpdate(animGraphInstance, timePassedInSeconds);

        // free the incoming refs
        FreeIncomingRefDatas(animGraphInstance);

        // mark as done
        animGraphInstance->EnableObjectFlags(mObjectIndex, AnimGraphInstance::OBJECTFLAGS_POSTUPDATE_READY);
    }


    // perform an update
    void AnimGraphNode::PerformUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // check if we already did update
        if (animGraphInstance->GetIsUpdateReady(mObjectIndex))
        {
            return;
        }

        // increase ref count for incoming nodes
        IncreaseInputRefCounts(animGraphInstance);
        IncreaseInputRefDataRefCounts(animGraphInstance);

        // perform the actual node update
        Update(animGraphInstance, timePassedInSeconds);

        // mark as output
        animGraphInstance->EnableObjectFlags(mObjectIndex, AnimGraphInstance::OBJECTFLAGS_UPDATE_READY);
    }


    //
    void AnimGraphNode::PerformOutput(AnimGraphInstance* animGraphInstance)
    {
        // check if we already did output
        if (animGraphInstance->GetIsOutputReady(mObjectIndex))
        {
            return;
        }

        // perform the output
        Output(animGraphInstance);

        // now decrease ref counts of all input nodes as we do not need the poses of this input node anymore for this node
        // once the pose ref count of a node reaches zero it will automatically release the poses back to the pool so they can be reused again by others
        FreeIncomingPoses(animGraphInstance);

        // mark as output
        animGraphInstance->EnableObjectFlags(mObjectIndex, AnimGraphInstance::OBJECTFLAGS_OUTPUT_READY);
    }


    // increase input ref counts
    void AnimGraphNode::IncreaseInputRefDataRefCounts(AnimGraphInstance* animGraphInstance)
    {
        for (Port& port : mInputPorts)
        {
            BlendTreeConnection* connection = port.mConnection;
            if (connection)
            {
                AnimGraphNode* sourceNode = connection->GetSourceNode();
                sourceNode->IncreaseRefDataRefCount(animGraphInstance);
            }
        }
    }


    // increase input ref counts
    void AnimGraphNode::IncreaseInputRefCounts(AnimGraphInstance* animGraphInstance)
    {
        for (Port& port : mInputPorts)
        {
            BlendTreeConnection* connection = port.mConnection;
            if (connection)
            {
                AnimGraphNode* sourceNode = connection->GetSourceNode();
                sourceNode->IncreasePoseRefCount(animGraphInstance);
            }
        }
    }


    void AnimGraphNode::RelinkPortConnections()
    {
        // After deserializing, nodes hold an array of incoming connections. Each node port caches a pointer to its connection object which we need to link.
        for (BlendTreeConnection* connection : mConnections)
        {
            AnimGraphNode* sourceNode = connection->GetSourceNode();
            const AZ::u16 targetPortNr = connection->GetTargetPort();
            const AZ::u16 sourcePortNr = connection->GetSourcePort();

            if (sourceNode)
            {
                if (sourcePortNr < sourceNode->mOutputPorts.size())
                {
                    sourceNode->GetOutputPort(sourcePortNr).mConnection = connection;
                }
                else
                {
                    AZ_Error("EMotionFX", false, "Can't link output port %i of '%s' with the connection going to %s at port %i.", sourcePortNr, sourceNode->GetName(), GetName(), targetPortNr);
                }
            }

            if (targetPortNr < mInputPorts.size())
            {
                mInputPorts[targetPortNr].mConnection = connection;
            }
            else
            {
                AZ_Error("EMotionFX", false, "Can't link input port %i of '%s' with the connection coming from %s at port %i.", targetPortNr, GetName(), sourceNode->GetName(), sourcePortNr);
            }
        }
    }


    // do we have a child of a given type?
    bool AnimGraphNode::CheckIfHasChildOfType(const AZ::TypeId& nodeType) const
    {
        for (const AnimGraphNode* childNode : mChildNodes)
        {
            if (azrtti_typeid(childNode) == nodeType)
            {
                return true;
            }
        }

        return false;
    }


    // check if we can visualize
    bool AnimGraphNode::GetCanVisualize(AnimGraphInstance* animGraphInstance) const
    {
        return (mVisEnabled && animGraphInstance->GetVisualizationEnabled() && EMotionFX::GetRecorder().GetIsInPlayMode() == false);
    }


    // remove internal attributes in all anim graph instances
    void AnimGraphNode::RemoveInternalAttributesForAllInstances()
    {
        // for all output ports
        for (Port& port : mOutputPorts)
        {
            const uint32 internalAttributeIndex = port.mAttributeIndex;
            if (internalAttributeIndex != MCORE_INVALIDINDEX32)
            {
                const size_t numInstances = mAnimGraph->GetNumAnimGraphInstances();
                for (size_t i = 0; i < numInstances; ++i)
                {
                    AnimGraphInstance* animGraphInstance = mAnimGraph->GetAnimGraphInstance(i);
                    animGraphInstance->RemoveInternalAttribute(internalAttributeIndex);
                }

                mAnimGraph->DecreaseInternalAttributeIndices(internalAttributeIndex);
                port.mAttributeIndex = MCORE_INVALIDINDEX32;
            }
        }
    }


    // decrease values higher than a given param value
    void AnimGraphNode::DecreaseInternalAttributeIndices(uint32 decreaseEverythingHigherThan)
    {
        for (Port& port : mOutputPorts)
        {
            if (port.mAttributeIndex > decreaseEverythingHigherThan && port.mAttributeIndex != MCORE_INVALIDINDEX32)
            {
                port.mAttributeIndex--;
            }
        }
    }


    // init the internal attributes
    void AnimGraphNode::InitInternalAttributes(AnimGraphInstance* animGraphInstance)
    {
        // for all output ports of this node
        for (Port& port : mOutputPorts)
        {
            MCore::Attribute* newAttribute = MCore::GetAttributeFactory().CreateAttributeByType(port.mCompatibleTypes[0]); // assume compatibility type 0 to be the attribute type ID
            port.mAttributeIndex = animGraphInstance->AddInternalAttribute(newAttribute);
        }
    }


    void* AnimGraphNode::GetCustomData() const
    {
        return mCustomData;
    }


    void AnimGraphNode::SetCustomData(void* dataPointer)
    {
        mCustomData = dataPointer;
    }


    void AnimGraphNode::SetNodeInfo(const AZStd::string& info)
    {
        if (mNodeInfo != info)
        {
            mNodeInfo = info;

            SyncVisualObject();
        }
    }


    const AZStd::string& AnimGraphNode::GetNodeInfo() const
    {
        return mNodeInfo;
    }


    bool AnimGraphNode::GetIsEnabled() const
    {
        return (mDisabled == false);
    }


    void AnimGraphNode::SetIsEnabled(bool enabled)
    {
        mDisabled = !enabled;
    }


    bool AnimGraphNode::GetIsCollapsed() const
    {
        return mIsCollapsed;
    }


    void AnimGraphNode::SetIsCollapsed(bool collapsed)
    {
        mIsCollapsed = collapsed;
    }


    void AnimGraphNode::SetVisualizeColor(const AZ::Color& color)
    {
        mVisualizeColor = color;
        SyncVisualObject();
    }


    const AZ::Color& AnimGraphNode::GetVisualizeColor() const
    {
        return mVisualizeColor;
    }


    void AnimGraphNode::SetVisualPos(int32 x, int32 y)
    {
        mPosX = x;
        mPosY = y;
    }


    int32 AnimGraphNode::GetVisualPosX() const
    {
        return mPosX;
    }


    int32 AnimGraphNode::GetVisualPosY() const
    {
        return mPosY;
    }


    bool AnimGraphNode::GetIsVisualizationEnabled() const
    {
        return mVisEnabled;
    }


    void AnimGraphNode::SetVisualization(bool enabled)
    {
        mVisEnabled = enabled;
    }


    void AnimGraphNode::AddChildNode(AnimGraphNode* node)
    {
        mChildNodes.push_back(node);
        node->SetParentNode(this);
    }


    void AnimGraphNode::ReserveChildNodes(uint32 numChildNodes)
    {
        mChildNodes.reserve(numChildNodes);
    }


    const char* AnimGraphNode::GetName() const
    {
        return m_name.c_str();
    }


    const AZStd::string& AnimGraphNode::GetNameString() const
    {
        return m_name;
    }


    void AnimGraphNode::SetName(const char* name)
    {
        m_name = name;
    }


    void AnimGraphNode::GetAttributeStringForAffectedNodeIds(const AZStd::unordered_map<AZ::u64, AZ::u64>& convertedIds, AZStd::string& attributesString) const
    {
        // Default implementation is that the transition is not affected, therefore we don't do anything
        AZ_UNUSED(convertedIds);
        AZ_UNUSED(attributesString);
    }


    bool AnimGraphNode::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        const unsigned int version = classElement.GetVersion();
        if (version < 2)
        {
            int vizColorIndex = classElement.FindElement(AZ_CRC("visualizeColor", 0x6d52f421));
            if (vizColorIndex > 0)
            {
                AZ::u32 oldColor;
                AZ::SerializeContext::DataElementNode& dataElementNode = classElement.GetSubElement(vizColorIndex);
                const bool result = dataElementNode.GetData<AZ::u32>(oldColor);
                if (!result)
                {
                    return false;
                }
                const AZ::Color convertedColor(
                    MCore::ExtractRed(oldColor)/255.0f,
                    MCore::ExtractGreen(oldColor)/255.0f,
                    MCore::ExtractBlue(oldColor)/255.0f,
                    1.0f
                );
                classElement.RemoveElement(vizColorIndex);
                classElement.AddElementWithData(context, "visualizeColor", convertedColor);
            }
        }
        return true;
    }


    void AnimGraphNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphNode, AnimGraphObject>()
            ->Version(2, VersionConverter)
            ->PersistentId([](const void* instance) -> AZ::u64 { return static_cast<const AnimGraphNode*>(instance)->GetId(); })
            ->Field("id", &AnimGraphNode::m_id)
            ->Field("name", &AnimGraphNode::m_name)
            ->Field("posX", &AnimGraphNode::mPosX)
            ->Field("posY", &AnimGraphNode::mPosY)
            ->Field("visualizeColor", &AnimGraphNode::mVisualizeColor)
            ->Field("isDisabled", &AnimGraphNode::mDisabled)
            ->Field("isCollapsed", &AnimGraphNode::mIsCollapsed)
            ->Field("isVisEnabled", &AnimGraphNode::mVisEnabled)
            ->Field("childNodes", &AnimGraphNode::mChildNodes)
            ->Field("connections", &AnimGraphNode::mConnections)
            ->Field("actionSetup", &AnimGraphNode::m_actionSetup);
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphNode>("AnimGraphNode", "Base anim graph node")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ_CRC("AnimGraphNodeName", 0x15120d7d), &AnimGraphNode::m_name, "Name", "Name of the node")
            ->Attribute(AZ_CRC("AnimGraph", 0x0d53d4b3), &AnimGraphNode::GetAnimGraph)
        ;
    }
} // namespace EMotionFX
