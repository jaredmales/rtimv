
#ifndef __StretchCircle_h__
#define __StretchCircle_h__


#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QCursor>
#include <QObject>
#include <QTimer>
#include <QTextEdit>

class StretchCircle : public QObject, public QGraphicsEllipseItem
{
   Q_OBJECT

public:

   StretchCircle(QGraphicsItem * parent = 0);
   StretchCircle(const QRectF & rect, QGraphicsItem * parent = 0 );
   StretchCircle(qreal x, qreal y, qreal width, qreal height, QGraphicsItem * parent = 0 );

   QCursor defCursor;
   qreal xoff, yoff;
   
private:
   void initStretchCircle();
      
protected:
      
   int edgeTol;
      
   void hoverMoveEvent(QGraphicsSceneHoverEvent * e);
   void hoverLeaveEvent(QGraphicsSceneHoverEvent * e);
      
   void mousePressEvent ( QGraphicsSceneMouseEvent * event );
   void mouseReleaseEvent(QGraphicsSceneMouseEvent * event );
      
   void mouseMoveEvent(QGraphicsSceneMouseEvent * event);

   ///Tracks how the circle is currently being resized.
   int sizing;
      
   bool grabbing;
      
   ///Describes how the circle is being resized.
   enum sizingMode{szOff, szOn};
      
   ///Tracks whether the circle is being resized.
   bool isSizing;
      
   ///Trackes whether the circle is moving.
   bool isMoving;
      
   double x0, y0, w0, h0, xc0, yc0, mx0, my0, drad0;
      
   ///Alternative implementation of selection, does not change line stye
   /** Instead, the user knows it is selected by cursor change to a hand.
     */
   bool altSelected;
      
   ///Is this circle stretchable?  If not goes straight to grab for move.
   /** If stretchable == false, then mouse over goes straight to the hand.
     */
   bool stretchable;
      
   int cursorStatus;
   float cursorAngle;
   
   void setCursorStatus(int cs);
      
   int cursorTimeout;
      
   QTimer cursorTimer; ///< When this times out change cursor.
 
protected slots:
   
   virtual void cursorTimerOut();

public:
   int getEdgeTol(){ return edgeTol;}
   void setEdgeTol(int et){ edgeTol = et;}
    
   void setAltSelected(bool);
   bool isAltSelected(){return altSelected;}
      
   void setStretchable(bool);
   bool isStretchable(){return stretchable;}
        
signals:
   void moved(const QRectF &);
   void resized(const float &);
   void rejectMouse();
   void mouseIn();
   void mouseOut();
      
};

#endif //__StretchCircle_h__

