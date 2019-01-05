#include "tachometer.h"

FILE* output;
long long event_count;

int  buffer_count;
char buffer[4096];

static VALUE tachometer_record(VALUE self, VALUE event_type, VALUE filename, VALUE line_no, VALUE name) {
    struct timespec time;
    char type;

    int   file_len;
    char* file_str;
    int   cur_len;
    char* cur_str;

    int i;

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time);

    if (RSTRING_LEN(name) >= 0) {
        type = RSTRING_PTR(event_type)[0];
        if (type == 'c') {
            fprintf(output, "C%li.%09li ", (long)time.tv_sec, (long)time.tv_nsec);

            cur_len = file_len = (int)RSTRING_LEN(filename);
            cur_str = file_str = RSTRING_PTR(filename);

            for (i = 0; i < file_len && i < buffer_count; i++) {
                if (*cur_str != buffer[i]) break;
                cur_len--;
                cur_str++;
            }

            buffer_count = file_len;
            memcpy(buffer, file_str, file_len);

            if (file_str == cur_str) {
                fprintf(output, "$%.*s:%li %.*s\n", file_len, file_str, NUM2LONG(line_no),
                    (int)RSTRING_LEN(name), RSTRING_PTR(name));
            } else {
                fprintf(output, "%li@%.*s:%li %.*s\n", (cur_str - file_str), cur_len, cur_str, NUM2LONG(line_no),
                    (int)RSTRING_LEN(name), RSTRING_PTR(name));
            }
        } else if (type == 'r') {
            fprintf(output, "R%li.%09li\n", (long)time.tv_sec, (long)time.tv_nsec);
        }
        event_count++;
    }

    RB_GC_GUARD(event_type);
    RB_GC_GUARD(filename);
    RB_GC_GUARD(name);
    return Qnil;
}

static VALUE tachometer_start_record(VALUE self, VALUE filename, VALUE name) {
    event_count = 0;
    buffer_count = 0;

    output = fopen(StringValueCStr(filename), "wb");
    fprintf(output, "# %.*s\n", (int)RSTRING_LEN(name), RSTRING_PTR(name));

    RB_GC_GUARD(filename);
    RB_GC_GUARD(name);
    return Qnil;
}

static VALUE tachometer_stop_record(VALUE self) {
    fprintf(output, "F%lli\n", event_count);
    fclose(output);
    return Qnil;
}

VALUE rb_mTachometer;

void
Init_tachometer(void)
{
    rb_mTachometer = rb_define_module("Tachometer");
    rb_define_module_function(rb_mTachometer, "record", tachometer_record, 4);
    rb_define_module_function(rb_mTachometer, "start_record", tachometer_start_record, 2);
    rb_define_module_function(rb_mTachometer, "stop_record", tachometer_stop_record, 0);
}
