



#ifndef pixaccess_h
#define pixaccess_h

#include "ImageStruct.hpp"

///Function to cast the data type to float.
/** Accesses the imdata pointer at linear position idx, and then casts the result to float.
  *
  * \returns the value of the image at linear position idx cast to float
  * 
  * \tparam dataT the type of the image data.
  */ 
template<typename dataT>
float getPix( void *imdata, ///< [in] Pointer to the image data 
              size_t idx    ///< [in] Linear position of the pixel in the imdata
            )
{
   return (float) ((dataT*) imdata)[idx];
}

///Get the function pointer for getPix for the type
template<int imageStructDataT>
float (*getPixPointer())(void*, size_t)
{
   return &getPix<typename imageStructDataType<imageStructDataT>::type>;
}

#endif



