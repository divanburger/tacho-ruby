#ifndef TACHOMETER_H
#define TACHOMETER_H 1

#include "ruby.h"
#include <time.h>
#include <stdint.h>

typedef struct {
    uint32_t line_no;
    uint16_t method_name_length;
    uint16_t filename_offset;
    uint16_t filename_length;
} tch_call_body;
#define CALL_BODY_BYTES 10

typedef struct {
    VALUE    tracepoint;

    int      recording;
    FILE*    output;
    uint64_t start_time;
    int64_t  buffer_count;
    char     buffer[4096];
} tch_profile;

#endif /* TACHOMETER_H */
