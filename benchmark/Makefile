ifneq ($(KERNELRELEASE),)

# call from kernel build system
obj-m := benchmark.o
benchmark-y := benchmark_main.o benchmark_histogram.o

else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
