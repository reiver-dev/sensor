all: 
	mkdir -p build
	cd build; cmake .. ; make
	
static:
	mkdir -p build
	cd build; cmake -DSTATIC=1 .. ; make

debug:
	mkdir -p build
	cd build; cmake -DDEBUG=1 .. ; make

clean:
	rm -r build