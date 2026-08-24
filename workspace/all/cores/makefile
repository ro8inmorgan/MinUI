###########################################################

# pi-lens-ignore: SC1073, SC1065, SC1064, SC1072
ifeq (,$(PLATFORM))
PLATFORM=$(UNION_PLATFORM)
endif

ifeq (,$(PLATFORM))
	$(error please specify PLATFORM, eg. PLATFORM=trimui make)
endif

PATCH = git apply
PROCS = -j4
BUILD_ROOT=../../$(PLATFORM)/cores
.DEFAULT_GOAL := all

.PHONY: FORCE
FORCE:

define TEMPLATE=

$1_HASH := $$(shell python3 ../../../scripts/lock.py core $(1) sha)
$1_MAKE ?= make $(and $($1_MAKEFILE),-f $($1_MAKEFILE)) platform=$(PLATFORM) $($(1)_FLAGS)
$1_BUILD_PATH ?= $(1)
$1_LOCK_STAMP := src/$(1)/.locked-$$($1_HASH)
$1_OUTPUT := $$(notdir $$(if $$($1_CORE),$$($1_CORE),$(1)_libretro.so))

$$($1_LOCK_STAMP): ../../../dependencies.lock.json FORCE
	python3 ../../../scripts/checkout_locked.py core $(1) src/$(1)
	rm -f src/$(1)/.locked-*
	touch $$@
	$(if $($1_SUBMODULES),cd src/$(1) && git submodule init $($1_SUBMODULES) && git submodule update $($1_SUBMODULES),cd src/$(1) && git submodule update --init --recursive)

src/$(1)/.patched: $$($1_LOCK_STAMP)
	(test ! -f patches/$(1).patch) || (test -f src/$(1)/.patched) || (cd src/$(1) && $(PATCH) -p1 < ../../patches/$(1).patch && touch .patched && true)

src/$(1)/.patched-all: $$($1_LOCK_STAMP)
	(test ! -d ../../all/cores/patches/$(1)) || (test -f src/$(1)/.patched-all) || (cd src/$(1) && $(foreach patch, $(sort $(wildcard ../../all/cores/patches/$(1)/*.patch)), $(PATCH) -p1 < ../../$(patch) &&) touch .patched-all && true)

output/$$($1_OUTPUT): src/$(1)/.patched src/$(1)/.patched-all
	mkdir -p output
	cd src/$$($1_BUILD_PATH) && $$($1_MAKE) $(PROCS)
	mv src/$$($1_BUILD_PATH)/$(if $($(1)_CORE),$($(1)_CORE),$(1)_libretro.so) ./output/$$($1_OUTPUT)

clone-$(1): $$($1_LOCK_STAMP)

status-$(1): $$($1_LOCK_STAMP)
	cd src/$(1) && git show --oneline -s

patch-$(1): src/$(1)/.patched

clean-$(1):
	test ! -d src/$(1) || cd src/$$($1_BUILD_PATH) && $$($1_MAKE) clean
	rm -rf output/$$($1_OUTPUT)

$(1): output/$$($1_OUTPUT)

endef

all: cores

$(foreach CORE,$(CORES),$(eval $(call TEMPLATE,$(CORE))))

cores: $(foreach CORE,$(CORES),$(CORE))

clean: $(foreach CORE,$(CORES),clean-$(CORE))

nuke:
	rm -rf ./output
	rm -rf ./src
