CONTIKI_PROJECT = contact_tracing_node
all: $(CONTIKI_PROJECT)

MODULES += os/net/app-layer/mqtt

CONTIKI = ../..

include $(CONTIKI)/Makefile.include
