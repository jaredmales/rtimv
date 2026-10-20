/** \file rtimvInterfaces.hpp
 * \brief Interface definitions shared by rtimv plugins and overlays.
 */

#ifndef rtimvInterfaces_hpp
#define rtimvInterfaces_hpp

#include <unordered_set>
#include <string>
#include <string_view>
#include <iostream>
#include <vector>

#include <QtPlugin>
#include <QGraphicsScene>
#include <QKeyEvent>

#include "rtimvDictionary.hpp"
#include "rtimvGraphicsView.hpp"
#include "StretchBox.hpp"
#include "StretchCircle.hpp"
#include "StretchLine.hpp"

#include <unordered_map>

#include <mx/app/application.hpp>

#include "rtimvLog.hpp"

/// The rtimv namespace
/**
 * \todo put everything in this namespace
 *
 */
namespace rtimv
{

/// States to indicate whether or not data are being saved
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
    /// Destroy the plugin interface.
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

/// Plugin interface for publishing and consuming shared dictionary state.
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
    virtual int attachDictionary( dictionaryT *dictionary,         /**< [in] Dictionary owned by the main window. */
                                  mx::app::appConfigurator &config /**< [in] Plugin configuration. */
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

    /// Non-owning access to the main window's color-scaling region.
    StretchBox *m_colorBox{ nullptr };

    /// Non-owning access to the main window's statistics region.
    StretchBox *m_statsBox{ nullptr };

    /// Non-owning access to the main window's user-drawn boxes.
    std::unordered_set<StretchBox *> *m_userBoxes{ nullptr };

    /// Non-owning access to the main window's user-drawn circles.
    std::unordered_set<StretchCircle *> *m_userCircles{ nullptr };

    /// Non-owning access to the main window's user-drawn lines.
    std::unordered_set<StretchLine *> *m_userLines{ nullptr };

    /// Non-owning access to the image display and its overlay scene.
    rtimvGraphicsView *m_graphicsView{ nullptr };

    /// Shared dictionary owned by the main window; plugins must coordinate structural changes.
    dictionaryT *m_dictionary{ nullptr };
};

/// Image coordinates and sampled values supplied to mouse-coordinate consumers.
struct rtimvMouseCoord
{
    /// Horizontal coordinate of the sampled pixel.
    float pixelX{ 0 };

    /// Vertical coordinate of the sampled pixel.
    float pixelY{ 0 };

    /// Uncalibrated image value at the sampled pixel.
    float imageValue{ 0 };

    /// Processed display value at the sampled pixel.
    float displayValue{ 0 };

    /// True when a dark-frame sample is available.
    bool darkValid{ false };

    /// Dark-frame value when darkValid is true.
    float darkValue{ 0 };

    /// True when a saturation-mask sample is available.
    bool satMaskValid{ false };

    /// Saturation-mask value when satMaskValid is true.
    float satMaskValue{ 0 };

    /// True when an ordinary mask sample is available.
    bool maskValid{ false };

    /// Ordinary mask value when maskValid is true.
    float maskValue{ 0 };
};

/// Interface for adding information to the display
class rtimvOverlayInterface : public rtimvInterface
{
  public:
    /// Destroy the overlay interface.
    virtual ~rtimvOverlayInterface() = default;

    /// Configure and attach the overlay
    /**
     * \return 0 on success
     * \return <0 on error
     * \return >0 on not-configured
     */
    virtual int attachOverlay( rtimvOverlayAccess &access, /**< [in] Non-owning access to main-window components. */
                               mx::app::appConfigurator &config /**< [in] Plugin configuration. */
                               ) = 0;

    /// Update the overlay
    virtual int updateOverlay() = 0;

    /// Handle a key press event
    /** Note that there is currently no way to register and deconflict keys
     *
     */
    virtual void keyPressEvent( QKeyEvent *ke /**< [in] Key event forwarded by the main window. */ ) = 0;

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
