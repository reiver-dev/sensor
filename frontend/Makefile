CC = gcc
CFLAGS = -g3 -DDEBUG
SENSOR_PATH=/home/reiver/DEVELOP/epic-ids/epicids/libsensor
LFLAGS = ${SENSOR_PATH}/bin/libsensor.so
INCLUDES = -I${SENSOR_PATH}/headers/
DESTDIR = bin
SOURCEDIR = source
BUILDDIR = build

SOURCEFILES = $(notdir $(wildcard $(SOURCEDIR)/*.c))
OBJECTFILES = $(SOURCEFILES:.c=.o)


all: build

build: $(OBJECTFILES)
	$(CC) $(BUILDDIR)/$(OBJECTFILES) $(LFLAGS) -o $(DESTDIR)/mockapp
	@echo --- Build successfull ---


%.o: ${SOURCEDIR}/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $(BUILDDIR)/$@ -c $<
	@echo --- File $@ compiled successfully ---
