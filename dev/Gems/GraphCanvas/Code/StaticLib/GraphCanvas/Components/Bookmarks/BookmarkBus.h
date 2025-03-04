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

#include <QColor>
#include <QRect>

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/std/string/string.h>

namespace GraphCanvas
{   
    const int k_findShortcut = -1;
    const int k_unusedShortcut = -2;

    class BookmarkManagerRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        // Creates a bookmark anchor at the specified point with the given index.
        virtual bool CreateBookmarkAnchor(const AZ::Vector2& position, int bookmarkIndex) = 0;
    
        // Registers the given bookmark with the Bookmark Manager
        virtual void RegisterBookmark(const AZ::EntityId& bookmarkId) = 0;

        // Unregisters the given bookmark with the bookmark manager
        virtual void UnregisterBookmark(const AZ::EntityId& bookmarkId) = 0;

        // Returns whether or not the specified bookmark is registered to the BookmarkManager
        virtual bool IsBookmarkRegistered(const AZ::EntityId& bookmarkId) const = 0;

        virtual AZ::EntityId FindBookmarkForShortcut(int shortcut) const = 0;

        // Remaps the given bookmark to the specified quick index.
        virtual bool RequestShortcut(const AZ::EntityId& bookmarkId, int shortcut) = 0;
        
        // Activates the specified quick bookmark
        virtual void ActivateShortcut(int index) = 0;        

        // Jumps to the given bookmark
        virtual void JumpToBookmark(const AZ::EntityId& bookmarkId) = 0;
    };
    
    using BookmarkManagerRequestBus = AZ::EBus<BookmarkManagerRequests>;

    class BookmarkManagerNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void OnBookmarkAdded(const AZ::EntityId& /*bookmark*/) { };
        virtual void OnBookmarkRemoved(const AZ::EntityId& /*bookmark*/) { }
        virtual void OnShortcutChanged(int /*shortcut*/, const AZ::EntityId& /*oldBookmark*/, const AZ::EntityId& /*newBookmark*/) { };
    };

    using BookmarkManagerNotificationBus = AZ::EBus<BookmarkManagerNotifications>;

    class BookmarkRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void RemoveBookmark() = 0;

        virtual int GetShortcut() const = 0;
        virtual void SetShortcut(int quickIndex) = 0;

        virtual AZStd::string GetBookmarkName() const = 0;
        virtual void SetBookmarkName(const AZStd::string& bookmarkName) = 0;
        virtual QRectF GetBookmarkTarget() const = 0;
        virtual QColor GetBookmarkColor() const = 0;
    };

    using BookmarkRequestBus = AZ::EBus<BookmarkRequests>;

    class BookmarkNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        // Sends out a signal whenever the bookmark name changes.
        virtual void OnBookmarkNameChanged() { };

        virtual void OnBookmarkColorChanged() { };

        virtual void OnBookmarkTriggered() { };
    };

    using BookmarkNotificationBus = AZ::EBus<BookmarkNotifications>;

    class SceneBookmarkRequests
        : public AZ::EBusTraits
    {
    public:
        // BusId here the scene that the bookmark belongs to.
        // Mainly used for enumeration as a method of gathering all of the bookmark ids.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual AZ::EntityId GetBookmarkId() const = 0;
    };

    using SceneBookmarkRequestBus = AZ::EBus<SceneBookmarkRequests>;

    class SceneBookmarkActions
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual AZ::u32 GetNewBookmarkCounter() = 0;
    };

    using SceneBookmarkActionBus = AZ::EBus<SceneBookmarkActions>;
    
    class BookmarkTableSourceModel;
    
    // Bus is used for the Model to talk to the View(couple of cases of data manipulation, where it's convenient to do so).
    class BookmarkTableRequests
        : public AZ::EBusTraits
    {
    public:
        // Key is the modle that is making the request, for whatever view is displaying the model.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = BookmarkTableSourceModel*;

        virtual void ClearSelection() = 0;
    };

    using BookmarkTableRequestBus = AZ::EBus<BookmarkTableRequests>;
}