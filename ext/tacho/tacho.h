#ifndef TACHOMETER_H
#define TACHOMETER_H 1

#include "ruby.h"
#include <time.h>
#include <stdint.h>

typedef struct {
    uint32_t line_no;
    uint32_t method_id;
    uint16_t method_name_length;
    uint16_t filename_offset;
    uint16_t filename_length;
} tch_call_body;
#define CALL_BODY_BYTES 14

typedef struct {
    uint32_t thread_id;
    uint32_t fiber_id;
} tch_thread_switch;
#define THREAD_SWITCH_BYTES 8

typedef struct {
    VALUE     tracepoint;
    clockid_t clock_type;
    int       recording;
    FILE*     output;
    uint64_t  start_time;
    int64_t   buffer_count;
    char      buffer[4096];

    uint32_t  last_thread_id;
    uint32_t  last_fiber_id;
} tch_profile;

#endif /* TACHOMETER_H */
