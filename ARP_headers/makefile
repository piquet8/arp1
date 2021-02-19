
SUBDIRS := addresstable misc bitvector connection handshake initstep message statistics
# VPATH := $(patsubst %, %:, $(SUBDIRS))
DEP_OBJ = $(foreach dir, $(SUBDIRS), $(wildcard $(dir)/*.o))

all: subdirs libarpnet.a
	@echo static library compiled

libarpnet.a: subdirs 
	ar rc $@ $(DEP_OBJ)
	ranlib $@

subdirs: $(SUBDIRS)
	@echo calling subdirs

$(SUBDIRS):
	+$(MAKE) -s -C $@
	
.PHONY: subdirs $(SUBDIRS) clean all

clean:
	rm -rf $(DEP_OBJ)
# to compile the source simply do
# gcc -L <path_to_ARP_headers> -o file file.c -larpnet
# after including "arpnet.h" in the file