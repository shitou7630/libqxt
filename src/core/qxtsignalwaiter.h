/****************************************************************************
**
** Copyright (C) Qxt Foundation. Some rights reserved.
**
** This file is part of the QxtCore module of the Qxt library.
**
** This library is free software; you can redistribute it and/or modify it
** under the terms of the Common Public License, version 1.0, as published by
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
** <http://libqxt.org>  <foundation@libqxt.org>
**
****************************************************************************/


#ifndef QXTSIGNALWAITER_H
#define QXTSIGNALWAITER_H
#include <qxtglobal.h>
#include <qxtpimpl.h>

#include <QObject>
#include <QEventLoop>

class QxtSignalWaiterPrivate;

class QXT_CORE_EXPORT QxtSignalWaiter : public QObject
{
    Q_OBJECT
    QXT_DECLARE_PRIVATE(QxtSignalWaiter)
public:
    QxtSignalWaiter(const QObject* sender, const char* signal);

    static bool wait(const QObject* sender, const char* signal, int msec = -1, QEventLoop::ProcessEventsFlags flags = QEventLoop::AllEvents);
    bool wait(int msec = -1, QEventLoop::ProcessEventsFlags flags = QEventLoop::AllEvents);
    bool hasCapturedSignal() const;

public Q_SLOTS:
    void signalCaught();
    void cancelWait();

private:
    void timerEvent(QTimerEvent* event);
};

#endif
