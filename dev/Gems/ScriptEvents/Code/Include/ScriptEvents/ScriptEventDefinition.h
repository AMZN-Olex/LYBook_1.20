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

#include <AzCore/std/containers/vector.h>

#include <ScriptEvents/ScriptEventParameter.h>
#include <ScriptEvents/ScriptEventMethod.h>
#include <ScriptEvents/Internal/VersionedProperty.h>

namespace ScriptEvents
{
    //! Defines a Script Event.
    //! This is the user-facing Script Event definition.
    //! When users create Script Events from Lua or in the editor this is the data definition that 
    //! a Script Event Asset will serialize.
    class ScriptEvent
    {
    public:

        AZ_TYPE_INFO(ScriptEvent, "{10A08CD3-32C9-4E18-8039-4B8A8157918E}");

        bool IsAddressRequired() const
        {
            AZ::Uuid id = GetAddressType();
            return id != azrtti_typeid<void>() && id != AZ::Uuid::CreateNull();
        }

        ScriptEvent()
            : m_version(1)
            , m_name("Name")
            , m_category("Category")
            , m_tooltip("Tooltip")
            , m_addressType("Address Type")
        {
            m_name.Set("EventName");
            m_category.Set("Script Events");
            m_tooltip.Set("");
            m_addressType.Set(azrtti_typeid<void>());
        }

        ScriptEvent(AZ::ScriptDataContext& dc)
            : ScriptEvent()
        {
            if (dc.GetNumArguments() > 0)
            {
                AZStd::string name;
                if (dc.ReadArg(0, name))
                {
                    m_name.Set(name);
                }

                if (dc.GetNumArguments() > 1)
                {
                    AZ::Uuid addressType;
                    if (dc.ReadArg(1, addressType))
                    {
                        m_addressType.Set(addressType);
                    }
                    else
                    {
                        m_addressType.Set(azrtti_typeid<void>());
                    }
                }
            }
        }

        void MakeBackup()
        {
            IncreaseVersion();
        }

        void AddMethod(AZ::ScriptDataContext& dc)
        {
            Method& method = NewMethod();
            method = Method(dc);
            dc.PushResult(method);
        }

        void RegisterInternal();
        void Register(AZ::ScriptDataContext& dc);
        void Release(AZ::ScriptDataContext& dc);

        Method& NewMethod()
        {
            m_methods.push_back();
            return m_methods.back();
        }

        bool FindMethod(const AZ::Crc32& eventId, Method& outMethod) const
        {
            for (auto method : m_methods)
            {
                if (method.GetEventId() == eventId)
                {
                    outMethod = method;
                    return true;
                }
            }

            return false;
        }

        bool FindMethod(const AZStd::string_view& name, Method& outMethod) const
        {
            outMethod = {};

            for (auto& method : m_methods)
            {
                if (method.GetName().compare(name) == 0)
                {
                    outMethod = method;
                    return true;
                }
            }
            return false;
        }

        bool HasMethod(const AZ::Crc32& eventId) const
        {
            for (auto& method : m_methods)
            {
                if (method.GetEventId() == eventId)
                {
                    return true;
                }
            }
            
            return false;
        }

        static void Reflect(AZ::ReflectContext* context);

        AZ::u32 GetVersion() const { return m_version; }
        AZStd::string GetName() const { return m_name.Get<AZStd::string>() ? *m_name.Get<AZStd::string>() : ""; }
        AZStd::string GetCategory() const { return m_category.Get<AZStd::string>() ? *m_category.Get<AZStd::string>() : ""; }
        AZStd::string GetTooltip() const { return m_tooltip.Get<AZStd::string>() ? *m_tooltip.Get<AZStd::string>() : ""; }
        AZ::Uuid GetAddressType() const { return m_addressType.Get<AZ::Uuid>() ? *m_addressType.Get<AZ::Uuid>() : AZ::Uuid::CreateNull(); }
        
        const AZStd::vector<Method>& GetMethods() const { return m_methods; }

        AZStd::string_view GetLabel() const { return GetName(); }

        void SetVersion(AZ::u32 version) { m_version = version; }
        ScriptEventData::VersionedProperty& GetNameProperty() { return m_name; }
        ScriptEventData::VersionedProperty& GetCategoryProperty() { return m_category; }
        ScriptEventData::VersionedProperty& GetTooltipProperty() { return m_tooltip; }
        ScriptEventData::VersionedProperty& GetAddressTypeProperty() { return m_addressType; }

        const ScriptEventData::VersionedProperty& GetNameProperty() const { return m_name; }
        const ScriptEventData::VersionedProperty& GetCategoryProperty() const { return m_category; }
        const ScriptEventData::VersionedProperty& GetTooltipProperty() const { return m_tooltip; }
        const ScriptEventData::VersionedProperty& GetAddressTypeProperty() const { return m_addressType; }

        //! Validates that the asset data being stored is valid and supported.
        AZ::Outcome<bool, AZStd::string> Validate() const;

        void IncreaseVersion() 
        { 
            m_name.PreSave();
            m_category.PreSave();
            m_tooltip.PreSave();
            m_addressType.PreSave();

            for (Method& method : m_methods)
            {
                method.PreSave();
            }

            ++m_version; 
        }

    private:

        AZ::u32 m_version;
        ScriptEventData::VersionedProperty m_name;
        ScriptEventData::VersionedProperty m_category;
        ScriptEventData::VersionedProperty m_tooltip;
        ScriptEventData::VersionedProperty m_addressType;
        AZStd::vector<Method> m_methods;

    };
}
