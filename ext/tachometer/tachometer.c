#include "tachometer.h"

#include <ruby/ruby.h>
#include <ruby/debug.h>

static inline void write_header(tch_profile* profile, char type, uint64_t time) {
    uint64_t c = ((time - profile->start_time) << 8) | type;
    fwrite(&c, sizeof(uint64_t), 1, profile->output);
}

static void tachometer_event(VALUE tpval, void *data) {
    tch_profile* profile = (tch_profile*)data;

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

    tch_call_body call_body;

    rb_trace_arg_t* trace_arg;
    rb_event_flag_t event_flag;

    trace_arg = rb_tracearg_from_tracepoint(tpval);
    event_flag = rb_tracearg_event_flag(trace_arg);

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time);
    itime = (long)time.tv_sec * 1000000000 + (long)time.tv_nsec;

    if (event_flag == RUBY_EVENT_CALL) {
        write_header(profile, 'C', itime);
        call_body.line_no = FIX2INT(rb_tracearg_lineno(trace_arg));

        path = rb_tracearg_path(trace_arg);

        method_id = SYM2ID(rb_tracearg_method_id(trace_arg));

        method_name = rb_id2name(method_id);
        method_length = strlen(method_name);

        cur_len = file_len = (int)RSTRING_LEN(path);
        cur_str = file_str = RSTRING_PTR(path);

        for (i = 0; i < file_len && i < profile->buffer_count; i++) {
            if (*cur_str != profile->buffer[i]) break;
            cur_len--;
            cur_str++;
        }

        profile->buffer_count = file_len;
        memcpy(profile->buffer, file_str, file_len);

        call_body.method_name_length = method_length;
        call_body.filename_offset = file_len - cur_len;
        call_body.filename_length = cur_len;
        fwrite(&call_body, CALL_BODY_BYTES, 1, profile->output);
        fwrite(method_name, call_body.method_name_length, 1, profile->output);
        fwrite(cur_str, call_body.filename_length, 1, profile->output);
    } else if (event_flag == RUBY_EVENT_RETURN) {
        write_header(profile, 'R', itime);
    }
}

VALUE rb_mTachometer;
VALUE rb_cTachometerProfile;

static tch_profile* tch_profile_get_profile(VALUE self) {
    return (tch_profile*)RDATA(self)->data;
}

static size_t tch_profile_size(const void* data) {
	return sizeof(tch_profile);
}

static void tch_profile_mark(void* data)
{
	rb_gc_mark(((tch_profile*)data)->tracepoint);
}

static const rb_data_type_t tch_profile_type = {
	.wrap_struct_name = "tachometer_profile",
	.function = {
		.dfree = RUBY_DEFAULT_FREE,
		.dsize = tch_profile_size,
		.dmark = tch_profile_mark,
	},
	.flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static VALUE tch_profile_allocate(VALUE self) {
    tch_profile* profile;
    return TypedData_Make_Struct(self, tch_profile, &tch_profile_type, profile);
}

static VALUE tch_profile_initialize(VALUE self) {
    tch_profile* profile = tch_profile_get_profile(self);
    profile->recording = 0;
    return self;
}

static VALUE tch_profile_start(VALUE self, VALUE filename, VALUE name) {
    tch_profile* profile = tch_profile_get_profile(self);

    struct timespec time;
    uint32_t name_length;

    profile->output = fopen(StringValueCStr(filename), "wb");
    profile->buffer_count = 0;

    if (!profile->output) {
        rb_raise(rb_eRuntimeError, "Could not open file %s", StringValueCStr(filename));
        RB_GC_GUARD(filename);
        return Qnil;
    }
    RB_GC_GUARD(filename);

    profile->recording = 1;

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time);
    profile->start_time = (long)time.tv_sec * 1000000000 + (long)time.tv_nsec;
    write_header(profile, 'S', profile->start_time);

    name_length = (uint32_t)RSTRING_LEN(name);
    fwrite(&name_length, sizeof(uint32_t), 1, profile->output);
    fwrite(RSTRING_PTR(name), name_length, 1, profile->output);
    RB_GC_GUARD(name);

    profile->tracepoint = rb_tracepoint_new(Qnil, RUBY_EVENT_CALL | RUBY_EVENT_RETURN, tachometer_event, profile);
    rb_tracepoint_enable(profile->tracepoint);

    return Qnil;
}

static VALUE tch_profile_stop(VALUE self) {
    tch_profile* profile = tch_profile_get_profile(self);

    struct timespec time;
    uint64_t itime;

    if (profile->recording) {
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time);
        itime = (long)time.tv_sec * 1000000000 + (long)time.tv_nsec;
        write_header(profile, 'F', itime);

        fflush(profile->output);
        fclose(profile->output);

        rb_tracepoint_disable(profile->tracepoint);

        profile->recording = 0;
    } else {
        rb_raise(rb_eRuntimeError, "Not currently recording");
    }

    return Qnil;
}

void
Init_tachometer(void) {
    rb_mTachometer = rb_define_module("Tachometer");

    rb_cTachometerProfile = rb_define_class_under(rb_mTachometer, "Profile", rb_cObject);
	rb_define_alloc_func(rb_cTachometerProfile, tch_profile_allocate);
	rb_define_method(rb_cTachometerProfile, "initialize", tch_profile_initialize, 0);
    rb_define_method(rb_cTachometerProfile, "start", tch_profile_start, 2);
    rb_define_method(rb_cTachometerProfile, "stop", tch_profile_stop, 0);
}
