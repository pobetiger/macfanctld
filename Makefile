#
# Makefile for macfanctl
#
# Mikael Strom, August 2010
# Ben Sung Hsu, Nov 2016
#

CC = gcc -xc -c
LD = g++
CXX = g++ -c
CFLAGS += -Wall
CXXFLAGS += -std=c++17 -Wall
SBIN_DIR = $(DESTDIR)/usr/sbin
ETC_DIR = $(DESTDIR)/etc

# check debug flag
ifeq ($(DEBUG),1)
	CFLAGS += -g
endif

SRC += macfanctl.cc
SRC += control.c
SRC += config.c 

TEST_SRC += test-readconfig.c \
			config.c

OBJ_1 := $(SRC:%.c=%.o)
OBJ   := $(OBJ_1:%.cc=%.o)

OBJ_TEST_1 := $(TEST_SRC:%.c=%.o)
OBJ_TEST := $(OBJ_TEST_1:%.cc=%.o)


# Build Targets

all: macfanctld test-readconfig

show:
	$(ECHO) "target: $(OBJ_1)"


macfanctld: $(OBJ)
	$(LD) $(CFLAGS) -o $@ $(OBJ)

test-readconfig: $(OBJ_TEST)
	$(LD) $(CFLAGS) -o $@ $(OBJ_TEST)

clean:
	rm -rf *.o macfanctld test-readconfig

install:
	chmod +x macfanctld
	cp macfanctld $(SBIN_DIR)
	$(shell [[ ! -f $(ETC_DIR)/macfanctl.conf ]] && \
		cp macfanctl.conf $(ETC_DIR) && ] \
		echo -e "*** install default macfanctl.conf file ***")

uninstall:
	rm $(SBIN_DIR)/macfanctld $(INITD_DIR)/macfanctl 

install-systemd:
	cp macfanctld.systemd.service /etc/systemd/system/macfanctld.service

uninstall-systemd:
	rm /etc/systemd/system/macfanctld.service

install-all: install install-systemd

# only remove conf file when doing uninstall-all
uninstall-all: uninstall uninstall-systemd
	rm $(ETC_DIR)/macfanctl.conf

.PHONY: clean install uninstall install-all uninstall-systemd install-systemd uninstall-all


# Build Rules

.c.o:
	$(CC) $(CFLAGS) -o $@ $<

.cc.o:
	$(CXX) $(CXXFlAGS) -o $@ $<


