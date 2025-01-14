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

#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QVector>
#include <QMap>
#include <QWidget>
#include <QTypeInfo>

namespace AzQtComponents
{
    namespace LogicalTabOrderingInternal
    {
        struct TabKeyEntry
        {
            QWidget* next;
            QWidget* prev;
        };

        AZ_QT_COMPONENTS_API bool focusNextPrevChild(bool next, QMap<QObject*, TabKeyEntry>& entries, QWidget* first, QWidget* last);
        AZ_QT_COMPONENTS_API void findTabKeyOrdering(QVector<QWidget*>& items, QWidget* widget);
        AZ_QT_COMPONENTS_API void createTabKeyMapping(const QVector<QWidget*>& items, QMap<QObject*, TabKeyEntry>& entries);
    } // namespace Internal

    /**
     * LogicalTabOrderingWidget
     *
     * Use this under a number of different configurations to pick a color and manipulate palettes.
     *
     */
    template <typename WidgetType>
    class LogicalTabOrderingWidget : public WidgetType
    {
    public:
        explicit LogicalTabOrderingWidget(QWidget* parent = nullptr) : WidgetType(parent) {}

        bool focusNextPrevChild(bool next) override;

        void markToRecalculateTabKeyOrdering() { m_dirty = true; }

    private:
        void recalculate();

        bool m_dirty = true;
        QMap<QObject*, LogicalTabOrderingInternal::TabKeyEntry> m_entries;
        QWidget* m_first;
        QWidget* m_last;
    };

    template <typename WidgetType>
    bool LogicalTabOrderingWidget<WidgetType>::focusNextPrevChild(bool next)
    {
        if (m_dirty)
        {
            recalculate();
            m_dirty = false;
        }

        bool customLogicWorked = LogicalTabOrderingInternal::focusNextPrevChild(next, m_entries, m_first, m_last);
        if (customLogicWorked)
        {
            return true;
        }

        // fallback
        return WidgetType::focusNextPrevChild(next);
    }

    template <typename WidgetType>
    void LogicalTabOrderingWidget<WidgetType>::recalculate()
    {
        using namespace LogicalTabOrderingInternal;

        m_entries.clear();

        m_first = m_last = nullptr;

        QVector<QWidget*> items;
        findTabKeyOrdering(items, this);

        if (items.empty())
        {
            return;
        }

        // connect the destroyed signal to recalculate
        for (QWidget* widget : items)
        {
            QObject::connect(widget, &QObject::destroyed, this, &LogicalTabOrderingWidget<WidgetType>::markToRecalculateTabKeyOrdering, Qt::UniqueConnection);
        }

        createTabKeyMapping(items, m_entries);

        m_first = items.first();
        m_last = items.last();
    }
} // namespace AzQtComponents

// Declare Qt Type Info, at global scope, otherwise a warning on some compilers will be generated if TabKeyEntry is used in a QMap.
// Qt will default to treating the struct as a complex type and call the destructor for TabKeyEntry in QMap's
// destructor, resulting in a compiler warning about removing a recursive method with no side effects.
Q_DECLARE_TYPEINFO(AzQtComponents::LogicalTabOrderingInternal::TabKeyEntry, Q_MOVABLE_TYPE | Q_PRIMITIVE_TYPE);




