LIBS := -lExc1553Px -lExc4000 -lexchost2dpr -ldl -lrt -lpthread -lftd2 -lftd3
LDFLAGS := -L ../../lib -fPIC
CFLAGS := -fPIC
CPPFLAGS := -fPIC
#CFLAGS := -H #Show header includes
OBJS +=  icu_1553.o config.o mil_std_1553.o client.o ini.o
PROGS +=  icu_1553

BUILD_DIR = build

all: icu_1553

%.o: %.c
	gcc -I. -I.. -Wall $(CFLAGS) -c -g $<
%.o: %.cpp
	g++ -I. -I.. -Wall $(CPPFLAGS) -c -g $<
icu_1553: $(OBJS)
	g++ -o $@ $^ $(LDFLAGS) $(LIBS)
	mkdir -p $(BUILD_DIR)
	mv -f *.o $(BUILD_DIR)/ 2>/dev/null || true

clean:
	rm -f $(OBJS)  $(PROGS)
	rm -rf $(BUILD_DIR)

