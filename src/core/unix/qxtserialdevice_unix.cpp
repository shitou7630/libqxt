#include "qxtserialdevice.h"
/****************************************************************************
** Copyright (c) 2006 - 2011, the LibQxt project.
** See the Qxt AUTHORS file for a list of authors and copyright holders.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the LibQxt project nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
** <http://libqxt.org>  <foundation@libqxt.org>
*****************************************************************************/

#include "qxtserialdevice_p.h"
#include <QVarLengthArray>
#include <QSocketNotifier>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#if defined(Q_OS_BSD4) || defined(Q_OS_DARWIN) || defined(Q_OS_BSDI) || defined(Q_OS_NETBSD) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
// definition of FIONREAD
#include <sys/filio.h>
#endif

qint64 QxtSerialDevicePrivate::deviceBuffer() const {
    int bytes;
    ioctl(fd, FIONREAD, &bytes);
    return bytes;
}

qint64 QxtSerialDevice::bytesAvailable() const {
    return QIODevice::bytesAvailable() + qxt_d().buffer.length() + qxt_d().deviceBuffer();
}

void QxtSerialDevice::close() {
    if(isOpen()) {
        emit aboutToClose();
        tcsetattr(qxt_d().fd, TCSANOW, &qxt_d().reset);
        ::close(qxt_d().fd);
        qxt_d().notifier->setEnabled(false);
        qxt_d().notifier->deleteLater();
        qxt_d().notifier = 0;
        setOpenMode(QIODevice::NotOpen);
    }
}

bool QxtSerialDevice::open(OpenMode mode) {
    if(qxt_d().notifier) {
        delete qxt_d().notifier;
        qxt_d().notifier = 0;
    }
    int m;
    if(mode & QIODevice::ReadOnly && mode & QIODevice::WriteOnly) { // r/w
        m = O_RDWR;
    } else if(mode & QIODevice::ReadOnly) {
        m = O_RDONLY;
    } else {
        m = O_WRONLY;
    }
    qxt_d().fd = ::open(qxt_d().device.toLocal8Bit().constData(), m | O_NOCTTY);
    if(qxt_d().fd < 0) {
        // TODO: set error message
        return false;
    }
    fcntl(qxt_d().fd, F_SETFL, O_NONBLOCK);
    tcgetattr(qxt_d().fd, &qxt_d().reset);
    cfmakeraw(&qxt_d().settings);

    qxt_d().notifier = new QSocketNotifier(qxt_d().fd, QSocketNotifier::Read, this);
    if(mode & QIODevice::Unbuffered) {
        QObject::connect(qxt_d().notifier, SIGNAL(activated(int)), this, SIGNAL(readyRead()));
    } else {
        QObject::connect(qxt_d().notifier, SIGNAL(activated(int)), &qxt_d(), SLOT(fillBuffer()));
    }
    setOpenMode(mode);
    return qxt_d().updateSettings();
}

int QxtSerialDevicePrivate::constFillBuffer() const {
    notifier->setEnabled(false);
    int sz = deviceBuffer();
    QVarLengthArray<char, 1024> buf(sz);
    int rv = read(fd, buf.data(), sz);
    if(rv < 0) {
        notifier->setEnabled(true);
        if(errno == EAGAIN) return 0; // harmless
        return errno;
    }
    buffer += QByteArray(buf.constData(), rv);
    notifier->setEnabled(true);
    return 0;
}

int QxtSerialDevicePrivate::fillBuffer() {
    int sz = buffer.length();
    int rv = constFillBuffer();
    if(rv) {
        notifier->setEnabled(false);
        qxt_p().setErrorString(strerror(rv));
    }
    if(buffer.length() != sz)
        QMetaObject::invokeMethod(&qxt_p(), "readyRead", Qt::QueuedConnection);
    return rv;
}

bool QxtSerialDevice::setBaud(BaudRate rate) {
    switch(rate) {
    case Baud110: qxt_d().baud = B110; break;
    case Baud300: qxt_d().baud = B300; break;
    case Baud600: qxt_d().baud = B600; break;
    case Baud1200: qxt_d().baud = B1200; break;
    case Baud2400: qxt_d().baud = B2400; break;
    case Baud4800: qxt_d().baud = B4800; break;
    case Baud9600: qxt_d().baud = B9600; break;
    case Baud19200: qxt_d().baud = B19200; break;
    case Baud38400: qxt_d().baud = B38400; break;
    case Baud57600: qxt_d().baud = B57600; break;
    case Baud115200: qxt_d().baud = B115200; break;
    };
    return qxt_d().updateSettings();
}

QxtSerialDevice::BaudRate QxtSerialDevice::baud() const {
    switch(qxt_d().baud) {
    case B110: return Baud110;
    case B300: return Baud300;
    case B600: return Baud600;
    case B1200: return Baud1200;
    case B2400: return Baud2400;
    case B4800: return Baud4800;
    case B19200: return Baud19200;
    case B38400: return Baud38400;
    case B57600: return Baud57600;
    case B115200: return Baud115200;
    default: return Baud9600;
    };
}

qint64 QxtSerialDevice::readData(char* data, qint64 maxSize) {
    qxt_d().notifier->setEnabled(true);

    int b = bytesAvailable();
    if(maxSize > b) maxSize = b;
    if(!(openMode() & QIODevice::Unbuffered)) {
        if(qxt_d().fillBuffer()) return 0;
    }
    int bufsize = qxt_d().buffer.length();
    int rv = 0;
    if(maxSize <= bufsize) {
        memcpy(data, qxt_d().buffer.constData(), maxSize);
        qxt_d().buffer = qxt_d().buffer.mid(maxSize);
        return maxSize;
    } else {
        memcpy(data, qxt_d().buffer.constData(), bufsize);
        qxt_d().buffer.clear();
        rv = bufsize;
        data += bufsize;
        maxSize -= bufsize;
    }

    int readVal = ::read(qxt_d().fd, data, maxSize);
    if(readVal < 0) {
        qxt_d().notifier->setEnabled(false);
        setErrorString(strerror(errno));
        return -1;
    } else {
        rv += readVal;
    }
    return rv;
}

qint64 QxtSerialDevice::writeData(const char* data, qint64 maxSize) {
    int rv = ::write(qxt_d().fd, data, maxSize);
    if(rv < 0) {
        qxt_d().notifier->setEnabled(false);
        setErrorString(strerror(errno));
    }
    return rv;
}

bool QxtSerialDevicePrivate::setPortSettings(QxtSerialDevice::PortSettings setup) {
    int bits = setup & QxtSerialDevice::BitMask;
    switch(bits) {
    case QxtSerialDevice::Bit8: format = CS8; break;
    case QxtSerialDevice::Bit7: format = CS7; break;
    case QxtSerialDevice::Bit6: format = CS6; break;
    case QxtSerialDevice::Bit5: format = CS5; break;
    };
    if(setup & QxtSerialDevice::Stop2)
        format |= CSTOPB;
    int parity = setup & QxtSerialDevice::ParityMask;
    if(parity != QxtSerialDevice::ParityNone) {
        format |= PARENB;
        if(parity == QxtSerialDevice::ParityOdd) {
            format |= PARODD;
        } else if(parity == QxtSerialDevice::ParityMark || parity == QxtSerialDevice::ParitySpace) {
#ifdef CMSPAR
            format |= CMSPAR;
            if(parity == QxtSerialDevice::ParityMark)
                format |= PARODD;
#else
            qxt_p().setErrorString("Space/Mark parity not supported");
            return false;
#endif
        }
    }
    int flowbits = setup & QxtSerialDevice::FlowMask;
    if(flowbits == QxtSerialDevice::FlowRtsCts) {
#ifdef CRTSCTS
        flow = CRTSCTS;
#else
        qxt_p().setErrorString("Hardware flow control not supported");
        return false;
#endif
    } else if(flowbits == QxtSerialDevice::FlowXonXoff) {
        flow = IXON | IXOFF;
    }
    return updateSettings();
}

bool QxtSerialDevicePrivate::updateSettings() {
    if(qxt_p().isOpen()) {
        settings.c_cflag = baud | flow | format | CLOCAL | CREAD;
        tcflush(fd, TCIFLUSH);
        if(tcsetattr(fd, TCSANOW, &settings)) {
            notifier->setEnabled(false);
            qxt_p().setErrorString(strerror(errno));
            return false;
        }
    }
    return true;
}
