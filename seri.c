#include "seri.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Compile-time or runtime endianness detection */
static inline bool check_little_endian(void) {
    uint16_t num = 0x0001;
    return *((uint8_t*)&num) == 0x01;
}

#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
#define IS_LITTLE_ENDIAN (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#else
#define IS_LITTLE_ENDIAN check_little_endian()
#endif

/* Optimized byte-swap utilities using compiler built-ins */
static inline uint16_t bswap_16(uint16_t val) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap16(val);
#elif defined(_MSC_VER)
    return _byteswap_ushort(val);
#else
    return (uint16_t)((val >> 8) | (val << 8));
#endif
}

static inline uint32_t bswap_32(uint32_t val) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap32(val);
#elif defined(_MSC_VER)
    return _byteswap_ulong(val);
#else
    return ((val >> 24) & 0x000000FFU) |
           ((val >> 8)  & 0x0000FF00U) |
           ((val << 8)  & 0x00FF0000U) |
           ((val << 24) & 0xFF000000U);
#endif
}

static inline uint64_t bswap_64(uint64_t val) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap64(val);
#elif defined(_MSC_VER)
    return _byteswap_uint64(val);
#else
    val = ((val & 0x00000000FFFFFFFFULL) << 32) | ((val >> 32) & 0x00000000FFFFFFFFULL);
    val = ((val & 0x0000FFFF0000FFFFULL) << 16) | ((val >> 16) & 0x0000FFFF0000FFFFULL);
    val = ((val & 0x00FF00FF00FF00FFULL) << 8)  | ((val >> 8)  & 0x00FF00FF00FF00FFULL);
    return val;
#endif
}

/* Optimized in-place byte reversal utility for common element sizes */
static inline void swap_bytes(void* ptr, size_t size) {
    uint8_t* p = (uint8_t*)ptr;
    switch (size) {
        case 2: {
            uint16_t val;
            memcpy(&val, p, 2);
            val = bswap_16(val);
            memcpy(p, &val, 2);
            break;
        }
        case 4: {
            uint32_t val;
            memcpy(&val, p, 4);
            val = bswap_32(val);
            memcpy(p, &val, 4);
            break;
        }
        case 8: {
            uint64_t val;
            memcpy(&val, p, 8);
            val = bswap_64(val);
            memcpy(p, &val, 8);
            break;
        }
        default: {
            for (size_t i = 0; i < size / 2; ++i) {
                uint8_t tmp = p[i];
                p[i] = p[size - 1 - i];
                p[size - 1 - i] = tmp;
            }
            break;
        }
    }
}

/* 8 KB optimized chunk size for batched byte-swapping I/O */
#define CHUNK_BUFFER_SIZE 8192

int seri_write_array(FILE* file, uint8_t element_size, uint64_t num_elements, const void* data) {
    if (!file || (num_elements > 0 && !data)) {
        return -1;
    }

    // Guard against potential multiplication overflow
    if (num_elements > 0 && element_size > 0) {
        if (num_elements > SIZE_MAX / element_size) {
            return -1; // Overflow detected
        }
    }

    // 1. Write element_size (1 byte)
    if (fwrite(&element_size, sizeof(element_size), 1, file) != 1) {
        return -1;
    }

    // 2. Write num_elements (8 bytes, converted to big-endian)
    uint64_t n_be = IS_LITTLE_ENDIAN ? bswap_64(num_elements) : num_elements;
    if (fwrite(&n_be, sizeof(n_be), 1, file) != 1) {
        return -1;
    }

    if (num_elements == 0 || element_size == 0) {
        return 0;
    }

    // 3. Write data elements
    if (IS_LITTLE_ENDIAN && element_size > 1) {
        const size_t chunk_elems = (CHUNK_BUFFER_SIZE / element_size > 0) ? (CHUNK_BUFFER_SIZE / element_size) : 1;
        size_t buffer_bytes = chunk_elems * element_size;
        uint8_t* buf = (uint8_t*)malloc(buffer_bytes);
        if (!buf) {
            return -1;
        }

        const uint8_t* src = (const uint8_t*)data;
        uint64_t remaining = num_elements;

        while (remaining > 0) {
            size_t batch = (remaining < (uint64_t)chunk_elems) ? (size_t)remaining : chunk_elems;
            size_t bytes_to_process = batch * element_size;

            memcpy(buf, src, bytes_to_process);
            for (size_t i = 0; i < batch; ++i) {
                swap_bytes(buf + (i * element_size), element_size);
            }

            if (fwrite(buf, element_size, batch, file) != batch) {
                free(buf);
                return -1;
            }

            src += bytes_to_process;
            remaining -= batch;
        }
        free(buf);
    } else {
        if (fwrite(data, element_size, (size_t)num_elements, file) != (size_t)num_elements) {
            return -1;
        }
    }

    return 0;
}

/* Internal helper: Reads header without rewinding */
static int seri_read_header(FILE* file, uint8_t* element_size, uint64_t* num_elements) {
    uint8_t es;
    if (fread(&es, sizeof(es), 1, file) != 1) {
        return -1;
    }
    *element_size = es;

    uint64_t n_be;
    if (fread(&n_be, sizeof(n_be), 1, file) != 1) {
        return -1;
    }
    *num_elements = IS_LITTLE_ENDIAN ? bswap_64(n_be) : n_be;
    return 0;
}

/* Internal helper: Reads and converts array payload data */
static int seri_read_array_payload(FILE* file, uint8_t element_size, uint64_t num_elements, void* data) {
    if (num_elements == 0 || element_size == 0) {
        return 0;
    }

    if (num_elements > SIZE_MAX / element_size) {
        return -1;
    }

    uint8_t* dest = (uint8_t*)data;
    if (IS_LITTLE_ENDIAN && element_size > 1) {
        const size_t chunk_elems = (CHUNK_BUFFER_SIZE / element_size > 0) ? (CHUNK_BUFFER_SIZE / element_size) : 1;
        size_t buffer_bytes = chunk_elems * element_size;
        uint8_t* buf = (uint8_t*)malloc(buffer_bytes);
        if (!buf) {
            return -1;
        }

        uint64_t remaining = num_elements;
        uint64_t offset = 0;

        while (remaining > 0) {
            size_t batch = (remaining < (uint64_t)chunk_elems) ? (size_t)remaining : chunk_elems;
            size_t bytes_to_read = batch * element_size;

            if (fread(buf, element_size, batch, file) != batch) {
                free(buf);
                return -1;
            }

            for (size_t i = 0; i < batch; ++i) {
                swap_bytes(buf + (i * element_size), element_size);
            }

            memcpy(dest + (offset * element_size), buf, bytes_to_read);
            offset += batch;
            remaining -= batch;
        }
        free(buf);
    } else {
        if (fread(dest, element_size, (size_t)num_elements, file) != (size_t)num_elements) {
            return -1;
        }
    }

    return 0;
}

int seri_read_array_dim(FILE* file, uint8_t* element_size, uint64_t* num_elements) {
    if (!file || !element_size || !num_elements) {
        return -1;
    }

    long start_pos = ftell(file);
    if (start_pos < 0) {
        return -1;
    }

    if (seri_read_header(file, element_size, num_elements) != 0) {
        return -1;
    }

    if (fseek(file, start_pos, SEEK_SET) != 0) {
        return -1;
    }

    return 0;
}

int seri_read_array(FILE* file, uint8_t* element_size, uint64_t* num_elements, void* data) {
    if (!file || !element_size || !num_elements || !data) {
        return -1;
    }

    if (seri_read_header(file, element_size, num_elements) != 0) {
        return -1;
    }

    return seri_read_array_payload(file, *element_size, *num_elements, data);
}

int seri_write_fixed_buffer(FILE* file, const void* data, uint64_t size) {
    return seri_write_array(file, 1, size, data);
}

int seri_read_fixed_buffer(FILE* file, void* data, uint64_t expected_size) {
    if (!file || !data) {
        return -1;
    }

    uint8_t element_size = 0;
    uint64_t num_elements = 0;

    if (seri_read_header(file, &element_size, &num_elements) != 0) {
        return -1;
    }

    if (element_size != 1 || num_elements != expected_size) {
        return -1;
    }

    return seri_read_array_payload(file, element_size, num_elements, data);
}

/* --- Helper Pattern for Scalars --- */
static int seri_read_scalar(FILE* file, uint8_t expected_size, void* dest) {
    uint8_t elem_size = 0;
    uint64_t num_elements = 0;

    if (seri_read_header(file, &elem_size, &num_elements) != 0) {
        return -1;
    }

    if (elem_size != expected_size || num_elements != 1) {
        return -1;
    }

    return seri_read_array_payload(file, elem_size, num_elements, dest);
}

/* --- Boolean --- */
int seri_write_bool(FILE* file, bool value) {
    uint8_t val = value ? 1 : 0;
    return seri_write_array(file, sizeof(uint8_t), 1, &val);
}

int seri_read_bool(FILE* file, bool* value) {
    uint8_t val;
    if (seri_read_scalar(file, sizeof(uint8_t), &val) != 0) {
        return -1;
    }
    *value = (val != 0);
    return 0;
}

/* --- 32-bit Integer (Signed) --- */
int seri_write_int32(FILE* file, int32_t value) {
    return seri_write_array(file, sizeof(int32_t), 1, &value);
}

int seri_read_int32(FILE* file, int32_t* value) {
    return seri_read_scalar(file, sizeof(int32_t), value);
}

/* --- 32-bit Integer (Unsigned) --- */
int seri_write_uint32(FILE* file, uint32_t value) {
    return seri_write_array(file, sizeof(uint32_t), 1, &value);
}

int seri_read_uint32(FILE* file, uint32_t* value) {
    return seri_read_scalar(file, sizeof(uint32_t), value);
}

/* --- 64-bit Integer (Signed) --- */
int seri_write_int64(FILE* file, int64_t value) {
    return seri_write_array(file, sizeof(int64_t), 1, &value);
}

int seri_read_int64(FILE* file, int64_t* value) {
    return seri_read_scalar(file, sizeof(int64_t), value);
}

/* --- 64-bit Integer (Unsigned) --- */
int seri_write_uint64(FILE* file, uint64_t value) {
    return seri_write_array(file, sizeof(uint64_t), 1, &value);
}

int seri_read_uint64(FILE* file, uint64_t* value) {
    return seri_read_scalar(file, sizeof(uint64_t), value);
}

/* --- Float --- */
int seri_write_float(FILE* file, float value) {
    return seri_write_array(file, sizeof(float), 1, &value);
}

int seri_read_float(FILE* file, float* value) {
    return seri_read_scalar(file, sizeof(float), value);
}

/* --- Double --- */
int seri_write_double(FILE* file, double value) {
    return seri_write_array(file, sizeof(double), 1, &value);
}

int seri_read_double(FILE* file, double* value) {
    return seri_read_scalar(file, sizeof(double), value);
}
