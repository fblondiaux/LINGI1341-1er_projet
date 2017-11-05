all: receiver sender

create_socket.o : src/EnvoyerRecevoir/create_socket.c src/EnvoyerRecevoir/create_socket.h
		gcc -c src/EnvoyerRecevoir/create_socket.c src/EnvoyerRecevoir/create_socket.h -Wall -Werror -g

real_address.o: src/EnvoyerRecevoir/real_address.c src/EnvoyerRecevoir/real_address.h
	gcc -c src/EnvoyerRecevoir/real_address.c src/EnvoyerRecevoir/real_address.h -Wall -Werror -g

wait_for_client.o : src/EnvoyerRecevoir/wait_for_client.c src/EnvoyerRecevoir/wait_for_client.h
	gcc -c src/EnvoyerRecevoir/wait_for_client.c src/EnvoyerRecevoir/wait_for_client.h -Wall -Werror -g

packet_implem.o : src/FormatSegments/packet_interface.h src/FormatSegments/packet_implem.c
	gcc -c src/FormatSegments/packet_implem.c src/FormatSegments/packet_interface.h -Wall -Werror -g

receiver : src/Receiver/receiver.c src/Receiver/receiver.h src/Receiver/receptionDonnes.c src/Receiver/receptionDonnes.h create_socket.o real_address.o wait_for_client.o packet_implem.o
	gcc -o receiver src/Receiver/receiver.c src/Receiver/receptionDonnes.c create_socket.o real_address.o wait_for_client.o packet_implem.o -Wall -Werror -Wshadow -g -lz

sender: src/Sender/sender.h src/Sender/sender.c src/Sender/envoieDonnes.h src/Sender/envoieDonnes.c  create_socket.o real_address.o wait_for_client.o packet_implem.o
	gcc -o sender src/Sender/sender.h src/Sender/sender.c src/Sender/envoieDonnes.h src/Sender/envoieDonnes.c  create_socket.o real_address.o wait_for_client.o packet_implem.o -Wall -Werror -Wshadow -g -lz
clear : CLEAR
		rm *.o

tests: tests/testSP512.sh tests/testSP15000.sh tests/test0SP15000.sh tests/testAP512.sh tests/testAP15000.sh tests/test0AP15000.sh FORCE
	 ./tests/testSP512.sh
	 ./tests/testSP15000.sh
	 ./tests/test0SP15000.sh
	 ./tests/testAP512.sh
	 ./tests/testAP15000.sh
	 ./tests/test0AP15000.sh
FORCE:
