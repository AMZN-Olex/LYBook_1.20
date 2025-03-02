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

#include "Error.h"

#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Contracts/ContractRTTI.h>
#include <ScriptCanvas/Core/Contracts/ConnectionLimitContract.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            void Error::OnInit()
            {
                AddSlot("In", "", SlotType::ExecutionIn);
                AddSlot("This", "", SlotType::DataOut); // \todo for testing only, we need to ability to arbitrarily connect nodes themeselve to slots (as input to function call or error handling) or directly (as the flow of execution with arrows if easier to read...)
                AddInputDatumSlot("Description", "", AZStd::move(Data::Type::String()), Datum::eOriginality::Copy);
            }

            void Error::OnInputSignal(const SlotId&)
            {
                const Datum* input = GetInput(GetSlotId("Description"));
                if (const Data::StringType* desc = input ? input->GetAs<Data::StringType>() : nullptr)
                {
                    SCRIPTCANVAS_REPORT_ERROR((*this), desc->data());
                }
                else
                {
                    // \todo get a more descriptive default error report
                    SCRIPTCANVAS_REPORT_ERROR((*this), "Undescribed error");
                }                
            }

            void Error::Reflect(AZ::ReflectContext* reflectContext)
            {
                if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
                {
                    serializeContext->Class<Error, Node>()
                        ->Version(0)
                        ;

                    if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                    {
                        editContext->Class<Error>("Error", "")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute(AZ::Edit::Attributes::Category, "Utilities/Debug")
                                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Error.png")
                                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                            ;
                    }
                }
            }

        } // namespace Core
    } // namespace Nodes
} // namespace ScriptCanvas
