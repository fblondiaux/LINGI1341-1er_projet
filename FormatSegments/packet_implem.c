#include "packet_interface.h"
#include <zlib.h> /* crc32 */
#include <stdlib.h> /* malloc/calloc */
#include <string.h> /* memcpy */
#include <arpa/inet.h> /* htons */
#include <stdio.h> /* fread */

/* Extra #includes */
/* Your code will be inserted here */

struct __attribute__((__packed__)) pkt {
	uint8_t window:5; // voir ordre en rouge dans les slides du feedback
	uint8_t trFlag:1;
	uint8_t type:2;
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
	pkt_t *pkt = (pkt_t *)malloc(sizeof(pkt_t));
	if(pkt == NULL){
		return NULL;
	}


	pkt->type = 0;
	pkt->trFlag = 0;
	pkt->window = 0;
	pkt->seqNum = 0;
	pkt->length = 0;
	pkt->timestamp = 0;
	pkt->crc1 = 0;
	pkt->payload = NULL;
	pkt->crc2 = 0;
	return pkt;
}

void pkt_del(pkt_t *pkt)
{
	if(pkt->payload != NULL)
	free(pkt->payload);

	free(pkt);
}

// pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt)
// {
// 	// packet recu en network byte order ! ->est ce qu'il faut mettre des ntohs un peu partout?
//
// 	size_t err = 0;
// 	pkt_status_code ret = 0;
//
// 	memcpy(pkt, data, 1);
//
// 	if(pkt->type!=1 && pkt->type!=2 && pkt->type!=3) //verifie validite type packet
// 		return E_TYPE;
//
// 	uint8_t seqnum = 0;
// 	uint16_t length = 0;
// 	uint32_t timestamp = 0;
// 	uint32_t crc1 = 0;
// 	uint32_t crc2 = 0;
//
// 	memcpy(&seqnum, data, 1);
// 	ret = pkt_set_seqnum(pkt, seqnum);
// 	if(ret!= PKT_OK)
// 		return E_UNCONSISTENT;
//
// 	memcpy(&length, data, 2);
// 	ret = pkt_set_length(pkt, length);
// 	if(ret != PKT_OK)
// 		return E_UNCONSISTENT;
//
// 	memcpy(&timestamp, data, 4);
// 	ret = pkt_set_timestamp(pkt, timestamp);
// 	if(ret!= PKT_OK)
// 		return E_UNCONSISTENT;
//
//}

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
	int count = 0;
	if( buf==NULL || *len == 0)
	return E_NOMEM;

	if( pkt == NULL)
	return E_UNCONSISTENT;

	if(*len < 12)
	return E_NOMEM;

	if(pkt->length != 0 && *len < (pkt->length)+4+12 ) // si payload du pkt non nul, il faut verifier qu'il y a assez de place dans buf
	{
		return E_NOMEM;
	}

	///
	memcpy(buf, pkt, 8); // copie de window,tr,type, length, timestamp
	pkt_t* temp = (pkt_t *) malloc(8);
	if(temp == NULL)
	return E_NOMEM;
	memcpy(temp, pkt, 8);
	temp->trFlag = 0;
	count = 8;
	char* buftemp = (char *) malloc(8);
	if(buftemp == NULL)
	return E_NOMEM;

	memcpy(buftemp, temp ,8); // copie de window,tr=0,type, seqnum, length, timestamp

	uLong crc = crc32(0L, Z_NULL, 0);
	uLong crc1 = crc32(crc,(Bytef*) temp, count);

	memcpy(buf+count, &crc1, 4);
	count +=4;

	if (pkt_get_length(pkt) != 0) {
		memcpy(buf+count,pkt_get_payload(pkt),pkt_get_length(pkt));
		count += pkt_get_length(pkt);

		uLong crc2 = crc32(crc, (Bytef*) pkt_get_payload(pkt), pkt_get_length(pkt));

		memcpy(buf+count, &crc2, 4);
		count += 4;
	}
	*len = count;
	free(temp);
	free(buftemp);

	return PKT_OK;
}
/* Accesseurs pour les champs toujours presents du paquet.
* Les valeurs renvoyees sont toutes dans l'endianness native
* de la machine!
*/
ptypes_t pkt_get_type  (const pkt_t* pkt)
{
	return pkt->type;
}

uint8_t  pkt_get_tr(const pkt_t* pkt)
{
	return pkt->trFlag;
}

uint8_t  pkt_get_window(const pkt_t* pkt)
{
	return pkt->window;
}

uint8_t  pkt_get_seqnum(const pkt_t* pkt)
{
	return pkt->seqNum;
}

uint16_t pkt_get_length(const pkt_t* pkt)
{
	return ntohs(pkt->length);
}

uint32_t pkt_get_timestamp   (const pkt_t* pkt)
{
	return pkt->timestamp;
}

uint32_t pkt_get_crc1   (const pkt_t* pkt)
{
	return ntohl(pkt->crc1);
}
/* Renvoie le CRC2 dans l'endianness native de la machine. Si
* ce field n'est pas present, retourne 0.
*/
uint32_t pkt_get_crc2   (const pkt_t* pkt)
{
	if(pkt_get_tr(pkt) != 0 || pkt_get_length(pkt) == 0){
		return 0;
	}
	return ntohl(pkt->crc2);
}

/* Renvoie un pointeur vers le payload du paquet, ou NULL s'il n'y
* en a pas.
*/
const char* pkt_get_payload(const pkt_t* pkt)
{
	if(pkt_get_length(pkt) == 0){
		return NULL;
	}
	return pkt->payload;
}

/* Setters pour les champs obligatoires du paquet. Si les valeurs
* fournies ne sont pas dans les limites acceptables, les fonctions
* doivent renvoyer un code d'erreur adapte.
* Les valeurs fournies sont dans l'endianness native de la machine!
*/

pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type)
{
	// + vérifier que sur 2 bit?

	if(type == PTYPE_DATA || type == PTYPE_ACK || type == PTYPE_NACK)
	{
		pkt->type = type;
	}
	else
	return E_TYPE;

	return PKT_OK;
}

pkt_status_code pkt_set_tr(pkt_t *pkt, const uint8_t tr)
{
	if(tr != 0 && tr!= 1){
		return E_TR;
	}
	pkt->trFlag = tr;
	return PKT_OK;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
{
	if(window >= 32)
	{
		pkt->window = 0; // a verifier
		return E_WINDOW;
	}
	pkt->window = window;
	return PKT_OK;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum)
{
	if( seqnum >= 255)
	return E_SEQNUM;

	if(pkt->type==PTYPE_DATA || pkt->type==PTYPE_NACK || pkt->type==PTYPE_ACK)
	{
		pkt->seqNum = seqnum;
		return PKT_OK;
	}
	return E_UNCONSISTENT;

}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length)
{
	if(length >= 512)
	return E_LENGTH;

	pkt->length = htons(length);
	return PKT_OK;
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp)
{
	pkt->timestamp = timestamp;
	return PKT_OK;
}

pkt_status_code pkt_set_crc1(pkt_t *pkt, const uint32_t crc1)
{
	pkt->crc1 = htonl(crc1);
	return PKT_OK;
}

pkt_status_code pkt_set_crc2(pkt_t *pkt, const uint32_t crc2)
{
	if(pkt->payload != NULL && pkt->trFlag == 0)
	{
		pkt->crc2 = htonl(crc2);
		return PKT_OK;
	}
	return E_UNCONSISTENT;

}

/* Defini la valeur du champs payload du paquet.
* @data: Une succession d'octets representants le payload
* @length: Le nombre d'octets composant le payload
* @POST: pkt_get_length(pkt) == length */
pkt_status_code pkt_set_payload(pkt_t *pkt, const char *data, const uint16_t length)
{
	//length &= 0xFFFF;
	// question : length en host byte order??

	if(pkt == NULL)
	return E_UNCONSISTENT; // packet incohérent

	if(pkt->payload != NULL) // il y avait deja un packet
	{
		free(pkt->payload);
		pkt->payload = NULL;
		pkt_status_code err = pkt_set_length(pkt, htons(0));
		if(err != PKT_OK)
		return E_LENGTH;
	}

	if(data == NULL || length == 0) // payload nul
	{
		return E_UNCONSISTENT; // a changer?
	}
	else
	{
		if(length <= 512)
		{
			pkt->payload = (char *)malloc(length*sizeof(char));
			if(pkt->payload == NULL)
			return E_NOMEM; // a verifier

			memcpy((void *)pkt->payload, (void *) data, length);
			pkt_status_code err = pkt_set_length(pkt, htons(length));
			if(err != PKT_OK)
			return E_LENGTH;

			return PKT_OK;
		}
		else // data trop grand
		{
			return E_TR;
		}
	}
	return E_UNCONSISTENT;
}
