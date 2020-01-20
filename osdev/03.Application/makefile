all: OSLibrary HelloWorld

OSLibrary:
	@echo 
	@echo ============= Build UserLibrary ===============
	@echo 
	
	make -C UserLibrary

	@echo 
	@echo =============== Build Complete ===============
	@echo 

HelloWorld:
	@echo 
	@echo ============= Build HelloWorld ===============
	@echo 
	
	make -C 00.HelloWorld

	@echo 
	@echo =============== Build Complete ===============
	@echo 

clean:
	make -C UserLibrary clean
	make -C 00.HelloWorld clean
