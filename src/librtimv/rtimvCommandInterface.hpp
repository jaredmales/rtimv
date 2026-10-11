/** \file rtimvCommandInterface.hpp
 * \brief Optional protocol-independent plugin commands and coherent property snapshots.
 */
#ifndef rtimvCommandInterface_hpp
#define rtimvCommandInterface_hpp

#include <QMap>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QVariant>
#include <QVector>

/// Typed property values supported by the command bridge.
enum class rtimvPropertyType
{
    number,
    toggle,
    text
};

/// One outgoing property vector; numeric and boolean elements retain their types.
struct rtimvCommand
{
    QString m_device;                                      ///< Destination device.
    QString m_property;                                    ///< Destination property.
    rtimvPropertyType m_type{ rtimvPropertyType::number }; ///< Value type.
    QMap<QString, QVariant> m_elements;                    ///< Elements sent together in one property.
};

/// Immutable copy of a complete received property at one connection generation.
struct rtimvPropertySnapshot
{
    bool m_valid{ false };    ///< False before definition, after deletion, or when disconnected.
    bool m_complete{ false }; ///< Latest message supplied every element in the definition.
    bool m_writable{ false }; ///< Permission from the current definition.
    rtimvPropertyType m_type{ rtimvPropertyType::number }; ///< Defined element type.
    QString m_state;                                       ///< idle, ok, busy, or alert.
    quint64 m_generation{ 0 };          ///< Connection generation; changes invalidate pending actions.
    quint64 m_revision{ 0 };            ///< Receive sequence within this generation.
    QMap<QString, QVariant> m_elements; ///< Values copied together from the received vector.
};

/// Optional provider implemented alongside an existing dictionary plugin.
class rtimvCommandInterface
{
  public:
    /// Destroy the protocol provider.
    virtual ~rtimvCommandInterface() = default;

    /// Return the protocol routed by this provider.
    virtual QString commandProtocol() const = 0;

    /// Submit an ordered batch without replay; success means transport submission only.
    virtual bool sendCommands( const QVector<rtimvCommand> &commands, /**< [in] Ordered property vectors. */
                               QString &error /**< [out] Local rejection or send failure. */ ) = 0;

    /// Return a coherent cached property; an unavailable property has m_valid false.
    virtual rtimvPropertySnapshot propertySnapshot( const QString &device, /**< [in] Device name. */
                                                    const QString &property /**< [in] Property name. */ ) const = 0;
};
#define rtimvCommandInterface_iid "rtimv.commandInterface/1.0"
Q_DECLARE_INTERFACE( rtimvCommandInterface, rtimvCommandInterface_iid )

/// Main-window child routing optional providers independently of plugin load order.
/** All methods run on the GUI thread. Providers must synchronize incoming data before exposing snapshots.
 * Existing plugin base interfaces and overlay-access layouts remain unchanged.
 */
class rtimvCommandRouter : public QObject
{
    Q_OBJECT
  protected:
    QVector<QPointer<QObject>> m_providers; ///< Weak provider references; never owns plugin instances.

    /// Find exactly one live provider for a protocol.
    rtimvCommandInterface *provider( const QString &protocol /**< [in] Requested protocol. */ ) const;

  public:
    /// Construct the router owned by the main window.
    explicit rtimvCommandRouter( QObject *parent = nullptr /**< [in] Owning main window or test object. */ );

    /// Register an optional provider after successful plugin attachment.
    void addProvider( QObject *object /**< [in] Plugin QObject implementing the optional interface. */ );

    /// Send once through exactly one provider, returning a local failure if unavailable.
    bool send( const QString &protocol,               /**< [in] Destination protocol. */
               const QVector<rtimvCommand> &commands, /**< [in] Ordered vectors. */
               QString &error /**< [out] Local error, empty on submission. */ );

    /// Copy a complete cached property from the selected provider.
    rtimvPropertySnapshot snapshot( const QString &protocol, /**< [in] Source protocol. */
                                    const QString &device,   /**< [in] Device name. */
                                    const QString &property /**< [in] Property name. */ ) const;
};
#endif
