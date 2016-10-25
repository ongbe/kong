
PREFIX = /usr/local
INCLUDEDIR = ./include
LIBDIR = ./lib

CPPFLAGS = -Wall -O2 -g -std=gnu++11 -I$(INCLUDEDIR)
LDFLAGS = -lboost_date_time -lsqlite3 -lglog $(LIBDIR)/*.so

# targets
CTP = ctp
TARGETS = $(CTP)

all: $(TARGETS)

$(CTP): src/main.o src/rc.o
	g++ $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGETS)
	rm -f src/*.o

install: $(TARGETS)
	install -D -m 755 -t $(PREFIX) $(TARGETS)
	install -D -m 755 -t $(PREFIX)/lib lib/*
	install -D -m 644 -t $(PREFIX) ctp.sql ctp.xml

.PHONY: clean install
