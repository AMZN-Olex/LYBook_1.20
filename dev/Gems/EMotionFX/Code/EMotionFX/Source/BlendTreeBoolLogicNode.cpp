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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "EMotionFXConfig.h"
#include "BlendTreeBoolLogicNode.h"


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeBoolLogicNode, AnimGraphAllocator, 0)

    BlendTreeBoolLogicNode::BlendTreeBoolLogicNode()
        : AnimGraphNode()
        , m_functionEnum(FUNCTION_AND)
        , m_trueResult(1.0f)
        , m_falseResult(0.0f)
        , m_defaultValue(false)
    {
        // setup the input ports
        InitInputPorts(2);
        SetupInputPort("x", INPUTPORT_X, MCore::AttributeFloat::TYPE_ID, PORTID_INPUT_X);
        SetupInputPort("y", INPUTPORT_Y, MCore::AttributeFloat::TYPE_ID, PORTID_INPUT_Y);

        // setup the output ports
        InitOutputPorts(2);
        SetupOutputPort("Float", OUTPUTPORT_VALUE, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_VALUE);
        SetupOutputPort("Bool", OUTPUTPORT_BOOL, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_BOOL);

        if (mAnimGraph)
        {
            Reinit();
        }
    }


    BlendTreeBoolLogicNode::~BlendTreeBoolLogicNode()
    {
    }


    void BlendTreeBoolLogicNode::Reinit()
    {
        switch (m_functionEnum)
        {
        case FUNCTION_AND:
            m_function = BoolLogicAND;
            SetNodeInfo("x AND y");
            break;
        case FUNCTION_OR:
            m_function = BoolLogicOR;
            SetNodeInfo("x OR y");
            break;
        case FUNCTION_XOR:
            m_function = BoolLogicXOR;
            SetNodeInfo("x XOR y");
            break;
        case FUNCTION_NAND:
            m_function = BoolLogicNAND;
            SetNodeInfo("x NAND y");
            break;
        case FUNCTION_NOR:
            m_function = BoolLogicNOR;
            SetNodeInfo("x NOR y");
            break;
        case FUNCTION_XNOR:
            m_function = BoolLogicXNOR;
            SetNodeInfo("x XNOR y");
            break;
        case FUNCTION_NOT_X:
            m_function = BoolLogicNOTX;
            SetNodeInfo("NOT x");
            break;
        case FUNCTION_NOT_Y:
            m_function = BoolLogicNOTY;
            SetNodeInfo("NOT y");
            break;
        default:
            AZ_Assert(false, "EMotionFX: Math function unknown.");
        }

        AnimGraphNode::Reinit();
    }


    bool BlendTreeBoolLogicNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }

    
    // get the palette name
    const char* BlendTreeBoolLogicNode::GetPaletteName() const
    {
        return "Bool Logic";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeBoolLogicNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_LOGIC;
    }


    // the update function
    void BlendTreeBoolLogicNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // if there are no incoming connections, there is nothing to do
        if (mConnections.empty())
        {
            return;
        }

        // update all inputs
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        // if both x and y inputs have connections
        bool x, y;
        if (mConnections.size() == 2)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_X));
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_Y));

            x = GetInputNumberAsBool(animGraphInstance, INPUTPORT_X);
            y = GetInputNumberAsBool(animGraphInstance, INPUTPORT_Y);
        }
        else // only x or y is connected
        {
            // if only x has something plugged in
            if (mConnections[0]->GetTargetPort() == INPUTPORT_X)
            {
                OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_X));
                x = GetInputNumberAsBool(animGraphInstance, INPUTPORT_X);
                y = m_defaultValue;
            }
            else // only y has an input
            {
                MCORE_ASSERT(mConnections[0]->GetTargetPort() == INPUTPORT_Y);
                OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_Y));
                x = m_defaultValue;
                y = GetInputNumberAsBool(animGraphInstance, INPUTPORT_Y);
            }
        }

        // execute the logic function
        if (m_function(x, y))
        {
            GetOutputFloat(animGraphInstance, OUTPUTPORT_BOOL)->SetValue(1.0f);
            GetOutputFloat(animGraphInstance, OUTPUTPORT_VALUE)->SetValue(m_trueResult);
        }
        else
        {
            GetOutputFloat(animGraphInstance, OUTPUTPORT_BOOL)->SetValue(0.0f);
            GetOutputFloat(animGraphInstance, OUTPUTPORT_VALUE)->SetValue(m_falseResult);
        }
    }


    void BlendTreeBoolLogicNode::SetFunction(EFunction func)
    {
        m_functionEnum = func;
        if (mAnimGraph)
        {
            Reinit();
        }
    }

    BlendTreeBoolLogicNode::EFunction BlendTreeBoolLogicNode::GetFunction() const
    {
        return m_functionEnum;
    }

    AZ::Color BlendTreeBoolLogicNode::GetVisualColor() const
    {
        return AZ::Color(0.2f, 1.0f, 0.2f, 1.0f);
    }


    //-----------------------------------------------
    // the condition functions
    //-----------------------------------------------
    bool BlendTreeBoolLogicNode::BoolLogicAND(bool x, bool y)       { return (x && y); }
    bool BlendTreeBoolLogicNode::BoolLogicOR(bool x, bool y)        { return (x || y); }
    bool BlendTreeBoolLogicNode::BoolLogicXOR(bool x, bool y)       { return (x ^ y); }
    bool BlendTreeBoolLogicNode::BoolLogicNAND(bool x, bool y)      { return !BoolLogicAND(x,y); }
    bool BlendTreeBoolLogicNode::BoolLogicNOR(bool x, bool y)       { return !BoolLogicOR(x, y); }
    bool BlendTreeBoolLogicNode::BoolLogicXNOR(bool x, bool y)      { return !BoolLogicXOR(x, y); }
    bool BlendTreeBoolLogicNode::BoolLogicNOTX(bool x, bool y)      { return !x; }
    bool BlendTreeBoolLogicNode::BoolLogicNOTY(bool x, bool y)      { return !y; }

    void BlendTreeBoolLogicNode::SetDefaultValue(bool defaultValue)
    {
        m_defaultValue = defaultValue;
    }

    void BlendTreeBoolLogicNode::SetTrueResult(float trueResult)
    {
        m_trueResult = trueResult;
    }

    void BlendTreeBoolLogicNode::SetFalseResult(float falseResult)
    {
        m_falseResult = falseResult;
    }

    void BlendTreeBoolLogicNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeBoolLogicNode, AnimGraphNode>()
            ->Version(1)
            ->Field("logicFunction", &BlendTreeBoolLogicNode::m_functionEnum)
            ->Field("defaultValue", &BlendTreeBoolLogicNode::m_defaultValue)
            ->Field("trueResult", &BlendTreeBoolLogicNode::m_trueResult)
            ->Field("falseResult", &BlendTreeBoolLogicNode::m_falseResult)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeBoolLogicNode>("Bool Logic", "Bool logic attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeBoolLogicNode::m_functionEnum, "Logic Function", "The logic function to use.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeBoolLogicNode::Reinit)
            ->EnumAttribute(FUNCTION_AND,  "AND")
            ->EnumAttribute(FUNCTION_OR,   "OR")
            ->EnumAttribute(FUNCTION_XOR,  "XOR")
            ->EnumAttribute(FUNCTION_NAND, "NAND")
            ->EnumAttribute(FUNCTION_NOR, "NOR")
            ->EnumAttribute(FUNCTION_XNOR, "XNOR")
            ->EnumAttribute(FUNCTION_NOT_X, "NOT x")
            ->EnumAttribute(FUNCTION_NOT_Y, "NOT y")
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeBoolLogicNode::m_defaultValue, "Default Value", "Value used for x or y when the input port has no connection.")
            ->EnumAttribute(FUNCTION_AND,  "False")
            ->EnumAttribute(FUNCTION_OR,   "True")
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeBoolLogicNode::m_trueResult, "Float Result When True", "The float value returned when the expression is true.")
            ->Attribute(AZ::Edit::Attributes::Min, -std::numeric_limits<float>::max())
            ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeBoolLogicNode::m_falseResult, "Float Result When False", "The float value returned when the expression is false.")
            ->Attribute(AZ::Edit::Attributes::Min, -std::numeric_limits<float>::max())
            ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
        ;
    }
} // namespace EMotionFX
