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
} CallBody;
#define CALL_BODY_BYTES 10

#endif /* TACHOMETER_H */
