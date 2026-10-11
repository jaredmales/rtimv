/** \file rtimvCommand_test.cpp
 * \brief Optional router ownership, box edit hooks, and transient status layout tests.
 */
#include "rtimvCommandInterface.hpp"
#include "StretchBox.hpp"
#include "rtimvGraphicsView.hpp"
#include <QApplication>
#include <QFontMetrics>
#include <QGraphicsSceneMouseEvent>
#include <QTextBlock>
#include <iostream>
#include <stdexcept>

namespace
{
/// Fail with a useful message even in release builds where assert is disabled.
void require( bool passed, /**< [in] Condition under test. */ const char *message /**< [in] Failure description. */ )
{
    if( !passed )
        throw std::runtime_error( message );
}

/// Offline provider used to verify the separate optional interface and weak ownership.
class Provider : public QObject, public rtimvCommandInterface
{
    Q_OBJECT
    Q_INTERFACES( rtimvCommandInterface )
  public:
    int m_sends{ 0 }; ///< Number of submitted batches.
    /// Return the offline protocol identifier.
    QString commandProtocol() const override;
    /// Count one submitted batch.
    bool sendCommands( const QVector<rtimvCommand> &commands, /**< [in] Ordered vectors. */
                       QString &error /**< [out] Local error. */ ) override;
    /// Return one complete property.
    rtimvPropertySnapshot propertySnapshot( const QString &device, /**< [in] Source device. */
                                            const QString &property /**< [in] Source property. */ ) const override;
};
QString Provider::commandProtocol() const
{
    return "test";
}
bool Provider::sendCommands( const QVector<rtimvCommand> &, QString & )
{
    ++m_sends;
    return true;
}
rtimvPropertySnapshot Provider::propertySnapshot( const QString &, const QString & ) const
{
    rtimvPropertySnapshot result;
    result.m_valid = result.m_complete = true;
    result.m_generation = 12;
    result.m_elements["x"] = 0.0;
    return result;
}

/// Drive actual protected mouse handlers, bypassing only hover timing.
class Box : public StretchBox
{
  public:
    /// Forward actual mouse events through the production handlers.
    void mouse( QEvent::Type type, /**< [in] Press, move, or release. */
                QPointF point /**< [in] Position in the scene. */ );
    /// Select moving or resizing as if the hover timer had completed.
    void mode( bool resize /**< [in] True for right-edge resizing. */ );
};
void Box::mode( bool resize )
{
    m_grabbing = !resize;
    m_sizing = resize ? szRight : szOff;
}
void Box::mouse( QEvent::Type type, QPointF point )
{
    QGraphicsSceneMouseEvent event( type );
    event.setScenePos( point );
    event.setPos( mapFromScene( point ) );
    if( type == QEvent::GraphicsSceneMousePress )
        mousePressEvent( &event );
    else if( type == QEvent::GraphicsSceneMouseMove )
        mouseMoveEvent( &event );
    else
        mouseReleaseEvent( &event );
}

/// Check provider lookup before/after registration and destruction.
void routerTest()
{
    QObject host;
    rtimvCommandRouter router( &host );
    require( host.findChild<rtimvCommandRouter *>( "rtimv.commandRouter" ) == &router, "late-bound router lookup" );
    QString error;
    QVector<rtimvCommand> commands{ { "camera", "roi", rtimvPropertyType::number, { { "target", 0.0 } } } };
    require( !router.send( "test", commands, error ), "missing provider must fail" );
    Provider provider;
    router.addProvider( &provider );
    router.addProvider( &provider );
    require( router.send( "test", commands, error ) && provider.m_sends == 1,
             "duplicate registration must not duplicate a send" );
    require( router.snapshot( "test", "camera", "roi" ).m_complete, "complete snapshot" );
    {
        Provider second;
        router.addProvider( &second );
        require( !router.send( "test", commands, error ), "ambiguous providers must fail" );
    }
    require( router.send( "test", commands, error ), "destroyed providers must not remain ambiguous" );
    require( !router.send( "test", {}, error ), "empty requests must fail" );
}

/// Check that movement and maintain-center resize each have one start and one release completion.
void boxTest()
{
    Box box;
    box.setRect( 0, 0, 64, 64 );
    box.setMaintainCenter( true );
    int starts = 0, finishes = 0;
    QObject::connect( &box, &StretchBox::editStarted, [&]( StretchBox * ) { ++starts; } );
    QObject::connect( &box, &StretchBox::editFinished, [&]( StretchBox * ) { ++finishes; } );
    box.mode( false );
    box.mouse( QEvent::GraphicsSceneMousePress, { 0, 32 } );
    require( starts == 1 && finishes == 0, "movement starts once" );
    box.mouse( QEvent::GraphicsSceneMouseMove, { 3, 30 } );
    require( finishes == 0, "movement must not complete before release" );
    box.mouse( QEvent::GraphicsSceneMouseRelease, { 3, 30 } );
    require( finishes == 1 && box.rect().size() == QSizeF( 64, 64 ), "movement preserves dimensions" );
    box.setPos( 0, 0 );
    box.mode( true );
    box.mouse( QEvent::GraphicsSceneMousePress, { 64, 32 } );
    require( starts == 2, "resize starts before first motion" );
    box.mouse( QEvent::GraphicsSceneMouseMove, { 60, 32 } );
    box.mouse( QEvent::GraphicsSceneMouseRelease, { 60, 32 } );
    require( finishes == 2 && box.rect().center() == QPointF( 32, 32 ) && box.rect().width() == 56,
             "maintain-center resize completes once" );
    box.mode( false );
    box.mouse( QEvent::GraphicsSceneMousePress, { 4, 32 } );
    box.cancelEdit();
    box.mouse( QEvent::GraphicsSceneMouseRelease, { 4, 32 } );
    require( finishes == 2, "cancel must not complete a discarded edit" );
}

/// Require readable text with every laid-out line contained by the visible status box.
void requireMessageFits( rtimvGraphicsView &view /**< [in] View displaying the message under test. */ )
{
    auto *text = view.zoomText();
    require( text->currentFont().pixelSize() == view.zoomFontSize(), "status message keeps configured font size" );
    require( view.rect().contains( text->geometry() ), "status box stays inside the view" );
    require( text->document()->size().height() <= text->viewport()->height(), "all status lines are visible" );
    const int available = text->viewport()->width() - 2 * text->document()->documentMargin();
    for( auto block = text->document()->begin(); block.isValid(); block = block.next() )
        for( int n = 0; n < block.layout()->lineCount(); ++n )
            require( block.layout()->lineAt( n ).naturalTextWidth() <= available, "status line fits without clipping" );
}

/// Check that transient messages remain readable and fully visible as their text and view size change.
void messageTest()
{
    rtimvGraphicsView view;
    view.resize( 800, 800 );
    view.show();
    QApplication::processEvents();
    view.zoomText( "ROI adjusted; Alt+R to apply" );
    auto *text = view.zoomText();
    require( text->currentFont().pixelSize() == view.zoomFontSize(),
             "adjustment notice retains the configured font size" );
    const int available = text->viewport()->width() - 2 * text->document()->documentMargin();
    require( QFontMetrics( text->currentFont() ).horizontalAdvance( text->toPlainText() ) <= available,
             "adjustment notice fits" );
    require( text->document()->size().height() <= text->viewport()->height(),
             "adjustment notice stays on one visible line" );
    requireMessageFits( view );
    const int singleLineHeight = text->height();

    view.resize( 260, 400 );
    QApplication::processEvents();
    requireMessageFits( view );
    require( text->height() > singleLineHeight, "a narrow view wraps the notice instead of shrinking its font" );

    view.resize( 800, 800 );
    QApplication::processEvents();
    requireMessageFits( view );
    require( text->height() == singleLineHeight, "a wider view restores the single-line layout" );

    view.zoomText( "ROI applied" );
    requireMessageFits( view );
    require( text->width() < view.width(), "short messages return to a compact status box" );
    view.zoomText( "1.0x" );
    requireMessageFits( view );

    view.zoomFontSize( 32 );
    view.zoomText( "ROI adjusted; Alt+R to apply" );
    requireMessageFits( view );
}
} // namespace

/// Run independent GUI-library tests using an offline Qt platform.
int main( int argc, /**< [in] Argument count. */ char **argv /**< [in] Qt platform options. */ )
{
    QApplication app( argc, argv );
    try
    {
        routerTest();
        boxTest();
        messageTest();
    }
    catch( const std::exception &error )
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "Router, edit hooks, and status layout passed\n";
    return 0;
}
#include "rtimvCommand_test.moc"
