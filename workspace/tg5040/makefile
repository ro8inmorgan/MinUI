###########################################################

# pi-lens-ignore: SC1073, SC1065, SC1064, SC1072
ifeq (,$(PLATFORM))
PLATFORM=$(UNION_PLATFORM)
endif

ifeq (,$(PLATFORM))
$(error please specify PLATFORM, eg. PLATFORM=rgb30 make)
endif

.DEFAULT_GOAL := all
REQUIRES_EVTEST=other/evtest
REQUIRES_JSTEST=other/jstest
REQUIRES_UNZIP60=other/unzip60
REQUIRES_FILEMGR=other/NextCommander
REQUIRES_EVTEST_LOCK=$(REQUIRES_EVTEST)/.locked-$(shell python3 ../../scripts/lock.py build evtest sha)
REQUIRES_JSTEST_LOCK=$(REQUIRES_JSTEST)/.locked-$(shell python3 ../../scripts/lock.py build joyutils sha)
REQUIRES_UNZIP60_LOCK=$(REQUIRES_UNZIP60)/.locked-$(shell python3 ../../scripts/lock.py build unzip60 sha)
REQUIRES_FILEMGR_LOCK=$(REQUIRES_FILEMGR)/.locked-$(shell python3 ../../scripts/lock.py build NextCommander sha)

.PHONY: LOCK_FORCE
LOCK_FORCE:

all: readmes

early: $(REQUIRES_EVTEST_LOCK) $(REQUIRES_JSTEST_LOCK) $(REQUIRES_UNZIP60_LOCK) $(REQUIRES_FILEMGR_LOCK)
	mkdir -p other
	cd $(REQUIRES_FILEMGR) && make -j
	cd $(REQUIRES_EVTEST) && $(CROSS_COMPILE)gcc -o evtest evtest.c
	cd $(REQUIRES_JSTEST) && $(CROSS_COMPILE)gcc -o jstest jstest.c
	cd $(REQUIRES_UNZIP60) && make -f unix/Makefile.trimuismart unzip

clean:
	cd $(REQUIRES_FILEMGR) && make clean
	cd $(REQUIRES_UNZIP60) && make -f unix/Makefile.trimuismart clean

$(REQUIRES_FILEMGR_LOCK): ../../dependencies.lock.json LOCK_FORCE
	python3 ../../scripts/checkout_locked.py build NextCommander $(REQUIRES_FILEMGR)
	rm -f $(REQUIRES_FILEMGR)/.locked-*
	touch $@

$(REQUIRES_EVTEST_LOCK): ../../dependencies.lock.json LOCK_FORCE
	python3 ../../scripts/checkout_locked.py build evtest $(REQUIRES_EVTEST)
	rm -f $(REQUIRES_EVTEST)/.locked-*
	touch $@

$(REQUIRES_JSTEST_LOCK): ../../dependencies.lock.json LOCK_FORCE
	python3 ../../scripts/checkout_locked.py build joyutils $(REQUIRES_JSTEST)
	rm -f $(REQUIRES_JSTEST)/.locked-*
	touch $@

$(REQUIRES_UNZIP60_LOCK): ../../dependencies.lock.json LOCK_FORCE
	python3 ../../scripts/checkout_locked.py build unzip60 $(REQUIRES_UNZIP60)
	rm -f $(REQUIRES_UNZIP60)/.locked-*
	touch $@

include ../all/readmes/makefile
