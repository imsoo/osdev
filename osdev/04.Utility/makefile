all: ImageMaker SerialTransfer

ImageMaker:
	@echo 
	@echo ============= Build ImageMaker ===============
	@echo 
	
	make -C 00.ImageMaker

	@echo 
	@echo =============== Build Complete ===============
	@echo 
	
SerialTransfer:
	@echo 
	@echo ============= Build SerialTransfer ===============
	@echo 
	
	make -C 01.SerialTransfer

	@echo 
	@echo =============== Build Complete ===============
	@echo 
	
clean:
	make -C 00.ImageMaker clean
	make -C 01.SerialTransfer clean