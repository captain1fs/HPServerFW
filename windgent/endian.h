#ifndef __ENDIAN_H__
#define __ENDIAN_H__

#define WINDGENT_LITTLE_ENDIAN 1
#define WINDGENT_BIG_ENDIAN 2

#include <byteswap.h>
#include <stdint.h>
#include <iostream>

namespace windgent {

//std::enable_if是模板结构体，typename enable_if<_Test, _Ty>::type，_Test如果是true, std::enable_if的type为_Ty；如果是false, type没有定义，这种情况在程序编译的时候就会报错。
//作用：根据函数的返回值/参数，选择不同的模板。以下三个函数用于交换一个2、4、8字节整数的字节顺序
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type byteswap(T val) {
    return (T)bswap_64((uint64_t)val);
}

template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type byteswap(T val) {
    return (T)bswap_32((uint32_t)val);
}

template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type byteswap(T val) {
    return (T)bswap_16((uint16_t)val);
}

//如果当前机器的字节序为大端
#if BYTE_ORDER == BIG_ENDIAN
#define WINDGENT_BYTE_ORDER WINDGENT_BIG_ENDIAN
#else
#define WINDGENT_BYTE_ORDER WINDGENT_LITTLE_ENDIAN
#endif

//因为网络字节序默认为大端，所以如果当前机器是大端无需做什么，但在小端机器上需要byteswap
#if WINDGENT_BYTE_ORDER == WINDGENT_BIG_ENDIAN

template<class T>
T byteswapOnLittleEndian(T t) {
    return t;
}

template<class T>
T byteswapOnBigEndian(T t) {
    return byteswap(t);
}

#else

//当前机器字节序为小端，调用此模板函数
template<class T>
T byteswapOnLittleEndian(T t) {
    return byteswap(t);
}

template<class T>
T byteswapOnBigEndian(T t) {
    return t;
}
#endif

}

#endif