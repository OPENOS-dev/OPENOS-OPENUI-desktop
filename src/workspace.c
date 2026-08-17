#define _POSIX_C_SOURCE 200809L
/* OPENOS 工作区协议合成器端实现 (见 workspace.h)
 * 为每个绑定客户端的每个工作区创建 handle resource, 发送 name/state/done;
 * 处理 activate 请求并回调合成器切换工作区。
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <wayland-server-core.h>
#include "workspace.h"
#include "openos-workspace-unstable-v1-server-protocol.h"

struct openos_ws_client {
    struct wl_resource *resource;
    struct wl_list link;     /* manager.clients */
    struct wl_list handles;  /* openos_ws_handle.link */
};

struct openos_ws_handle {
    struct wl_resource *resource;
    struct wl_list link;     /* client.handles */
    struct openos_ws_client *client;
    struct openos_workspace_manager *mgr;
    int index;
};

struct openos_workspace_manager {
    struct wl_display *display;
    struct wl_global *global;
    struct wl_list clients;  /* openos_ws_client.link */
    int count;
    int active;
    void (*on_activate)(void *data, int index);
    void *data;
    struct wl_listener display_destroy;
};

static void send_handle(struct openos_ws_handle *h, int active) {
    char name[16];
    snprintf(name, sizeof name, "%d", h->index + 1);
    openos_workspace_handle_v1_send_name(h->resource, name);
    struct wl_array states;
    wl_array_init(&states);
    if (h->index == active) {
        uint32_t *s = wl_array_add(&states, sizeof(uint32_t));
        if (s) *s = OPENOS_WORKSPACE_HANDLE_V1_STATE_ACTIVE;
    }
    openos_workspace_handle_v1_send_state(h->resource, &states);
    wl_array_release(&states);
    openos_workspace_handle_v1_send_done(h->resource);
}

static void send_all(struct openos_workspace_manager *mgr) {
    struct openos_ws_client *c;
    wl_list_for_each(c, &mgr->clients, link) {
        struct openos_ws_handle *h;
        wl_list_for_each(h, &c->handles, link)
            send_handle(h, mgr->active);
    }
}

/* ---- 请求处理 ---- */
static void handle_activate(struct wl_client *client, struct wl_resource *resource) {
    (void)client;
    struct openos_ws_handle *h = wl_resource_get_user_data(resource);
    if (!h || !h->mgr) return;
    struct openos_workspace_manager *mgr = h->mgr;
    if (mgr->on_activate)
        mgr->on_activate(mgr->data, h->index);
}

static void handle_destroy(struct wl_client *client, struct wl_resource *resource) {
    (void)client;
    wl_resource_destroy(resource);
}

static const struct openos_workspace_handle_v1_interface handle_impl = {
    .activate = handle_activate,
    .destroy = handle_destroy,
};

static void manager_stop(struct wl_client *client, struct wl_resource *resource) {
    (void)client;
    wl_resource_destroy(resource);
}

static const struct openos_workspace_manager_v1_interface manager_impl = {
    .stop = manager_stop,
};

/* ---- resource 销毁清理 ---- */
static void handle_resource_destroy(struct wl_resource *resource) {
    struct openos_ws_handle *h = wl_resource_get_user_data(resource);
    if (!h) return;
    wl_list_remove(&h->link);
    free(h);
}

static void client_resource_destroy(struct wl_resource *resource) {
    struct openos_ws_client *c = wl_resource_get_user_data(resource);
    if (!c) return;
    struct openos_ws_handle *h, *tmp;
    wl_list_for_each_safe(h, tmp, &c->handles, link) {
        wl_resource_set_user_data(h->resource, NULL);
        wl_list_remove(&h->link);
        free(h);
    }
    wl_list_remove(&c->link);
    free(c);
}

static void manager_bind(struct wl_client *client, void *data,
                         uint32_t version, uint32_t id) {
    struct openos_workspace_manager *mgr = data;
    struct openos_ws_client *c = calloc(1, sizeof *c);
    if (!c) {
        wl_client_post_no_memory(client);
        return;
    }
    c->resource = wl_resource_create(client,
        &openos_workspace_manager_v1_interface, version, id);
    wl_list_init(&c->handles);
    wl_resource_set_implementation(c->resource, &manager_impl, c,
                                   client_resource_destroy);
    wl_list_insert(&mgr->clients, &c->link);

    for (int i = 0; i < mgr->count; i++) {
        struct openos_ws_handle *h = calloc(1, sizeof *h);
        if (!h) {
            wl_client_post_no_memory(client);
            break;
        }
        h->index = i;
        h->client = c;
        h->mgr = mgr;
        h->resource = wl_resource_create(client,
            &openos_workspace_handle_v1_interface, 1, 0);
        wl_resource_set_implementation(h->resource, &handle_impl, h,
                                       handle_resource_destroy);
        wl_list_insert(&c->handles, &h->link);
        openos_workspace_manager_v1_send_workspace(c->resource, h->resource);
    }
    send_all(mgr);
}

static void display_destroy_handler(struct wl_listener *l, void *data) {
    (void)data;
    struct openos_workspace_manager *mgr =
        wl_container_of(l, mgr, display_destroy);
    openos_workspace_manager_destroy(mgr);
}

struct openos_workspace_manager *openos_workspace_manager_create(
        struct wl_display *display, int count, int active,
        void (*on_activate)(void *data, int index), void *data) {
    struct openos_workspace_manager *mgr = calloc(1, sizeof *mgr);
    if (!mgr) return NULL;
    mgr->display = display;
    mgr->count = count;
    mgr->active = active;
    mgr->on_activate = on_activate;
    mgr->data = data;
    wl_list_init(&mgr->clients);
    mgr->global = wl_global_create(display,
        &openos_workspace_manager_v1_interface, 1, mgr, manager_bind);
    if (!mgr->global) {
        free(mgr);
        return NULL;
    }
    mgr->display_destroy.notify = display_destroy_handler;
    wl_display_add_destroy_listener(display, &mgr->display_destroy);
    return mgr;
}

void openos_workspace_set_active(struct openos_workspace_manager *mgr, int index) {
    if (!mgr || index < 0 || index >= mgr->count) return;
    if (index == mgr->active) return;
    mgr->active = index;
    send_all(mgr);
}

void openos_workspace_manager_destroy(struct openos_workspace_manager *mgr) {
    if (!mgr) return;
    wl_list_remove(&mgr->display_destroy.link);
    if (mgr->global)
        wl_global_destroy(mgr->global);
    struct openos_ws_client *c, *tmp;
    wl_list_for_each_safe(c, tmp, &mgr->clients, link)
        client_resource_destroy(c->resource);
    free(mgr);
}
