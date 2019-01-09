#include "tacho.h"

#include <ruby/ruby.h>
#include <ruby/debug.h>

VALUE rb_mTacho;
VALUE rb_cTachoProfile;
ID    id_wall;
ID    id_process;
ID    id_thread;

static inline void write_header(tch_profile* profile, char type, uint64_t time) {
    uint64_t c = ((time - profile->start_time) << 8) | type;
    fwrite(&c, sizeof(uint64_t), 1, profile->output);
}

static inline void write_thread_switch_maybe(tch_profile* profile) {
    char c = 'T';
    tch_thread_switch thread_switch;
    VALUE thread = rb_thread_current();
    VALUE fiber = rb_fiber_current();

    thread_switch.thread_id = rb_obj_id(thread);
    thread_switch.fiber_id = rb_obj_id(fiber);

    if (thread_switch.thread_id != profile->last_thread_id || thread_switch.fiber_id != profile->last_fiber_id) {
        profile->last_thread_id = thread_switch.thread_id;
        profile->last_fiber_id = thread_switch.fiber_id;

        fwrite(&c, 1, 1, profile->output);
        fwrite(&thread_switch, sizeof(tch_thread_switch), 1, profile->output);
    }
}

static void tacho_event(VALUE tpval, void *data) {
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

    uint32_t return_body;
    tch_call_body call_body;

    rb_trace_arg_t* trace_arg = rb_tracearg_from_tracepoint(tpval);
    rb_event_flag_t event_flag = rb_tracearg_event_flag(trace_arg);

    clock_gettime(profile->clock_type, &time);
    itime = (long)time.tv_sec * 1000000000 + (long)time.tv_nsec;

    write_thread_switch_maybe(profile);

    if (event_flag == RUBY_EVENT_CALL) {
        write_header(profile, 'C', itime);
        call_body.line_no = FIX2INT(rb_tracearg_lineno(trace_arg));

        path = rb_tracearg_path(trace_arg);

        method_id = SYM2ID(rb_tracearg_method_id(trace_arg));
        call_body.method_id = method_id;

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

        return_body = SYM2ID(rb_tracearg_method_id(trace_arg));
        fwrite(&return_body, sizeof(uint32_t), 1, profile->output);
    }
}

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
	.wrap_struct_name = "tacho_profile",
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

static VALUE tch_profile_start(VALUE self, VALUE filename, VALUE name, VALUE clock_type) {
    tch_profile* profile = tch_profile_get_profile(self);

    struct timespec time;
    uint32_t name_length;
    ID clock_type_id;

    profile->output = fopen(StringValueCStr(filename), "wb");
    profile->buffer_count = 0;

    if (!profile->output) {
        rb_raise(rb_eRuntimeError, "Could not open file %s", StringValueCStr(filename));
        RB_GC_GUARD(filename);
        return Qnil;
    }
    RB_GC_GUARD(filename);

    profile->recording = 1;

    if (NIL_P(clock_type)) {
        profile->clock_type = CLOCK_MONOTONIC;
    } else if (TYPE(clock_type) == T_SYMBOL) {
        clock_type_id = SYM2ID(clock_type);
        if (id_wall == clock_type_id) {
            profile->clock_type = CLOCK_MONOTONIC;
        } else if (id_process == clock_type_id) {
            profile->clock_type = CLOCK_PROCESS_CPUTIME_ID;
        } else if (id_thread == clock_type_id) {
            profile->clock_type = CLOCK_THREAD_CPUTIME_ID;
        } else {
            rb_raise(rb_eTypeError, "not valid value. Must be one of :wall, :process or :thread got :%s", rb_id2name(clock_type_id));
        }
    } else {
        rb_raise(rb_eTypeError, "not valid value. Must be a symbol of one of :wall, :process or :thread");
    }

    clock_gettime(profile->clock_type, &time);
    profile->start_time = (long)time.tv_sec * 1000000000 + (long)time.tv_nsec;
    write_header(profile, 'S', profile->start_time);

    name_length = (uint32_t)RSTRING_LEN(name);
    fwrite(&name_length, sizeof(uint32_t), 1, profile->output);
    fwrite(RSTRING_PTR(name), name_length, 1, profile->output);
    RB_GC_GUARD(name);

    profile->tracepoint = rb_tracepoint_new(Qnil, RUBY_EVENT_CALL | RUBY_EVENT_RETURN, tacho_event, profile);
    rb_tracepoint_enable(profile->tracepoint);

    return Qnil;
}

static VALUE tch_profile_stop(VALUE self) {
    tch_profile* profile = tch_profile_get_profile(self);

    struct timespec time;
    uint64_t itime;

    if (profile->recording) {
        clock_gettime(profile->clock_type, &time);
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
Init_tacho(void) {
    rb_mTacho = rb_define_module("Tacho");

    rb_cTachoProfile = rb_define_class_under(rb_mTacho, "Profile", rb_cObject);
	rb_define_alloc_func(rb_cTachoProfile, tch_profile_allocate);
	rb_define_method(rb_cTachoProfile, "initialize", tch_profile_initialize, 0);
    rb_define_method(rb_cTachoProfile, "start", tch_profile_start, 3);
    rb_define_method(rb_cTachoProfile, "stop", tch_profile_stop, 0);

    id_wall = rb_intern("wall");
    id_process = rb_intern("process");
    id_thread = rb_intern("thread");
}
