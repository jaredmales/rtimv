
#ifndef rtimvInterfaces_hpp
#define rtimvInterfaces_hpp

#include <QtPlugin>
#include <QGraphicsScene>
#include <QKeyEvent>

#include <unordered_map>

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

class rtimvDictionaryInterface
{
   public:
      virtual ~rtimvDictionaryInterface() = default;
      
      virtual int attachDictionary(std::unordered_map<std::string, rtimvDictBlob> *) = 0;
};

#define rtimvDictionaryInterface_iid "rtimv.dictionaryInterface/1.0"

Q_DECLARE_INTERFACE(rtimvDictionaryInterface, rtimvDictionaryInterface_iid)

class rtimvOverlayInterface
{
   public:
      virtual ~rtimvOverlayInterface() = default;

      virtual int attachOverlay(QGraphicsScene*, std::unordered_map<std::string, rtimvDictBlob> *) = 0; 
      
      virtual int updateOverlay() = 0;

      /// Handle a key press event
      virtual void keyPressEvent(QKeyEvent * ke) = 0;
      
      /// Check if the overlay is currently enabled.
      virtual bool overlayEnabled() = 0;
      
      virtual void enableOverlay() = 0;

      virtual void disableOverlay() = 0;
      
};

#define rtimvOverlayInterface_iid "rtimv.overlayInterface/1.0"

Q_DECLARE_INTERFACE(rtimvOverlayInterface, rtimvOverlayInterface_iid)

#endif //rtimvInterfaces_hpp
