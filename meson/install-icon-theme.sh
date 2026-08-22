#!/bin/sh
# Copyright (C) 2026 OPENOS-dev
# This program is free software: you can redistribute it and/or modify
# it under the terms of the OPENOS-PROJECT-LICENSE (OPL) v1.2.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# OPL for more details.
#
# You should have received a copy of the OPL along with this program.
# If not, see <https://github.com/OPENOS-dev/OPL>.
#
# 安装后钩子: 把 OpenOS 主题设为系统默认图标主题
# 写入 /usr/share/glib-2.0/schemas/ + 重新编译 schema
set -e

prefix="$MESON_INSTALL_PREFIX"
datadir="$prefix/share"
theme_dir="$datadir/icons/OpenOS"

if [ ! -d "$theme_dir" ]; then
    echo "OpenOS 图标主题未安装到 $theme_dir, 跳过"
    exit 0
fi

# 写入 settings schema 把 icon-theme 设为 OpenOS (XDG 设置层)
schema_file="$datadir/glib-2.0/schemas/00-openos-icon-theme.gschema.override"
mkdir -p "$(dirname "$schema_file")"
cat > "$schema_file" <<EOF
[org.gnome.desktop.interface]
icon-theme='OpenOS'
EOF

# 重新编译 glib schema (若 glib-compile-schemas 存在)
if command -v glib-compile-schemas >/dev/null 2>&1; then
    glib-compile-schemas "$datadir/glib-2.0/schemas/" || true
fi

echo "OpenOS 图标主题已注册为系统默认"
