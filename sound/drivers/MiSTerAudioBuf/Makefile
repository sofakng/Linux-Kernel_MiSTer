obj-$(CONFIG_SND_MISTER_RING) := MiSTerAudioBuf.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# You may need to be root to compile the module.
#
# You must be root to insert it into a running kernel
default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f *.orig *~

	
