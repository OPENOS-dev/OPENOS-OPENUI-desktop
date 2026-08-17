#ifndef OPENOS_WORKSPACE_H
#define OPENOS_WORKSPACE_H

#include <wayland-server-core.h>

/* OPENOS 自研工作区协议 (openos-workspace-unstable-v1) 合成器端实现。
 * 让面板等客户端列出工作区并请求切换 (工作区切换器 UI)。
 * 协议 XML: protocols/openos-workspace-unstable-v1.xml
 */

struct openos_workspace_manager;

/* 创建 manager。count=工作区数量, active=当前激活索引,
 * on_activate 在收到客户端激活请求时回调 (data 由调用方传入)。 */
struct openos_workspace_manager *openos_workspace_manager_create(
    struct wl_display *display, int count, int active,
    void (*on_activate)(void *data, int index), void *data);

/* 更新激活工作区并向所有客户端广播状态 */
void openos_workspace_set_active(struct openos_workspace_manager *mgr, int index);

void openos_workspace_manager_destroy(struct openos_workspace_manager *mgr);

#endif /* OPENOS_WORKSPACE_H */
