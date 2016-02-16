
/*
 * Copyright (C) Andrey Zelenkov
 * Copyright (C) Nginx, Inc.
 */

#include <ngx_http.h>


static ngx_int_t ngx_add_quit_event(ngx_cycle_t *cycle);
static void ngx_http_init_connection_wrap(ngx_connection_t *c);
static void ngx_set_ngx_quit(void *data);


static ngx_core_module_t  ngx_regex_module_ctx = {
    ngx_string("quit"),
    NULL,
    NULL
};


ngx_module_t  ngx_quit_module = {
    NGX_MODULE_V1,
    &ngx_regex_module_ctx,         /* module context */
    NULL,                          /* module directives */
    NGX_CORE_MODULE,               /* module type */
    NULL,                          /* init master */
    NULL,                          /* init module */
    ngx_add_quit_event,            /* init process */
    NULL,                          /* init thread */
    NULL,                          /* exit thread */
    NULL,                          /* exit process */
    NULL,                          /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_int_t
ngx_add_quit_event(ngx_cycle_t *cycle)
{
    ngx_uint_t                    i;
    ngx_listening_t              *ls;

    ls = cycle->listening.elts;
    for (i = 0; i < cycle->listening.nelts; i++) {

        ls[i].handler = ngx_http_init_connection_wrap;

    }

   return NGX_OK;
}

void
ngx_http_init_connection_wrap(ngx_connection_t *c)
{
    ngx_pool_cleanup_t        *cln;

    cln = ngx_pool_cleanup_add(c->pool, 0);
    cln->handler = ngx_set_ngx_quit;

    ngx_http_init_connection(c);
}

static void
ngx_set_ngx_quit(void *data)
{
   ngx_quit = 1;
}
