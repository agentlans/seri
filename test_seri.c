#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include "seri.h"

#define RUN_TEST(test_func) \
    do { \
        printf("[RUNNING] %s...\n", #test_func); \
        test_func(); \
        printf("[PASSED]  %s\n", #test_func); \
    } while (0)

/* --- 1. Scalar Round-Trip Tests --- */
static void test_scalars(void) {
    FILE* f = tmpfile();
    assert(f != NULL);

    // Boolean
    bool w_bool = true, r_bool = false;
    assert(seri_write_bool(f, w_bool) == 0);
    
    // Int32
    int32_t w_i32 = -123456;
    int32_t r_i32 = 0;
    assert(seri_write_int32(f, w_i32) == 0);

    // UInt32
    uint32_t w_u32 = 4294967295U;
    uint32_t r_u32 = 0;
    assert(seri_write_uint32(f, w_u32) == 0);

    // Int64
    int64_t w_i64 = -9223372036854775807LL - 1LL;
    int64_t r_i64 = 0;
    assert(seri_write_int64(f, w_i64) == 0);

    // UInt64
    uint64_t w_u64 = 18446744073709551615ULL;
    uint64_t r_u64 = 0;
    assert(seri_write_uint64(f, w_u64) == 0);

    // Float
    float w_f = 3.14159f;
    float r_f = 0.0f;
    assert(seri_write_float(f, w_f) == 0);

    // Double
    double w_d = 2.718281828459045;
    double r_d = 0.0;
    assert(seri_write_double(f, w_d) == 0);

    // Rewind for reading
    rewind(f);

    assert(seri_read_bool(f, &r_bool) == 0 && r_bool == w_bool);
    assert(seri_read_int32(f, &r_i32) == 0 && r_i32 == w_i32);
    assert(seri_read_uint32(f, &r_u32) == 0 && r_u32 == w_u32);
    assert(seri_read_int64(f, &r_i64) == 0 && r_i64 == w_i64);
    assert(seri_read_uint64(f, &r_u64) == 0 && r_u64 == w_u64);
    assert(seri_read_float(f, &r_f) == 0 && fabsf(r_f - w_f) < 1e-6f);
    assert(seri_read_double(f, &r_d) == 0 && fabs(r_d - w_d) < 1e-12);

    fclose(f);
}

/* --- 2. Array Round-Trip Tests --- */
static void test_arrays(void) {
    FILE* f = tmpfile();
    assert(f != NULL);

    uint32_t w_arr[] = { 10, 20, 30, 40, 50 };
    uint64_t num_elems = 5;

    assert(seri_write_array(f, sizeof(uint32_t), num_elems, w_arr) == 0);

    rewind(f);

    uint8_t read_es = 0;
    uint64_t read_ne = 0;
    uint32_t r_arr[5] = { 0 };

    assert(seri_read_array(f, &read_es, &read_ne, r_arr) == 0);
    assert(read_es == sizeof(uint32_t));
    assert(read_ne == num_elems);

    for (uint64_t i = 0; i < num_elems; ++i) {
        assert(r_arr[i] == w_arr[i]);
    }

    fclose(f);
}

/* --- 3. Dimension Peeking (`seri_read_array_dim`) Tests --- */
static void test_array_dim(void) {
    FILE* f = tmpfile();
    assert(f != NULL);

    int64_t w_data[] = { 100, 200, 300 };
    assert(seri_write_array(f, sizeof(int64_t), 3, w_data) == 0);

    rewind(f);

    uint8_t es = 0;
    uint64_t ne = 0;
    // Peek dimensions
    assert(seri_read_array_dim(f, &es, &ne) == 0);
    assert(es == sizeof(int64_t));
    assert(ne == 3);

    // Ensure peeking didn't consume stream position permanently; read should succeed normally
    int64_t r_data[3] = { 0 };
    uint8_t r_es;
    uint64_t r_ne;
    assert(seri_read_array(f, &r_es, &r_ne, r_data) == 0);
    assert(r_data[0] == 100 && r_data[1] == 200 && r_data[2] == 300);

    fclose(f);
}

/* --- 4. Fixed Buffer Tests --- */
static void test_fixed_buffer(void) {
    FILE* f = tmpfile();
    assert(f != NULL);

    const char* original_msg = "Hello Serialization!";
    uint64_t len = strlen(original_msg) + 1;

    assert(seri_write_fixed_buffer(f, original_msg, len) == 0);

    rewind(f);

    char buffer[32] = { 0 };
    // Read with matching expected size
    assert(seri_read_fixed_buffer(f, buffer, len) == 0);
    assert(strcmp(buffer, original_msg) == 0);

    // Test size mismatch detection
    rewind(f);
    assert(seri_read_fixed_buffer(f, buffer, len + 5) == -1);

    fclose(f);
}

/* --- 5. Error Handling & Edge Cases --- */
static void test_error_handling(void) {
    FILE* f = tmpfile();
    assert(f != NULL);

    // Null pointer arguments
    assert(seri_write_array(NULL, 4, 1, NULL) == -1);
    assert(seri_read_array_dim(NULL, NULL, NULL) == -1);
    assert(seri_write_bool(NULL, true) == -1);

    // Multiplication overflow protection test
    uint64_t massive_elems = UINT64_MAX;
    int32_t dummy = 0;
    assert(seri_write_array(f, sizeof(int32_t), massive_elems, &dummy) == -1);

    fclose(f);
}

int main(void) {
    printf("=== Starting seri Module Test Suite ===\n");
    
    RUN_TEST(test_scalars);
    RUN_TEST(test_arrays);
    RUN_TEST(test_array_dim);
    RUN_TEST(test_fixed_buffer);
    RUN_TEST(test_error_handling);

    printf("=== All tests passed successfully! ===\n");
    return 0;
}
