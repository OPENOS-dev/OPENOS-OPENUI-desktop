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

/* OPENOS 桌面环境 — 主循环
 * 连接 X 服务器，初始化 WM/面板/菜单，分发事件。
 */
#include "de.h"

Display *dpy;
int screen;
Window root;
Window panel_win = 0;
Window menu_win = 0;
int panel_height = PANEL_HEIGHT;

Atom a_wm_delete;
Atom a_net_supported;
Atom a_net_wm_check;
Atom a_net_active;
Atom a_net_client_list;

static int xerr_handler(Display *d, XErrorEvent *e) {
    (void)d; (void)e;
    return 0; /* 忽略 BadWindow 等，避免崩溃 */
}

int main(void) {
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "openos-de: 无法打开 X 显示，请在 OPENOS(或 XQuartz) 下运行。\n");
        return 1;
    }
    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);
    XSetErrorHandler(xerr_handler);

    de_init_atoms();
    wm_start();          /* 设置壁纸 + 夺取 WM 角色 + EWMH */
    panel_create();      /* 顶栏 */
    menu_init();         /* 应用菜单(隐藏) */

    fd_set fds;
    while (1) {
        if (XPending(dpy) == 0) {
            /* 无事件时每秒刷新时钟 */
            struct timeval tv = {1, 0};
            FD_ZERO(&fds);
            FD_SET(ConnectionNumber(dpy), &fds);
            if (select(ConnectionNumber(dpy) + 1, &fds, NULL, NULL, &tv) < 0)
                continue;
            panel_update_clock();
        }
        while (XPending(dpy)) {
            XEvent ev;
            XNextEvent(dpy, &ev);
            switch (ev.type) {
            case MapRequest:
                wm_handle_map_request(&ev.xmaprequest);
                break;
            case ConfigureRequest:
                wm_handle_configure_request(&ev.xconfigurerequest);
                break;
            case DestroyNotify:
                wm_handle_destroy(&ev.xdestroywindow);
                break;
            case ButtonPress:
                if (ev.xbutton.window == panel_win)
                    panel_handle_button(&ev.xbutton);
                else if (menu_contains(ev.xbutton.window))
                    menu_handle_button(&ev.xbutton);
                else
                    wm_handle_button(&ev.xbutton);
                break;
            case ButtonRelease:
                wm_handle_release(&ev.xbutton);
                break;
            case MotionNotify:
                wm_handle_motion(&ev.xmotion);
                break;
            case Expose:
                if (ev.xexpose.window == panel_win)
                    panel_draw();
                else if (menu_contains(ev.xexpose.window))
                    menu_draw();
                else
                    wm_draw_frame(ev.xexpose.window);
                break;
            case PropertyNotify:
                wm_handle_property(&ev.xproperty);
                break;
            default:
                break;
            }
        }
    }
    XCloseDisplay(dpy);
    return 0;
}
