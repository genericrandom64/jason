# 'make depend' uses makedepend to automatically generate dependencies 
#               (dependencies are added to end of Makefile)
# 'make'        build executable file 'mycc'
# 'make clean'  removes all .o and executable files
#
#

# define the C compiler to use
CC = gcc

# define any compile-time flags
CFLAGS = -Wall -g

# define any directories containing header files other than /usr/include
#
INCLUDES = -I./cpu

# define library paths in addition to /usr/lib
#   if I wanted to include libraries not in /usr/lib I'd specify
#   their path using -Lpath, something like:
LFLAGS = -L./lib

# define any libraries to link into executable:
#   if I want to link in libraries (libx.so or libx.a) I use the -llibname 
#   option, something like (this will link in libmylib.so and libm.so:
LIBS = 

# define the C source files
SRC_DIR=./cpu
TEST_DIR=./tests
SRCS = $(SRC_DIR)/cpu.c \

TESTS = $(TEST_DIR)/flag.c \
	$(TEST_DIR)/jmp.c \
	$(TEST_DIR)/jmp-page-bound-error.c \
	$(TEST_DIR)/branch.c \
	$(TEST_DIR)/sysrq.c

# define the C object files 
#
# This uses Suffix Replacement within a macro:
#   $(name:string1=string2)
#         For each word in 'name' replace 'string1' with 'string2'
# Below we are replacing the suffix .c of all words in the macro SRCS
# with the .o suffix
#
SOBJS = $(SRCS:.c=.o)
TOBJS = $(TESTS:.c=.o)
OBJS = $(SOBJS) $(TOBJS)

# define the executable file 
MAIN = $(TESTS:.c=.out)

#
# The following part of the makefile is generic; it can be used to 
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'
#

.PHONY: depend clean

all:    $(MAIN)
	@echo  All done!!

$(MAIN): $(TOBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(SOBJS) $(subst .out,.o,$@) $(LFLAGS) $(LIBS)

# this is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file) 
# (see the gnu make manual section about automatic variables)
.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)

depend: $(SRCS)
	makedepend $(INCLUDES) $^

# DO NOT DELETE THIS LINE -- make depend needs it

cpu/cpu.o: cpu/cpu.h /usr/include/stdint.h
cpu/cpu.o: /usr/include/bits/libc-header-start.h /usr/include/features.h
cpu/cpu.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
cpu/cpu.o: /usr/include/bits/wordsize.h /usr/include/bits/long-double.h
cpu/cpu.o: /usr/include/gnu/stubs.h /usr/include/bits/types.h
cpu/cpu.o: /usr/include/bits/timesize.h /usr/include/bits/typesizes.h
cpu/cpu.o: /usr/include/bits/time64.h /usr/include/bits/wchar.h
cpu/cpu.o: /usr/include/bits/stdint-intn.h /usr/include/bits/stdint-uintn.h
cpu/cpu.o: /usr/include/string.h /usr/include/bits/types/locale_t.h
cpu/cpu.o: /usr/include/bits/types/__locale_t.h /usr/include/strings.h
cpu/cpu.o: /usr/include/stdio.h /usr/include/bits/types/__fpos_t.h
cpu/cpu.o: /usr/include/bits/types/__mbstate_t.h
cpu/cpu.o: /usr/include/bits/types/__fpos64_t.h
cpu/cpu.o: /usr/include/bits/types/__FILE.h /usr/include/bits/types/FILE.h
cpu/cpu.o: /usr/include/bits/types/struct_FILE.h
cpu/cpu.o: /usr/include/bits/stdio_lim.h /usr/include/bits/floatn.h
cpu/cpu.o: /usr/include/bits/floatn-common.h
