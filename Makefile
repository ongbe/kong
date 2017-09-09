##
## Code & Maintenance by yon2kong <yon2kong@gmail.com>
##

PREFIX?=./_install

all:
	$(MAKE) -C src

clean:
	$(MAKE) -C src clean

install: all
	mkdir -p $(PREFIX)/plugins
	mkdir -p $(PREFIX)/lib
	mkdir -p $(PREFIX)/log
	install -m 755 src/kong $(PREFIX)
	install -m 755 src/plugins/*.so $(PREFIX)/plugins
	install -m 755 src/*.so lib/*.so* $(PREFIX)/lib
	install -m 644 kong.sql kong.xml $(PREFIX)

.PHONY: all clean install

liby:
	$(MAKE) -C thirdparty/liby clean install PREFIX=$(PWD) CC=$(CC)

liby-clean:
	$(MAKE) -C thirdparty/liby uninstall PREFIX=$(PWD) CC=$(CC)
