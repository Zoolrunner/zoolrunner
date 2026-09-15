/* ZoolRunner portability for SDKs predating libkern/OSByteOrder.h. */
#ifndef ZR_EARLY_DARWIN_BYTEORDER_H
#define ZR_EARLY_DARWIN_BYTEORDER_H

static inline uint16_t zr_swap16(uint16_t x)
{
    return (uint16_t)((x >> 8) | (x << 8));
}
static inline uint32_t zr_swap32(uint32_t x)
{
    return (x >> 24) | ((x >> 8) & 0xff00U) |
           ((x << 8) & 0xff0000U) | (x << 24);
}
static inline uint64_t zr_swap64(uint64_t x)
{
    return ((uint64_t)zr_swap32((uint32_t)x) << 32) |
           zr_swap32((uint32_t)(x >> 32));
}

#if defined(__BIG_ENDIAN__)
#define htobe16(x) ((uint16_t)(x))
#define htobe32(x) ((uint32_t)(x))
#define htobe64(x) ((uint64_t)(x))
#define htole16(x) zr_swap16(x)
#define htole32(x) zr_swap32(x)
#define htole64(x) zr_swap64(x)
#elif defined(__LITTLE_ENDIAN__)
#define htole16(x) ((uint16_t)(x))
#define htole32(x) ((uint32_t)(x))
#define htole64(x) ((uint64_t)(x))
#define htobe16(x) zr_swap16(x)
#define htobe32(x) zr_swap32(x)
#define htobe64(x) zr_swap64(x)
#else
#error Unknown Darwin byte order
#endif
#define be16toh(x) htobe16(x)
#define be32toh(x) htobe32(x)
#define be64toh(x) htobe64(x)
#define le16toh(x) htole16(x)
#define le32toh(x) htole32(x)
#define le64toh(x) htole64(x)
#endif
