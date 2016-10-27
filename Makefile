
PREFIX = /usr/local
INCLUDEDIR = ./include
LIBDIR = ./lib

CPPFLAGS = -Wall -O2 -g -std=gnu++11 -I$(INCLUDEDIR)
LDFLAGS = -lboost_date_time -lsqlite3 -lglog $(LIBDIR)/*.so

# targets
CTP = ctp
TARGET = $(CTP)

all: $(TARGET)

$(CTP): src/main.o src/rc.o
	g++ $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)
	rm -f src/*.o

install: $(TARGET)
	mkdir -p $(PREFIX)
	install -m 755 $(TARGET)       $(PREFIX)
	install -m 755 lib/*           $(PREFIX)/lib
	install -m 644 ctp.sql ctp.xml $(PREFIX)

.PHONY: clean install
