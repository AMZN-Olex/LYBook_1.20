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
#include "AudioProxyComponent.h"

#include <ISystem.h>
#include <MathConversion.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/Debug/Trace.h>

namespace LmbrCentral
{
    //=========================================================================
    void AudioProxyComponent::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<AudioProxyComponent, AZ::Component>()
                ->Version(1)
                ;

            auto editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<AudioProxyComponent>("Audio Proxy", "The Audio Proxy component is a required dependency if you add multiple audio components to an entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Audio")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AddableByUser, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-audio-proxy.html")
                    ;
            }
        }
    }

    //=========================================================================
    void AudioProxyComponent::Activate()
    {
        AZ_Assert(!m_audioProxy, "AudioProxyCompnent::Activate - Audio Proxy has been set already!");
        Audio::AudioSystemRequestBus::BroadcastResult(m_audioProxy, &Audio::AudioSystemRequestBus::Events::GetFreeAudioProxy);

        if (m_audioProxy)
        {
            AZStd::string proxyName = AZStd::string::format("%s_audioproxy", GetEntity()->GetName().c_str());
            m_audioProxy->Initialize(proxyName.c_str());
            m_audioProxy->SetObstructionCalcType(Audio::eAOOCT_IGNORE);

            // don't need to set position on the proxy now, but initialize the transform from the entity.
            AZ::TransformBus::EventResult(m_transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

            AudioProxyComponentRequestBus::Handler::BusConnect(GetEntityId());
            AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        }
    }

    //=========================================================================
    void AudioProxyComponent::Deactivate()
    {
        if (m_audioProxy)
        {
            AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());
            AudioProxyComponentRequestBus::Handler::BusDisconnect(GetEntityId());

            m_audioProxy->StopAllTriggers();
            m_audioProxy->Release();
            m_audioProxy = nullptr;
        }
    }

    //=========================================================================
    void AudioProxyComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_transform = world;
        if (m_tracksEntityPosition)
        {
            m_audioProxy->SetPosition(AZTransformToLYTransform(m_transform));
        }
    }

    //=========================================================================
    bool AudioProxyComponent::ExecuteSourceTrigger(const Audio::TAudioControlID triggerID, const Audio::SAudioCallBackInfos& callbackInfo, const Audio::SAudioSourceInfo& sourceInfo)
    {
        if (triggerID != INVALID_AUDIO_CONTROL_ID)
        {
            // set the position at the Entity's current location.
            // need to poll in case we haven't hit a transform update yet.
            m_audioProxy->SetPosition(AZTransformToLYTransform(m_transform));

            // ...and kick it off...
            m_audioProxy->ExecuteSourceTrigger(triggerID, sourceInfo, callbackInfo);
            return true;
        }

        return false;
    }

    //=========================================================================
    bool AudioProxyComponent::ExecuteTrigger(const Audio::TAudioControlID triggerID, const Audio::SAudioCallBackInfos& callbackInfo)
    {
        if (triggerID != INVALID_AUDIO_CONTROL_ID)
        {
            // set the position at the Entity's current location.
            // need to poll in case we haven't hit a transform update yet.
            m_audioProxy->SetPosition(AZTransformToLYTransform(m_transform));

            // ...and kick it off...
            m_audioProxy->ExecuteTrigger(triggerID, eLSM_None, callbackInfo);
            return true;
        }

        return false;
    }

    //=========================================================================
    void AudioProxyComponent::KillTrigger(const Audio::TAudioControlID triggerID)
    {
        m_audioProxy->StopTrigger(triggerID);
    }

    //=========================================================================
    void AudioProxyComponent::KillAllTriggers()
    {
        m_audioProxy->StopAllTriggers();
    }

    //=========================================================================
    void AudioProxyComponent::SetRtpcValue(const Audio::TAudioControlID rtpcID, float value)
    {
        m_audioProxy->SetRtpcValue(rtpcID, value);
    }

    //=========================================================================
    void AudioProxyComponent::SetSwitchState(const Audio::TAudioControlID switchID, const Audio::TAudioSwitchStateID stateID)
    {
        m_audioProxy->SetSwitchState(switchID, stateID);
    }

    //=========================================================================
    void AudioProxyComponent::SetEnvironmentAmount(const Audio::TAudioEnvironmentID environmentID, float amount)
    {
        m_audioProxy->SetEnvironmentAmount(environmentID, amount);
    }

    //=========================================================================
    void AudioProxyComponent::SetObstructionCalcType(const Audio::EAudioObjectObstructionCalcType type)
    {
        m_audioProxy->SetObstructionCalcType(type);
    }

    //=========================================================================
    void AudioProxyComponent::SetPosition(const Audio::SATLWorldPosition& position)
    {
        m_audioProxy->SetPosition(position);
    }

    //=========================================================================
    void AudioProxyComponent::SetMultiplePositions(const Audio::MultiPositionParams& params)
    {
        m_audioProxy->SetMultiplePositions(params);
    }

} // namespace LmbrCentral
