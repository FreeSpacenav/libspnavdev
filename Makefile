PREFIX ?= /usr/local

src = src/spnavdev.c src/serdev.o src/usbdev.o
obj = $(src:.c=.o)
dep = $(obj:.o=.d)
name = spnavdev

so_abi = 0
so_rev = 1

CFLAGS = -pedantic -Wall -g -MMD $(pic)

alib = lib$(name).a

sys ?= $(shell uname -s | sed 's/MINGW.*/mingw/')
ifeq ($(sys), mingw)
	sodir = bin
	solib = lib$(name).dll
	shared = -shared
else
	sodir = lib
	solib = lib$(name).so.$(so_abi).$(so_rev)
	soname = lib$(name).so.$(so_abi)
	ldname = lib$(name).so
	shared = -shared -Wl,-soname=$(soname)
	pic = -fPIC
endif

.PHONY: all
all: $(solib) $(alib)


$(alib): $(obj)
	$(AR) rcs $@ $(obj)

$(solib): $(obj)
	$(CC) -o $@ $(shared) $(obj) $(LDFLAGS)

-include $(dep)

.PHONY: clean
clean:
	$(RM) $(obj) $(alib) $(solib)

.PHONY: cleandep
cleandep:
	$(RM) $(dep)

.PHONY: install
install: $(solib) $(alib)
	mkdir -p $(DESTDIR)$(PREFIX)/include $(DESTDIR)$(PREFIX)/lib $(DESTDIR)$(PREFIX)$(sodir)
	cp src/spnavdev.h $(DESTDIR)$(PREFIX)/include/spnavdev.h
	cp $(alib) $(DESTDIR)$(PREFIX)/lib/$(alib)
	cp $(solib) $(DESTDIR)$(PREFIX)/$(sodir)/$(solib)
	[ -n "$(soname)" ] && \
		cd $(DESTDIR)$(PREFIX)/$(sodir) && \
		ln -s $(solib) $(soname) && \
		ln -s $(solib) $(ldname) || true

.PHONY: uninstall
uninstall:
	$(RM) $(DESTDIR)$(PREFIX)/include/spnavdev.h
	$(RM) $(DESTDIR)$(RPEFIX)/lib/$(alib)
	$(RM) $(DESTDIR)$(PREFIX)/$(sodir)/$(solib)
	[ -n "$(soname)" ] && \
		$(RM) $(DESTDIR)$(PREFIX)/$(sodir)/$(soname) && \
		$(RM) $(DESTDIR)$(PREFIX)/$(sodir)/$(ldname) || true
