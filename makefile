TARGETS = proxy
OBJS = ./multithread_server.o ./request_response.o

CFLAGS += -pthread -std=gnu11 -I.
JUNKF = $(OBJS) *~
JUNKD = *.dSYM

#UNAME = $(shell uname)
#ifeq ($(UNAME), Linux)
#LDFLAGS += -pthread
#endif

all: $(TARGETS)
$(TARGETS): $(OBJS)
tidy:    
	rm -f $(JUNKF); rm -rf $(JUNKD)
clean:
	rm -f $(JUNKF) $(TARGETS); rm -rf $(JUNKD)
