#base install folder for sub-20 library 
SUB20BASE=/home/daq/sckao/11_11_4/hcal/hcalUHTR/tool/moduleCheckSUB20/SUB-20-snap-110319/
#base install folder for libusb library
LIBUSBBASE=/home/daq/sckao/11_11_4/hcal/hcalUHTR/tool/moduleCheckSUB20/libusb-1.0.9/

SUPPORTCODE = uHTRPowerMezzInterface.cpp uHTRPowerMezzInterface.h
ifneq ($(words $(wildcard make.local)),0)
include make.local
endif
# end of the configurable region
# Directories to search for header files
SEARCHDIRS :=  -I. -I$(LIBUSBBASE)/include/libusb-1.0/ -I$(SUB20BASE)/lib
# variables
LINKER       := g++
DEPENDFLAGS  :=  -Wall ${SEARCHDIRS} -g
TOUCHHEADERS := ${MYCODEDIR}/*.h

# C
CC     := gcc
CFLAGS  = ${DEPENDFLAGS}

# C++
CXX      := g++
CXXFLAGS  = ${DEPENDFLAGS} -DURPI

%.o : %.cc
	${CXX} ${CPPFLAGS} -c $< -o $@ ${CXXFLAGS}

%.o : %.cpp
	${CXX} ${CPPFLAGS} -c $< -o $@ ${CXXFLAGS}


all:OUTPUT
	
server:uHTR_PowerMezz_Server.exe

OUTPUT:uHTR_PowerMezz_Test.exe uHTR_PowerMezz_Server.exe #uHTR_PowerMezz_Test_V2.exe #uHTR_ClockMezz_Test.exe uHTR_CtrlMezz_Test.exe sub20tool.exe

LIBSRCS_T := uHTRMezzInterface.cpp uHTRPowerMezzInterface.cpp uHTRMezzanines.cpp comInterface.cpp uHTRPowerMezzMenu.cpp io.cpp #uHTRClockMezzInterface.cpp 

LIBSRCS_S :=  comInterfaceServer.cpp #uHTRClockMezzInterface.cpp 

LIBOBJS_T :=$(patsubst %.cpp,%.o,${LIBSRCS_T}) gnublin.o
LIBOBJS_S :=$(patsubst %.cpp,%.o,${LIBSRCS_S}) gnublin.o

#uHTR_PowerMezz_Test_V2.exe: uHTR_PowerMezz_Test_V2.o ${LIBOBJS}
#	gcc -O2 -I. $^ -lboost_thread -lstdc++ -lm -o $@

uHTR_PowerMezz_Test.exe: uHTR_PowerMezz_Test.o ${LIBOBJS_T}
#	gcc -O2 -DLIBUSB_1_0 -I. -L$(LIBUSBBASE)/lib/ -L$(SUB20BASE)/lib/ $^ -lboost_thread -lsub -lusb-1.0 -lstdc++ -lm -o $@
	gcc -O2 -I. $^ -lboost_system -lboost_thread -lncurses -lpthread -lstdc++ -lm -o $@

uHTR_PowerMezz_Server.exe: uHTR_PowerMezz_Server.o ${LIBOBJS_S}
#	gcc -O2 -DLIBUSB_1_0 -I. -L$(LIBUSBBASE)/lib/ -L$(SUB20BASE)/lib/ $^ -lboost_thread -lsub -lusb-1.0 -lstdc++ -lm -o $@
	gcc -O2 -I. $^ -lboost_system -lpthread -lstdc++ -lm -o $@

#uHTR_ClockMezz_Test.exe: uHTR_ClockMezz_Test.o ${LIBOBJS}
#	gcc -O2 -DLIBUSB_1_0 -I. -L$(LIBUSBBASE)/lib/ -L$(SUB20BASE)/lib/ $^ -lsub -lusb-1.0 -lstdc++ -lm -o $@

#uHTR_CtrlMezz_Test.exe:  uHTRCtrlMezzInterface.cpp uHTR_CtrlMezz_Test.o
#	gcc -O2 -DLIBUSB__0 -I. -I$(LIBUSBBASE)/include/libusb-1.0/ -I$(SUB20BASE)/lib -I/usr/include/readline -L$(LIBUSBBASE)/lib/ -L$(SUB20BASE)/lib/ -L/usr/lib64 $^ -lsub -lusb-1.0 -lreadline -lncurses -lstdc++ -lm -o $@

#sub20tool.exe: sub20tool.o ${LIBOBJS}
#	gcc -O2 -DLIBUSB_1_0 -I. -L$(LIBUSBBASE)/lib/ -L$(SUB20BASE)/lib/ $^ -lreadline -lncurses -lsub -lusb-1.0 -lstdc++ -lm -o $@

#uHTR_ClockMezz_Test.o : ${LIBSRCS}

#uHTR_CtrlMezz_Test.o : uHTRCtrlMezzInterface.cpp uHTRCtrlMezzInterface.h

#uHTR_PowerMezz_Test_V2.o : ${LIBSRCS} uHTRMezzInterface.h uHTRPowerMezzInterface.h

gnublin.o : gnublin-api/gnublin.cpp gnublin-api/gnublin.h
	${CXX} ${CPPFLAGS} ${CXXFLAGS} -c -o $@ gnublin-api/gnublin.cpp

clean:	
	rm -f uHTR_PowerMezz_Test.exe uHTR_ClockMezz_Test.exe uHTR_CtrlMezz_Test.exe uHTRClockMezzInterface.o uHTR_ClockMezz_Test.o uHTRMezzInterface.o uHTRPowerMezzInterface.o uHTR_PowerMezz_Test.o uHTRMezzanines.o uHTR_CtrlMezz_Test.o sub20tool.o comInterface.o

# DO NOT DELETE

comInterface.o: comInterface.h io.h
comInterfaceServer.o: comInterfaceServer.h
io.o: io.h
sub20tool.o: sub20tool.h comInterface.h
uHTRClockMezzInterface.o: uHTRClockMezzInterface.h uHTRMezzInterface.h
uHTRClockMezzInterface.o: comInterface.h
uHTR_ClockMezz_Test.o: uHTRClockMezzInterface.h uHTRMezzInterface.h
uHTR_ClockMezz_Test.o: comInterface.h
uHTRCtrlMezzInterface.o: uHTRCtrlMezzInterface.h
uHTR_CtrlMezz_Test.o: uHTRCtrlMezzInterface.h
uHTRMezzanines.o: uHTRMezzanines.h uHTRPowerMezzInterface.h
uHTRMezzanines.o: uHTRMezzInterface.h comInterface.h io.h
uHTRMezzInterface.o: uHTRMezzInterface.h comInterface.h io.h
uHTRPowerMezzInterface.o: uHTRPowerMezzInterface.h uHTRMezzInterface.h
uHTRPowerMezzInterface.o: comInterface.h io.h
uHTRPowerMezzMenu.o: uHTRPowerMezzMenu.h uHTRPowerMezzInterface.h
uHTRPowerMezzMenu.o: uHTRMezzInterface.h comInterface.h uHTRMezzanines.h io.h
uHTR_PowerMezz_Server.o: comInterfaceServer.h
uHTR_PowerMezz_Test.o: io.h uHTRPowerMezzInterface.h uHTRMezzInterface.h
uHTR_PowerMezz_Test.o: comInterface.h uHTRPowerMezzMenu.h uHTRMezzanines.h
gnublin-api/gnublin.o: gnublin-api/gnublin.h
