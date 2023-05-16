NASM64 = nasm -f elf64
GCC64 = gcc -c -m64 -ffreestanding -mcmodel=large -fno-common -fno-pie
AR64 = ar rcs

OBJECTDIRECTORY = Temp
SOURCEDIRECTORY = Source

all: prepare libMINT64OS

prepare:
	mkdir -p $(OBJECTDIRECTORY)
dep:
	@echo === Make Dependancy File ===
	make -C $(OBJECTDIRECTORY) -f ../makefile InternalDependency
	@echo === Dependancy Search Complete ===
libMINT64OS: dep
	make -C $(OBJECTDIRECTORY) -f ../makefile libMINT64OS.a
	cp $(OBJECTDIRECTORY)/libMINT64OS.a ./		
clean:
	rm -f *.a
	rm -f $(OBJECTDIRECTORY)/*.*

CSOURCEFILES = $(wildcard ../$(SOURCEDIRECTORY)/*.c)
ASSEMBLYSOURCEFILES = $(wildcard ../$(SOURCEDIRECTORY)/*.asm)
COBJECTFILES = $(notdir $(patsubst %.c,%.o,$(CSOURCEFILES)))
ASSEMBLYOBJECTFILES = $(notdir $(patsubst %.asm,%.o,$(ASSEMBLYSOURCEFILES)))

%.o: ../$(SOURCEDIRECTORY)/%.c
	$(GCC64) -c $<

%.o: ../$(SOURCEDIRECTORY)/%.asm
	$(NASM64) -o $@ $<

InternalDependency:
	$(GCC64) -MM $(CSOURCEFILES) > Dependency.dep

libMINT64OS.a: $(ASSEMBLYOBJECTFILES) $(COBJECTFILES)
	$(AR64) $@ $^

ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif