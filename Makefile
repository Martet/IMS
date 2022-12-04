simulate: simulate.o
	c++ simulate.o -o simulate -LSFML-2.5.1/lib -lsfml-graphics -lsfml-window -lsfml-system

simulate.o: simulate.cpp
	c++ -std=c++17 -Wall -Wextra -g -c simulate.cpp -ISFML-2.5.1/include

run: simulate
	export LD_LIBRARY_PATH=SFML-2.5.1/lib && ./simulate

clean:
	rm -f simulate simulate.o

zip:
	tar -czvf 08_xzmitk01.tar.gz Makefile simulate.cpp SFML-2.5.1 dokumentace.pdf

.PHONY: run clean zip
