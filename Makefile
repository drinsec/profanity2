CC=g++
CDEFINES=
SOURCES=Dispatcher.cpp Mode.cpp precomp.cpp profanity.cpp SpeedSample.cpp
OBJECTS=$(SOURCES:.cpp=.o)

UNAME_S := $(shell uname -s | cut -c 1-6)
UNAME_M := $(shell uname -m)

EXECUTABLE=profanity2.$(UNAME_M)

ifeq ($(UNAME_S),Darwin)
	LDFLAGS=-framework OpenCL
	CFLAGS=-c -std=c++11 -Wall -mmmx -O2
else
	LDFLAGS=-s OpenCL.lib -mcmodel=large
	CFLAGS=-c -std=c++11 -Wall -mmmx -O2 -mcmodel=large
	CYGWIN =
	ifeq ($(UNAME_S),CYGWIN)
		CYGWIN = 1
	else
		ifeq ($(UNAME_S),MINGW3)
			CYGWIN = 1
		else
			ifeq ($(UNAME_S),MINGW6)
				CYGWIN = 1
			endif
		endif
	endif

	ifdef CYGWIN
		EXECUTABLE=profanity2-$(UNAME_M).exe
		LDFLAGS+=-static
	endif
endif

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $(CDEFINES) $< -o $@

clean:
	rm -rf *.o

