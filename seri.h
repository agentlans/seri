#ifndef SERI_H
#define SERI_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Writes an array to file in big-endian format.
 * Returns 0 on success, -1 on error.
 */
int seri_write_array(FILE* file, uint8_t element_size, uint64_t num_elements, const void* data);

/**
 * Reads array dimensions (element_size and num_elements) and rewinds the file pointer.
 * Returns 0 on success, -1 on error.
 */
int seri_read_array_dim(FILE* file, uint8_t* element_size, uint64_t* num_elements);

/**
 * Reads an array from file, converting from big-endian to native endian.
 * Returns 0 on success, -1 on error.
 */
int seri_read_array(FILE* file, uint8_t* element_size, uint64_t* num_elements, void* data);

/* --- Fixed-Size Buffer Convenience Functions --- */

/**
 * Writes a fixed-size byte buffer to the file.
 * Returns 0 on success, -1 on error.
 */
int seri_write_fixed_buffer(FILE* file, const void* data, uint64_t size);

/**
 * Reads a fixed-size byte buffer from the file.
 * Verifies that the stored dimensions match the expected size.
 * Returns 0 on success, -1 on error (including size or type mismatch).
 */
int seri_read_fixed_buffer(FILE* file, void* data, uint64_t expected_size);

/* --- Scalar Convenience Functions --- */

int seri_write_bool(FILE* file, bool value);
int seri_read_bool(FILE* file, bool* value);

int seri_write_int32(FILE* file, int32_t value);
int seri_read_int32(FILE* file, int32_t* value);

int seri_write_uint32(FILE* file, uint32_t value);
int seri_read_uint32(FILE* file, uint32_t* value);

int seri_write_int64(FILE* file, int64_t value);
int seri_read_int64(FILE* file, int64_t* value);

int seri_write_uint64(FILE* file, uint64_t value);
int seri_read_uint64(FILE* file, uint64_t* value);

int seri_write_float(FILE* file, float value);
int seri_read_float(FILE* file, float* value);

int seri_write_double(FILE* file, double value);
int seri_read_double(FILE* file, double* value);

#ifdef __cplusplus
}
#endif

#endif /* SERI_H */
