/* OPENOS 桌面环境 — 窗口管理器 (基础重父化 WM)
 * 功能: 接管客户端窗口、加标题栏、拖拽移动、关闭按钮。
 */
#include "de.h"

typedef struct {
    Window client;
    Window frame;
    int x, y, w, h;
} Client;

#define MAX_CLIENTS 256
static Client clients[MAX_CLIENTS];
static int nclients = 0;
static Window dragging = 0;
static int drag_off_x, drag_off_y;
static Window active_client = 0;

static Client *find_client(Window w) {
    for (int i = 0; i < nclients; i++)
        if (clients[i].client == w || clients[i].frame == w)
            return &clients[i];
    return NULL;
}

void de_init_atoms(void) {
    a_wm_delete       = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    a_net_supported   = XInternAtom(dpy, "_NET_SUPPORTED", False);
    a_net_wm_check    = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
    a_net_active      = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
    a_net_client_list = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
}

void wm_start(void) {
    /* 壁纸: 纯色 */
    XSetWindowBackground(dpy, root, BlackPixel(dpy, screen));
    XClearWindow(dpy, root);

    /* 夺取 WM 角色 */
    XSelectInput(dpy, root,
                 SubstructureRedirectMask | SubstructureNotifyMask | ButtonPressMask);

    /* EWMH: 声明本 WM 支持的基本属性 */
    Window check = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
    XChangeProperty(dpy, root, a_net_wm_check, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&check, 1);
    Atom supported[] = { a_net_wm_check, a_net_active, a_net_client_list };
    XChangeProperty(dpy, root, a_net_supported, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)supported, 3);
}

void wm_handle_map_request(XMapRequestEvent *e) {
    XWindowAttributes wa;
    if (XGetWindowAttributes(dpy, e->window, &wa) && wa.override_redirect)
        return;                     /* 弹出菜单等跳过 */
    if (find_client(e->window))
        return;
    if (nclients >= MAX_CLIENTS)
        return;

    XWindowAttributes a;
    XGetWindowAttributes(dpy, e->window, &a);
    int x = a.x, y = a.y, w = a.width, h = a.height;
    if (w <= 0) w = 480;
    if (h <= 0) h = 320;

    Window frame = XCreateSimpleWindow(dpy, root, x, y, w, h + TITLE_HEIGHT,
                                        1, BlackPixel(dpy, screen),
                                        WhitePixel(dpy, screen));
    XSelectInput(dpy, frame,
                 SubstructureRedirectMask | SubstructureNotifyMask |
                 ButtonPressMask | ExposureMask);
    XReparentWindow(dpy, e->window, frame, 0, TITLE_HEIGHT);
    XAddToSaveSet(dpy, e->window);
    XMapWindow(dpy, frame);
    XMapWindow(dpy, e->window);
    XSetWMProtocols(dpy, e->window, &a_wm_delete, 1);

    Client *c = &clients[nclients++];
    c->client = e->window;
    c->frame = frame;
    c->x = x; c->y = y; c->w = w; c->h = h;

    if (c->y < panel_height) {       /* 避免被顶栏遮挡 */
        c->y = panel_height;
        XMoveWindow(dpy, frame, c->x, c->y);
    }
    active_client = c->client;
    wm_draw_frame(frame);
}

void wm_draw_frame(Window frame) {
    Client *c = find_client(frame);
    if (!c) return;
    GC gc = XDefaultGC(dpy, screen);

    XClearArea(dpy, frame, 0, 0, c->w, TITLE_HEIGHT, False);
    XSetForeground(dpy, gc, BlackPixel(dpy, screen));
    XFillRectangle(dpy, frame, gc, 0, 0, c->w, TITLE_HEIGHT);

    char name[256] = "OPENOS";
    XFetchName(dpy, c->client, name);
    XSetForeground(dpy, gc, WhitePixel(dpy, screen));
    XDrawString(dpy, frame, gc, 6, 16, name, strlen(name));

    /* 关闭按钮 (右上角方框 + 叉) */
    int bx = c->w - 18;
    XDrawRectangle(dpy, frame, gc, bx, 4, 14, 14);
    XDrawLine(dpy, frame, gc, bx + 3, 7, bx + 13, 17);
    XDrawLine(dpy, frame, gc, bx + 3, 17, bx + 13, 7);
}

void wm_handle_button(XButtonEvent *e) {
    Client *c = find_client(e->window);
    if (!c) return;
    if (e->window == c->frame && e->y < TITLE_HEIGHT) {
        if (e->x > c->w - 20) {                 /* 关闭 */
            XClientMessageEvent cm;
            memset(&cm, 0, sizeof cm);
            cm.type = ClientMessage;
            cm.window = c->client;
            cm.message_type = a_wm_delete;
            cm.format = 32;
            cm.data.l[0] = a_wm_delete;
            XSendEvent(dpy, c->client, False, NoEventMask, (XEvent *)&cm);
            XSync(dpy, False);
        } else {                                /* 开始拖拽 */
            dragging = c->frame;
            drag_off_x = e->x;
            drag_off_y = e->y;
            XGrabPointer(dpy, c->frame, False,
                         PointerMotionMask | ButtonReleaseMask,
                         GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
        }
    }
}

void wm_handle_motion(XMotionEvent *e) {
    if (!dragging) return;
    Client *c = find_client(dragging);
    if (!c) return;
    c->x = e->x_root - drag_off_x;
    c->y = e->y_root - drag_off_y;
    if (c->y < panel_height) c->y = panel_height;
    XMoveWindow(dpy, c->frame, c->x, c->y);
}

void wm_handle_release(XButtonEvent *e) {
    (void)e;
    if (dragging) {
        XUngrabPointer(dpy, CurrentTime);
        dragging = 0;
    }
}

void wm_handle_configure_request(XConfigureRequestEvent *e) {
    Client *c = find_client(e->window);
    if (!c) {                            /* 未托管窗口: 直接放行 */
        XWindowChanges wc;
        wc.x = e->x; wc.y = e->y; wc.width = e->width;
        wc.height = e->height; wc.border_width = e->border_width;
        wc.sibling = e->above; wc.stack_mode = e->detail;
        XConfigureWindow(dpy, e->window, e->value_mask, &wc);
        return;
    }
    if (e->value_mask & CWWidth)  c->w = e->width;
    if (e->value_mask & CWHeight) c->h = e->height;
    XResizeWindow(dpy, c->frame, c->w, c->h + TITLE_HEIGHT);
    XMoveResizeWindow(dpy, c->client, 0, TITLE_HEIGHT, c->w, c->h);
    wm_draw_frame(c->frame);
}

void wm_handle_destroy(XDestroyWindowEvent *e) {
    for (int i = 0; i < nclients; i++) {
        if (clients[i].client == e->window || clients[i].frame == e->window) {
            for (int j = i; j < nclients - 1; j++)
                clients[j] = clients[j + 1];
            nclients--;
            break;
        }
    }
}

void wm_handle_property(XPropertyEvent *e) {
    Client *c = find_client(e->window);
    if (c && e->atom == XA_WM_NAME)
        wm_draw_frame(c->frame);
}
