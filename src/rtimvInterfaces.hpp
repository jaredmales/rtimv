
#ifndef rtimvInterfaces_hpp
#define rtimvInterfaces_hpp

#include <unordered_set>

#include <QtPlugin>
#include <QGraphicsScene>
#include <QKeyEvent>

#include "rtimvGraphicsView.hpp"
#include "StretchBox.hpp"
#include "StretchCircle.hpp"
#include "StretchLine.hpp"

#include <unordered_map>

#include <mx/app/application.hpp>

struct rtimvDictBlob
{
   size_t m_sz {0};
   size_t m_memSz {0};
   
   void * m_blob {nullptr};
   
   bool m_owner {false};
   
   rtimvDictBlob()
   {
   }
   
   rtimvDictBlob( rtimvDictBlob & rdb)
   {
      clearBlob();
      m_blob = rdb.m_blob;
      m_sz = rdb.m_sz;
      m_memSz = rdb.m_memSz;
      m_owner = rdb.m_owner;
      rdb.m_owner = false;
   }
   
   rtimvDictBlob( const rtimvDictBlob & rdb) = delete;
   
   void clearBlob()
   {
      if(m_blob && m_owner) 
      {
         free(m_blob);
         m_blob = nullptr;
         m_owner = false;
         m_sz = 0;
         m_memSz = 0;
      }
   }
   
   void setBlob( const void * blob,
                 size_t sz
               )
   {
      if(sz > m_memSz || m_owner == false)
      {
         clearBlob();
      
         m_blob = malloc(sz);
         m_sz = sz;
         m_memSz = sz;
         memcpy(m_blob, blob, sz);
      
         m_owner = true;
      }
      else
      {
         m_sz = sz;
         memcpy(m_blob, blob, sz);
      }
   }
   
   ~rtimvDictBlob()
   {
      clearBlob();
   }
   
};

///The dictionary type, a std::map
typedef std::map<std::string, rtimvDictBlob> dictionaryT;
      
///The dictionary iterator type.
typedef std::map<std::string, rtimvDictBlob>::iterator dictionaryIteratorT;
      
class rtimvDictionaryInterface
{
   public:
      virtual ~rtimvDictionaryInterface() = default;
      
      virtual int attachDictionary( dictionaryT *,
                                    mx::app::appConfigurator &
                                  ) = 0;
};

#define rtimvDictionaryInterface_iid "rtimv.dictionaryInterface/1.0"

Q_DECLARE_INTERFACE(rtimvDictionaryInterface, rtimvDictionaryInterface_iid)

/// Contains the internal components of rtimvMainWindow exposed to overlay plugins.
/** Plugins should copy the passed object of this type to an internal variable.
  */ 
struct rtimvOverlayAccess
{
   StretchBox * m_colorBox {nullptr};
   StretchBox * m_statsBox {nullptr};
   std::unordered_set<StretchBox *> * m_userBoxes {nullptr};
   std::unordered_set<StretchCircle *> * m_userCircles {nullptr};
   std::unordered_set<StretchLine *> * m_userLines {nullptr};
   rtimvGraphicsView * m_graphicsView {nullptr};  
   dictionaryT * m_dictionary {nullptr};
};

class rtimvOverlayInterface : public QObject
{
   public:
      
      
      virtual ~rtimvOverlayInterface() = default;

      virtual int attachOverlay( rtimvOverlayAccess &,
                                 mx::app::appConfigurator &
                               ) = 0; 
      
      virtual int updateOverlay() = 0;

      /// Handle a key press event
      virtual void keyPressEvent(QKeyEvent * ke) = 0;
      
      /// Check if the overlay is currently enabled.
      virtual bool overlayEnabled() = 0;
      
      virtual void enableOverlay() = 0;

      virtual void disableOverlay() = 0;
      
   //Derived class must declare:
   /*
   signals:
         
      void newStretchBox(StretchBox *);
      void newStretchCircle(StretchCircle *);
      void newStretchLine(StretchLine *);
   */
};

#define rtimvOverlayInterface_iid "rtimv.overlayInterface/1.0"

Q_DECLARE_INTERFACE(rtimvOverlayInterface, rtimvOverlayInterface_iid)

#endif //rtimvInterfaces_hpp
