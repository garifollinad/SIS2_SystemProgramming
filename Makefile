all:
	cd kernel && make
	cd user && make

clean:
	cd kernel && make clean
	cd user && make clean
