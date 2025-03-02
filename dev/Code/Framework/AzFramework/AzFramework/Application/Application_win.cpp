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

#include <AzFramework/API/ApplicationAPI_win.h>
#include <AzFramework/Application/Application.h>

#include <Windows.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class ApplicationWindows
        : public Application::Implementation
        , public WindowsLifecycleEvents::Bus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        AZ_CLASS_ALLOCATOR(ApplicationWindows, AZ::SystemAllocator, 0);
        ApplicationWindows();
        ~ApplicationWindows() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // WindowsLifecycleEvents
        void OnMinimized() override; // Suspend
        void OnRestored() override;  // Resume

        void OnKillFocus() override; // Constrain
        void OnSetFocus() override;  // Unconstrain

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Application::Implementation
        void PumpSystemEventLoopOnce() override;
        void PumpSystemEventLoopUntilEmpty() override;

    protected:
        void ProcessSystemEvent(MSG& msg); // Returns true if an event was processed, false otherwise

    private:
        ApplicationLifecycleEvents::Event m_lastEvent;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    Application::Implementation* Application::Implementation::Create()
    {
        return aznew ApplicationWindows();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const char* Application::Implementation::GetAppRootPath()
    {
        return nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ApplicationWindows::ApplicationWindows()
        : m_lastEvent(ApplicationLifecycleEvents::Event::None)
    {
        WindowsLifecycleEvents::Bus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ApplicationWindows::~ApplicationWindows()
    {
        WindowsLifecycleEvents::Bus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationWindows::OnMinimized()
    {
        // guard against duplicate events
        if (m_lastEvent != ApplicationLifecycleEvents::Event::Suspend)
        {
            EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnApplicationSuspended, m_lastEvent);
            m_lastEvent = ApplicationLifecycleEvents::Event::Suspend;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationWindows::OnRestored()
    {
        // guard against duplicate events
        if (m_lastEvent != ApplicationLifecycleEvents::Event::Resume)
        {
            EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnApplicationResumed, m_lastEvent);
            m_lastEvent = ApplicationLifecycleEvents::Event::Resume;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationWindows::OnKillFocus()
    {
        // guard against duplicate events
        if (m_lastEvent != ApplicationLifecycleEvents::Event::Constrain)
        {
            EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnApplicationConstrained, m_lastEvent);
            m_lastEvent = ApplicationLifecycleEvents::Event::Constrain;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationWindows::OnSetFocus()
    {
        // guard against duplicate events
        if (m_lastEvent != ApplicationLifecycleEvents::Event::Unconstrain)
        {
            EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnApplicationUnconstrained, m_lastEvent);
            m_lastEvent = ApplicationLifecycleEvents::Event::Unconstrain;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationWindows::PumpSystemEventLoopOnce()
    {
        MSG msg;
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            ProcessSystemEvent(msg);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationWindows::PumpSystemEventLoopUntilEmpty()
    {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            ProcessSystemEvent(msg);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationWindows::ProcessSystemEvent(MSG& msg)
    {
        if (msg.message == WM_QUIT)
        {
            ApplicationRequests::Bus::Broadcast(&ApplicationRequests::ExitMainLoop);
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

} // namespace AzFramework
