all: MINT64OSLibrary HelloWorld

MINT64OSLibrary:
	@echo Build UserLibrary
	make -C UserLibrary
	@echo Build Complete

HelloWorld:
	@echo Build HelloWorld
	make -C 00.HelloWorld
	@echo Build Complete

clean:
	make -C UserLibrary clean
	make -C 00.HelloWorld clean