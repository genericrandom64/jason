# 'make depend' uses makedepend to automatically generate dependencies 
#               (dependencies are added to end of Makefile)
# 'make'        build executable file 'mycc'
# 'make clean'  removes all .o and executable files
#
#

# define the C compiler to use
CC = cc

# define any compile-time flags
CFLAGS = -Wall -ggdb3 -Wextra -O2
# TODO test with -Wpedantic

# define any directories containing header files other than /usr/include
#
# INCLUDES = -I./cpu

# define library paths in addition to /usr/lib
#   if I wanted to include libraries not in /usr/lib I'd specify
#   their path using -Lpath, something like:
# LFLAGS = -L./lib

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
	$(TEST_DIR)/asl.c \
	$(TEST_DIR)/jmp-page-bound-error.c \
	$(TEST_DIR)/branch.c \
	$(TEST_DIR)/sysrq.c \
	util/html.c \
	util/j65-monitor.c \

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

all:    $(SOBJS) $(MAIN)
	@echo  Make done

$(MAIN): $(TOBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(SOBJS) $(subst .out,.o,$@) $(LFLAGS) $(LIBS)

# this is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file) 
# (see the gnu make manual section about automatic variables)
.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN) $(SOBJS) $(TOBJS)

depend: $(SRCS)
	makedepend $(INCLUDES) $^

# DO NOT DELETE THIS LINE -- make depend needs it

cpu/cpu.o: cpu/cpu.h /usr/include/stdint.h /usr/include/sys/cdefs.h
cpu/cpu.o: /usr/include/machine/cdefs.h /usr/include/powerpc/cdefs.h
cpu/cpu.o: /usr/include/machine/_types.h /usr/include/powerpc/_types.h
cpu/cpu.o: /usr/include/stddef.h /usr/include/sys/_null.h
cpu/cpu.o: /usr/include/sys/_types.h /usr/include/string.h
cpu/cpu.o: /usr/include/strings.h /usr/include/stdio.h
cpu/cpu.o: /usr/include/sys/types.h /usr/include/sys/endian.h
cpu/cpu.o: /usr/include/sys/_endian.h /usr/include/machine/endian.h
cpu/cpu.o: /usr/include/powerpc/endian.h /usr/include/stdlib.h
cpu/cpu.o: cpu/internal/common.h
