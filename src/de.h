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

#ifndef OPENOS_DE_H
#define OPENOS_DE_H

/* OPENOS 桌面环境 — 公共头文件 (C / X11)
 * 版本: DEV2026.1
 */

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

#define OPENOS_DE_VERSION "DEV2026.1"
#define PANEL_HEIGHT 24

/* 悬浮窗口装饰 */
#define TITLE_HEIGHT     28
#define TITLE_BAR_W      120
#define CONTROLS_W       80
#define FLOATING_MARGIN  6
#define BTN_W            22
#define BTN_H            22
#define BTN_GAP          4

/* 共享状态 (在 main.c 中定义) */
extern Display *dpy;
extern int screen;
extern Window root;
extern Window panel_win;
extern Window menu_win;
extern int panel_height;

/* EWMH / ICCCM 原子 */
extern Atom a_wm_delete;
extern Atom a_net_supported;
extern Atom a_net_wm_check;
extern Atom a_net_active;
extern Atom a_net_client_list;

/* 初始化 */
void de_init_atoms(void);

/* 窗口管理器 */
void wm_start(void);
void wm_handle_map_request(XMapRequestEvent *e);
void wm_handle_configure_request(XConfigureRequestEvent *e);
void wm_handle_destroy(XDestroyWindowEvent *e);
void wm_handle_button(XButtonEvent *e);
void wm_handle_motion(XMotionEvent *e);
void wm_handle_release(XButtonEvent *e);
void wm_handle_property(XPropertyEvent *e);
void wm_draw_frame(Window frame);

/* 顶栏 */
void panel_create(void);
void panel_draw(void);
void panel_update_clock(void);
void panel_handle_button(XButtonEvent *e);

/* 应用菜单 */
void menu_init(void);
void menu_toggle(void);
void menu_draw(void);
void menu_handle_button(XButtonEvent *e);
int  menu_contains(Window w);

#endif /* OPENOS_DE_H */
