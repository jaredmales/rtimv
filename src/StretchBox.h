
#ifndef __StretchBox_h__
#define __StretchBox_h__


#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QCursor>
#include <QObject>
#include <QTimer>

class StretchBox : public QObject, public QGraphicsRectItem
{
   Q_OBJECT

   public:

      StretchBox(QGraphicsItem * parent = 0);
      StretchBox(const QRectF & rect, QGraphicsItem * parent = 0 );
      StretchBox(qreal x, qreal y, qreal width, qreal height, QGraphicsItem * parent = 0 );

      QCursor defCursor;
      qreal xoff, yoff;
   private:
      void initStretchBox();
      
   protected:
      
      int edgeTol;
      
      void hoverMoveEvent(QGraphicsSceneHoverEvent * e);
      void hoverLeaveEvent(QGraphicsSceneHoverEvent * e);
      
      void mousePressEvent ( QGraphicsSceneMouseEvent * event );
      void mouseReleaseEvent(QGraphicsSceneMouseEvent * event );
      
      void mouseMoveEvent(QGraphicsSceneMouseEvent * event);

      ///Tracks how the box is currently being resized.
      int sizing;
      
      ///Describes how the box is being resized.
      enum sizingMode{szOff, szLeft, szTopl, szTop, szTopr, szRight, szBotr, szBot, szBotl};
      
      ///Tracks whether the box is being resized.
      bool isSizing;
      
      ///Trackes whether the box is moving.
      bool isMoving;
      
      double x0, y0, w0, h0, mx0, my0;
            
      ///Is this box stretchable?  If not goes straight to grab for move.
      /** If stretchable == false, then mouse over goes straight to the hand.
        */
      bool stretchable;
      
      bool grabbing;
      
      int cursorStatus;
      
      void setCursorStatus(int cs);
      
      int cursorTimeout;
      
      QTimer cursorTimer; ///< When this times out change cursor.

protected slots:
   
   virtual void cursorTimerOut();
      
public:

   int getEdgeTol(){ return edgeTol;}
   void setEdgeTol(int et){ edgeTol = et;}
      
//    void setAltSelected(bool);
//    bool isAltSelected(){return altSelected;}
      
   void setStretchable(bool);
   bool isStretchable(){return stretchable;}

   void setCursorTimeout(int cto);
   int getCursorTimeout();
   
signals:
   void moved(const QRectF &);
   void rejectMouse();
};

#endif //__StretchBox_h__

