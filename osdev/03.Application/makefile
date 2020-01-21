all: OSLibrary HelloWorld TextViewer

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

TextViewer:
	@echo 
	@echo ============= Build TextViewer ===============
	@echo 
	
	make -C 01.TextViewer

	@echo 
	@echo =============== Build TextViewer ===============
	@echo 

clean:
	make -C UserLibrary clean
	make -C 00.HelloWorld clean
	make -C 01.TextViewer clean
