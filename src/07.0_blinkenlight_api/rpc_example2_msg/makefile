

# rpcgen ... "NEWSTYLE (multiple arguments)
#	and own "main()"

#
# CC Command
#

OS_CCDEFS = -lrt -lm -D_GNU_SOURCE
CC_DBG_FLAGS = -ggdb3 -O0




# fuer beaglebone und angstrom stand alone toolchain
CCARM = arm-angstrom-linux-gnueabi-gcc -mthumb -march=armv7-a -std=c99 -U__STRICT_ANSI__ $(CC_DBG_FLAGS) $(OS_CCDEFS) -I.


#hello_server hello_client *.o


#hello_server:	gpiotest.c gpio.c control_module.c
#	$(CC) $^ -o $@ $(LDFLAGS)



# Parameters

# program names
IFNAME=msgrpc
CLIENT = msg_client
SERVER_X86 = msg_server_x86
SERVER_ARM = msg_server_arm

# application sources

# "msg_proc" is the procedure to be run remotely on server
# "msgrpc_clnt_main" is the client main()
# the server main() is auto generated, 
SOURCES_CLNT.c = $(IFNAME)_clnt_main.c
SOURCES_CLNT.h = 
SOURCES_SVC.c = $(IFNAME)_svc_main.c msg_proc.c
SOURCES_SVC.h = 
SOURCES.x = $(IFNAME).x

# _svc, _clnt, _xdr generated by rpcgen, do not change names!
TARGETS_SVC.c = $(IFNAME)_svc.c $(IFNAME)_xdr.c
TARGETS_CLNT.c = $(IFNAME)_clnt.c $(IFNAME)_xdr.c
TARGETS_SERVERPROCDEMO.c=demo_$(SERVER)_procs.c
TARGETS_CLIENTDEMO.c=demo_$(CLIENT)_main.c
TARGETS_MAKEFILEDEMO=demo_makefile
# the xdr is both in server and client
TARGETS = $(IFNAME).h $(IFNAME)_svc.c  $(IFNAME)_clnt.c $(IFNAME)_xdr.c
TARGETS_DEMO =$(TARGETS_SERVERPROCDEMO.c) $(TARGETS_CLIENTDEMO.c) $(TARGETS_MAKEFILEDEMO)

OBJECTS_CLNT = $(SOURCES_CLNT.c:%.c=%.o) $(TARGETS_CLNT.c:%.c=%.o)
OBJECTS_SVC = $(SOURCES_SVC.c:%.c=%.o) $(TARGETS_SVC.c:%.c=%.o) 
# Compiler flags 

CFLAGS += -g 
LDLIBS += -lnsl
# -N = new style.
RPCGENFLAGS = -N

# Targets 

all : $(CLIENT) $(SERVER_X86) $(SERVER_ARM)
# make ARM separately $(SERVER_ARM)

$(TARGETS) : $(IFNAME).x
	@echo + generating interface header and stub sources
	# build all demo code as by-produkt: $(IFNAME)_
	rpcgen $(RPCGENFLAGS) -Sc -o $(TARGETS_CLIENTDEMO.c) $(IFNAME).x
	rpcgen $(RPCGENFLAGS) -Ss -o $(TARGETS_SERVERPROCDEMO.c) $(IFNAME).x
	# rpcgen -Sm -o $(TARGETS_MAKEFILEDEMO) $(IFNAME).x
	# xdr file
	rpcgen $(RPCGENFLAGS) -c -o $(IFNAME)_xdr.c $(IFNAME).x
	# header file
	rpcgen $(RPCGENFLAGS) -h -o $(IFNAME).h $(IFNAME).x
	# client stub
	rpcgen $(RPCGENFLAGS) -l -o $(IFNAME)_clnt.c $(IFNAME).x
	# server stub without main()
	rpcgen $(RPCGENFLAGS) -m -o $(IFNAME)_svc.c $(IFNAME).x

$(OBJECTS_CLNT) : $(SOURCES_CLNT.c) $(SOURCES_CLNT.h) $(TARGETS_CLNT.c) 

$(OBJECTS_SVC) : $(SOURCES_SVC.c) $(SOURCES_SVC.h) $(TARGETS_SVC.c) 

$(CLIENT) : $(OBJECTS_CLNT) 
	rm -f $(OBJECTS_CLNT)
	make $(OBJECTS_CLNT)  
	$(LINK.c) -o $(CLIENT) $(OBJECTS_CLNT) $(LDLIBS) 

$(SERVER_X86) : $(OBJECTS_SVC) 
	# alles neu übersetzen, vorher lief vielleicht ARM
	rm -f $(OBJECTS_SVC) 
	# make clean
	# rebuild obj for x86
	make $(OBJECTS_SVC)
	$(LINK.c) -o $(SERVER_X86) $(OBJECTS_SVC) $(LDLIBS)

$(SERVER_ARM) : $(OBJECTS_SVC) 
	# alles neu übersetzen für ARM
	rm -f $(OBJECTS_SVC) 
	# make clean
	# rebuild obj for ARM
	make CC="$(CCARM)" $(OBJECTS_SVC)
	$(CCARM) -o $(SERVER_ARM) $(OBJECTS_SVC) $(LDLIBS)
#	$(LINK.c) -o $(SERVER) $(OBJECTS_SVC) $(LDLIBS)

 clean:
	 $(RM) core $(TARGETS) $(TARGETS_DEMO) $(OBJECTS_CLNT) $(OBJECTS_SVC) \
	 $(CLIENT) $(SERVER_X86) $(SERVER_ARM)

