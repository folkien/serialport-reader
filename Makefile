run: serport-detonator
	sudo ./serport-detonator
serport-detonator: main.cpp
	gcc -lpthread -Wall -pedantic main.cpp -o serport-detonator
