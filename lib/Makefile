C_SRCS += \
bcframe.c \
bcget.c \
bcrtmsg.c \
bcset.c \
blockRead.c \
deviceio.c \
deviceio_px.c \
error_px.c \
gget.c \
gset.c \
initcard.c \
kbhit.c \
mon.c \
monlkup.c \
monseq.c \
peekNpoke.c \
rtset.c 

OBJS += \
bcframe.o \
bcget.o \
bcrtmsg.o \
bcset.o \
blockRead.o \
deviceio.o \
deviceio_px.o \
error_px.o \
gget.o \
gset.o \
initcard.o \
kbhit.o \
mon.o \
monlkup.o \
monseq.o \
peekNpoke.o \
rtset.o 


libExc1553Px.a: $(OBJS) 
	ar -r libExc1553Px.a $(OBJS)

%.o: %.c
	gcc -I. -Wall -c -g -fPIC $<
%.o: %.cpp
	g++ -I. -Wall -c -g -fPIC $<
clean:
	rm -f $(OBJS) libExc1553Px.a

