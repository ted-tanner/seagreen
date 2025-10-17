#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "seagreen.h"

// Large struct with many different field types
typedef struct {
    // Basic types
    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    float f32;
    double f64;
    char c;
    _Bool b;
    
    // Arrays
    int arr_int[10];
    float arr_float[5];
    char arr_char[20];
    
    // Nested struct
    struct {
        int nested_int;
        double nested_double;
        char nested_string[15];
    } nested;
    
    // More complex fields
    void *ptr;
    size_t size;
    
    // Large array
    uint32_t large_array[50];
    
    // String fields
    char string1[30];
    char string2[40];
    
    // More numeric fields
    long long_val;
    unsigned long ulong_val;
    short short_val;
    unsigned short ushort_val;
    
    // Additional arrays
    double arr_double[8];
    uint64_t arr_uint64[12];
    
    // Final fields
    int final_int;
    double final_double;
    char final_char;
} LargeStruct;

// Function to initialize a large struct with test data
void init_large_struct(LargeStruct *s, int base_value) {
    // Basic types
    s->i8 = (int8_t)(base_value + 1);
    s->i16 = (int16_t)(base_value + 2);
    s->i32 = base_value + 3;
    s->i64 = (int64_t)(base_value + 4);
    s->u8 = (uint8_t)(base_value + 5);
    s->u16 = (uint16_t)(base_value + 6);
    s->u32 = (uint32_t)(base_value + 7);
    s->u64 = (uint64_t)(base_value + 8);
    s->f32 = (float)(base_value + 9.5);
    s->f64 = (double)(base_value + 10.5);
    s->c = (char)('A' + (base_value % 26));
    s->b = (base_value % 2) == 0;
    
    // Arrays
    for (int i = 0; i < 10; i++) {
        s->arr_int[i] = base_value + i;
    }
    for (int i = 0; i < 5; i++) {
        s->arr_float[i] = (float)(base_value + i + 0.5);
    }
    for (int i = 0; i < 20; i++) {
        s->arr_char[i] = (char)('a' + (i % 26));
    }
    
    // Nested struct
    s->nested.nested_int = base_value + 100;
    s->nested.nested_double = (double)(base_value + 101.5);
    snprintf(s->nested.nested_string, sizeof(s->nested.nested_string), "nested_%d", base_value);
    
    // Complex fields
    s->ptr = (void *)(uintptr_t)(base_value + 200);
    s->size = (size_t)(base_value + 201);
    
    // Large array
    for (int i = 0; i < 50; i++) {
        s->large_array[i] = (uint32_t)(base_value + i);
    }
    
    // String fields
    snprintf(s->string1, sizeof(s->string1), "string1_%d", base_value);
    snprintf(s->string2, sizeof(s->string2), "string2_%d", base_value);
    
    // More numeric fields
    s->long_val = (long)(base_value + 300);
    s->ulong_val = (unsigned long)(base_value + 301);
    s->short_val = (short)(base_value + 302);
    s->ushort_val = (unsigned short)(base_value + 303);
    
    // Additional arrays
    for (int i = 0; i < 8; i++) {
        s->arr_double[i] = (double)(base_value + i + 0.25);
    }
    for (int i = 0; i < 12; i++) {
        s->arr_uint64[i] = (uint64_t)(base_value + i);
    }
    
    // Final fields
    s->final_int = base_value + 400;
    s->final_double = (double)(base_value + 401.75);
    s->final_char = (char)('Z' - (base_value % 26));
}

// Function to validate a large struct
int validate_large_struct(const LargeStruct *s, int expected_base_value) {
    // Validate basic types
    if (s->i8 != (int8_t)(expected_base_value + 1)) return 1;
    if (s->i16 != (int16_t)(expected_base_value + 2)) return 2;
    if (s->i32 != expected_base_value + 3) return 3;
    if (s->i64 != (int64_t)(expected_base_value + 4)) return 4;
    if (s->u8 != (uint8_t)(expected_base_value + 5)) return 5;
    if (s->u16 != (uint16_t)(expected_base_value + 6)) return 6;
    if (s->u32 != (uint32_t)(expected_base_value + 7)) return 7;
    if (s->u64 != (uint64_t)(expected_base_value + 8)) return 8;
    if (s->f32 != (float)(expected_base_value + 9.5)) return 9;
    if (s->f64 != (double)(expected_base_value + 10.5)) return 10;
    if (s->c != (char)('A' + (expected_base_value % 26))) return 11;
    if (s->b != ((expected_base_value % 2) == 0)) return 12;
    
    // Validate arrays
    for (int i = 0; i < 10; i++) {
        if (s->arr_int[i] != expected_base_value + i) return 20 + i;
    }
    for (int i = 0; i < 5; i++) {
        if (s->arr_float[i] != (float)(expected_base_value + i + 0.5)) return 30 + i;
    }
    for (int i = 0; i < 20; i++) {
        if (s->arr_char[i] != (char)('a' + (i % 26))) return 40 + i;
    }
    
    // Validate nested struct
    if (s->nested.nested_int != expected_base_value + 100) return 100;
    if (s->nested.nested_double != (double)(expected_base_value + 101.5)) return 101;
    char expected_nested_string[15];
    snprintf(expected_nested_string, sizeof(expected_nested_string), "nested_%d", expected_base_value);
    if (strcmp(s->nested.nested_string, expected_nested_string) != 0) return 102;
    
    // Validate complex fields
    if (s->ptr != (void *)(uintptr_t)(expected_base_value + 200)) return 200;
    if (s->size != (size_t)(expected_base_value + 201)) return 201;
    
    // Validate large array
    for (int i = 0; i < 50; i++) {
        if (s->large_array[i] != (uint32_t)(expected_base_value + i)) return 250 + i;
    }
    
    // Validate string fields
    char expected_string1[30];
    char expected_string2[40];
    snprintf(expected_string1, sizeof(expected_string1), "string1_%d", expected_base_value);
    snprintf(expected_string2, sizeof(expected_string2), "string2_%d", expected_base_value);
    if (strcmp(s->string1, expected_string1) != 0) return 300;
    if (strcmp(s->string2, expected_string2) != 0) return 301;
    
    // Validate more numeric fields
    if (s->long_val != (long)(expected_base_value + 300)) return 302;
    if (s->ulong_val != (unsigned long)(expected_base_value + 301)) return 303;
    if (s->short_val != (short)(expected_base_value + 302)) return 304;
    if (s->ushort_val != (unsigned short)(expected_base_value + 303)) return 305;
    
    // Validate additional arrays
    for (int i = 0; i < 8; i++) {
        if (s->arr_double[i] != (double)(expected_base_value + i + 0.25)) return 310 + i;
    }
    for (int i = 0; i < 12; i++) {
        if (s->arr_uint64[i] != (uint64_t)(expected_base_value + i)) return 320 + i;
    }
    
    // Validate final fields
    if (s->final_int != expected_base_value + 400) return 400;
    if (s->final_double != (double)(expected_base_value + 401.75)) return 401;
    if (s->final_char != (char)('Z' - (expected_base_value % 26))) return 402;
    
    return 0; // All validations passed
}

// Async function that processes a large struct
async uint64_t process_large_struct(void *p) {
    LargeStruct *s = (LargeStruct *)p;
    
    printf("Processing large struct with base value %d\n", s->i32 - 3);
    
    // Validate the struct
    int validation_result = validate_large_struct(s, s->i32 - 3);
    if (validation_result != 0) {
        printf("Struct validation failed at field %d\n", validation_result);
        return validation_result;
    }
    
    // Yield to test context switching with large structs
    async_yield();
    
    // Perform some calculations on the struct
    uint64_t sum = 0;
    sum += s->i8 + s->i16 + s->i32 + s->i64;
    sum += s->u8 + s->u16 + s->u32 + s->u64;
    sum += (uint64_t)s->f32 + (uint64_t)s->f64;
    sum += s->c + s->b;
    
    // Sum array elements
    for (int i = 0; i < 10; i++) {
        sum += s->arr_int[i];
    }
    for (int i = 0; i < 5; i++) {
        sum += (uint64_t)s->arr_float[i];
    }
    
    // Sum nested struct
    sum += s->nested.nested_int + (uint64_t)s->nested.nested_double;
    
    // Sum large array
    for (int i = 0; i < 50; i++) {
        sum += s->large_array[i];
    }
    
    // Sum more fields
    sum += s->long_val + s->ulong_val + s->short_val + s->ushort_val;
    sum += s->final_int + (uint64_t)s->final_double + s->final_char;
    
    printf("Large struct processed, sum: %llu\n", sum);
    
    return sum;
}

// Async function that modifies a large struct
async uint64_t modify_large_struct(void *p) {
    LargeStruct *s = (LargeStruct *)p;
    
    printf("Modifying large struct with base value %d\n", s->i32 - 3);
    
    // Modify some fields
    s->i32 += 1000;
    s->f64 += 1000.0;
    s->final_int += 2000;
    
    // Modify arrays
    for (int i = 0; i < 10; i++) {
        s->arr_int[i] += 100;
    }
    for (int i = 0; i < 50; i++) {
        s->large_array[i] += 200;
    }
    
    // Modify nested struct
    s->nested.nested_int += 300;
    s->nested.nested_double += 300.0;
    
    // Modify strings
    snprintf(s->string1, sizeof(s->string1), "modified_%d", s->i32);
    snprintf(s->string2, sizeof(s->string2), "updated_%d", s->final_int);
    
    async_yield();
    
    printf("Large struct modified\n");
    
    return s->i32 + s->final_int;
}

// Async function that calls another async function with large struct
async uint64_t nested_large_struct_func(void *p) {
    LargeStruct *s = (LargeStruct *)p;
    
    printf("Nested async function processing large struct\n");
    
    // Call another async function with the same struct
    CGNThreadHandle nested_handle = async_run(process_large_struct, s);
    uint64_t nested_result = await(nested_handle);
    
    // Modify the struct
    s->i32 += 500;
    s->final_int += 500;
    
    return nested_result + s->i32 + s->final_int;
}

// Async function to process very large struct
async uint64_t process_very_large_struct(void *p) {
    typedef struct {
        LargeStruct base;
        int extra_fields[100];
        double extra_doubles[50];
        char extra_strings[10][20];
    } VeryLargeStruct;
    
    VeryLargeStruct *vls = (VeryLargeStruct *)p;
    
    printf("Processing very large struct\n");
    
    // Process base struct
    uint64_t base_sum = 0;
    base_sum += vls->base.i32 + vls->base.final_int;
    
    // Process extra fields
    for (int i = 0; i < 100; i++) {
        base_sum += vls->extra_fields[i];
    }
    for (int i = 0; i < 50; i++) {
        base_sum += (uint64_t)vls->extra_doubles[i];
    }
    
    async_yield();
    
    return base_sum;
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();
    
    printf("Testing async functions with large structs...\n");
    
    // Test 1: Single large struct
    printf("\n=== Test 1: Single large struct ===\n");
    LargeStruct struct1;
    init_large_struct(&struct1, 100);
    
    CGNThreadHandle handle1 = async_run(process_large_struct, &struct1);
    uint64_t result1 = await(handle1);
    
    printf("Single large struct result: %llu\n", result1);
    assert(result1 > 0); // Should be a positive sum
    
    // Test 2: Multiple large structs
    printf("\n=== Test 2: Multiple large structs ===\n");
    LargeStruct structs[5];
    CGNThreadHandle handles[5];
    
    for (int i = 0; i < 5; i++) {
        init_large_struct(&structs[i], 200 + i * 10);
        handles[i] = async_run(process_large_struct, &structs[i]);
    }
    
    uint64_t total_sum = 0;
    for (int i = 0; i < 5; i++) {
        uint64_t result = await(handles[i]);
        printf("Struct %d result: %llu\n", i, result);
        total_sum += result;
        assert(result > 0);
    }
    
    printf("Total sum from multiple structs: %llu\n", total_sum);
    
    // Test 3: Struct modification
    printf("\n=== Test 3: Struct modification ===\n");
    LargeStruct struct3;
    init_large_struct(&struct3, 300);
    
    // Store original values
    int original_i32 = struct3.i32;
    int original_final_int = struct3.final_int;
    
    CGNThreadHandle handle3 = async_run(modify_large_struct, &struct3);
    uint64_t result3 = await(handle3);
    
    printf("Struct modification result: %llu\n", result3);
    
    // Verify modifications
    assert(struct3.i32 == original_i32 + 1000);
    assert(struct3.final_int == original_final_int + 2000);
    assert(strstr(struct3.string1, "modified_") != NULL);
    assert(strstr(struct3.string2, "updated_") != NULL);
    
    // Test 4: Large struct with nested async calls
    printf("\n=== Test 4: Large struct with nested async calls ===\n");
    LargeStruct struct4;
    init_large_struct(&struct4, 400);
    
    CGNThreadHandle handle4 = async_run(nested_large_struct_func, &struct4);
    uint64_t result4 = await(handle4);
    
    printf("Nested large struct result: %llu\n", result4);
    assert(result4 > 0);
    
    // Test 5: Very large struct (stress test)
    printf("\n=== Test 5: Very large struct stress test ===\n");
    
    // Create an even larger struct
    typedef struct {
        LargeStruct base;
        int extra_fields[100];
        double extra_doubles[50];
        char extra_strings[10][20];
    } VeryLargeStruct;
    
    VeryLargeStruct very_large;
    
    // Initialize base struct
    init_large_struct(&very_large.base, 500);
    
    // Initialize extra fields
    for (int i = 0; i < 100; i++) {
        very_large.extra_fields[i] = 500 + i;
    }
    for (int i = 0; i < 50; i++) {
        very_large.extra_doubles[i] = 500.0 + i + 0.5;
    }
    for (int i = 0; i < 10; i++) {
        snprintf(very_large.extra_strings[i], 20, "extra_%d_%d", 500, i);
    }
    
    CGNThreadHandle handle5 = async_run(process_very_large_struct, &very_large);
    uint64_t result5 = await(handle5);
    
    printf("Very large struct result: %llu\n", result5);
    assert(result5 > 0);
    
    // Test 6: Concurrent large struct processing
    printf("\n=== Test 6: Concurrent large struct processing ===\n");
    LargeStruct concurrent_structs[10];
    CGNThreadHandle concurrent_handles[10];
    
    for (int i = 0; i < 10; i++) {
        init_large_struct(&concurrent_structs[i], 600 + i * 5);
        concurrent_handles[i] = async_run(process_large_struct, &concurrent_structs[i]);
    }
    
    uint64_t concurrent_total = 0;
    for (int i = 0; i < 10; i++) {
        uint64_t result = await(concurrent_handles[i]);
        concurrent_total += result;
        assert(result > 0);
    }
    
    printf("Concurrent large struct processing total: %llu\n", concurrent_total);
    
    seagreen_free_rt();
    
    return 0;
}
