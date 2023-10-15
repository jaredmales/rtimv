
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
#include <mutex>

#include <mx/app/application.hpp>

struct rtimvDictBlob
{
protected:
   size_t m_sz {0};
   size_t m_memSz {0};
   
   void * m_blob {nullptr};
   
   bool m_owner {false};
   
   timespec m_lastMod {0,0};

   std::mutex m_mutex;

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


public:


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
   
   rtimvDictBlob( const rtimvDictBlob & rdb) = delete; //no copies!
   
   /// Set the blob
   /** Allocates m_blob and copies \p sz bytes from \p blob.
     * Allocation only occurs if more size is needed according to m_memSz.
     * Sets m_sz to sz.
     */    
   void setBlob( const void * blob, ///< [in] The new blob data
                 size_t sz          ///< [in] The number of bytes in the new blob data
               )
   {
      std::unique_lock<std::mutex> lock(m_mutex);
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

      clock_gettime(CLOCK_REALTIME, &m_lastMod);
   }
   
   /// Get the data stored in the blob
   /** Copies the smaller of \p mxsz and m_sz bytes to \p blob 
     * \returns the number of bytes copied
     */
   size_t getBlob( char * blob, ///< [out] The pointer to the array to copy the data to
                   size_t mxsz, ///< [in] The max number of bytes to copy to blob
                   timespec * ts = nullptr /// [out] [optional] if not null this will be filled in with the time of last modification
                 )
   {
      std::unique_lock<std::mutex> lock(m_mutex);

      size_t cpsz = m_sz;
      if( cpsz > mxsz ) cpsz = mxsz;

      if(m_blob)
      {
         memcpy(blob, m_blob, cpsz);
      }
      else cpsz = 0;

      if(ts != nullptr) *ts = m_lastMod;

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
   size_t getBlobStr( char * blob, ///< [in/out] The pointer to the array to copy the data to
                      size_t mxsz,  ///< [in] The max number of bytes to copy to blob
                      timespec * ts = nullptr /// [out] [optional] if not null this will be filled in with the time of last modification
                    )
   {
      size_t cpsz = getBlob(blob, mxsz, ts);

      if(cpsz == 0)
      {
         if(mxsz > 0) 
         {
            blob[0] = '\0';
            cpsz = 1;
         }
      }
      else blob[cpsz-1] = '\0';

      return cpsz;
   }

   size_t getBlobSize()
   {
      std::unique_lock<std::mutex> lock(m_mutex);
      
      if(m_blob == nullptr) return 0;
      return m_sz;
      
   }

   ~rtimvDictBlob()
   {
      std::unique_lock<std::mutex> lock(m_mutex);
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

#define rtimvDictionaryInterface_iid "rtimv.dictionaryInterface/1.1"

Q_DECLARE_INTERFACE(rtimvDictionaryInterface, rtimvDictionaryInterface_iid)

/// Contains the internal components of rtimvMainWindow exposed to overlay plugins.
/** Plugins should copy the passed object of this type to an internal variable.
  */ 
struct rtimvOverlayAccess
{
   /// QObject handle for the main window to allow connecting signals and slots from within plugin
   QObject * m_mainWindowObject {nullptr};
   
   StretchBox * m_colorBox {nullptr};
   
   StretchBox * m_statsBox {nullptr};
   
   std::unordered_set<StretchBox *> * m_userBoxes {nullptr};
   
   std::unordered_set<StretchCircle *> * m_userCircles {nullptr};
   
   std::unordered_set<StretchLine *> * m_userLines {nullptr};
   
   rtimvGraphicsView * m_graphicsView {nullptr};  
   
   dictionaryT * m_dictionary {nullptr};
};

struct rtimvMouseCoord
{
   float pixelX {0};
   float pixelY {0};
   
   float imageValue {0};
   
   float displayValue {0};
   
   bool darkValid {false};
   float darkValue {0};
   
   bool satMaskValid {false};
   float satMaskValue {0};
   
   bool maskValid {false};
   float maskValue {0};
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
   
};

#define rtimvOverlayInterface_iid "rtimv.overlayInterface/1.1"

Q_DECLARE_INTERFACE(rtimvOverlayInterface, rtimvOverlayInterface_iid)

#endif //rtimvInterfaces_hpp
