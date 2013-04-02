all: pcs

pcs: pcs.c
	arm-linux-gnueabi-gcc -o $@ $<
