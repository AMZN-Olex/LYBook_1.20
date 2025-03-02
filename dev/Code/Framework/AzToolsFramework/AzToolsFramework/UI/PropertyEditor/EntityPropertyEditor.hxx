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

#ifndef ENTITY_PROPERTY_EDITOR_H
#define ENTITY_PROPERTY_EDITOR_H

#pragma once

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/function/function_fwd.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/ToolsComponents/ComponentMimeData.h>
#include <AzToolsFramework/ToolsComponents/EditorInspectorComponentBus.h>
#include <QtWidgets/QWidget>
#include <QtGui/QIcon>
#include <QComboBox>

class QLabel;
class QSpacerItem;
class QMenu;
class QMimeData;
class QStandardItem;

namespace Ui
{
    class EntityPropertyEditorUI;
}

namespace AZ
{
    struct ClassDataReflection;
    class Component;
    class Entity;
}

namespace UndoSystem
{
    class URSequencePoint;
}

namespace AzToolsFramework
{
    class ComponentEditor;
    class ComponentPaletteWidget;
    struct SourceControlFileInfo;

    namespace AssetBrowser
    {
        class ProductAssetBrowserEntry;
    }

    using ProductCallback = AZStd::function<void(const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*)>;

    using ComponentEditorVector = AZStd::vector<ComponentEditor*>;

    /**
     * the entity property editor shows all components for a given entity or set of entities.
     * it displays their values and lets you edit them.  The editing actually happens through the sub editor parts, though.
     * only components which the selected entities have in common are displayed (if theres more than one)
     * if there are components that are not in common, there is a message indicating that this is the case.
     * each component is shown as a heading which can be expanded into an actual component specific property editor.
     * so this widget is actually only interested in specifically what entities are selected, what their components are,
     * and what is in common.
     */
    class EntityPropertyEditor
        : public QWidget
        , private ToolsApplicationEvents::Bus::Handler
        , public IPropertyEditorNotify
        , public AzToolsFramework::EditorEntityContextNotificationBus::Handler
        , public AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler
        , public EditorInspectorComponentNotificationBus::MultiHandler
        , private AzToolsFramework::ComponentModeFramework::EditorComponentModeNotificationBus::Handler
        , public AZ::EntitySystemBus::Handler
    {
        Q_OBJECT;
    public:

        AZ_CLASS_ALLOCATOR(EntityPropertyEditor, AZ::SystemAllocator, 0)

        EntityPropertyEditor(QWidget* pParent = NULL, Qt::WindowFlags flags = 0, bool isLevelEntityEditor = false);
        virtual ~EntityPropertyEditor();

        virtual void BeforeUndoRedo();
        virtual void AfterUndoRedo();

        static void Reflect(AZ::ReflectContext* context);

        // implementation of IPropertyEditorNotify:

        // CALLED FOR UNDO PURPOSES
        void BeforePropertyModified(InstanceDataNode* pNode) override;
        void AfterPropertyModified(InstanceDataNode* pNode) override;
        void SetPropertyEditingActive(InstanceDataNode* pNode) override;
        void SetPropertyEditingComplete(InstanceDataNode* pNode) override;
        void SealUndoStack() override;

        // Context menu population for entity component properties.
        void RequestPropertyContextMenu(InstanceDataNode* node, const QPoint& globalPos) override;

        /// Set filter for what appears in the "Add Components" menu.
        void SetAddComponentMenuFilter(ComponentFilter componentFilter);

        void SetAllowRename(bool allowRename);

        void SetOverrideEntityIds(const AzToolsFramework::EntityIdList& entities);

        void SetSystemEntityEditor(bool isSystemEntityEditor);

    Q_SIGNALS:
        void SelectedEntityNameChanged(const AZ::EntityId& entityId, const AZStd::string& name);

    private:
        bool m_disabled = false;

        struct SharedComponentInfo
        {
            SharedComponentInfo(AZ::Component* component, AZ::Component* sliceReferenceComponent);
            AZ::Entity::ComponentArrayType m_instances;
            AZ::Component* m_sliceReferenceComponent;
        };

        using SharedComponentArray = AZStd::vector<SharedComponentInfo>;

        //////////////////////////////////////////////////////////////////////////
        // ToolsApplicationEvents::Bus::Handler
        void BeforeEntitySelectionChanged() override;
        void AfterEntitySelectionChanged(const AzToolsFramework::EntityIdList& newlySelectedEntities, const AzToolsFramework::EntityIdList& newlyDeselectedEntities) override;

        virtual void EntityParentChanged(AZ::EntityId, AZ::EntityId, AZ::EntityId) {}
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        /// AzToolsFramework::EditorEntityContextNotificationBus implementation
        void OnStartPlayInEditor() override;
        void OnStopPlayInEditor() override;
        void OnContextReset() override;
        void OnEditorEntitiesReplacedBySlicedEntities(const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& replacedEntitiesMap) override;
        //////////////////////////////////////////////////////////////////////////

        void OnEntityComponentPropertyChanged(AZ::ComponentId /*componentId*/) override;

        void OnComponentOrderChanged() override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::EntitySystemBus::Handler
        void OnEntityInitialized(const AZ::EntityId&) override;
        void OnEntityDestroyed(const AZ::EntityId&) override;
        void OnEntityActivated(const AZ::EntityId& entityId) override;
        void OnEntityDeactivated(const AZ::EntityId& entityId) override;
        void OnEntityNameChanged(const AZ::EntityId& entityId, const AZStd::string& name) override;
        //////////////////////////////////////////////////////////////////////////

        // EditorComponentModeNotificationBus
        void EnteredComponentMode(const AZStd::vector<AZ::Uuid>& componentModeTypes) override;
        void LeftComponentMode(const AZStd::vector<AZ::Uuid>& componentModeTypes) override;
        void ActiveComponentModeChanged(const AZ::Uuid& componentType) override;

        bool IsEntitySelected(const AZ::EntityId& id) const;
        bool IsSingleEntitySelected(const AZ::EntityId& id) const;

        virtual void GotSceneSourceControlStatus(AzToolsFramework::SourceControlFileInfo& fileInfo);
        virtual void PerformActionsBasedOnSceneStatus(bool sceneIsNew, bool readOnly);

        // enable/disable editor
        void EnableEditor(bool enabled);

        virtual void InvalidatePropertyDisplay(PropertyModificationRefreshLevel level);

        void MarkPropertyEditorBusyStart();
        void MarkPropertyEditorBusyEnd();

        void QueuePropertyRefresh();
        void ClearInstances(bool invalidateImmediately = true);

        void GetAllComponentsForEntityInOrder(const AZ::Entity* entity, AZ::Entity::ComponentArrayType& componentsOnEntity);
        void SortComponentsByPriority(AZ::Entity::ComponentArrayType& componentsOnEntity);
        void BuildSharedComponentArray(SharedComponentArray& sharedComponentArray, bool containsLayerEntity);
        void BuildSharedComponentUI(SharedComponentArray& sharedComponentArray);
        bool ComponentMatchesCurrentFilter(SharedComponentInfo& sharedComponentInfo) const;
        ComponentEditor* CreateComponentEditor();
        void UpdateEntityIcon();
        void UpdateEntityDisplay();
        bool DoesComponentPassFilter(const AZ::Component* component) const;
        bool IsComponentRemovable(const AZ::Component* component) const;
        bool AreComponentsRemovable(const AZ::Entity::ComponentArrayType& components) const;
        bool AreComponentsCopyable(const AZ::Entity::ComponentArrayType& components) const;

        void AddMenuOptionsForComponents(QMenu& menu, const QPoint& position);
        void AddMenuOptionsForFields(InstanceDataNode* fieldNode, InstanceDataNode* componentNode, const AZ::SerializeContext::ClassData* componentClassData, QMenu& menu);
        void AddMenuOptionsForRevert(InstanceDataNode* fieldNode, InstanceDataNode* componentNode, const AZ::SerializeContext::ClassData* componentClassData, QMenu& menu);

        bool HasAnyVisibleElements(const InstanceDataNode& node);

        void ContextMenuActionPullFieldData(AZ::Component* parentComponent, InstanceDataNode* fieldNode);
        void ContextMenuActionSetDataFlag(InstanceDataNode* node, AZ::DataPatch::Flag flag, bool additive);

        /// Given an InstanceDataNode, calculate a DataPatch address relative to the entity.
        /// @return true if successful.
        bool GetEntityDataPatchAddress(const InstanceDataNode* componentFieldNode, AZ::DataPatch::AddressType& dataPatchAddressOut, AZ::EntityId* entityIdOut = nullptr) const;

        enum class AddressRootType
        {
            RootAtComponentsContainer,  // Address will be made rooted at the entity's components container (from "Components" container down).
            RootAtEntity,               // Address will be made rooted at the entity itself (from "AZ::Entity" down).
        };

        // Input address is expected to be relative to a component, as each property editor corresponds to a component and its own data hierarchy.
        // \param componentFieldNode instance data hierarchy node corresponding to a field under a component, as provided by the component's ReflectedPropertyEditor.
        // \param rootType type of root the address should be adjusted to. See \ref AddressRootType.
        // \param outAddress output parameter to be populated with the adjusted node address.
        void CalculateAndAdjustNodeAddress(const InstanceDataNode& componentFieldNode, AddressRootType rootType, InstanceDataNode::Address& outAddress) const;

        // Custom function for comparing values of InstanceDataNodes
        bool InstanceNodeValueHasNoPushableChange(const InstanceDataNode* sourceNode, const InstanceDataNode* targetNode);

        // Custom function for determining whether InstanceDataNodes are read-only
        bool QueryInstanceDataNodeReadOnlyStatus(const InstanceDataNode* node);
        bool QueryInstanceDataNodeReadOnlySetStatus(const InstanceDataNode* node);

        // Custom function for determining whether InstanceDataNodes are hidden
        bool QueryInstanceDataNodeHiddenStatus(const InstanceDataNode* node);
        bool QueryInstanceDataNodeHiddenSetStatus(const InstanceDataNode* node);

        bool QueryInstanceDataNodeSetStatus(const InstanceDataNode* node, AZ::DataPatch::Flag testFlag);
        bool QueryInstanceDataNodeEffectStatus(const InstanceDataNode* node, AZ::DataPatch::Flag testFlag);

        const char* GetAppropriateIndicator(const InstanceDataNode* node);

        void OnDisplayComponentEditorMenu(const QPoint& position);
        void OnRequestRequiredComponents(const QPoint& position, const QSize& size, const AZStd::vector<AZ::ComponentServiceType>& services);

        AZ::Component* ExtractMatchingComponent(AZ::Component* component, AZ::Entity::ComponentArrayType& availableComponents);

        void SetEntityIconToDefault();
        void PopupAssetBrowserForEntityIcon();

        void HideComponentPalette();
        void ShowComponentPalette(
            ComponentPaletteWidget* componentPalette,
            const QPoint& position,
            const QSize& size,
            const AZStd::vector<AZ::ComponentServiceType>& serviceFilter);

        enum class SelectionEntityTypeInfo
        {
            None,
            OnlyStandardEntities,
            OnlyLayerEntities,
            Mixed,
            LevelEntity
        };
        /**
         * Returns what kinds of entities are in the current selection. This is used because mixed selection
         * and layer entities requires special logic in the property editor.
         * \param selection The selected entities.
         */
        SelectionEntityTypeInfo GetSelectionEntityTypeInfo(const EntityIdList& selection) const;

        /**
         * Returns true if a selection matching the passed in selection informatation allows components to be added.
         */
        bool CanAddComponentsToSelection(const SelectionEntityTypeInfo& selectionEntityTypeInfo) const;

        QVector<QAction*> m_entityComponentActions;
        QAction* m_actionToAddComponents = nullptr;
        QAction* m_actionToDeleteComponents = nullptr;
        QAction* m_actionToCutComponents = nullptr;
        QAction* m_actionToCopyComponents = nullptr;
        QAction* m_actionToPasteComponents = nullptr;
        QAction* m_actionToEnableComponents = nullptr;
        QAction* m_actionToDisableComponents = nullptr;
        QAction* m_actionToMoveComponentsUp = nullptr;
        QAction* m_actionToMoveComponentsDown = nullptr;
        QAction* m_actionToMoveComponentsTop = nullptr;
        QAction* m_actionToMoveComponentsBottom = nullptr;
        QAction* m_resetToSliceAction = nullptr;

        bool m_isShowingContextMenu = false;

        void CreateActions();
        void UpdateActions();

        bool CanPasteComponentsOnSelectedEntities() const;
        bool CanPasteComponentsOnEntity(const ComponentTypeMimeData::ClassDataContainer& classDataForComponentsToPaste, const AZ::Entity* entity) const;

        AZ::Entity::ComponentArrayType GetCopyableComponents() const;
        void DeleteComponents(const AZ::Entity::ComponentArrayType& components);
        void DeleteComponents();
        void CutComponents();
        void CopyComponents();
        void PasteComponents();
        void EnableComponents(const AZ::Entity::ComponentArrayType& components);
        void EnableComponents();
        void DisableComponents(const AZ::Entity::ComponentArrayType& components);
        void DisableComponents();
        void MoveComponentsUp();
        void MoveComponentsDown();
        void MoveComponentsTop();
        void MoveComponentsBottom();

        //component reorder and drag drop helpers

        //reorder source before target
        void MoveComponentBefore(const AZ::Component* sourceComponent, const AZ::Component* targetComponent, ScopedUndoBatch &undo);

        //reorder source after target
        void MoveComponentAfter(const AZ::Component* sourceComponent, const AZ::Component* targetComponent, ScopedUndoBatch &undo);

        //reorder each element of source before corresponding element of target
        void MoveComponentRowBefore(const AZ::Entity::ComponentArrayType &sourceComponents, const AZ::Entity::ComponentArrayType &targetComponents, ScopedUndoBatch &undo);

        //reorder each element of source after corresponding element of target
        void MoveComponentRowAfter(const AZ::Entity::ComponentArrayType &sourceComponents, const AZ::Entity::ComponentArrayType &targetComponents, ScopedUndoBatch &undo);

        //determine if any neighboring component editor rows can be exchanged
        bool IsMoveAllowed(const ComponentEditorVector& componentEditors) const;

        //determine if the specified component editor rows can be exchanged
        bool IsMoveAllowed(const ComponentEditorVector& componentEditors, size_t sourceComponentEditorIndex, size_t targetComponentEditorIndex) const;

        bool IsMoveComponentsUpAllowed() const;
        bool IsMoveComponentsDownAllowed() const;

        void ResetToSlice();

        bool DoesOwnFocus() const;
        QRect GetWidgetGlobalRect(const QWidget* widget) const;
        bool DoesIntersectWidget(const QRect& globalRect, const QWidget* widget) const;
        bool DoesIntersectSelectedComponentEditor(const QRect& globalRect) const;
        bool DoesIntersectNonSelectedComponentEditor(const QRect& globalRect) const;

        void ClearComponentEditorDragging();
        void ClearComponentEditorSelection();
        void SelectRangeOfComponentEditors(const AZ::s32 index1, const AZ::s32 index2, bool selected = true);
        void SelectIntersectingComponentEditors(const QRect& globalRect, bool selected = true);
        bool SelectIntersectingComponentEditorsSafe(const QRect& globalRect);
        void ToggleIntersectingComponentEditors(const QRect& globalRect);
        AZ::s32 GetComponentEditorIndex(const ComponentEditor* componentEditor) const;
        AZ::s32 GetComponentEditorIndexFromType(const AZ::Uuid& componentType) const;
        ComponentEditorVector GetIntersectingComponentEditors(const QRect& globalRect) const;

        const ComponentEditorVector& GetSelectedComponentEditors() const;
        const AZ::Entity::ComponentArrayType& GetSelectedComponents() const;
        const AZStd::unordered_map<AZ::EntityId, AZ::Entity::ComponentArrayType>& GetSelectedComponentsByEntityId() const;
        void UpdateSelectionCache();

        ComponentEditorVector m_selectedComponentEditors;
        AZ::Entity::ComponentArrayType m_selectedComponents;
        AZStd::unordered_map<AZ::EntityId, AZ::Entity::ComponentArrayType> m_selectedComponentsByEntityId;

        void SaveComponentEditorState();
        void SaveComponentEditorState(ComponentEditor* componentEditor);
        void LoadComponentEditorState();
        void ClearComponentEditorState();

        struct ComponentEditorSaveState
        {
            bool m_selected = false;
        };
        AZStd::unordered_map<AZ::ComponentId, ComponentEditorSaveState> m_componentEditorSaveStateTable;

        void UpdateOverlay();

        friend class EntityPropertyEditorOverlay;
        class EntityPropertyEditorOverlay* m_overlay = nullptr;

        //widget overrides
        void resizeEvent(QResizeEvent* event) override;
        void contextMenuEvent(QContextMenuEvent* event) override;
        bool eventFilter(QObject* object, QEvent* event) override;

        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dragMoveEvent(QDragMoveEvent* event) override;
        void dropEvent(QDropEvent* event) override;

        bool HandleSelectionEvents(QObject* object, QEvent* event);
        bool m_selectionEventAccepted;

        // drag and drop events
        QRect GetInflatedRectFromPoint(const QPoint& point, int radius) const;
        bool IsDragAllowed(const ComponentEditorVector& componentEditors) const;
        bool IsDropAllowed(const QMimeData* mimeData, const QPoint& posGlobal) const;
        bool IsDropAllowedForComponentReorder(const QMimeData* mimeData, const QPoint& posGlobal) const;
        bool CreateComponentWithAsset(const AZ::Uuid& componentType, const AZ::Data::AssetId& assetId);

        // given mimedata, filter it down to only the entries that can actually be spawned in this context.
        void GetCreatableAssetEntriesFromMimeData(const QMimeData* mimeData, ProductCallback callbackFunction) const;

        ComponentEditor* GetReorderDropTarget(const QRect& globalRect) const;
        bool ResetDrag(QMouseEvent* event);
        bool UpdateDrag(const QPoint& localPos, Qt::MouseButtons mouseButtons, const QMimeData* mimeData);
        bool StartDrag(QMouseEvent* event);
        bool HandleDrop(QDropEvent* event);
        bool HandleDropForComponentTypes(QDropEvent* event);
        bool HandleDropForComponentAssets(QDropEvent* event);
        bool HandleDropForAssetBrowserEntries(QDropEvent* event);
        bool HandleDropForComponentReorder(QDropEvent* event);
        bool CanDropForComponentTypes(const QMimeData* mimeData) const;
        bool CanDropForComponentAssets(const QMimeData* mimeData) const;
        bool CanDropForAssetBrowserEntries(const QMimeData* mimeData) const;
        AZStd::vector<AZ::s32> ExtractComponentEditorIndicesFromMimeData(const QMimeData* mimeData) const;
        ComponentEditorVector GetComponentEditorsFromIndices(const AZStd::vector<AZ::s32>& indices) const;
        ComponentEditor* GetComponentEditorsFromIndex(const AZ::s32 index) const;
        QPoint m_dragStartPosition;
        bool m_dragStarted;

        void ResetAutoScroll();
        void QueueAutoScroll();
        void UpdateAutoScroll();
        int m_autoScrollCount;
        int m_autoScrollMargin;
        bool m_autoScrollQueued;

        void UpdateInternalState();

        enum StatusType
        {
            StatusStartActive,
            StatusStartInactive,
            StatusEditorOnly,
            StatusItems
        };

        QStringList m_itemNames{ "Start active","Start inactive","Editor only" };

        void UpdateStatusComboBox();

        bool m_isBuildingProperties;

        Ui::EntityPropertyEditorUI* m_gui;

        // Global app serialization context, cached for internal usage during the life of the control.
        AZ::SerializeContext* m_serializeContext;

        AZ::s32 m_componentEditorLastSelectedIndex;
        AZ::s32 m_componentEditorsUsed;
        ComponentEditorVector m_componentEditors;

        using ComponentPropertyEditorMap = AZStd::unordered_map<AZ::Component*, ComponentEditor*>;
        ComponentPropertyEditorMap m_componentToEditorMap;

        ComponentPaletteWidget* m_componentPalette;

        AzToolsFramework::UndoSystem::URSequencePoint* m_currentUndoOperation;
        InstanceDataNode* m_currentUndoNode;

        bool m_sceneIsNew;

        // The busy system tracks when components are being changed, this allows
        // a refresh when the busy counter hits zero, in case multiple things are making
        // changes to an object to mark it as busy.
        int m_propertyEditBusy;

        bool m_isSystemEntityEditor;
        bool m_isLevelEntityEditor = false;

        // the spacer's job is to make sure that its always at the end of the list of components.
        QSpacerItem* m_spacer;
        bool m_isAlreadyQueuedRefresh;
        bool m_shouldScrollToNewComponents;
        bool m_shouldScrollToNewComponentsQueued;

        AZStd::string m_filterString;

        // IDs of entities currently bound to this property editor.
        EntityIdList m_selectedEntityIds;

        ComponentFilter m_componentFilter;
        // Tracks if a custom filter has been set. A comparison operator is not available for
        // the filter, so it can't be checked if it's the default filter.
        bool m_customFilterSet = false;

        // Pointer to entity that first entity is compared against for the purpose of rendering deltas vs. slice in the property grid.
        AZStd::unique_ptr<AZ::Entity> m_sliceCompareToEntity;

        // Temporary buffer to use when calculating a data patch address.
        AZ::DataPatch::AddressType m_dataPatchAddressBuffer;

        QIcon m_emptyIcon;
        QIcon m_clearIcon;
        QIcon m_checkmarkIcon;
        QIcon m_rectangleIcon;
        QIcon m_spacerIcon;

        QStandardItem* m_comboItems[StatusItems];
        EntityIdList m_overrideSelectedEntityIds;

        void GetSelectedEntities(EntityIdList& selectedEntityIds);
        bool IsLockedToSpecificEntities() { return m_overrideSelectedEntityIds.size() > 0; }

        // When m_initiatingPropertyChangeNotification is set to true, it means this EntityPropertyEditor is
        // broadcasting a change to all listeners about a property change for a given entity.  This is needed
        // so that we don't update the values twice for this inspector
        bool m_initiatingPropertyChangeNotification = false;
        void ConnectToEntityBuses(const AZ::EntityId& entityId);
        void DisconnectFromEntityBuses(const AZ::EntityId& entityId);

    private slots:
        void OnPropertyRefreshRequired(); // refresh is needed for a property.
        void UpdateContents();
        void OnAddComponent();
        void OnEntityNameChanged();
        void ScrollToNewComponent();
        void QueueScrollToNewComponent();
        void OnStatusChanged(int index);
        void BuildEntityIconMenu();

        void OnSearchTextChanged();
        void ClearSearchFilter();

        void OpenPinnedInspector();

        bool SelectedEntitiesAreFromSameSourceSliceEntity() const;

        void CloseInspectorWindow();

        AZ::Entity* GetSelectedEntityById(AZ::EntityId& entityId) const;
    };

    void SortComponentsByOrder(AZ::EntityId entityId, AZ::Entity::ComponentArrayType& componentsOnEntity);
    void SaveComponentOrder(AZ::EntityId entityId, const AZ::Entity::ComponentArrayType& componentsInOrder);

} // namespace AzToolsFramework

class StatusComboBox : public QComboBox
{
    Q_OBJECT
public:
    StatusComboBox(QWidget* parent = nullptr);

    void setHeaderOverride(QString overrideString);

protected:
    void paintEvent(QPaintEvent* event) override;
    void showPopup() override;

    QString m_headerOverride = "";
};

#endif