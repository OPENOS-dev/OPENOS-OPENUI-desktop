/*
 * Copyright (C) 2026 OPENOS-dev
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the OPENOS-PROJECT-LICENSE (OPL) v1.2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OPL for more details.
 *
 * You should have received a copy of the OPL along with this program.
 * If not, see <https://github.com/OPENOS-dev/OPL>.
 */

/* OPENOS 桌面环境 — 窗口管理器 (基础重父化 WM)
 * 功能: 接管客户端窗口、加悬浮标题栏、拖拽移动、关闭按钮。
 * 悬浮设计: 标题栏(左) + 控制按钮(右) 两个独立块
 * Windows 风格: 关闭/最大化/最小化按钮在右侧
 */
#include "de.h"

/* 按钮区域枚举 */
enum { BTN_NONE, BTN_CLOSE, BTN_MAX, BTN_MIN };

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

    /* 悬浮装饰: 标题栏高度包含浮动间距 */
    int deco_h = TITLE_HEIGHT + FLOATING_MARGIN * 2;

    Window frame = XCreateSimpleWindow(dpy, root, x, y, w, h + deco_h,
                                        1, BlackPixel(dpy, screen),
                                        WhitePixel(dpy, screen));
    XSelectInput(dpy, frame,
                 SubstructureRedirectMask | SubstructureNotifyMask |
                 ButtonPressMask | ExposureMask);
    XReparentWindow(dpy, e->window, frame, 0, deco_h);
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

/* 检查按钮命中: 相对 frame 坐标 */
static int hit_button(Client *c, int x, int y) {
    /* 只检查装饰区域 */
    int deco_h = TITLE_HEIGHT + FLOATING_MARGIN * 2;
    if (y < 0 || y > deco_h) return BTN_NONE;

    /* 控制块按钮区 (右侧) */
    int cx = c->w - CONTROLS_W - FLOATING_MARGIN;
    int btn_right = CONTROLS_W - FLOATING_MARGIN;
    int btn_center_y = (TITLE_HEIGHT - BTN_H) / 2 + FLOATING_MARGIN;

    /* 关闭按钮 (最右) */
    int cbx = cx + btn_right - BTN_W;
    if (x >= cbx && x < cbx + BTN_W &&
        y >= btn_center_y && y < btn_center_y + BTN_H)
        return BTN_CLOSE;

    /* 最大化按钮 (中间) */
    int mbx = cx + btn_right - BTN_W - BTN_GAP - BTN_W;
    if (x >= mbx && x < mbx + BTN_W &&
        y >= btn_center_y && y < btn_center_y + BTN_H)
        return BTN_MAX;

    /* 最小化按钮 (左) */
    int mnbx = cx + btn_right - 2 * (BTN_W + BTN_GAP) - BTN_W;
    if (x >= mnbx && x < mnbx + BTN_W &&
        y >= btn_center_y && y < btn_center_y + BTN_H)
        return BTN_MIN;

    return BTN_NONE;
}

void wm_draw_frame(Window frame) {
    Client *c = find_client(frame);
    if (!c) return;
    GC gc = XDefaultGC(dpy, screen);

    int deco_h = TITLE_HEIGHT + FLOATING_MARGIN * 2;

    /* 清空装饰区 */
    XClearArea(dpy, frame, 0, 0, c->w, deco_h, False);

    /* 背景色 (dark surface) */
    XSetForeground(dpy, gc, 0x141414);

    /* ---- 左: 悬浮标题栏 ---- */
    XFillRectangle(dpy, frame, gc, FLOATING_MARGIN, FLOATING_MARGIN,
                   TITLE_BAR_W, TITLE_HEIGHT);

    /* 窗口名称 */
    char name[256] = "OPENOS";
    XFetchName(dpy, c->client, name);
    XSetForeground(dpy, gc, 0xF5F5F5);
    XDrawString(dpy, frame, gc, FLOATING_MARGIN + 10,
                FLOATING_MARGIN + TITLE_HEIGHT / 2 + 5,
                name, strlen(name));

    /* ---- 右: 悬浮控制块 ---- */
    int cx = c->w - CONTROLS_W - FLOATING_MARGIN;
    XSetForeground(dpy, gc, 0x141414);
    XFillRectangle(dpy, frame, gc, cx, FLOATING_MARGIN,
                   CONTROLS_W, TITLE_HEIGHT);

    /* 按钮从右到左排列 */
    int btn_right = CONTROLS_W - FLOATING_MARGIN;
    int btn_center_y = FLOATING_MARGIN + (TITLE_HEIGHT - BTN_H) / 2;

    /* 关闭按钮 (最右, 红色) */
    XSetForeground(dpy, gc, 0x882222);
    XFillRectangle(dpy, frame, gc,
                   cx + btn_right - BTN_W, btn_center_y, BTN_W, BTN_H);
    XSetForeground(dpy, gc, 0xFFFFFF);
    XDrawString(dpy, frame, gc,
                cx + btn_right - BTN_W + 7, btn_center_y + 15, "x", 1);

    /* 最大化按钮 (中间) */
    XSetForeground(dpy, gc, 0x333333);
    XFillRectangle(dpy, frame, gc,
                   cx + btn_right - BTN_W - BTN_GAP - BTN_W,
                   btn_center_y, BTN_W, BTN_H);
    XSetForeground(dpy, gc, 0xCCCCCC);
    XDrawRectangle(dpy, frame, gc,
                   cx + btn_right - BTN_W - BTN_GAP - BTN_W + 5,
                   btn_center_y + 5, BTN_W - 10, BTN_H - 10);

    /* 最小化按钮 (左) */
    XSetForeground(dpy, gc, 0x333333);
    XFillRectangle(dpy, frame, gc,
                   cx + btn_right - 2 * (BTN_W + BTN_GAP) - BTN_W,
                   btn_center_y, BTN_W, BTN_H);
    XSetForeground(dpy, gc, 0xCCCCCC);
    XDrawLine(dpy, frame, gc,
              cx + btn_right - 2 * (BTN_W + BTN_GAP) - BTN_W + 5,
              btn_center_y + BTN_H / 2,
              cx + btn_right - 2 * (BTN_W + BTN_GAP) - BTN_W + BTN_W - 5,
              btn_center_y + BTN_H / 2);
}

void wm_handle_button(XButtonEvent *e) {
    Client *c = find_client(e->window);
    if (!c) return;

    int btn = hit_button(c, e->x, e->y);
    if (btn == BTN_CLOSE) {
        XClientMessageEvent cm;
        memset(&cm, 0, sizeof cm);
        cm.type = ClientMessage;
        cm.window = c->client;
        cm.message_type = a_wm_delete;
        cm.format = 32;
        cm.data.l[0] = a_wm_delete;
        XSendEvent(dpy, c->client, False, NoEventMask, (XEvent *)&cm);
        XSync(dpy, False);
    } else if (btn == BTN_NONE && e->y < TITLE_HEIGHT + FLOATING_MARGIN * 2) {
        /* 在装饰区 (非按钮) 开始拖拽 */
        dragging = c->frame;
        drag_off_x = e->x;
        drag_off_y = e->y;
        XGrabPointer(dpy, c->frame, False,
                     PointerMotionMask | ButtonReleaseMask,
                     GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
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
    int deco_h = TITLE_HEIGHT + FLOATING_MARGIN * 2;
    if (e->value_mask & CWWidth)  c->w = e->width;
    if (e->value_mask & CWHeight) c->h = e->height;
    XResizeWindow(dpy, c->frame, c->w, c->h + deco_h);
    XMoveResizeWindow(dpy, c->client, 0, deco_h, c->w, c->h);
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