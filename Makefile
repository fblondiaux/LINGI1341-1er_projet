all: receiver sender

create_socket.o : src/EnvoyerRecevoir/create_socket.c src/EnvoyerRecevoir/create_socket.h
		gcc -c src/EnvoyerRecevoir/create_socket.c src/EnvoyerRecevoir/create_socket.h -Wall -Werror

real_address.o: src/EnvoyerRecevoir/real_address.c src/EnvoyerRecevoir/real_address.h
	gcc -c src/EnvoyerRecevoir/real_address.c src/EnvoyerRecevoir/real_address.h -Wall -Werror

wait_for_client.o : src/EnvoyerRecevoir/wait_for_client.c src/EnvoyerRecevoir/wait_for_client.h
	gcc -c src/EnvoyerRecevoir/wait_for_client.c src/EnvoyerRecevoir/wait_for_client.h -Wall -Werror

packet_implem.o : src/FormatSegments/packet_interface.h src/FormatSegments/packet_implem.c
	gcc -c src/FormatSegments/packet_implem.c src/FormatSegments/packet_interface.h -Wall -Werror

receiver : src/Receiver/receiver.c src/Receiver/receiver.h src/Receiver/receptionDonnes.c src/Receiver/receptionDonnes.h create_socket.o real_address.o wait_for_client.o packet_implem.o
	gcc -o receiver src/Receiver/receiver.c src/Receiver/receptionDonnes.c create_socket.o real_address.o wait_for_client.o packet_implem.o -Wall -Werror -Wshadow -lz

sender: src/Sender/sender.c ser/Sender/sender.h src/Sender/envoieDonnes.c src/Sender/envoieDonnes.h create_socket.o real_address.o wait_for_client.o packet_implem.o
	gcc -o sender src/Sender/sender.c create_socket.o real_address.o wait_for_client.o packet_implem.o -Wall -Werror -Wshadow -lz
