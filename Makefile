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

all: macfanctld

macfanctld: macfanctl.c control.c config.c control.h config.h
	$(CC) $(CFLAGS) macfanctl.c control.c config.c -o macfanctld 


clean:
	rm -rf *.o macfanctld

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

