SENSOR_PATH=/home/reiver/DEVELOP/epic-ids/epicids/libsensor
SENSOR_LIB = ${SENSOR_PATH}/bin/libsensor.so 

APP = sensor
ARCH = x86_64

# English compile messages
LANG = en_EN.UTF8
# Compiler
CC = gcc
# Build parameters
CFLAGS = -pipe -m64

# std -- C standart
# pipe -- Use pipes rather than temporary files
# O2 -- Optimization
# m64 -- for x86_64

# Build parameters for debug
DEBUGFLAGS = -g3 -DDEBUG
# Linker paramers
LFLAGS = $(SENSOR_LIB)

# Directories
HEADERSDIR = headers
SOURCEDIR = source
BUILDDIR = build
DESTDIR = bin
BUILD_DEBUG = build/debug
BUILD_RELEASE = build/release

# Includes
INCLUDE = -I$(HEADERSDIR) -I$(SENSOR_PATH)/headers/
# Target Files
SOURCEFILES = $(notdir $(wildcard $(SOURCEDIR)/*.c))
OBJECTFILES = $(SOURCEFILES:.c=.o)

all: 
	@make release

release: BUILDDIR = $(BUILD_RELEASE)
release: build

debug: CFLAGS += $(DEBUGFLAGS)
debug: BUILDDIR = $(BUILD_DEBUG)
debug: build

# Link object files to bin
.PHONY: build
build: $(OBJECTFILES)
	$(CC) $(LFLAGS) $(addprefix $(BUILDDIR)/,$(OBJECTFILES)) -o $(DESTDIR)/$(APP)
	@echo --- Build successfull ---

# Compile object files
%.o: $(SOURCEDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $(BUILDDIR)/$@ -c $<
	@echo --- File $@ compiled successfully ---

.PHONY: clean
clean:
	@rm -f $(BUILD_DEBUG)/*.o
	@rm -f $(BUILD_RELEASE)/*.o
