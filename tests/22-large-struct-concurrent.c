#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "seagreen.h"

typedef struct {
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
    
    int arr_int[10];
    float arr_float[5];
    char arr_char[20];
    
    struct {
        int nested_int;
        double nested_double;
        char nested_string[15];
    } nested;
    
    void *ptr;
    size_t size;
    
    uint32_t large_array[50];
    
    char string1[30];
    char string2[40];
    
    long long_val;
    unsigned long ulong_val;
    short short_val;
    unsigned short ushort_val;
    
    double arr_double[8];
    uint64_t arr_uint64[12];
    
    int final_int;
    double final_double;
    char final_char;
} LargeStruct;

void init_large_struct(LargeStruct *s, int base_value) {
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
    
    for (int i = 0; i < 10; i++) {
        s->arr_int[i] = base_value + i;
    }
    for (int i = 0; i < 5; i++) {
        s->arr_float[i] = (float)(base_value + i + 0.5);
    }
    for (int i = 0; i < 20; i++) {
        s->arr_char[i] = (char)('a' + (i % 26));
    }
    
    s->nested.nested_int = base_value + 100;
    s->nested.nested_double = (double)(base_value + 101.5);
    snprintf(s->nested.nested_string, sizeof(s->nested.nested_string), "nested_%d", base_value);
    
    s->ptr = (void *)(uintptr_t)(base_value + 200);
    s->size = (size_t)(base_value + 201);
    
    for (int i = 0; i < 50; i++) {
        s->large_array[i] = (uint32_t)(base_value + i);
    }
    
    snprintf(s->string1, sizeof(s->string1), "string1_%d", base_value);
    snprintf(s->string2, sizeof(s->string2), "string2_%d", base_value);
    
    s->long_val = (long)(base_value + 300);
    s->ulong_val = (unsigned long)(base_value + 301);
    s->short_val = (short)(base_value + 302);
    s->ushort_val = (unsigned short)(base_value + 303);
    
    for (int i = 0; i < 8; i++) {
        s->arr_double[i] = (double)(base_value + i + 0.25);
    }
    for (int i = 0; i < 12; i++) {
        s->arr_uint64[i] = (uint64_t)(base_value + i);
    }
    
    s->final_int = base_value + 400;
    s->final_double = (double)(base_value + 401.75);
    s->final_char = (char)('Z' - (base_value % 26));
}

async uint64_t process_large_struct(void *p) {
    LargeStruct *s = (LargeStruct *)p;
    
    printf("Processing large struct with base value %d\n", s->i32 - 3);
    
    async_yield();
    
    uint64_t sum = 0;
    sum += s->i8 + s->i16 + s->i32 + s->i64;
    sum += s->u8 + s->u16 + s->u32 + s->u64;
    sum += (uint64_t)s->f32 + (uint64_t)s->f64;
    sum += s->c + s->b;
    
    for (int i = 0; i < 10; i++) {
        sum += s->arr_int[i];
    }
    for (int i = 0; i < 5; i++) {
        sum += (uint64_t)s->arr_float[i];
    }
    
    sum += s->nested.nested_int + (uint64_t)s->nested.nested_double;
    
    for (int i = 0; i < 50; i++) {
        sum += s->large_array[i];
    }
    
    sum += s->long_val + s->ulong_val + s->short_val + s->ushort_val;
    sum += s->final_int + (uint64_t)s->final_double + s->final_char;
    
    printf("Large struct processed, sum: %llu\n", sum);
    
    return sum;
}

int main(void) {
    printf("Initializing runtime...\n");
    seagreen_init_rt();
    
    printf("Testing async functions with large structs...\n");
    
    printf("\n=== Test: Concurrent large struct processing ===\n");
    LargeStruct structs[10];
    CGNThreadHandle handles[10];
    
    for (int i = 0; i < 10; i++) {
        init_large_struct(&structs[i], 600 + i * 5);
        handles[i] = async_run(process_large_struct, &structs[i]);
    }
    
    uint64_t total_sum = 0;
    
    for (int i = 0; i < 10; i++) {
        uint64_t result = await(handles[i]);
        total_sum += result;
    }
    
    printf("Concurrent large struct processing total: %llu\n", total_sum);
    assert(total_sum > 0);
    
    seagreen_free_rt();
    
    return 0;
}
