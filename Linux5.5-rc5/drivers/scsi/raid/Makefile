SHELL := /bin/bash


TEST_MACHINE_DIR=/home/grandleaf/nfs7/raid.sim/
CFLAGS = -fno-strict-aliasing -mavx2 -march=ivybridge -Wall -std=gnu99 -fmax-errors=1
# -fno-omit-frame-pointer
# -mcmodel=medium -std=c++0x 

LDFLAGS = -pthread 
LDLIBS  = -lnuma 
#-static

ifdef LIB_FAST_RAID
	CFLAGS += -fPIC -DLIB_FAST_RAID
	BINNAME	= lib_fast_raid.so
	LDFLAGS += -shared 
else
	BINNAME	= raid.sim
endif

OUTDIR 		= $(HOME)/tmpfs/raid/out
UTILDIR		= util
DEPDIR 		= $(OUTDIR)/dep
OBJDIR 		= $(OUTDIR)/obj
BINDIR 		= $(OUTDIR)/bin
BIN 		= $(BINDIR)/$(BINNAME)
#MACROEXP	= src/test/scratchpad
MACROEXP	= src/test/test_scratchpad

#-Wno-unused-value
ifdef RELEASE
    CFLAGS += -O2 -DNDEBUG
else
    CFLAGS += -O0 -g -fno-omit-frame-pointer
endif

ifdef GCOV
	CFLAGS += -fprofile-arcs -ftest-coverage --profile-generate=./gcda
	LDLIBS += -lgcov
endif

SRCS = $(shell find . -name *.c)
#SRCS = $(wildcard *.c)
#OBJS=$(SRCS:.c=.o)
OBJS = $(patsubst %.c, $(OBJDIR)/%.o, $(SRCS)) 


$(shell mkdir -p $(DEPDIR) >/dev/null)
$(shell mkdir -p $(OBJDIR) >/dev/null)
$(shell mkdir -p $(BINDIR) >/dev/null)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

#DIRS = $(shell find src -type d)
#INCS = $(foreach d, $(DIRS), -I$d)
#COMPILE.cc = $(CXX) $(COMPILE.flags) -c
INCS = -Iutil -Isrc -Isrc/ci -Isrc/ci/pal/shared -Isrc/ci/pal/linux
COMPILE.flags = $(DEPFLAGS) $(CFLAGS) $(TARGET_ARCH) $(INCS)
COMPILE.cc = $(CC) $(COMPILE.flags) -c
LINK.cc = $(CC) $(LDFLAGS) 
POSTCOMPILE = @mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d



$(BIN): $(OBJS)
	$(shell $(UTILDIR)/version.rb)
	$(LINK.cc) $(OBJS) -o $(BIN) $(LDLIBS)
	@cp $(BIN) $(TEST_MACHINE_DIR)
	
$(OBJS): | $(OBJDIR)

$(OBJDIR)/%.o: %.c
$(OBJDIR)/%.o: %.c $(DEPDIR)/%.d
	$(shell mkdir -p $(dir $@) >/dev/null)
	$(shell mkdir -p $(dir $(filter-out $<, $^)) >/dev/null)
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

.PHONY: clean asm expand objdump run runrun sensor-watch cpu-watch kill gdb test cp

asm: src/test/list_test.c
	$(COMPILE.cc) -Wa,-adhln -g $(OUTPUT_OPTION) $< > $(basename $<).s
	
objdump: $(OBJDIR)/pal_xor_p_simd.o	
	objdump -d -M intel -S pal_xor_p_simd.o > pal_xor_p_simd.s
	 
expand: $(MACROEXP).c
	cpp $(COMPILE.flags) $(MACROEXP).c > $(OUTDIR)/expanded.i
	tail -n 40 $(OUTDIR)/expanded.i

run:
	@cd $(BINDIR); catchsegv ./$(BINNAME)
	
kill:
	@pkill raid.sim	
	
runrun:
	number=1 ; while [[ $$number -le $(NUMCPUS) ]] ; do \
		$(BIN)&		\
		((number = number + 1)) ; \
	done

sensor-watch:
	@watch -n 1 -d sensors

cpu-watch:
	@top -H
#	@mpstat -P ALL 1

numa-watch:
	@watch -n 1 -d numastat

core-binding:
	for i in $(pgrep raid.sim); do ps -mo pid,tid,fname,user,psr -p $i;done
	
gdb:
	gdb -c core $(BIN)

clean:
	@rm -rf *~ *.i *.s asm nohup.out core $(BIN) $(OUTDIR) 
	
test:
	@echo $(INCS)
	
cp:
	@cp $(BIN) $(TEST_MACHINE_DIR)

-include $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS)))

