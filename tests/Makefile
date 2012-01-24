.PHONY: runtest clean

runtest: tests
	./tests

clean:
	-rm tests

tests: tests.cpp
	g++ `find ../src/bullet -name "*.cpp"` tests.cpp -I ../src/bullet -o tests