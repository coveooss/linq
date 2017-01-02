VPATH = lib/coveo:lib/coveo/enumerable:lib/coveo/enumerable/detail:lib/coveo/linq:lib/coveo/linq/detail:tests:tests/coveo:tests/coveo/enumerable:tests/coveo/linq

all_tests.out: all_tests.o enumerable_tests.o linq_tests.o tests_main.o
	$(CXX) -o all_tests.out all_tests.o enumerable_tests.o linq_tests.o tests_main.o

all_tests.o: all_tests.cpp all_tests.h
	$(CXX) -c -std=c++1z tests/coveo/linq/all_tests.cpp -Ilib -Itests
enumerable_tests.o: enumerable_tests.cpp enumerable_tests.h enumerable.h enumerable_detail.h
	$(CXX) -c -std=c++1z tests/coveo/enumerable/enumerable_tests.cpp -Ilib -Itests
linq_tests.o: linq_tests.cpp linq_tests.h linq.h linq_detail.h
	$(CXX) -c -std=c++1z tests/coveo/linq/linq_tests.cpp -Ilib -Itests
tests_main.o: tests_main.cpp
	$(CXX) -c -std=c++1z tests/tests_main.cpp -Ilib -Itests

clean:
	rm all_tests.out all_tests.o enumerable_tests.o linq_tests.o tests_main.o

