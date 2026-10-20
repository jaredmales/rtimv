/** \file rtimvDictionary_test.cpp
 * \brief Characterize dictionary storage, ownership, and linked-library behavior.
 */
#include "rtimvDictionary.hpp"

#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

static_assert( std::is_constructible_v<rtimvDictBlob, rtimvDictBlob &> );
static_assert( !std::is_constructible_v<rtimvDictBlob, const rtimvDictBlob &> );
static_assert( std::is_same_v<dictionaryT, std::map<std::string, rtimvDictBlob>> );
static_assert( std::is_same_v<dictionaryIteratorT, dictionaryT::iterator> );

namespace
{
/// Fail with a useful message even when assertions are disabled.
void require( bool passed,        /**< [in] Condition under test. */
              const char *message /**< [in] Failure description. */
)
{
    if( !passed )
        throw std::runtime_error( message );
}

/// Check payload length and bytes without allowing writes past the payload.
void requirePayload( rtimvDictBlob &blob,  /**< [in] Blob to read. */
                     const char *expected, /**< [in] Expected payload bytes. */
                     size_t size           /**< [in] Expected payload length. */
)
{
    std::vector<char> buffer( size + 2, '?' );
    require( blob.getBlobSize() == size, "payload size" );
    require( blob.getBlob( buffer.data(), buffer.size() ) == size, "copied payload size" );
    require( std::memcmp( buffer.data(), expected, size ) == 0, "payload bytes" );
    require( buffer[size] == '?' && buffer[size + 1] == '?', "read preserves trailing bytes" );
}

/// Check empty blobs and reads that have no destination capacity.
void emptyTest()
{
    rtimvDictBlob blob;
    char buffer[2]{ '?', '?' };
    timespec timestamp{ -1, -1 };
    require( blob.getBlobSize() == 0, "default size is zero" );
    require( blob.getBlob( buffer, sizeof( buffer ), &timestamp ) == 0, "default read is empty" );
    require( buffer[0] == '?', "empty binary read preserves destination" );
    require( timestamp.tv_sec == 0 && timestamp.tv_nsec == 0, "default timestamp is zero" );
    require( blob.getBlobStr( buffer, 0 ) == 0 && buffer[0] == '?', "zero capacity string read writes nothing" );
    require( blob.getBlobStr( buffer, sizeof( buffer ) ) == 1, "empty string read returns one byte" );
    require( buffer[0] == '\0' && buffer[1] == '?', "empty string is terminated" );

    blob.setBlob( nullptr, 0 );
    require( blob.getBlobSize() == 0, "registering an empty blob preserves zero size" );
}

/// Check binary writes, capacity reuse, truncation, and timestamps.
void binaryTest()
{
    rtimvDictBlob blob;
    const char initial[]{ 'a', '\0', 'b', 'c' };
    blob.setBlob( initial, sizeof( initial ) );
    requirePayload( blob, initial, sizeof( initial ) );

    char buffer[8];
    std::memset( buffer, '?', sizeof( buffer ) );
    timespec timestamp{ 0, 0 };
    require( blob.getBlob( buffer, 2, &timestamp ) == 2, "binary read truncates to capacity" );
    require( buffer[0] == 'a' && buffer[1] == '\0' && buffer[2] == '?', "truncated bytes and boundary" );
    require( timestamp.tv_sec != 0 || timestamp.tv_nsec != 0, "write sets a timestamp" );
    require( timestamp.tv_nsec >= 0 && timestamp.tv_nsec < 1000000000, "timestamp nanoseconds are valid" );
    timespec repeated{ 0, 0 };
    require( blob.getBlob( buffer, 0, &repeated ) == 0, "zero capacity binary read copies nothing" );
    require( buffer[0] == 'a', "zero capacity binary read preserves destination" );
    require( repeated.tv_sec == timestamp.tv_sec && repeated.tv_nsec == timestamp.tv_nsec,
             "reads preserve the modification timestamp" );

    const char equal[]{ 'd', 'e', '\0', 'f' };
    blob.setBlob( equal, sizeof( equal ) );
    requirePayload( blob, equal, sizeof( equal ) );
    const char shorter[]{ 'g', 'h' };
    blob.setBlob( shorter, sizeof( shorter ) );
    requirePayload( blob, shorter, sizeof( shorter ) );
    const char larger[]{ 'i', 'j', 'k', '\0', 'l', 'm', 'n', 'o' };
    blob.setBlob( larger, sizeof( larger ) );
    requirePayload( blob, larger, sizeof( larger ) );

    blob.setBlob( nullptr, 0 );
    require( blob.getBlobSize() == 0, "zero-length write clears an owned payload" );
    require( blob.getBlob( buffer, sizeof( buffer ) ) == 0, "cleared payload reads empty" );
    blob.setBlob( initial, sizeof( initial ) );
    requirePayload( blob, initial, sizeof( initial ) );
}

/// Check string termination and returned lengths for all destination capacities.
void stringTest()
{
    rtimvDictBlob blob;
    const char text[]{ "camera" };
    blob.setBlob( text, sizeof( text ) );
    char buffer[16];
    std::memset( buffer, '?', sizeof( buffer ) );
    timespec timestamp{ 0, 0 };
    require( blob.getBlobStr( buffer, sizeof( buffer ), &timestamp ) == sizeof( text ), "full string size" );
    require( std::strcmp( buffer, text ) == 0 && buffer[sizeof( text )] == '?', "full string bytes" );
    timespec binaryTimestamp{ 0, 0 };
    blob.getBlob( buffer, sizeof( buffer ), &binaryTimestamp );
    require( timestamp.tv_sec == binaryTimestamp.tv_sec && timestamp.tv_nsec == binaryTimestamp.tv_nsec,
             "string read returns the stored timestamp" );
    require( blob.getBlobStr( buffer, sizeof( text ) ) == sizeof( text ), "exact capacity string size" );
    require( std::strcmp( buffer, text ) == 0, "exact capacity string terminator" );

    std::memset( buffer, '?', sizeof( buffer ) );
    require( blob.getBlobStr( buffer, 4 ) == 4, "truncated string size" );
    require( std::strcmp( buffer, "cam" ) == 0 && buffer[4] == '?', "truncated string replaces last byte" );
    require( blob.getBlobStr( buffer, 1 ) == 1 && buffer[0] == '\0', "capacity one only writes terminator" );
    buffer[0] = '?';
    require( blob.getBlobStr( buffer, 0 ) == 0 && buffer[0] == '?', "nonempty zero capacity read writes nothing" );

    blob.setBlob( text, sizeof( text ) - 1 );
    require( blob.getBlobStr( buffer, sizeof( buffer ) ) == sizeof( text ) - 1, "unterminated payload size" );
    require( std::strcmp( buffer, "camer" ) == 0, "unterminated payload loses its last byte" );
    blob.setBlob( "", 1 );
    require( blob.getBlobStr( buffer, sizeof( buffer ) ) == 1 && buffer[0] == '\0', "stored empty string" );
}

/// Check legacy ownership transfer while keeping every alias read within the owner's lifetime.
void ownershipTest()
{
    const char text[]{ "owned" };
    auto source = std::make_unique<rtimvDictBlob>();
    source->setBlob( text, sizeof( text ) );
    rtimvDictBlob owner( *source );
    requirePayload( owner, text, sizeof( text ) );
    requirePayload( *source, text, sizeof( text ) );
    char buffer[16];
    timespec timestamp{ -1, -1 };
    owner.getBlob( buffer, sizeof( buffer ), &timestamp );
    require( timestamp.tv_sec == 0 && timestamp.tv_nsec == 0, "transfer does not copy the timestamp" );
    source.reset();
    requirePayload( owner, text, sizeof( text ) );

    rtimvDictBlob alias;
    alias.setBlob( text, sizeof( text ) );
    {
        rtimvDictBlob temporaryOwner( alias );
        requirePayload( temporaryOwner, text, sizeof( text ) );
    }
    // The remaining alias is only destroyed; its dangling storage must not be read.
}

/// Check that map insertion, iteration, and erasure use the public blob API.
void dictionaryTest()
{
    dictionaryT dictionary;
    const char first[]{ "first" };
    const char second[]{ "second" };
    dictionary["rtimv.test.first"].setBlob( first, sizeof( first ) );
    dictionary["rtimv.test.second"].setBlob( second, sizeof( second ) );
    require( dictionary.size() == 2 && dictionary.find( "missing" ) == dictionary.end(), "dictionary lookup" );
    dictionaryIteratorT entry = dictionary.begin();
    require( entry->first == "rtimv.test.first", "dictionary preserves ordered iteration" );
    requirePayload( entry->second, first, sizeof( first ) );
    ++entry;
    requirePayload( entry->second, second, sizeof( second ) );
    dictionary.erase( "rtimv.test.first" );
    require( dictionary.size() == 1, "dictionary erasure" );
}
} // namespace

/// Run dictionary characterization without requiring a GUI event loop.
int main()
{
    try
    {
        emptyTest();
        binaryTest();
        stringTest();
        ownershipTest();
        dictionaryTest();
    }
    catch( const std::exception &error )
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "Dictionary storage and ownership checks passed\n";
    return 0;
}
