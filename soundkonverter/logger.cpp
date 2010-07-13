
#include "logger.h"

#include <klocale.h>
#include <kstandarddirs.h>

#include <cstdlib>
#include <ctime>


LoggerItem::LoggerItem()
{}

LoggerItem::~LoggerItem()
{}


Logger::Logger( QObject *parent)
    : QObject( parent )
{
    LoggerItem *item = new LoggerItem();
    item->filename = KUrl("soundKonverter");
    item->id = 1000;
    item->completed = true;
    item->state = 1;
    item->file.setFileName( KStandardDirs::locateLocal("data",QString("soundkonverter/log/%1.log").arg(item->id)) );
    // TODO error handling
    item->file.open( QIODevice::WriteOnly );
    item->textStream.setDevice( &(item->file) );

    processes.append( item );

    srand( (unsigned)time(NULL) );
}

Logger::~Logger()
{
    for( QList<LoggerItem*>::Iterator it = processes.begin(); it != processes.end(); ++it )
    {
        (*it)->file.close();
        (*it)->file.remove();
        delete *it;
    }
}

// void Logger::cleanUp()
// {
//     for( QList<LoggerItem*>::Iterator it = processes.begin(); it != processes.end(); ++it )
//     {
//         if( (*it)->id != 1000 )
//         {
//             emit removedProcess( (*it)->id );
//             (*it)->file.close();
//             (*it)->file.remove();
//             delete *it;
//         }
//     }
//     processes.clear();
// }

int Logger::registerProcess( const KUrl& filename )
{
    // NOTE ok, it works now, but why prepend() and not append()?
    LoggerItem *item = new LoggerItem();
    item->filename = filename;
    item->id = getNewID();
    item->completed = false;
    item->file.setFileName( KStandardDirs::locateLocal("data",QString("soundkonverter/log/%1.log").arg(item->id)) );
    // TODO error handling
    item->file.open( QIODevice::WriteOnly );
    item->textStream.setDevice( &(item->file) );

    processes.append( item );
    
    log( item->id, i18n("Filename") + ": " + item->filename.pathOrUrl() );
    log( item->id, i18n("Log ID") + ": " + QString::number(item->id) );

    emit updateProcess( item->id );

    return item->id;
}

void Logger::log( int id, const QString& data )
{
    for( QList<LoggerItem*>::Iterator it = processes.begin(); it != processes.end(); ++it )
    {
        if( (*it)->id == id )
        {
            (*it)->data.append( data );
            if( (*it)->data.count() > 10000 ) (*it)->data.removeFirst();
            (*it)->textStream << data;
            (*it)->textStream << "\n";
            (*it)->textStream.flush(); // TODO make file saving configureable
            if( id == 1000 ) emit updateProcess( id );
            return;
        }
    }
}

int Logger::getNewID()
{
    bool ok;
    int id;

    do {
        id = rand();
        ok = true;

        for( QList<LoggerItem*>::Iterator it = processes.begin(); it != processes.end(); ++it )
        {
            if( (*it)->id == id ) ok = false;
        }

    } while( !ok );

    return id;
}

LoggerItem* Logger::getLog( int id )
{
    for( QList<LoggerItem*>::Iterator it = processes.begin(); it != processes.end(); ++it )
    {
        if( (*it)->id == id ) return *it;
    }

    return 0;
}

QList<LoggerItem*> Logger::getLogs()
{
/*    QList<LoggerItem*> items;

    for( QList<LoggerItem*>::Iterator it = processes.begin(); it != processes.end(); ++it ) {
        if( (*it)->completed ) items.append( *it );
    }

    return items;*/
    return processes;
}

void Logger::processCompleted( int id, int state )
{
    LoggerItem* removeItem = 0;
    QTime time = QTime::currentTime();

    for( int i=0; i<processes.count(); i++ )
    {
        if( processes.at(i)->time < time && processes.at(i)->completed && processes.at(i)->state == 0 )
        {
            time = processes.at(i)->time;
            removeItem = processes.at(i);
        }
        else if( processes.at(i)->id == id )
        {
            processes.at(i)->state = state;
            processes.at(i)->completed = true;
            processes.at(i)->time = processes.at(i)->time.currentTime();
            processes.at(i)->textStream << i18n("Finished logging");
            processes.at(i)->file.close();
            emit updateProcess( id );
        }
    }

    if( removeItem && processes.count() > 11 )
    {
        emit removedProcess( removeItem->id );
        removeItem->file.remove();
        processes.removeAt( processes.indexOf(removeItem) );
        delete removeItem;
    }
}

