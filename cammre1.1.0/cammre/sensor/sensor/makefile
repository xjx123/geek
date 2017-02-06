obj-m :=sensor.o
PWD := $(shell pwd)
KEPD := /root/linux-3.0.8
all:
	make -C $(KEPD) M=$(PWD) modules 
clean:
	rm -f *.mod.c *.o Module* *.ko 