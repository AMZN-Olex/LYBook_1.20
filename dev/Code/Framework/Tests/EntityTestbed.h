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

#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/IO/StreamerComponent.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/PlatformIncl.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Asset/AssetCatalogComponent.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
//#include <AzToolsFramework/UI/Outliner/OutlinerWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyManagerComponent.h>
#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QApplication>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QFileDialog>
#include <QtCore/QTimer>

#pragma once

namespace UnitTest
{
    using namespace AZ;

    class EntityTestbed
        : public AllocatorsFixture
        , public QObject
    {
    public:

        class TestbedApplication
            : public AzToolsFramework::ToolsApplication
        {
        public:
            AZ_CLASS_ALLOCATOR(TestbedApplication, AZ::SystemAllocator, 0);

            TestbedApplication(EntityTestbed& testbed)
                : m_testbed(testbed) {}

            void AddSystemComponents(AZ::Entity* systemEntity) override
            {
                AzToolsFramework::ToolsApplication::AddSystemComponents(systemEntity);

                m_testbed.OnReflect(*GetSerializeContext(), *systemEntity);
            }

            EntityTestbed& m_testbed;
        };

        QTimer* m_tickBusTimer                                      = nullptr;
        TestbedApplication* m_componentApplication                  = nullptr;
        AZ::Entity* m_systemEntity                                  = nullptr;
        QApplication* m_qtApplication                               = nullptr;
        QWidget* m_window                                           = nullptr;
        //AzToolsFramework::OutlinerWidget* m_outliner                = nullptr;
        AzToolsFramework::EntityPropertyEditor* m_propertyEditor    = nullptr;
        AZ::u32 m_entityCounter                                     = 0;
        AZ::IO::LocalFileIO m_localFileIO;

        EntityTestbed()
            : AllocatorsFixture()
        {
        }

        virtual ~EntityTestbed()
        {
            if (m_tickBusTimer)
            {
                m_tickBusTimer->stop();
                delete m_tickBusTimer;
                m_tickBusTimer = nullptr;
            }

            Destroy();
        }

        virtual void OnSetup() {}
        virtual void OnAddButtons(QHBoxLayout& layout) { (void)layout; }
        virtual void OnEntityAdded(AZ::Entity& entity) { (void)entity; }
        virtual void OnEntityRemoved(AZ::Entity& entity) { (void)entity; }
        virtual void OnReflect(AZ::SerializeContext& context, AZ::Entity& systemEntity) { (void)context; (void)systemEntity; }
        virtual void OnDestroy() {}

        void Run(int argc = 0, char** argv = nullptr)
        {
            SetupComponentApplication();

            QString libraryPath = "";
            EBUS_EVENT_RESULT(libraryPath, AZ::ComponentApplicationBus, GetExecutableFolder);
            libraryPath = QString("%1%2%3%4").arg(libraryPath, "qtlibs", QDir::separator(), "plugins");
            QApplication::addLibraryPath(libraryPath);

            m_qtApplication = new QApplication(argc, argv);

            m_tickBusTimer = new QTimer(this);
            m_qtApplication->connect(m_tickBusTimer, &QTimer::timeout,
                []()
                {
                    AZ::TickBus::ExecuteQueuedEvents();
                    EBUS_EVENT(AZ::TickBus, OnTick, 0.3f, AZ::ScriptTimePoint());
                }
                );

            m_tickBusTimer->start();

            SetupUI();

            OnSetup();

            m_window->show();
            m_qtApplication->exec();
        }

        void SetupUI()
        {
            m_window = new QWidget();
            //m_outliner = aznew AzToolsFramework::OutlinerWidget(nullptr);
            m_propertyEditor = aznew AzToolsFramework::EntityPropertyEditor(nullptr);

            AZ::SerializeContext* serializeContext = nullptr;
            EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);

            m_window->setMinimumHeight(600);
            m_propertyEditor->setMinimumWidth(600);
            //m_outliner->setMinimumWidth(100);

            QVBoxLayout* leftLayout = new QVBoxLayout();
            QHBoxLayout* outlinerLayout = new QHBoxLayout();
            QHBoxLayout* outlinerButtonLayout = new QHBoxLayout();
            //outlinerLayout->addWidget(m_outliner);
            leftLayout->addLayout(outlinerLayout);
            leftLayout->addLayout(outlinerButtonLayout);

            QVBoxLayout* rightLayout = new QVBoxLayout();
            QHBoxLayout* propertyLayout = new QHBoxLayout();
            QHBoxLayout* propertyButtonLayout = new QHBoxLayout();
            propertyLayout->addWidget(m_propertyEditor);
            rightLayout->addLayout(propertyLayout);
            rightLayout->addLayout(propertyButtonLayout);

            QHBoxLayout* mainLayout = new QHBoxLayout();
            m_window->setLayout(mainLayout);

            mainLayout->addLayout(leftLayout, 1);
            mainLayout->addLayout(rightLayout, 3);

            // Add default buttons.
            QPushButton* addEntity = new QPushButton(QString("Create"));
            QPushButton* deleteEntities = new QPushButton(QString("Delete"));
            outlinerButtonLayout->addWidget(addEntity);
            outlinerButtonLayout->addWidget(deleteEntities);
            m_qtApplication->connect(addEntity, &QPushButton::pressed, [ this ]() { this->AddEntity(); });
            m_qtApplication->connect(deleteEntities, &QPushButton::pressed, [ this ]() { this->DeleteSelected(); });

            // Test-specific buttons.
            OnAddButtons(*outlinerButtonLayout);
        }

        void SetupComponentApplication()
        {
            AZ::ComponentApplication::Descriptor desc;
            desc.m_enableDrilling = true;
            desc.m_allocationRecords = true;
            desc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_FULL;
            desc.m_stackRecordLevels = 10;
            desc.m_useExistingAllocator = true;
            m_componentApplication = aznew TestbedApplication(*this);

            AZ::IO::FileIOBase::SetInstance(&m_localFileIO);

            m_componentApplication->Start(desc);

            m_componentApplication->CalculateAppRoot();
            AZ::SerializeContext* serializeContext = m_componentApplication->GetSerializeContext();
            serializeContext->CreateEditContext();

            AzToolsFramework::Components::PropertyManagerComponent::CreateDescriptor();

            const char* dir = m_componentApplication->GetExecutableFolder();
            m_componentApplication->SetAssetRoot(dir);

            m_localFileIO.SetAlias("@assets@", dir);
            m_localFileIO.SetAlias("@devassets@", dir);
        }

        void Destroy()
        {
            OnDestroy();

            //delete m_outliner;
            delete m_propertyEditor;
            delete m_window;
            delete m_qtApplication;
            delete m_componentApplication;

            //m_outliner = nullptr;
            m_propertyEditor = nullptr;
            m_window = nullptr;
            m_qtApplication = nullptr;
            m_componentApplication = nullptr;

            if (AZ::Data::AssetManager::IsReady())
            {
                AZ::Data::AssetManager::Destroy();
            }

            AZ::IO::FileIOBase::SetInstance(nullptr);
        }

        void AddEntity()
        {
            AZStd::string entityName = AZStd::string::format("Entity%u", m_entityCounter);
            AZ::Entity* entity = nullptr;
            EBUS_EVENT_RESULT(entity, AzToolsFramework::EditorEntityContextRequestBus, CreateEditorEntity, entityName.c_str());
            ++m_entityCounter;

            entity->Deactivate();
            OnEntityAdded(*entity);
            entity->Activate();
        }

        void DeleteSelected()
        {
            EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, DeleteSelected);
        }

        void SaveRoot()
        {
            const QString saveAs = QFileDialog::getSaveFileName(nullptr,
                    QString("Save As..."), QString("."), QString("Xml Files (*.xml)"));
            if (!saveAs.isEmpty())
            {
                AZ::SliceComponent* rootSlice;
                EBUS_EVENT_RESULT(rootSlice, AzToolsFramework::EditorEntityContextRequestBus, GetEditorRootSlice);
                AZ::Utils::SaveObjectToFile(saveAs.toUtf8().constData(), AZ::DataStream::ST_XML, rootSlice->GetEntity());
            }
        }

        void ResetRoot()
        {
            EBUS_EVENT(AzToolsFramework::EditorEntityContextRequestBus, ResetEditorContext);
        }
    };
} // namespace UnitTest;
