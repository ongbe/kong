##
## Code & Maintenance by yon2kong <yon2kong@gmail.com>
##

PREFIX?=./_install

all:
	$(MAKE) -C src kong plugins ana

# convenient for macosx
ana:
	$(MAKE) -C src ana

clean:
	$(MAKE) -C src clean

install: all
	mkdir -p $(PREFIX)/lib
	mkdir -p $(PREFIX)/log
	mkdir -p $(PREFIX)/plugins
	cp -a src/kong src/ana $(PREFIX)
	cp -a src/*.so lib/*.so* $(PREFIX)/lib
	cp -a src/plugins/*.so $(PREFIX)/plugins
	cp -a kong.sql kong.xml $(PREFIX)

.PHONY: all clean install

liby:
	$(MAKE) -C thirdparty/liby clean install PREFIX=$(PWD) CC=$(CC)

liby-clean:
	$(MAKE) -C thirdparty/liby uninstall PREFIX=$(PWD) CC=$(CC)

sqlite:
	mkdir -p include lib
	cd thirdparty/sqlite && make distclean && ./configure && cd -
	$(MAKE) -C thirdparty/sqlite
	cp -a thirdparty/sqlite/.libs/*.so* lib
	install -m 644 thirdparty/sqlite/*.h include

sqlite-clean:
	rm -f lib/libsqlite3.* include/sqlite3*.h

.PHONY: liby liby-clean sqlite sqlite-clean
