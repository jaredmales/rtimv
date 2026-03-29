
/** \file rtimvInterfaces.hpp
 * \brief Interface definitions shared by rtimv plugins and overlays.
 *
 * \author Jared R. Males (jaredmales@gmail.com)
 *
 */

#ifndef rtimvInterfaces_hpp
#define rtimvInterfaces_hpp

#include <unordered_set>
#include <string>
#include <string_view>
#include <iostream>

#include <QtPlugin>
#include <QGraphicsScene>
#include <QKeyEvent>

#include "rtimvGraphicsView.hpp"
#include "StretchBox.hpp"
#include "StretchCircle.hpp"
#include "StretchLine.hpp"

#include <unordered_map>
#include <mutex>

#include <mx/app/application.hpp>

#include "rtimvLog.hpp"

/// The rtimv namespace
/**
 * \todo put everything in this namespace
 *
 */
namespace rtimv
{

/// States to indicated whether or not data are being saved
enum class savingState
{
    on,      ///< images are not being saved
    waiting, ///< the next image will be saved
    off      ///< images are being saved
};

/// Warning levels used to change colors
/**
 * \todo put this in an rtimv.hpp file.  Or, maybe, rtimvInterfaces.hpp should be rtimv.hpp
 */
enum class warningLevel
{
    normal,  ///< No border, this is fine
    info,    ///< Info: information on which the user should take action.  Default color is white.
    caution, ///< Caution: user should pay attention.  Default color is yellow.
    warning, ///< Warning: user should fix something.  Default color is red.
    alert    ///< Alert: user must take immediate action.  Default color is magenta.
};
} // namespace rtimv

///\todo need a separate dictionary file(s)
struct rtimvDictBlob
{
  protected:
    size_t m_sz{ 0 };
    size_t m_memSz{ 0 };

    void *m_blob{ nullptr };

    bool m_owner{ false };

    timespec m_lastMod{ 0, 0 };

    std::mutex m_mutex;

    void clearBlob()
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

  public:
    rtimvDictBlob()
    {
    }

    rtimvDictBlob( rtimvDictBlob &rdb )
    {
        clearBlob();
        m_blob = rdb.m_blob;
        m_sz = rdb.m_sz;
        m_memSz = rdb.m_memSz;
        m_owner = rdb.m_owner;
        rdb.m_owner = false;
    }

    rtimvDictBlob( const rtimvDictBlob &rdb ) = delete; // no copies!

    /// Set the blob
    /** Allocates m_blob and copies \p sz bytes from \p blob.
     * Allocation only occurs if more size is needed according to m_memSz.
     * Sets m_sz to sz.
     */
    void setBlob( const void *blob, ///< [in] The new blob data
                  size_t sz         ///< [in] The number of bytes in the new blob data
    )
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

    /// Get the data stored in the blob
    /** Copies the smaller of \p mxsz and m_sz bytes to \p blob
     * \returns the number of bytes copied
     */
    size_t
    getBlob( char *blob,  ///< [out] The pointer to the array to copy the data to
             size_t mxsz, ///< [in] The max number of bytes to copy to blob
             timespec *ts =
                 nullptr /// [out] [optional] if not null this will be filled in with the time of last modification
    )
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

    /// Get the data stored in the blob as a C string
    /** Copies the smaller of \p mxsz and m_sz bytes to \p blob.
     *
     * This guarantees that blob[sz-1] = 0
     * where sz is the number of bytes copied.  If the returned size == mxsz then you should not trust the
     * contents unless you know that the correct size was copied or don't care if the string is truncated.
     *
     * If m_blob has 0 size or is nullptr, then this sets blob[0] = 0 and returns 1.
     *
     * \returns the number of bytes copied
     */
    size_t
    getBlobStr( char *blob,  ///< [in/out] The pointer to the array to copy the data to
                size_t mxsz, ///< [in] The max number of bytes to copy to blob
                timespec *ts =
                    nullptr /// [out] [optional] if not null this will be filled in with the time of last modification
    )
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

    size_t getBlobSize()
    {
        std::unique_lock<std::mutex> lock( m_mutex );

        if( m_blob == nullptr )
            return 0;
        return m_sz;
    }

    ~rtimvDictBlob()
    {
        std::unique_lock<std::mutex> lock( m_mutex );
        clearBlob();
    }
};

/// The dictionary type, a std::map
typedef std::map<std::string, rtimvDictBlob> dictionaryT;

/// The dictionary iterator type.
typedef std::map<std::string, rtimvDictBlob>::iterator dictionaryIteratorT;

/// The base plugin class type
/** All supported plugins inherit from this
 *
 */
class rtimvInterface : public QObject
{
  protected:
    /// Program called-name used in standardized plugin log prefixes.
    std::string m_logCalledName{ "rtimv" };

    /// True to include called-name in standardized plugin log prefixes.
    bool m_logIncludeAppName{ true };

    /// Image key/name used in standardized plugin log prefixes.
    std::string m_logImage0{ "unknown" };

    /// Plugin label added to plugin log message text.
    std::string m_pluginName{ "plugin" };

  public:
    virtual ~rtimvInterface() = default;

    /// Get the information about this plugin for user display
    /** The first entry should start with the plugin name and type, e.g.:
     * \verbatim
     * INDI dictionary
     * \endverbatim
     * plus additional information.
     * Subsequent entries should start with spaces to move past the name and type.
     */
    virtual std::vector<std::string> info() = 0;

    /// Report whether this plugin provides a toggleable text overlay.
    virtual bool hasTextOverlay()
    {
        return false;
    }

    /// Get the shortcut key used to toggle the plugin text overlay.
    virtual char textOverlayKey()
    {
        return '\0';
    }

    /// Get the short title used to describe the plugin text overlay.
    virtual std::string textOverlayTitle()
    {
        return "";
    }

    /// Generate the plugin text overlay contents on demand.
    virtual std::string textOverlayText()
    {
        return "";
    }

    /// Generate one line of plugin text overlay contents on demand.
    /** Return an empty string to request that the caller regenerate the full overlay text instead.
     */
    virtual std::string textOverlayLine( size_t line /**< [in] Zero-based line index to regenerate. */ )
    {
        static_cast<void>( line );
        return "";
    }

    /// Set context used by standardized plugin log formatters.
    void setLogContext( const std::string &calledName, /**< [in] Program called-name for log prefix. */
                        bool includeAppName,           /**< [in] True to include called-name in log prefix. */
                        const std::string &image0      /**< [in] Image key/name to include in log prefix. */
    )
    {
        m_logCalledName = calledName;
        m_logIncludeAppName = includeAppName;
        m_logImage0 = image0.empty() ? "unknown" : image0;
    }

    /// Set plugin label included in plugin log message text.
    void setPluginName( const std::string &pluginName /**< [in] Plugin label for plugin log message text. */ )
    {
        if( pluginName.empty() )
        {
            m_pluginName = "plugin";
        }
        else
        {
            m_pluginName = pluginName;
        }
    }

    /// Format a standardized plugin log message.
    std::string formatPluginLogMessage( std::string_view message /**< [in] Message text to append. */ ) const
    {
        rtimv::logContext ctx;
        ctx.calledName = m_logCalledName;
        ctx.image0 = m_logImage0;
        ctx.includeAppName = m_logIncludeAppName;

        return rtimv::formatLogMessage( ctx, std::format( "[plugin: {}] {}", m_pluginName, message ) );
    }

    /// Write a standardized plugin info message to stdout.
    void pluginLogInfo( std::string_view message /**< [in] Message text to append. */ ) const
    {
        std::cout << formatPluginLogMessage( message ) << '\n';
    }

    /// Write a standardized plugin error message to stderr.
    void pluginLogError( std::string_view message /**< [in] Message text to append. */ ) const
    {
        std::cerr << formatPluginLogMessage( message ) << '\n';
    }
};

class rtimvDictionaryInterface : public rtimvInterface
{
  public:
    /// Destructor, implement as needed.
    virtual ~rtimvDictionaryInterface() = default;

    /// Attach the dictionary plugin to rtimv
    /**
     * \returns 0 on successful configuration of the interface
     * \returns >0 if the dictionary is not configured but no error
     * \returns <0 if an error occurs
     */
    virtual int attachDictionary( dictionaryT *, ///< [in] pointer to the dictionary itself, which is part of rtimv
                                  mx::app::appConfigurator & ///< [in] configuration from which to extract specifics
                                  ) = 0;
};

#define rtimvDictionaryInterface_iid "rtimv.dictionaryInterface/1.4"

Q_DECLARE_INTERFACE( rtimvDictionaryInterface, rtimvDictionaryInterface_iid )

/// Contains the internal components of rtimvMainWindow exposed to overlay plugins.
/** Plugins should copy the passed object of this type to an internal variable.
 */
struct rtimvOverlayAccess
{
    /// QObject handle for the main window to allow connecting signals and slots from within plugin
    QObject *m_mainWindowObject{ nullptr };

    StretchBox *m_colorBox{ nullptr };

    StretchBox *m_statsBox{ nullptr };

    std::unordered_set<StretchBox *> *m_userBoxes{ nullptr };

    std::unordered_set<StretchCircle *> *m_userCircles{ nullptr };

    std::unordered_set<StretchLine *> *m_userLines{ nullptr };

    rtimvGraphicsView *m_graphicsView{ nullptr };

    dictionaryT *m_dictionary{ nullptr };
};

struct rtimvMouseCoord
{
    float pixelX{ 0 };
    float pixelY{ 0 };

    float imageValue{ 0 };

    float displayValue{ 0 };

    bool darkValid{ false };
    float darkValue{ 0 };

    bool satMaskValid{ false };
    float satMaskValue{ 0 };

    bool maskValid{ false };
    float maskValue{ 0 };
};

/// Interface for adding information to the display
class rtimvOverlayInterface : public rtimvInterface
{
  public:
    virtual ~rtimvOverlayInterface() = default;

    /// Configure and attach the overlay
    /**
     * \return 0 on success
     * \return <0 on error
     * \return >0 on not-configured
     */
    virtual int attachOverlay( rtimvOverlayAccess &,      ///< [in] The exposed information for overlays
                               mx::app::appConfigurator & ///< [in] configuration from which to extract specifics
                               ) = 0;

    /// Update the overlay
    virtual int updateOverlay() = 0;

    /// Handle a key press event
    /** Note that there is currently no way to register and deconflict keys
     *
     */
    virtual void keyPressEvent( QKeyEvent *ke ) = 0;

    /// Check if the overlay is currently enabled.
    virtual bool overlayEnabled() = 0;

    /// Enable the overlay
    virtual void enableOverlay() = 0;

    /// Disable the overlay
    virtual void disableOverlay() = 0;
};

#define rtimvOverlayInterface_iid "rtimv.overlayInterface/1.4"

Q_DECLARE_INTERFACE( rtimvOverlayInterface, rtimvOverlayInterface_iid )

#endif // rtimvInterfaces_hpp
