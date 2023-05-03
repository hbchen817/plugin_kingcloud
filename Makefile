CC := $(TOOLCHAIN_PREFIX)gcc
AR := $(TOOLCHAIN_PREFIX)ar
CFLAGS = -c -g3 -O2 -Wall -std=gnu11 -fPIC -fvisibility=hidden
CFLAGS_TEST = -g3 -O2 -Wall -std=gnu11
ifeq ($(TOOLCHAIN_PREFIX), )
	ARCH := x86_64
	CFLAGS += -fsanitize=address -fno-omit-frame-pointer
	CFLAGS_TEST += -fsanitize=address -fno-omit-frame-pointer
else
	ARCH := mipsle
endif

VERSION_MAJOR = 2
VERSION_MINOR = 0
VERSION_PATCH = 0
VERSION_GIT = $(shell git log --oneline | wc -l)
VERSION_HASH = $(shell git rev-parse --short HEAD)
VERSION := $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)-$(VERSION_GIT)-$(VERSION_HASH)

ROOT_DIR ?= ./

DEPEND_LIBS = 
CDEFINED = -DVERSION=\"$(VERSION)\"
CINCLUDE = -I$(ROOT_DIR)inc -I$(ROOT_DIR)contrib/paho.mqtt.c -I$(ROOT_DIR)contrib/cJSON -I$(ROOT_DIR)contrib/libyaml
CLIB = $(DEPEND_LIBS) -Llib/$(ARCH) -lpaho-mqtt3as -lcjson -lcurl -lyaml

SRC_DIR := $(ROOT_DIR)src
INT_DIR := $(subst src,_int_$(ARCH),$(SRC_DIR))
BIN_DIR := $(ROOT_DIR)_bin_$(ARCH)
TARGET = $(BIN_DIR)/mqtt_plugin.so.$(VERSION)
SRC_FILES := $(foreach dir,$(SRC_DIR),$(wildcard $(dir)/*.c))
OBJ_FILES := $(patsubst %.c,%.o,$(subst src,_int_$(ARCH),$(SRC_FILES)))
DEP_FILES := $(patsubst %.o,%.d,$(OBJ_FILES))

define gendepends
	@set -e; if [ ! -d $(ROOT_DIR)_int_$(ARCH) ]; then mkdir -p $(ROOT_DIR)_int_$(ARCH); fi; \
	if [ ! -d $(ROOT_DIR)_bin_$(ARCH) ]; then mkdir -p $(ROOT_DIR)_bin_$(ARCH); fi; \
	rm -f $@; $(CC) -MM -MF $@.$$$$ -std=gnu11 $(CINCLUDE) $(CDEFINED) $<; echo -n '$@' | sed 's,^\(.*/\).*,\1,g' > $@; cat $@.$$$$ >> $@; rm -f $@.$$$$
endef

define runcompile
	@set -e; echo $<; $(CC) $(CFLAGS) $(CINCLUDE) $(CDEFINED) $< -o $@
endef

$(TARGET): $(INT_DIR) $(DEP_FILES) $(OBJ_FILES)
	@set -e; echo $@; $(CC) -shared -g -o $@ $(OBJ_FILES) $(CLIB)
$(INT_DIR):
	@test -d $@ || mkdir -p $@
all: $(TARGET)

$(ROOT_DIR)_int_$(ARCH)/%.d: $(ROOT_DIR)src/%.c
	$(call gendepends)

-include $(DEP_FILES)

$(ROOT_DIR)_int_$(ARCH)/%.o: $(ROOT_DIR)src/%.c
	$(call runcompile)

clean:
	-@rm -f $(TARGET) $(OBJ_FILES) $(DEP_FILES)

debug:
	@echo $(OBJ_FILES)

$(BIN_DIR)/test_configure: test/test_configure.c src/configuration.c src/rbtree.c src/any.c
	@set -e; echo $@; $(CC) $(CFLAGS_TEST) -Icontrib/libyaml/ -Icontrib/cJSON/ -Iinc/ -Isrc/ $^ -Llib/$(ARCH) -lyaml -lcjson -o $@

$(BIN_DIR)/test_cron: test/test_cron.c
	@set -e; echo $@; $(CC) $(CFLAGS_TEST) -Isrc/ test/test_cron.c -o $@

$(BIN_DIR)/test_get_interface: test/test_get_interface.c src/if_helper.c src/if_helper.h
	@set -e; echo $@; $(CC) $(CFLAGS_TEST) -Isrc/ test/test_get_interface.c src/if_helper.c -o $@

$(BIN_DIR)/test_hmac: test/test_hmac.c src/hmac.h src/hmac.c
	@set -e; echo $@; $(CC) $(CFLAGS_TEST) -Isrc/ test/test_hmac.c src/hmac.c -lssl -lcrypto -o $@

$(BIN_DIR)/test_macro: test/test_macro.c src/rbtree.h src/rbtree.c
	@set -e; echo $@; $(CC) $(CFLAGS_TEST) -Isrc/ test/test_macro.c src/rbtree.c -o $@

$(BIN_DIR)/test_map: test/test_map.c src/map.h src/map.c src/rbtree.h src/rbtree.c
	@set -e; echo $@; $(CC) $(CFLAGS_TEST) -Isrc/ test/test_map.c src/map.c src/rbtree.c -o $@

$(BIN_DIR)/test_preprocessor: test/test_preprocessor.c src/preprocessor.h
	@set -e; echo $@; $(CC) $(CFLAGS_TEST) -Isrc/ test/test_preprocessor.c -o $@

$(BIN_DIR)/test_reflect: test/test_reflect.c src/reflect.h src/reflect.c
	@set -e; echo $@; $(CC) $(CFLAGS_TEST) -Isrc/ -Icontrib/cJSON/ test/test_reflect.c src/reflect.c -Llib/$(ARCH) -lcjson -o $@

$(BIN_DIR)/test_thread_pool: test/test_thread_pool.c src/thread_pool.c src/thread_pool.h
	@set -e; echo $@; $(CC) $(CFLAGS_TEST) -Isrc/ test/test_thread_pool.c src/thread_pool.c -lpthread -o $@

$(BIN_DIR)/test_timerfd: test/test_timerfd.c src/map.h src/map.c src/rbtree.h src/rbtree.c src/thread_pool.h src/thread_pool.c src/timer_manager.h src/timer_manager.c
	@set -e; echo $@; $(CC) $(CFLAGS_TEST) -Isrc/ test/test_timerfd.c src/map.c src/rbtree.c src/thread_pool.c src/timer_manager.c -lpthread -o $@

$(BIN_DIR)/test_yaml: test/test_yaml.c
	@set -e; echo $@; $(CC) $(CFLAGS_TEST) -Icontrib/libyaml/ $^ -Llib/$(ARCH) -lyaml -o $@

test: $(BIN_DIR)/test_get_interface $(BIN_DIR)/test_hmac $(BIN_DIR)/test_macro $(BIN_DIR)/test_map $(BIN_DIR)/test_preprocessor $(BIN_DIR)/test_reflect $(BIN_DIR)/test_thread_pool $(BIN_DIR)/test_timerfd

.PHONY: help clean
