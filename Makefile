#
# Freescale Vybrid Cortex-M4 boot utility (m4boot)
#

CC=$(CROSS_COMPILE)gcc
CPPFLAGS=-I libfdt -I .
CFLAGS=-Wall

all: m4boot libfdt


#
# Overall rules
#
ifdef V
VECHO = :
else
VECHO = echo "	"
ARFLAGS = rc
.SILENT:
endif

#
## Rules for libfdt
#
LIBFDT_objdir = libfdt
LIBFDT_srcdir = libfdt
LIBFDT_archive = $(LIBFDT_objdir)/libfdt.a
LIBFDT_lib = $(LIBFDT_objdir)/libfdt-$(DTC_VERSION).$(SHAREDLIB_EXT)
LIBFDT_include = $(addprefix $(LIBFDT_srcdir)/,$(LIBFDT_INCLUDES))
LIBFDT_version = $(addprefix $(LIBFDT_srcdir)/,$(LIBFDT_VERSION))

include $(LIBFDT_srcdir)/Makefile.libfdt


.PHONY: libfdt
libfdt: $(LIBFDT_archive) $(LIBFDT_lib)

$(LIBFDT_archive): $(addprefix $(LIBFDT_objdir)/,$(LIBFDT_OBJS))
$(LIBFDT_lib): $(addprefix $(LIBFDT_objdir)/,$(LIBFDT_OBJS))

libfdt_clean:
	@$(VECHO) CLEAN "(libfdt)"
	rm -f $(addprefix $(LIBFDT_objdir)/,$(STD_CLEANFILES))

ifneq ($(DEPTARGETS),)
-include $(LIBFDT_OBJS:%.o=$(LIBFDT_objdir)/%.d)
endif


M4BOOT_SRCS = m4boot.c fdthelper.c
M4BOOT_OBJS = $(M4BOOT_SRCS:%.c=%.o)

m4boot: $(M4BOOT_OBJS) $(LIBFDT_archive)

#m4boot: m4boot.o
#	@$(VECHO) CC
#	$(CC) $(CFLAGS) -o m4boot m4boot.o

STD_CLEANFILES = *~ *.o *.$(SHAREDLIB_EXT) *.d *.a *.i *.s

.PHONY : clean
clean: libfdt_clean
	@$(VECHO) CLEAN
	rm -f m4boot m4boot.o fdthelper.o

%: %.o
	@$(VECHO) LD $@
	$(LINK.c) -o $@ $^

%.o: %.c
	@$(VECHO) CC $@
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

%.a:
	@$(VECHO) AR $@
	$(AR) $(ARFLAGS) $@ $^

FORCE:
