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

/* OPENOS 桌面环境 — 应用菜单 (menu)
 * 点击顶栏 Menu 弹出，启动预注册应用。
 */
#include "de.h"

typedef struct { const char *label; const char *cmd; } App;

static App apps[] = {
    {"Terminal (xterm)", "xterm"},
    {"Files (mc)",       "xterm -e mc"},
    {"Editor (nano)",    "xterm -e nano"},
    {"Calculator",       "xcalc"},
    {NULL, NULL}
};

static int menu_visible = 0;
static const int item_h = 22;

void menu_init(void) {
    menu_win = XCreateSimpleWindow(dpy, root, 0, panel_height, 200, 10,
                                   1, 0x000000, 0xdddddd);
    XSelectInput(dpy, menu_win, ExposureMask | ButtonPressMask);
}

void menu_toggle(void) {
    if (!menu_win) return;
    if (menu_visible) {
        XUnmapWindow(dpy, menu_win);
        menu_visible = 0;
        return;
    }
    int n = 0;
    for (; apps[n].label; n++) ;
    XResizeWindow(dpy, menu_win, 200, n * item_h + 4);
    XMapWindow(dpy, menu_win);
    XRaiseWindow(dpy, menu_win);
    menu_visible = 1;
    menu_draw();
}

void menu_draw(void) {
    if (!menu_visible) return;
    GC gc = XDefaultGC(dpy, screen);
    int n = 0;
    for (; apps[n].label; n++) ;

    XSetForeground(dpy, gc, 0xdddddd);
    XFillRectangle(dpy, menu_win, gc, 0, 0, 200, n * item_h + 4);
    XSetForeground(dpy, gc, 0x000000);
    for (int i = 0; apps[i].label; i++)
        XDrawString(dpy, menu_win, gc, 6, i * item_h + 16,
                    apps[i].label, strlen(apps[i].label));
}

void menu_handle_button(XButtonEvent *e) {
    int idx = (e->y - 2) / item_h;
    if (apps[idx].label && apps[idx].cmd) {
        /* 非阻塞启动: fork 后由子进程执行 */
        if (fork() == 0) {
            execl("/bin/sh", "sh", "-c", apps[idx].cmd, (char *)NULL);
            _exit(127);
        }
    }
    menu_toggle();
}

int menu_contains(Window w) {
    return w == menu_win;
}
