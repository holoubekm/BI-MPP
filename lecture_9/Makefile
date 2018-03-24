obj-m := mpp_module.o
KDIR := /usr/lib/modules/4.3.0-zen-ARCH-g44d3c7f-dirty/build
PWD := $(shell pwd)

default: mpp_module.c
	$(MAKE) -C $(KDIR) M=$(PWD) modules

prog: prog.cpp
	g++ -std=c++11 -Wall -pedantic prog.cpp -o prog

clean:
	rm Module.symvers mpp_module.ko mpp_module.mod.o modules.order mpp_module.mod.c mpp_module.o

