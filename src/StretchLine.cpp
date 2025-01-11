
#include "StretchLine.hpp"
#include <cmath>

StretchLine::StretchLine( QGraphicsItem *parent ) : QGraphicsLineItem( parent )
{
    initStretchLine();
    return;
}

StretchLine::StretchLine( qreal xs, qreal ys, qreal xe, qreal ye, QGraphicsItem *parent )
    : QGraphicsLineItem( xs, ys, xe, ye, parent )
{
    initStretchLine();
    return;
}

void StretchLine::initStretchLine()
{
    connect( &m_cursorTimer, SIGNAL( timeout() ), this, SLOT( cursorTimerOut() ) );
    connect( &m_selectionTimer, SIGNAL( timeout() ), this, SLOT( selectionTimerOut() ) );
}

float StretchLine::length()
{
    return line().length();
}

float StretchLine::angle()
{
    return line().angle();
}

void StretchLine::hoverMoveEvent( QGraphicsSceneHoverEvent *e )
{
    handleHoverMoveEvent( e );
}

void StretchLine::hoverLeaveEvent( QGraphicsSceneHoverEvent *e )
{
    handleHoverLeaveEvent( e );
}

void StretchLine::mousePressEvent( QGraphicsSceneMouseEvent *e )
{
    handleMousePressEvent( e );
}

void StretchLine::mouseReleaseEvent( QGraphicsSceneMouseEvent *e )
{
    handleMouseReleaseEvent( e );
}

void StretchLine::mouseMoveEvent( QGraphicsSceneMouseEvent *e )
{
    handleMouseMoveEvent( e );
}

void StretchLine::keyPressEvent( QKeyEvent *ke )
{
    handleKeyPressEvent( ke );
}

void StretchLine::focusOutEvent( QFocusEvent *e )
{
    handleFocusOutEvent( e );
}

void StretchLine::passKeyPressEvent( QKeyEvent *ke )
{
    QGraphicsLineItem::keyPressEvent( ke );
}

void StretchLine::cursorTimerOut()
{
    slotCursorTimerOut();
}

void StretchLine::selectionTimerOut()
{
    slotSelectionTimerOut();
}

bool StretchLine::onHoverComputeSizing( QGraphicsSceneHoverEvent *e )
{
    float x = e->pos().x() - line().x1();
    float y = e->pos().y() - line().y1();

    float m, drad;
    if( line().x2() - line().x1() == 0 )
    {
        m = 1e50;
        drad = x;
    }
    else
    {
        m = ( line().y2() - line().y1() ) / ( line().x2() - line().x1() );
        drad = fabs( -m * x + y ) / sqrt( m * m + 1 );
    }

    if( drad > m_edgeTol )
    {
        return false;
    }

    float r1 = sqrt( x * x + y * y );

    float x2 = e->pos().x() - line().x2();
    float y2 = e->pos().y() - line().y2();

    float r2 = sqrt( x2 * x2 + y2 * y2 );
    float l = line().length();
    if( r2 < m_edgeTol || r2 < 0.1 * l ) // First make sure we can always change length
    {
        m_sizing = szAll;
    }
    else if( r1 < m_edgeTol || r1 < 0.2 * l ) // Don't allow rotate if too close to the 0
    {
        m_sizing = szOff;
        return true;
    }
    else
    {
        float ang = line().angle();
        if( ang >= 347.4 || ang <= 22.5 || ( ang >= 157.5 && ang <= 202.5 ) )
            m_sizing = szTop;
        else if( ( ang >= 22.5 && ang <= 67.5 ) || ( ang >= 202.5 && ang <= 247.5 ) )
            m_sizing = szBotr;
        else if( ( ang >= 67.5 && ang <= 112.5 ) || ( ang >= 247.5 && ang <= 292.5 ) )
            m_sizing = szLeft;
        else
            m_sizing = szBotl;
    }

    return true;
}

bool StretchLine::onMousePressCalcGrabbed( QGraphicsSceneMouseEvent *e )
{
    float x = e->pos().x() - line().x1();
    float y = e->pos().y() - line().y1();

    float m, drad;
    if( line().x2() - line().x1() == 0 )
    {
        m = 1e50;
        drad = x;
    }
    else
    {
        m = ( line().y2() - line().y1() ) / ( line().x2() - line().x1() );
        drad = fabs( -m * x + y ) / sqrt( m * m + 1 );
    }

    if( drad > m_edgeTol )
    {
        return false;
    }
    else
    {
        return true;
    }
}

void StretchLine::setGrabbedGeometry( QGraphicsSceneMouseEvent *e )
{
    m_start_x = line().x1();
    m_start_y = line().y1();
    m_end_x = line().x2();
    m_end_y = line().y2();

    m_mv_x0 = e->pos().x();
    m_mv_y0 = e->pos().y();
}

void StretchLine::setMovingGeometry( QGraphicsSceneMouseEvent *e )
{
    m_start_x = line().x1();
    m_start_y = line().y1();
    m_end_x = line().x2();
    m_end_y = line().y2();

    m_mv_x0 = e->pos().x();
    m_mv_y0 = e->pos().y();

    if( m_sizing != szAll ) // Only bother if we need it
    {
        m_mv_ang0 = QLineF( m_start_x, m_start_y, m_mv_x0, m_mv_y0 ).angle();
    }
}

void StretchLine::sizingCalcNewPos( QGraphicsSceneMouseEvent *e )
{
    if( m_sizing == szAll )
    {
        float dx = e->pos().x() - m_mv_x0;
        float dy = e->pos().y() - m_mv_y0;

        setLine( m_start_x, m_start_y, m_end_x + dx, m_end_y + dy );
    }
    else
    {
        float dang = -( QLineF( m_start_x, m_start_y, e->pos().x(), e->pos().y() ).angle() - m_mv_ang0 );

        float c = cos( dang * 3.14159 / 180. );
        float s = sin( dang * 3.14159 / 180. );

        float dx = ( m_end_x - m_start_x ) * c - ( m_end_y - m_start_y ) * s;
        float dy = ( m_end_x - m_start_x ) * s + ( m_end_y - m_start_y ) * c;

        setLine( m_start_x, m_start_y, m_start_x + dx, m_start_y + dy );
    }
}

void StretchLine::movingCalcNewPos( QGraphicsSceneMouseEvent *e )
{
    float dx = e->pos().x() - m_mv_x0;
    float dy = e->pos().y() - m_mv_y0;

    setLine( m_start_x + dx, m_start_y + dy, m_end_x + dx, m_end_y + dy );
}

void StretchLine::remove()
{
    emitRemove();
}

void StretchLine::emitMoved()
{
    emit moved( this );
}

void StretchLine::emitResized()
{
    emit resized( this );
}

void StretchLine::emitRejectMouse()
{
    emit rejectMouse( this );
}

void StretchLine::emitMouseIn()
{
    emit mouseIn( this );
}

void StretchLine::emitMouseOut()
{
    emit mouseOut( this );
}

void StretchLine::emitRemove()
{
    emit remove( this );
}

void StretchLine::emitSelected()
{
    emit selected( this );
}

void StretchLine::emitDeSelected()
{
    emit deSelected( this );
}
