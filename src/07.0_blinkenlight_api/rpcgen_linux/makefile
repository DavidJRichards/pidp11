
# make file to build Blinkenlight API sources with "rpcgen"
#
# to be called by client or server makefile
#



# Parameters

# program names
IFNAME=rpc_blinkenlight_api

# RPC API definition file
# lives  in .., is copied here in every rule
IFNAME.x = $(IFNAME).x

# _svc, _clnt, _xdr generated by rpcgen, do not change names!
TARGETS = $(IFNAME).h $(IFNAME)_svc.c  $(IFNAME)_clnt.c $(IFNAME)_xdr.c
TARGETS_SERVERPROCDEMO.c=demo_$(IFNAME)_server_procs.c
TARGETS_CLIENTDEMO.c=demo_$(IFNAME)_client_main.c
TARGETS_MAKEFILEDEMO=demo_makefile

TARGETS_DEMO =$(TARGETS_SERVERPROCDEMO.c) $(TARGETS_CLIENTDEMO.c) $(TARGETS_MAKEFILEDEMO)

#### As documentation (not builded here):
# needed by server
SERVER_OBJECTS=$(IFNAME)_svc.o $(IFNAME)_xdr.o
# needed by client
CLIENT_OBJECTS=$(IFNAME)_clnt.o $(IFNAME)_xdr.o

# -N = new style.
RPCGENFLAGS = -N

# rules

$(IFNAME.x): ../$(IFNAME.x)
	cp ../$(IFNAME.x) ./$(IFNAME.x)

all : demo sources

demo : $(IFNAME.x)
	@echo + generating demo stub sources
# build all demo code as by-produkt: $(IFNAME)_
	rm -f $(TARGETS_CLIENTDEMO.c)
	rpcgen $(RPCGENFLAGS) -Sc -o $(TARGETS_CLIENTDEMO.c) $(IFNAME.x)
	rm -f $(TARGETS_SERVERPROCDEMO.c)
	rpcgen $(RPCGENFLAGS) -Ss -o $(TARGETS_SERVERPROCDEMO.c) $(IFNAME.x)
# rm -f $(TARGETS_MAKEFILEDEMO)
# rpcgen $(RPCGENFLAGS) -Sm -o $(TARGETS_MAKEFILEDEMO) $(IFNAME.x)

sources: $(IFNAME.x)
	@echo + generating interface header and stub sources
# xdr file
	rm -f $(IFNAME)_xdr.c
	rpcgen $(RPCGENFLAGS) -c -o $(IFNAME)_xdr.c $(IFNAME.x)
# header file
	rm -f $(IFNAME).h
	rpcgen $(RPCGENFLAGS) -h -o $(IFNAME).h $(IFNAME.x)
# client stub
	rm -f $(IFNAME)_clnt.c
	rpcgen $(RPCGENFLAGS) -l -o $(IFNAME)_clnt.c $(IFNAME.x)
# server stub without main()
	rm -f $(IFNAME)_svc.c
	rpcgen $(RPCGENFLAGS) -m -o $(IFNAME)_svc.c $(IFNAME.x)


 clean:
	 $(RM) core $(SERVEROBJECTS) $(CLIENTOBJECTS) $(TARGETS) \
	$(TARGETS_SERVERPROCDEMO.c) $(TARGETS_CLIENTDEMO.c) $(TARGETS_MAKEFILEDEMO) \
	./$(IFNAME.x)

