/** \file rtimvDictionary.hpp
 * \brief Shared dictionary storage for rtimv plugins and overlays.
 */
#ifndef rtimvDictionary_hpp
#define rtimvDictionary_hpp

#include <cstddef>
#include <ctime>
#include <map>
#include <string>
#include <mutex>

/// Byte payload with owned storage and internally synchronized reads and writes.
/** The mutex protects this blob's access methods, not dictionary structure or object lifetime.
 * The legacy non-const constructor transfers ownership while leaving a source alias with a separate mutex.
 * Callers must prevent concurrent transfer and must not use that alias after its owner frees the storage.
 */
struct rtimvDictBlob
{
  protected:
    /// Number of payload bytes available for reads.
    size_t m_sz{ 0 };

    /// Allocated capacity available for reuse by the owner.
    size_t m_memSz{ 0 };

    /// Payload storage, owned here when m_owner is true and otherwise borrowed.
    void *m_blob{ nullptr };

    /// True when this blob is responsible for freeing the payload storage.
    bool m_owner{ false };

    /// CLOCK_REALTIME timestamp of the last write through this object.
    timespec m_lastMod{ 0, 0 };

    /// Serializes access through this object; does not synchronize other aliases.
    std::mutex m_mutex;

    /// Free and reset owned storage, leaving borrowed storage unchanged.
    /** Caller must hold m_mutex, except during construction before the object is shared.
     */
    void clearBlob();

  public:
    /// Construct an empty blob with no storage or modification timestamp.
    rtimvDictBlob();

    /// Transfer ownership from another blob while retaining its source alias.
    /** Does not lock the source or copy its timestamp. The destination timestamp starts at zero.
     * The source must not outlive the storage if its payload will be accessed again.
     */
    rtimvDictBlob( rtimvDictBlob &rdb /**< [in,out] Source whose ownership flag is cleared. */ );

    /// Disallow copying from a const source.
    rtimvDictBlob( const rtimvDictBlob &rdb /**< [in] Source that cannot transfer ownership. */ ) = delete;

    /// Store a payload and update its modification timestamp while holding the blob mutex.
    /** Reuses owned capacity or allocates when more space is needed or this object is not the owner.
     * A null input skips copying; newly allocated bytes are not initialized. A zero-length write clears
     * an owned payload but leaves a non-owner's pointer and sizes unchanged. Allocation failures are not handled.
     */
    void setBlob( const void *blob, /**< [in] New payload bytes, or null to skip copying. */
                  size_t sz         /**< [in] Number of bytes to store. */
    );

    /// Copy the payload and optional timestamp under the blob mutex.
    /** Copies at most mxsz bytes. The destination must be valid for the copied bytes.
     * \returns the number of bytes copied, or zero when no payload is available.
     */
    size_t getBlob( char *blob,            /**< [out] Destination for the payload bytes. */
                    size_t mxsz,           /**< [in] Capacity of the destination in bytes. */
                    timespec *ts = nullptr /**< [out] Optional destination for the modification timestamp. */
    );

    /// Copy the payload as a terminated C string, using getBlob() for the synchronized read.
    /** Replaces the last copied byte with a null terminator. If no bytes are available and mxsz is positive,
     * writes a terminator and returns one. Zero capacity returns zero without writing. A return equal to
     * mxsz may indicate truncation; the timestamp describes the stored payload, not the terminated copy.
     * \returns the number of bytes written, including the terminator when capacity permits.
     */
    size_t getBlobStr( char *blob,            /**< [out] Destination for the terminated string. */
                       size_t mxsz,           /**< [in] Capacity of the destination in bytes. */
                       timespec *ts = nullptr /**< [out] Optional destination for the modification timestamp. */
    );

    /// Get the current payload size under the blob mutex.
    /** A subsequent read can observe a different write; use its returned byte count.
     * \returns the stored payload size, or zero when storage is null.
     */
    size_t getBlobSize();

    /// Release owned storage under the blob mutex; callers must first stop using this object.
    ~rtimvDictBlob();
};

/// Shared dictionary whose structural changes and entry lifetimes must be coordinated by callers.
typedef std::map<std::string, rtimvDictBlob> dictionaryT;

/// Mutable iterator into the shared dictionary.
typedef std::map<std::string, rtimvDictBlob>::iterator dictionaryIteratorT;

#endif // rtimvDictionary_hpp
