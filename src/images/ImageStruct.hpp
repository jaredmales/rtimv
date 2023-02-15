#ifndef ImageStruct_hpp
#define ImageStruct_hpp

//milk includes
#ifdef RTIMV_MILK
#include <ImageStreamIO/ImageStreamIO.h>
#include <ImageStreamIO/ImageStruct.h>
#else
typedef struct {
    float re;
    float im;
} complex_float;

typedef struct {
    double re;
    double im;
} complex_double;
#endif

#ifndef ULONGLONG_IMG
#define ULONGLONG_IMG (80)
#endif

#define IMAGESTRUCT_UINT8                   1
#define IMAGESTRUCT_INT8                    2
#define IMAGESTRUCT_UINT16                  3
#define IMAGESTRUCT_INT16                   4
#define IMAGESTRUCT_UINT32                  5
#define IMAGESTRUCT_INT32                   6
#define IMAGESTRUCT_UINT64                  7
#define IMAGESTRUCT_INT64                   8
#define IMAGESTRUCT_FLOAT                   9
#define IMAGESTRUCT_DOUBLE                 10
#define IMAGESTRUCT_COMPLEX_FLOAT          11
#define IMAGESTRUCT_COMPLEX_DOUBLE         12
#define IMAGESTRUCT_EVENT_UI8_UI8_UI16_UI8 20

template<int is_dt>
struct imageStructDataType;

template<>
struct imageStructDataType<IMAGESTRUCT_UINT8>
{
   typedef uint8_t type;
   constexpr static size_t size = sizeof(type);
   constexpr static type max = std::numeric_limits<type>::max();
};

template<>
struct imageStructDataType<IMAGESTRUCT_INT8>
{
   typedef int8_t type;
   constexpr static size_t size = sizeof(type);
   constexpr static type max = std::numeric_limits<type>::max();
};

template<>
struct imageStructDataType<IMAGESTRUCT_UINT16>
{
   typedef uint16_t type;
   constexpr static size_t size = sizeof(type);
   constexpr static type max = std::numeric_limits<type>::max();
};

template<>
struct imageStructDataType<IMAGESTRUCT_INT16>
{
   typedef int16_t type;
   constexpr static size_t size = sizeof(type);
   constexpr static type max = std::numeric_limits<type>::max();
};

template<>
struct imageStructDataType<IMAGESTRUCT_UINT32>
{
   typedef uint32_t type;
   constexpr static size_t size = sizeof(type);
   constexpr static type max = std::numeric_limits<type>::max();
};

template<>
struct imageStructDataType<IMAGESTRUCT_INT32>
{
   typedef int32_t type;
   constexpr static size_t size = sizeof(type);
   constexpr static type max = std::numeric_limits<type>::max();
};

template<>
struct imageStructDataType<IMAGESTRUCT_UINT64>
{
   typedef uint64_t type;
   constexpr static size_t size = sizeof(type);
   constexpr static type max = std::numeric_limits<type>::max();
};

template<>
struct imageStructDataType<IMAGESTRUCT_INT64>
{
   typedef int64_t type;
   constexpr static size_t size = sizeof(type);
   constexpr static type max = std::numeric_limits<type>::max();
};

template<>
struct imageStructDataType<IMAGESTRUCT_FLOAT>
{
   typedef float type;
   constexpr static size_t size = sizeof(type);
   constexpr static type max = std::numeric_limits<type>::max();
};

template<>
struct imageStructDataType<IMAGESTRUCT_DOUBLE>
{
   typedef double type;
   constexpr static size_t size = sizeof(type);
   constexpr static type max = std::numeric_limits<type>::max();
};

template<>
struct imageStructDataType<IMAGESTRUCT_COMPLEX_FLOAT>
{
   typedef complex_float type;
   constexpr static size_t size = sizeof(type);
   constexpr static type max = std::numeric_limits<type>::max();
};

template<>
struct imageStructDataType<IMAGESTRUCT_COMPLEX_DOUBLE>
{
   typedef complex_double type;
   constexpr static size_t size = sizeof(type);
};

#endif //ImageStruct_hpp
