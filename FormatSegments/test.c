#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h> 

struct record {
	
	uint8_t header1 : 3; // 0 à 7
	uint8_t header2 : 5; // 0 à 31
	uint16_t header3; // 0 à 65535
};


int main(int argc, const char **argv, struct record *r)
{

	struct record rec;
	rec.header1 = 3;
	rec.header2 =  23;
	rec.header3 = 12565;

	uint8_t header1 = htons(rec.header1);
	uint8_t header2 = htons(rec.header2);
	uint8_t header3 = htons(rec.header3);

	printf("rec.header1 = %d\n", rec.header1);
	printf("rec.header2 = %d\n", rec.header2);
	printf("rec.header3 = %d\n", rec.header3);

	printf("htons(rec.header1) = %d\n", header1);
	printf("htons(rec.header2) = %d\n", header2);
	printf("htons(rec.header3) = %d\n", header3);

	printf("ntohs(rec.header1) = %d\n", ntohs(rec.header1));
	printf("ntohs(rec.header2) = %d\n", ntohs(rec.header2));
	printf("ntohs(rec.header3) = %d\n", ntohs(rec.header3));

	struct record rec2;
	uint8_t head1 = 3;
	uint8_t head2 = 23;
	uint16_t head3 = 12565;

	memcpy(&rec2, &head1, sizeof(uint8_t));
	memcpy(&rec2, &head2, sizeof(uint8_t));
	memcpy(&rec2, &head3, sizeof(uint16_t));

	printf("rec2.header1 = %d\n", htons(rec2.header1));
	printf("rec2.header2 = %d\n", htons(rec2.header2));
	printf("rec2.header3 = %d\n", htons(rec2.header3));




}