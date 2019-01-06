#include "tachometer.h"

#include <ruby/ruby.h>
#include <ruby/debug.h>

FILE* output;
uint64_t start_time;
int64_t  buffer_count;
char buffer[4096];

VALUE tracepoint;

inline void write_header(char type, uint64_t time) {
    uint64_t c = ((time - start_time) << 8) | type;
    fwrite(&c, sizeof(uint64_t), 1, output);
}

static void tachometer_event(VALUE tpval, void *data) {
    struct timespec time;
    uint64_t itime;

    VALUE path;
    VALUE method_id;
    const char* method_name;
    int method_length;

    int   file_len;
    char* file_str;
    int   cur_len;
    char* cur_str;
    int i;

    CallBody call_body;

    rb_trace_arg_t* trace_arg = rb_tracearg_from_tracepoint(tpval);
    rb_event_flag_t event_flag = rb_tracearg_event_flag(trace_arg);

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time);
    itime = (long)time.tv_sec * 1000000000 + (long)time.tv_nsec;

    if (event_flag == RUBY_EVENT_CALL) {
        write_header('C', itime);
        call_body.line_no = FIX2INT(rb_tracearg_lineno(trace_arg));

        path = rb_tracearg_path(trace_arg);
        method_id = SYM2ID(rb_tracearg_method_id(trace_arg));

        method_name = rb_id2name(method_id);
        method_length = strlen(method_name);

        cur_len = file_len = (int)RSTRING_LEN(path);
        cur_str = file_str = RSTRING_PTR(path);

        for (i = 0; i < file_len && i < buffer_count; i++) {
            if (*cur_str != buffer[i]) break;
            cur_len--;
            cur_str++;
        }

        buffer_count = file_len;
        memcpy(buffer, file_str, file_len);

        call_body.method_name_length = method_length;
        call_body.filename_offset = file_len - cur_len;
        call_body.filename_length = cur_len;
        fwrite(&call_body, CALL_BODY_BYTES, 1, output);
        fwrite(method_name, call_body.method_name_length, 1, output);
        fwrite(cur_str, call_body.filename_length, 1, output);
    } else if (event_flag == RUBY_EVENT_RETURN) {
        write_header('R', itime);
    }
}


static VALUE tachometer_start_record(VALUE self, VALUE filename, VALUE name) {
    struct timespec time;

    output = fopen(StringValueCStr(filename), "wb");
    buffer_count = 0;

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time);
    start_time = (long)time.tv_sec * 1000000000 + (long)time.tv_nsec;

    tracepoint = rb_tracepoint_new(Qnil, RUBY_EVENT_CALL | RUBY_EVENT_RETURN, tachometer_event, NULL);
    rb_tracepoint_enable(tracepoint);

    RB_GC_GUARD(filename);
    RB_GC_GUARD(name);
    return Qnil;
}

static VALUE tachometer_stop_record(VALUE self) {
    struct timespec time;
    uint64_t itime;

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time);
    itime = (long)time.tv_sec * 1000000000 + (long)time.tv_nsec;
    write_header('F', itime);

    fclose(output);

    rb_tracepoint_disable(tracepoint);

    return Qnil;
}

VALUE rb_mTachometer;

void
Init_tachometer(void)
{
    rb_mTachometer = rb_define_module("Tachometer");
    rb_define_module_function(rb_mTachometer, "start_record", tachometer_start_record, 2);
    rb_define_module_function(rb_mTachometer, "stop_record", tachometer_stop_record, 0);
}
