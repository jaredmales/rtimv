/** \file rtimvDictionary.cpp
 * \brief Storage and synchronized access for dictionary payloads.
 */
#include "rtimvDictionary.hpp"

#include <cstdlib>
#include <cstring>

void rtimvDictBlob::clearBlob()
{
    if( m_blob && m_owner )
    {
        free( m_blob );
        m_blob = nullptr;
        m_owner = false;
        m_sz = 0;
        m_memSz = 0;
    }
}

rtimvDictBlob::rtimvDictBlob()
{
}

rtimvDictBlob::rtimvDictBlob( rtimvDictBlob &rdb )
{
    clearBlob();
    m_blob = rdb.m_blob;
    m_sz = rdb.m_sz;
    m_memSz = rdb.m_memSz;
    m_owner = rdb.m_owner;
    rdb.m_owner = false;
}

void rtimvDictBlob::setBlob( const void *blob, size_t sz )
{
    std::unique_lock<std::mutex> lock( m_mutex );
    if( sz > m_memSz || m_owner == false )
    {
        clearBlob();

        if( sz > 0 )
        {
            m_blob = malloc( sz );
            m_sz = sz;
            m_memSz = sz;
            m_owner = true;
        }

        if( blob != nullptr && sz > 0 )
        {
            memcpy( m_blob, blob, sz );
        }
    }
    else
    {
        // We're the owner and not reallocating
        m_sz = sz;
        if( blob != nullptr && sz > 0 )
        {
            memcpy( m_blob, blob, sz );
        }
    }

    clock_gettime( CLOCK_REALTIME, &m_lastMod );
}

size_t rtimvDictBlob::getBlob( char *blob, size_t mxsz, timespec *ts )
{
    std::unique_lock<std::mutex> lock( m_mutex );

    size_t cpsz = m_sz;
    if( cpsz > mxsz )
        cpsz = mxsz;

    if( m_blob )
    {
        memcpy( blob, m_blob, cpsz );
    }
    else
        cpsz = 0;

    if( ts != nullptr )
        *ts = m_lastMod;

    return cpsz;
}

size_t rtimvDictBlob::getBlobStr( char *blob, size_t mxsz, timespec *ts )
{
    size_t cpsz = getBlob( blob, mxsz, ts );

    if( cpsz == 0 )
    {
        if( mxsz > 0 )
        {
            blob[0] = '\0';
            cpsz = 1;
        }
    }
    else
        blob[cpsz - 1] = '\0';

    return cpsz;
}

size_t rtimvDictBlob::getBlobSize()
{
    std::unique_lock<std::mutex> lock( m_mutex );

    if( m_blob == nullptr )
        return 0;
    return m_sz;
}

rtimvDictBlob::~rtimvDictBlob()
{
    std::unique_lock<std::mutex> lock( m_mutex );
    clearBlob();
}
