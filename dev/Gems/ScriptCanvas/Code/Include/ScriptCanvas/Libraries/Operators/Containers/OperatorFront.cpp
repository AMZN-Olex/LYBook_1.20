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

#include "OperatorFront.h"

#include <ScriptCanvas/Core/Contracts/SupportsMethodContract.h>
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            void OperatorFront::ConfigureContracts(SourceType sourceType, AZStd::vector<ContractDescriptor>& contractDescs)
            {
                if (sourceType == SourceType::SourceInput)
                {
                    ContractDescriptor supportsMethodContract;
                    supportsMethodContract.m_createFunc = [this]() -> SupportsMethodContract* { return aznew SupportsMethodContract("Front"); };
                    contractDescs.push_back(AZStd::move(supportsMethodContract));
                }
            }

            void OperatorFront::OnSourceTypeChanged()
            {
                if (Data::IsVectorContainerType(GetSourceAZType()))
                {
                    // Add the OUTPUT slots, most of the time there will only be one
                    Data::Type type = Data::FromAZType(m_sourceTypes[0]);
                    m_outputSlots.insert(AddOutputTypeSlot(Data::GetName(type), "The value at the specified index", type, Node::OutputStorage::Required, false));
                }
            }

            void OperatorFront::InvokeOperator()
            {
                const SlotSet& slotSets = GetSourceSlots();

                if (!slotSets.empty())
                {
                    SlotId sourceSlotId = (*slotSets.begin());

                    if (const Datum* containerDatum = GetInput(sourceSlotId))
                    {
                        if (containerDatum && !containerDatum->Empty())
                        {
                            const Datum* inputKeyDatum = GetInput(*m_inputSlots.begin());
                            AZ::Outcome<Datum, AZStd::string> valueOutcome = BehaviorContextMethodHelper::CallMethodOnDatumUnpackOutcomeSuccess(*containerDatum, "Front", *inputKeyDatum);
                            if (!valueOutcome.IsSuccess())
                            {
                                SCRIPTCANVAS_REPORT_ERROR((*this), "Failed to call Front on container: %s", valueOutcome.GetError().c_str());
                                return;
                            }

                            if (Data::IsVectorContainerType(containerDatum->GetType()))
                            {
                                PushOutput(valueOutcome.TakeValue(), *GetSlot(*m_outputSlots.begin()));
                            }
                        }
                    }
                }

                SignalOutput(GetSlotId("Out"));
            }

            void OperatorFront::OnInputSignal(const SlotId& slotId)
            {
                const SlotId inSlotId = OperatorBaseProperty::GetInSlotId(this);
                if (slotId == inSlotId)
                {
                    InvokeOperator();
                }
            }
        }
    }
}

#include <Include/ScriptCanvas/Libraries/Operators/Containers/OperatorFront.generated.cpp>