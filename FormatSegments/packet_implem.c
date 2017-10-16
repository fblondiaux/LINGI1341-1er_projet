#include "packet_interface.h"
# include <zlib.h> /* crc32 */


/* Extra #includes */
/* Your code will be inserted here */

struct __attribute__((__packed__)) pkt {
	uint8_t type:2;
	uint8_t trFlag:1;
	uint8_t window:5;
	uint8_t seqNum;
	uint16_t length;
	uint32_t timestamp;
	uint32_t crc1;
	char *payload;
	uint32_t crc2;
};

/* Extra code */
/* Your code will be inserted here */

pkt_t* pkt_new()
{
	/* Your code will be inserted here */
}

void pkt_del(pkt_t *pkt)
{
    /* Your code will be inserted here */
}

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt)
{
	/* Your code will be inserted here */
}

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
	/* Your code will be inserted here */
}

ptypes_t pkt_get_type  (const pkt_t*)
{
	/* Your code will be inserted here */
}

uint8_t  pkt_get_tr(const pkt_t*)
{
	/* Your code will be inserted here */
}

uint8_t  pkt_get_window(const pkt_t*)
{
	/* Your code will be inserted here */
}

uint8_t  pkt_get_seqnum(const pkt_t*)
{
	/* Your code will be inserted here */
}

uint16_t pkt_get_length(const pkt_t*)
{
	/* Your code will be inserted here */
}

uint32_t pkt_get_timestamp   (const pkt_t*)
{
	/* Your code will be inserted here */
}

uint32_t pkt_get_crc1   (const pkt_t*)
{
	/* Your code will be inserted here */
}

uint32_t pkt_get_crc2   (const pkt_t*)
{
	/* Your code will be inserted here */
}

const char* pkt_get_payload(const pkt_t*)
{
	/* Your code will be inserted here */
}


pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type)
{
	/* Your code will be inserted here */
}

pkt_status_code pkt_set_tr(pkt_t *pkt, const uint8_t tr)
{
	/* Your code will be inserted here */
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
{
	/* Your code will be inserted here */
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum)
{
	/* Your code will be inserted here */
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length)
{
	/* Your code will be inserted here */
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp)
{
	/* Your code will be inserted here */
}

pkt_status_code pkt_set_crc1(pkt_t *pkt, const uint32_t crc1)
{
	/* Your code will be inserted here */
}

pkt_status_code pkt_set_crc2(pkt_t *pkt, const uint32_t crc2)
{
	/* Your code will be inserted here */
}

pkt_status_code pkt_set_payload(pkt_t *pkt,
							    const char *data,
								const uint16_t length)
{
	/* Your code will be inserted here */
}