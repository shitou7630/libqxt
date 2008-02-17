/****************************************************************************
**
** Copyright (C) Qxt Foundation. Some rights reserved.
**
** This file is part of the QxtGui module of the Qt eXTension library
**
** This library is free software; you can redistribute it and/or modify it
** under the terms of th Common Public License, version 1.0, as published by
** IBM.
** 
** This file is provided "AS IS", without WARRANTIES OR CONDITIONS OF ANY
** KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT LIMITATION, ANY
** WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY OR
** FITNESS FOR A PARTICULAR PURPOSE.
**
** You should have received a copy of the CPL along with this file.
** See the LICENSE file and the cpl1.0.txt file included with the source
** distribution for more information. If you did not receive a copy of the
** license, contact the Qxt Foundation.
**
** <http://libqxt.sourceforge.net>  <libqxt@gmail.com>
**
****************************************************************************/
#ifndef QXTSCHEDULEVIEW_H_INCLUDED
#define QXTSCHEDULEVIEW_H_INCLUDED

#include <QAbstractScrollArea>
#include <QAbstractItemView>
#include <QTime>
#include <QDate>
#include <QDateTime>
#include <QHeaderView>
#include <QRect>

#include "qxtpimpl.h"
#include "qxtglobal.h"
#include "qxtnamespace.h"

class QxtScheduleItemDelegate;
class QResizeEvent;
class QxtScheduleViewPrivate;
class QxtScheduleInternalItem;
class QAbstractItemModel;
class QWidget;

#if 0
enum
{
    TIMERANGEPERCOL = 86400,
    TIMESTEP = 900
};
#endif

/**
@author Benjamin Zeller <zbenjamin@users.sourceforge.net>
*/
class QXT_GUI_EXPORT QxtScheduleView : public QAbstractScrollArea
{
    Q_OBJECT

    QXT_DECLARE_PRIVATE(QxtScheduleView);
    friend class QxtScheduleInternalItem;
    friend class QxtScheduleViewHeaderModel;

    public:
        
        enum ViewMode
        {
            MinuteView,
            HourView,
            DayView,
            CustomView
        };
    
        QxtScheduleView                                               ( QWidget *parent = 0 );
        
        void                        setModel                        ( QAbstractItemModel *model );
        QAbstractItemModel*         model                           ( ) const;
        
        void                        setViewMode                     ( const QxtScheduleView::ViewMode mode );
        QxtScheduleView::ViewMode   viewMode                        ( ) const;
        
        void                        setDateRange                    ( const QDate & fromDate , const QDate & toDate );
        void                        setTimeRange                    ( const QDateTime &fromDateTime , const QDateTime &toTime );
        
        void                        setZoomStepWidth                ( const int zoomWidth , const Qxt::Timeunit unit = Qxt::Second );
        void                        setCurrentZoomDepth             ( const int depth , const Qxt::Timeunit unit = Qxt::Second );
        int                         currentZoomDepth                ( const Qxt::Timeunit unit = Qxt::Second);
        
        QPoint                      mapFromViewport                 ( const QPoint& point ) const;
        QPoint                      mapToViewport                   ( const QPoint& point ) const;
        
        QModelIndex                 indexAt                         ( const QPoint &pt );
        void                        raiseItem                       ( const QModelIndex &index );
        void                        handleItemConcurrency           ( const QModelIndex &index );
        QModelIndex                 currentIndex                    ( );
        int                         rows                            ( ) const;
        int                         cols                            ( ) const;

    Q_SIGNALS:
        void                        itemMoved                       ( int rows,int cols, QModelIndex index );
        void                        indexSelected                   ( QModelIndex index );
        void                        indexClicked                    ( QModelIndex index );
        void                        contextMenuRequested            ( QModelIndex index );
        void                        newZoomDepth                    ( const int newDepthInSeconds );
        void                        viewModeChanged                 ( const int newViewMode );
        

    protected:
        virtual int                 timePerColumn                   ( ) const;
        
        virtual void                scrollContentsBy                ( int dx, int dy );
        virtual void                paintEvent                      ( QPaintEvent  *e );
        virtual void                mouseMoveEvent                  ( QMouseEvent  * e );
        virtual void                mousePressEvent                 ( QMouseEvent  * e );
        virtual void                mouseReleaseEvent               ( QMouseEvent  * e );
        virtual void                resizeEvent                     ( QResizeEvent * e );
        virtual void                wheelEvent                      ( QWheelEvent  * e ); 

    public Q_SLOTS:        
        void                        dataChanged                     (const QModelIndex & topLeft,const  QModelIndex & bottomRight);
        void                        updateGeometries                ( );
        void                        zoomIn                          ( );
        void                        zoomOut                         ( );

    protected Q_SLOTS:
        virtual void                rowsAboutToBeRemoved            ( const QModelIndex & parent, int start, int end );
        virtual void                rowsAboutToBeInserted           ( const QModelIndex & parent, int start, int end );
        virtual void                rowsRemoved                     ( const QModelIndex & parent, int start, int end );
        virtual void                rowsInserted                    ( const QModelIndex & parent, int start, int end );
};

#endif //QXTSCHEDULEVIEW_H_INCLUDED