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

#include <QCompleter>
#include <QAbstractItemModel>
#include <QRegExp>
#include <QString>
#include <QSortFilterProxyModel>
#include <QTableView>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <ScriptCanvas/Data/Data.h>

#include <GraphCanvas/Widgets/StyledItemDelegates/IconDecoratedNameDelegate.h>

namespace ScriptCanvasEditor
{ 
    class ContainerWizard;
    class DataTypePaletteSortFilterProxyModel;
    class DataTypePaletteModel;

    class VariablePaletteTableView
        : public QTableView
    {
        Q_OBJECT
    public:
        VariablePaletteTableView(QWidget* parent);
        ~VariablePaletteTableView();

        void SetActiveScene(const AZ::EntityId& scriptCanvasGraphId);
        
        void PopulateVariablePalette(const AZStd::unordered_set< AZ::Uuid >& objectTypes);
        void SetFilter(const QString& filter);

        QCompleter* GetVariableCompleter();
        void TryCreateVariableByTypeName(const AZStd::string& typeName);        

        // QObject
        void hideEvent(QHideEvent* hideEvent) override;
        void showEvent(QShowEvent* showEvent) override;
        ////

    public slots:
        void OnClicked(const QModelIndex& modelIndex);
        void OnContainerPinned(const AZ::TypeId& typeId);

    signals:
        void CreateVariable(const ScriptCanvas::Data::Type& variableType);
        void CreateNamedVariable(const AZStd::string& variableName, const ScriptCanvas::Data::Type& variableType);

    private:

        void OnCreateContainerVariable(const AZStd::string& variableName, const AZ::TypeId& typeId);

        ContainerWizard*                        m_containerWizard;

        DataTypePaletteSortFilterProxyModel*    m_proxyModel;
        DataTypePaletteModel*                   m_model;

        QCompleter*                             m_completer;
    };
}