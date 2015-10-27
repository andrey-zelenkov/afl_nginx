
/*
 * Copyright (C) Andrey Zelenkov
 * Copyright (C) Nginx, Inc.
 */

#include <ngx_http.h>


static ngx_int_t ngx_conf_test_add_quit_event(ngx_cycle_t *cycle);
static void ngx_set_ngx_quit();


static ngx_http_module_t  ngx_conf_test_module_ctx = {
    NULL,                          /* preconfiguration */
    NULL,                          /* postconfiguration */

    NULL,                          /* create main configuration */
    NULL,                          /* init main configuration */

    NULL,                          /* create server configuration */
    NULL,                          /* merge server configuration */

    NULL,                          /* create location configuration */
    NULL                           /* merge location configuration */
};


ngx_module_t  ngx_conf_test_module = {
    NGX_MODULE_V1,
    &ngx_conf_test_module_ctx,     /* module context */
    NULL,                          /* module directives */
    NGX_HTTP_MODULE,               /* module type */
    NULL,                          /* init master */
    NULL,                          /* init module */
    ngx_conf_test_add_quit_event,  /* init process */
    NULL,                          /* init thread */
    NULL,                          /* exit thread */
    NULL,                          /* exit process */
    NULL,                          /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_int_t
ngx_conf_test_add_quit_event(ngx_cycle_t *cycle)
{
   static ngx_event_t           ev;
   static ngx_connection_t      dumb;

   ev.handler = ngx_set_ngx_quit;
   ev.log = cycle->log;
   ev.data = &dumb;
   dumb.fd = (ngx_socket_t) -1;

   ngx_add_timer(&ev, 1);

   return NGX_OK;
}


static void
ngx_set_ngx_quit()
{
   ngx_quit = 1;
}
