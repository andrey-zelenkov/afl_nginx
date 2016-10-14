
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) NGINX, Inc.
 */

#include <nxt_types.h>
#include <nxt_clang.h>
#include <nxt_string.h>
#include <nxt_stub.h>
#include <nxt_malloc.h>
#include <nxt_array.h>
#include <nxt_lvlhsh.h>
#include <nxt_mem_cache_pool.h>
#include <njscript.h>
#include <string.h>
#include <stdio.h>
#include <sys/resource.h>
#include <time.h>


typedef struct {
    nxt_str_t  script;
    nxt_str_t  ret;
} njs_unit_test_t;


typedef struct {
    nxt_mem_cache_pool_t  *mem_cache_pool;
    nxt_str_t             uri;
} njs_unit_test_req;


static njs_ret_t
njs_unit_test_r_get_uri_external(njs_vm_t *vm, njs_value_t *value, void *obj,
    uintptr_t data)
{
    njs_unit_test_req  *r;

    r = (njs_unit_test_req *) obj;

    return njs_string_create(vm, value, r->uri.start, r->uri.length, 0);
}


static njs_ret_t
njs_unit_test_r_set_uri_external(njs_vm_t *vm, void *obj, uintptr_t data,
    nxt_str_t *value)
{
    njs_unit_test_req  *r;

    r = (njs_unit_test_req *) obj;
    r->uri = *value;

    return NXT_OK;
}


static njs_ret_t
njs_unit_test_host_external(njs_vm_t *vm, njs_value_t *value, void *obj,
    uintptr_t data)
{
    return njs_string_create(vm, value, (u_char *) "АБВГДЕЁЖЗИЙ", 22, 0);
}


static njs_ret_t
njs_unit_test_header_external(njs_vm_t *vm, njs_value_t *value, void *obj,
    uintptr_t data)
{
    u_char             *s, *p;
    uint32_t           size;
    nxt_str_t          *h;
    njs_unit_test_req  *r;

    r = (njs_unit_test_req *) obj;
    h = (nxt_str_t *) data;

    size = 7 + h->length;

    s = nxt_mem_cache_alloc(r->mem_cache_pool, size);
    if (nxt_slow_path(s == NULL)) {
        return NXT_ERROR;
    }

    p = memcpy(s, h->start, h->length);
    p += h->length;
    *p++ = '|';
    memcpy(p, "АБВ", 6);

    return njs_string_create(vm, value, s, size, 0);
}


static njs_ret_t
njs_unit_test_header_foreach_external(njs_vm_t *vm, void *obj, void *next)
{
    u_char  *s;

    s = next;
    s[0] = '0';
    s[1] = '0';

    return NXT_OK;
}


static njs_ret_t
njs_unit_test_header_next_external(njs_vm_t *vm, njs_value_t *value, void *obj,
    void *next)
{
    u_char  *s;

    s = next;
    s[1]++;

    if (s[1] == '4') {
        return NXT_DONE;
    }

    return njs_string_create(vm, value, s, 2, 0);
}


static njs_ret_t
njs_unit_test_method_external(njs_vm_t *vm, njs_value_t *args, nxt_uint_t nargs,
    njs_index_t unused)
{
    nxt_int_t          ret;
    nxt_str_t          s;
    uintptr_t          next;
    njs_unit_test_req  *r;

    next = 0;

    if (nargs > 1) {

        ret = njs_value_string_copy(vm, &s, njs_argument(args, 1), &next);

        if (ret == NXT_OK && s.length == 3 && memcmp(s.start, "YES", 3) == 0) {
            r = njs_value_data(njs_argument(args, 0));
            njs_vm_return_string(vm, r->uri.start, r->uri.length);

            return NXT_OK;
        }
    }

    return NXT_ERROR;
}


static njs_external_t  njs_unit_test_r_external[] = {

    { nxt_string("uri"),
      NJS_EXTERN_PROPERTY,
      NULL,
      0,
      njs_unit_test_r_get_uri_external,
      njs_unit_test_r_set_uri_external,
      NULL,
      NULL,
      NULL,
      NULL,
      0 },

    { nxt_string("host"),
      NJS_EXTERN_PROPERTY,
      NULL,
      0,
      njs_unit_test_host_external,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      0 },

    { nxt_string("header"),
      NJS_EXTERN_OBJECT,
      NULL,
      0,
      njs_unit_test_header_external,
      NULL,
      NULL,
      njs_unit_test_header_foreach_external,
      njs_unit_test_header_next_external,
      NULL,
      0 },

    { nxt_string("some_method"),
      NJS_EXTERN_METHOD,
      NULL,
      0,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      njs_unit_test_method_external,
      0 },

};


static njs_external_t  nxt_test_external[] = {

    { nxt_string("$r"),
      NJS_EXTERN_OBJECT,
      njs_unit_test_r_external,
      nxt_nitems(njs_unit_test_r_external),
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      0 },

};


static nxt_int_t
njs_unit_test_externals(nxt_lvlhsh_t *externals, nxt_mem_cache_pool_t *mcp)
{
    nxt_lvlhsh_init(externals);

    return njs_vm_external_add(externals, mcp, 0, nxt_test_external,
                               nxt_nitems(nxt_test_external));
}


static void *
njs_alloc(void *mem, size_t size)
{
    return nxt_malloc(size);
}


static void *
njs_zalloc(void *mem, size_t size)
{
    void  *p;

    p = nxt_malloc(size);

    if (p != NULL) {
        memset(p, 0, size);
    }

    return p;
}


static void *
njs_align(void *mem, size_t alignment, size_t size)
{
    return nxt_memalign(alignment, size);
}


static void
njs_free(void *mem, void *p)
{
    nxt_free(p);
}


static const nxt_mem_proto_t  njs_mem_cache_pool_proto = {
    njs_alloc,
    njs_zalloc,
    njs_align,
    NULL,
    njs_free,
    NULL,
    NULL,
};


#define BUF_LIMIT   32 * 1024

int nxt_cdecl
main(int argc, char **argv)
{
    void                  *ext_object;
    u_char                buf[BUF_LIMIT];
    size_t                off;
    ssize_t               n;
    njs_vm_t              *vm, *nvm;
    nxt_int_t             ret;
    nxt_str_t             s, r_name;
    nxt_bool_t            disassemble;
    nxt_lvlhsh_t          externals;
    njs_function_t        *function;
    njs_vm_shared_t       *shared;
    njs_unit_test_req     r;
    njs_opaque_value_t    value;
    nxt_mem_cache_pool_t  *mcp;

    disassemble = 0;

    if (argc > 1) {
        disassemble = 1;
    }

    /*
     * Chatham Islands NZ-CHAT time zone.
     * Standard time: UTC+12:45, Daylight Saving time: UTC+13:45.
     */
    (void) putenv((char *) "TZ=Pacific/Chatham");
    tzset();

    shared = NULL;

    mcp = nxt_mem_cache_pool_create(&njs_mem_cache_pool_proto, NULL, NULL,
                                    2 * nxt_pagesize(), 128, 512, 16);
    if (nxt_slow_path(mcp == NULL)) {
        return NXT_ERROR;
    }

    r.mem_cache_pool = mcp;
    r.uri.length = 6;
    r.uri.start = (u_char *) "АБВ";

    ext_object = &r;

    if (njs_unit_test_externals(&externals, mcp) != NXT_OK) {
        return NXT_ERROR;
    }

    n = 0;
    off = 0;
    do {
        n = read(0, &buf[off], BUF_LIMIT - off);

        if (n > 0) {
            off +=n;
        }

    } while (n > 0);

    if (n < 0) {
        return NXT_ERROR;
    }

    vm = njs_vm_create(mcp, &shared, &externals);
    if (vm == NULL) {
        return NXT_ERROR;
    }

    u_char *start = &buf[0];

    ret = njs_vm_compile(vm, &start, start + off, &function);

    if (ret == NXT_OK) {
        if (disassemble) {
            njs_disassembler(vm);
        }

        nvm = njs_vm_clone(vm, NULL, &ext_object);
        if (nvm == NULL) {
            return NXT_ERROR;
        }

        r.uri.length = 6;
        r.uri.start = (u_char *) "АБВ";

        if (function != NULL) {
            r_name.length = 2;
            r_name.start = (u_char *) "$r";

            ret = njs_vm_external(nvm, NULL, &r_name, &value);
            if (ret != NXT_OK) {
                return NXT_ERROR;
            }

            ret = njs_vm_call(nvm, function, &value, 1);

        } else {
            ret = njs_vm_run(nvm);
        }

        if (ret == NXT_OK) {
            if (njs_vm_retval(nvm, &s) != NXT_OK) {
                return NXT_ERROR;
            }

        } else {
            njs_vm_exception(nvm, &s);
        }

    } else {
        njs_vm_exception(vm, &s);
        nvm = NULL;
    }

    if (nvm != NULL) {
        njs_vm_destroy(nvm);
    }

    nxt_mem_cache_pool_destroy(mcp);

    return NXT_OK;
}
