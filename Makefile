setup:
	mkdir build;
	cd build;

compile:
	pwd
	conan install ..;
	cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=1

clean:
	rm build -rf;
