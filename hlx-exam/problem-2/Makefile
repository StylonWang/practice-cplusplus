
default::
	g++ -Wall -g mainParent.cpp -o mainParent -pthread -lrt
	g++ -Wall -g childA.cpp -o childA -pthread
	g++ -Wall -g childB.cpp -o childB -pthread -lrt -lm

clean::
	rm -f mainParent childA childB
