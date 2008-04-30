/*!
\class QxtDaemon QxtDaemon
\ingroup QxtCore
\brief Qt Implementation of a Unix daemon

QxtDaemon will provide you with a standard UNIX daemon implementation.\n
after sucessfull forking it will install a messageHandler wich logs all qDebug/qWarning/etc... output to /var/log/mydaemon.log \n


example usage:\n
\code
int main(int argc, char ** argv)
    {
    QxtDaemon d("exampled");
    qDebug("about to fork");
    d.daemonize();
    qDebug("forked");
    d.changeUser("nobody");
    QCoreApplication app(argc,argv);
    return app.exec();
    }
\endcode
*/

/*!
\fn QxtDaemon::QxtDaemon(QString applicationName=QCoreApplication::applicationName());
constructs a new QxtDaemon \n
the applicationName is used as a base for log and pid files
*/
/*!
\fn bool QxtDaemon::daemonize(bool pidfile=true);
forks the current Process \n
you can specify weather it will write a pidfile to /var/run/mydaemon.pid or not \n
if you specify true (the default) QxtDaemon will also try to lock the pidfile. If it can't get a lock it will assume another daemon of the same name is already running and exit \n
be aware that after calling this function all file descriptors are invalid. QFile will not detect the change, you have to explicitly close all files before forking.\n
return true on sucess
*/
/*!
\fn int QxtDaemon::pid();
returns the current processId
*/
/*!
\fn  bool QxtDaemon::changeUser(QString name);
changes the current user of this process. \n
do this after forking to drop root rights.\n
returns true on sucess
*/
#include "qxtdaemon.h"
#include <cassert>
#include <QFile>
#include <QCoreApplication>
#include <QDateTime>


#ifdef Q_OS_UNIX
    #include <signal.h>
    #include <fcntl.h>
    #include <pwd.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <sys/types.h>
#endif


static QxtDaemon * qxt_daemon_singleton=0;

#ifdef Q_OS_UNIX
void QxtDaemon::signalHandler(int sig)
{
        emit(qxt_daemon_singleton->signal(sig));
        if(sig==SIGHUP)
        {
                qDebug("hangup signal catched");
                emit(qxt_daemon_singleton->hangup());
        }
        else if(sig==SIGTERM)
        {
                qDebug("terminate signal catched");
                emit(qxt_daemon_singleton->terminate());
                QCoreApplication::exit(0);
        }
}
#endif

 void QxtDaemon::messageHandler(QtMsgType type, const char *msg)
{
    QFile * f=qxt_daemon_singleton->logfile;
    f->write("[");    
    f->write(QDateTime::currentDateTime ().toString(Qt::ISODate).toLocal8Bit());
    f->write("] ");
    if (type==QtDebugMsg)
        f->write("[qDebug] ");
    else if (type==QtWarningMsg)
        f->write("[qWarning] ");
    else if (type==QtCriticalMsg)
        f->write("[qCritical] ");
    else if (type==QtFatalMsg)
    {
        f->write("[qFatal] ");
        f->write(msg);
        f->write("\n");
        f->write("aborting \n");
        f->flush();
        abort();
    }

    f->write(msg);
    f->write("\n");
    f->flush();
}

QxtDaemon::QxtDaemon(QString name)
{
#ifdef Q_OS_UNIX
    if(name.isEmpty())
    {
        qFatal("you need to set an applicationName (e.g. using  QCoreApplication::setApplicationName() )");
    }

    if(qxt_daemon_singleton)
    {
        qFatal("unable to construct more then one QxtDaemon instance");
    }
    else
    {
        qxt_daemon_singleton=this;
        m_name=name;
        logfile=new QFile("/var/log/"+m_name+".log");
    }
#else
    qFatal("currently QxtDaemon is only implemented on unix");
#endif
}

/*based on work of Levent Karakas <levent at mektup dot at> May 2001*/
bool QxtDaemon::daemonize(bool pidfile)
{
#ifdef Q_OS_UNIX
    if(!logfile->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
          qFatal("cannot open logfile %s",qPrintable(logfile->fileName()));
     logfile->close();



    if (pidfile)
    {
        QFile f("/var/run/"+m_name+".pid");
        if(!f.open(QIODevice::WriteOnly | QIODevice::Text))
            qFatal("cannot open pidfile \"/var/run/%s.pid\"",qPrintable(m_name));
        if (lockf(f.handle(),F_TEST,0)<0)
            qFatal("can't get a lock on \"/var/run/%s.pid\". another instance is propably already running.",qPrintable(m_name));
        f.close();
    }

    if(!logfile->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
          qFatal("cannot open logfile %s",qPrintable(logfile->fileName()));
    logfile->close();

    int i;
    if(getppid()==1) return true; /* already a daemon */
    i=fork();


    /* double fork.*/
    if (i<0) return false;  /*fork error    */
    if (i>0) exit(0);       /*parent exits  */
    if (i<0) return false;  /*fork error    */
    if (i>0) exit(0);       /*parent exits  */

    /* child (daemon) continues */
    setsid(); /* obtain a new process group */

    for (i=getdtablesize();i>=0;--i) close(i); /* close all descriptors */


    #ifdef Q_OS_LINUX
    ::umask(027); /* some programmers don't understand security, so we set a sane default here */
    #endif
    ::signal(SIGCHLD,SIG_IGN); /* ignore child */
    ::signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
    ::signal(SIGTTOU,SIG_IGN);
    ::signal(SIGTTIN,SIG_IGN);
    ::signal(SIGHUP,  QxtDaemon::signalHandler); /* catch hangup signal */
    ::signal(SIGTERM, QxtDaemon::signalHandler); /* catch kill signal */

    if (pidfile)
    {
        int lfp=::open(qPrintable("/var/run/"+m_name+".pid"),O_RDWR|O_CREAT,0640);
        if (lfp<0)
            qFatal("cannot open pidfile \"/var/run/%s.pid\"",qPrintable(m_name));
        if (lockf(lfp,F_TLOCK,0)<0)
            qFatal("can't get a lock on \"/var/run/%s.pid\". another instance is propably already running.",qPrintable(m_name));

        QByteArray d=QByteArray::number(pid());
        ::write(lfp,d.constData(),d.size());


    }


    assert(logfile->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append));
    qInstallMsgHandler(QxtDaemon::messageHandler);

    return true;
#else
    return false;
#endif
}


int QxtDaemon::pid()
{
#ifdef Q_OS_UNIX
    return getpid();
#else
    return -1;
#endif
}

bool QxtDaemon::changeUser(QString name)
{
#ifdef Q_OS_UNIX
    ///acording to the posix spec, i'm not suposed to delete it. *shrug*  weird
    passwd *p=::getpwnam(qPrintable(name));
    if(!p)
        return false;
    return setuid(p->pw_uid)==0;
#else
    return -1;
#endif
}

