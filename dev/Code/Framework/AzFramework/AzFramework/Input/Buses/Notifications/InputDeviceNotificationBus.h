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

#include <AzCore/EBus/EBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    class InputDevice;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to listen for input events as they're broadcast from input devices when
    //! they connect or disconnect from the system. Some common input devices are assumed to always
    //! be connected, and will never generate these notifications. This interface could be extended
    //! to include notifications for other events related to input devices, for example low battery.
    class InputDeviceNotifications : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: input notifications are addressed to a single address
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: input notifications can be handled by multiple (ordered) listeners
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::MultipleAndOrdered;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        virtual ~InputDeviceNotifications() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified when input devices connect to the system
        //! \param[in] inputChannel The input device that connected
        virtual void OnInputDeviceConnectedEvent(const InputDevice& /*inputDevice*/) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified when input devices disconnect from the system
        //! \param[in] inputChannel The input device that disconnected
        virtual void OnInputDeviceDisconnectedEvent(const InputDevice& /*inputDevice*/) {}
        AZ_DEPRECATED(virtual void OnInputDeviceDisonnectedEvent(const InputDevice& /*inputDevice*/), "Deprecated (typo, missing 'c'), please use OnInputDeviceDisconnectedEvent)") {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the priority of the input notification handler (sorted from highest to lowest)
        //! \return Priority of the input notification handler
        virtual AZ::s32 GetPriority() const { return 0; }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Compare function required by BusHandlerOrderCompare = BusHandlerCompareDefault
        //! \param[in] other Another instance of the class to compare
        //! \return True if the priority of this handler is greater than the other, false otherwise
        inline bool Compare(const InputDeviceNotifications* other) const
        {
            return GetPriority() > other->GetPriority();
        }
    };
    using InputDeviceNotificationBus = AZ::EBus<InputDeviceNotifications>;

    AZ_DEPRECATED(typedef InputDeviceNotificationBus InputDeviceEventNotificationBus, "Renamed to InputDeviceNotificationBus");
    AZ_DEPRECATED(typedef InputDeviceNotifications InputDeviceEventNotifications, "Renamed to InputDeviceNotifications");
} // namespace AzFramework
