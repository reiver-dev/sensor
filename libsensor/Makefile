APP = sensor
ARCH = x86_64
LIBNAME = lib$(APP).so
EXPORTHEADER = sensor.h

# English compile messages
LANG = en_EN.UTF8

# Compiler
CC = gcc

# Build parameters
CFLAGS = -pipe -m64 -Wall -O2 -fPIC

# std -- C standart
# pipe -- Use pipes rather than temporary files
# O2 -- Optimization
# m64 -- for x86_64

# Build parameters for debug
DEBUGFLAGS = -g3 -DDEBUG

# Linker paramers
LFLAGS = -shared

# Directories
HEADERSDIR = headers
SOURCEDIR = source
BUILDDIR = build
DESTDIR = bin
BUILD_DEBUG = build/debug
BUILD_RELEASE = build/release


# Includes
INCLUDE = -I$(HEADERSDIR)

# Target Files
SOURCEFILES = $(notdir $(wildcard $(SOURCEDIR)/*.c))
OBJECTFILES = $(SOURCEFILES:.c=.o)

all: 
	@make release
	cp $(HEADERSDIR)/$(EXPORTHEADER) $(DESTDIR)/$(EXPORTHEADER)

release: BUILDDIR = $(BUILD_RELEASE)
release: build

debug: CFLAGS += $(DEBUGFLAGS)
debug: BUILDDIR = $(BUILD_DEBUG)
debug: build

# Link object files to bin
.PHONY: build
build: $(OBJECTFILES)
	$(CC) $(LFLAGS) $(addprefix $(BUILDDIR)/,$(OBJECTFILES)) -o $(DESTDIR)/$(LIBNAME)
	@echo --- Build successfull ---

# Compile object files
%.o: $(SOURCEDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $(BUILDDIR)/$@ -c $<
	@echo --- File $@ compiled successfully ---

.PHONY: clean
clean:
	@rm -f $(BUILD_DEBUG)/*.o
	@rm -f $(BUILD_RELEASE)/*.o
