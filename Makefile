# OPENOS 桌面环境 — 构建 (需 libX11)
# 在 OPENOS(Linux) 或安装 XQuartz + libx11 的 macOS 上构建

CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -O2
LIBS    ?= -lX11
PREFIX  ?= /usr

BIN = openos-de
SRC = src/main.c src/wm.c src/panel.c src/menu.c

all: $(BIN)

$(BIN): $(SRC) src/de.h
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LIBS)

clean:
	rm -f $(BIN)

install: all
	install -Dm755 $(BIN) $(DESTDIR)$(PREFIX)/bin/$(BIN)

.PHONY: all clean install
