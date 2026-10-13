/** \file rtimvCommandInterface.cpp
 * \brief Routes optional plugin commands without introducing a protocol dependency.
 */
#include "rtimvCommandInterface.hpp"

#include <exception>

rtimvCommandRouter::rtimvCommandRouter( QObject *parent ) : QObject( parent )
{
    setObjectName( "rtimv.commandRouter" );
}

void rtimvCommandRouter::addProvider( QObject *object )
{
    if( !object || !qobject_cast<rtimvCommandInterface *>( object ) )
        return;
    for( const auto &existing : m_providers )
        if( existing == object )
            return;
    m_providers.push_back( object );
}

rtimvCommandInterface *rtimvCommandRouter::provider( const QString &protocol ) const
{
    rtimvCommandInterface *found = nullptr;
    for( const auto &object : m_providers )
    {
        if( !object )
            continue;
        auto *candidate = qobject_cast<rtimvCommandInterface *>( object.data() );
        if( candidate && candidate->commandProtocol() == protocol )
        {
            if( found )
                return nullptr;
            found = candidate;
        }
    }
    return found;
}

bool rtimvCommandRouter::send( const QString &protocol, const QVector<rtimvCommand> &commands, QString &error )
{
    error.clear();
    auto *target = provider( protocol );
    if( !target )
    {
        error = "Command provider unavailable or ambiguous";
        return false;
    }
    if( commands.empty() )
    {
        error = "Empty command";
        return false;
    }
    try
    {
        return target->sendCommands( commands, error );
    }
    catch( const std::exception &e )
    {
        error = QString::fromUtf8( e.what() );
    }
    catch( ... )
    {
        error = "Command failed";
    }
    return false;
}

rtimvPropertySnapshot
rtimvCommandRouter::snapshot( const QString &protocol, const QString &device, const QString &property ) const
{
    auto *source = provider( protocol );
    if( !source )
        return {};
    try
    {
        return source->propertySnapshot( device, property );
    }
    catch( ... )
    {
        return {};
    }
}
