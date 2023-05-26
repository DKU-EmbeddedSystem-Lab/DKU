obj-m :=ramdisk.o

KDIR :=/lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(PWD) modules
	gcc -o write write.c
clean:
	make -C $(KDIR) M=$(PWD) clean
	rm write

