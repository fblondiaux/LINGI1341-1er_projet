all: receiver sender
receiver: -o srcreceiver

create_socket.o : src/EnvoyerRecevoir/create_socket.c src/EnvoyerRecevoir/create_socket.h
		gcc -c src/EnvoyerRecevoir/create_socket.c -Wall -Werror

real_address.o: src/EnvoyerRecevoir/real_address.c src/EnvoyerRecevoir/real_address.h
	gcc -c src/EnvoyerRecevoir/real_address.c -Wall -Werror

wait_for_client.o : src/EnvoyerRecevoir/wait_for_client.c src/EnvoyerRecevoir/wait_for_client.h
	gcc -c src/EnvoyerRecevoir/wait_for_client.c -Wall -Werror
packet.o : src/FormatSegments/packet_interface.h src/FormatSegments/packet_implem.c
	gcc -c src/FormatSegments/packet_implem.c -Wall -Werror

receiver : src/Receiver/receiver.c create_socket.o real_address.o wait_for_client.o packet.o
	gcc -o receiver src/Receiver/receiver.c create_socket.o real_address.o wait_for_client.o packet.o -Wall -Werror -Wshadow -lz

sender: src/Sender/sender.c create_socket.o real_address.o wait_for_client.o packet.o
	gcc -o sender src/Sender/sender.c create_socket.o real_address.o wait_for_client.o packet.o -Wall -Werror -Wshadow -lz
