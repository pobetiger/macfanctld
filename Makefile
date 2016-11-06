#
# Makefile for macfanctl
#
# Mikael Strom, August 2010
# Ben Sung Hsu, Nov 2016
#

CC = gcc
CFLAGS += -Wall
SBIN_DIR = $(DESTDIR)/usr/sbin
ETC_DIR = $(DESTDIR)/etc

# check debug flag
ifeq ($(DEBUG),1)
	CFLAGS += -g
endif

SRC += macfanctl.c \
	   control.c \
	   config.c 

TEST_SRC += test-readconfig.c \
			config.c

OBJ := $(SRC:%.c=%.o)

OBJ_TEST := $(TEST_SRC:%.c=%.o)


# Build Targets

all: macfanctld test-readconfig

macfanctld: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

test-readconfig: $(OBJ_TEST)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf *.o macfanctld test-readconfig

install:
	chmod +x macfanctld
	cp macfanctld $(SBIN_DIR)
	cp macfanctl.conf $(ETC_DIR)

uninstall:
	rm $(SBIN_DIR)/macfanctld $(INITD_DIR)/macfanctl $(ETC_DIR)/macfanctl.conf

install-systemd:
	cp macfanctld.systemd.service /etc/systemd/system/macfanctld.service

uninstall-systemd:
	rm /etc/systemd/system/macfanctld.service

install-all: install install-systemd

uninstall-all: uninstall uninstall-systemd

.PHONY: clean install uninstall install-all uninstall-systemd install-systemd uninstall-all


# Build Rules

.o : .c
	$(CC) $(CFLAGS) -o $@ $^

