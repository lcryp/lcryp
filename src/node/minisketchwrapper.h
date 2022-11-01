#ifndef LCRYP_NODE_MINISKETCHWRAPPER_H
#define LCRYP_NODE_MINISKETCHWRAPPER_H
#include <minisketch.h>
#include <cstddef>
#include <cstdint>
namespace node {
Minisketch MakeMinisketch32(size_t capacity);
Minisketch MakeMinisketch32FP(size_t max_elements, uint32_t fpbits);
}
#endif
