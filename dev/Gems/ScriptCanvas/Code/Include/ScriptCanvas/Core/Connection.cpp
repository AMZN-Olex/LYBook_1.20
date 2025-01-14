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


#include "Connection.h"
#include "Slot.h"
#include "Node.h"
#include "NodeBus.h"

#include <AzCore/Serialization/SerializeContext.h>

namespace ScriptCanvas
{
    AZ::Outcome<void, AZStd::string> MatchContracts(Slot& firstSlot, Slot& secondSlot)
    {
        AZ::Outcome<void, AZStd::string> outcome = AZ::Success();

        AZStd::string failedContract;
        AZStd::all_of(firstSlot.GetContracts().begin(), firstSlot.GetContracts().end(), [&firstSlot, &secondSlot, &outcome](const AZStd::unique_ptr<Contract>& contract)
        {
            outcome = contract->Evaluate(firstSlot, secondSlot);
            if (outcome.IsSuccess())
            {
                return true;
            }

            return false;
        }); 

        return outcome;
    }

    Connection::Connection(const ID& fromNode, const SlotId& fromSlot, const ID& toNode, const SlotId& toSlot)
        : m_sourceEndpoint(fromNode, fromSlot)
        , m_targetEndpoint(toNode, toSlot)
    {
    }

    Connection::Connection(const Endpoint& fromConnection, const Endpoint& toConnection)
        : m_sourceEndpoint(fromConnection)
        , m_targetEndpoint(toConnection)
    {
    }

    Connection::~Connection()
    {
        ConnectionRequestBus::Handler::BusDisconnect();
    }

    void Connection::Reflect(AZ::ReflectContext* reflection)
    {
        Endpoint::Reflect(reflection);
        NamedEndpoint::Reflect(reflection);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<Connection, AZ::Component>()
                ->Version(0)
                ->Field("sourceEndpoint", &Connection::m_sourceEndpoint)
                ->Field("targetEndpoint", &Connection::m_targetEndpoint)
                ;
        }

    }

    void Connection::Init()
    {
        ConnectionRequestBus::Handler::BusConnect(GetEntityId());
    }

    void Connection::Activate()
    {
    }

    void Connection::Deactivate()
    {
    }

    AZ::Outcome<void, AZStd::string> Connection::ValidateEndpoints(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint)
    {
        Slot* sourceSlot{};
        NodeRequestBus::EventResult(sourceSlot, sourceEndpoint.GetNodeId(), &NodeRequests::GetSlot, sourceEndpoint.GetSlotId());
        Slot* targetSlot{};
        NodeRequestBus::EventResult(targetSlot, targetEndpoint.GetNodeId(), &NodeRequests::GetSlot, targetEndpoint.GetSlotId());

        if (!sourceSlot)
        {
            return AZ::Failure(AZStd::string("Source slot does not exist."));
        }

        if (!targetSlot)
        {
            return AZ::Failure(AZStd::string("Target slot does not exist."));
        }

        if (sourceSlot->IsData())
        {
            if (!sourceSlot->IsTypeMatchFor((*targetSlot)))
            {
                return AZ::Failure(AZStd::string("Slots are not a type match for each other"));
            }
        }

        auto connectionSourceToTarget = MatchContracts(*sourceSlot, *targetSlot);
        if (!connectionSourceToTarget.IsSuccess())
        {
            return connectionSourceToTarget;
        }

        auto connectionTargetToSource = MatchContracts(*targetSlot, *sourceSlot);
        if (!connectionTargetToSource.IsSuccess())
        {
            return connectionTargetToSource;
        }

        return AZ::Success();

    }

    const SlotId& Connection::GetSourceSlot() const
    {
        return m_sourceEndpoint.GetSlotId();
    }

    const SlotId& Connection::GetTargetSlot() const
    {
        return m_targetEndpoint.GetSlotId();
    }

    const ID& Connection::GetTargetNode() const
    {
        return m_targetEndpoint.GetNodeId();
    }

    const ID& Connection::GetSourceNode() const
    {
        return m_sourceEndpoint.GetNodeId();
    }

    const Endpoint& Connection::GetTargetEndpoint() const
    {
        return m_targetEndpoint;
    }

    const Endpoint& Connection::GetSourceEndpoint() const
    {
        return m_sourceEndpoint;
    }

    void Connection::OnNodeRemoved(const ID& nodeId)
    {
        if (nodeId == m_sourceEndpoint.GetNodeId() || nodeId == m_targetEndpoint.GetNodeId())
        {
            GraphRequestBus::Event(*GraphNotificationBus::GetCurrentBusId(), &GraphRequests::DisconnectById, GetEntityId());
        }
    }
}