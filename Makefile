##
## Code & Maintenance by yon2kong <yon2kong@gmail.com>
##

PREFIX?=./_install

all:
	$(MAKE) -C src all plugins

clean:
	$(MAKE) -C src clean

install: all
	mkdir -p $(PREFIX)/lib
	mkdir -p $(PREFIX)/log
	mkdir -p $(PREFIX)/plugins
	cp -a src/kong $(PREFIX)
	cp -a src/*.so lib/*.so* $(PREFIX)/lib
	cp -a src/plugins/*.so $(PREFIX)/plugins
	cp -a kong.sql kong.xml $(PREFIX)

.PHONY: all clean install

liby:
	$(MAKE) -C thirdparty/liby clean install PREFIX=$(PWD) CC=$(CC)

liby-clean:
	$(MAKE) -C thirdparty/liby uninstall PREFIX=$(PWD) CC=$(CC)
