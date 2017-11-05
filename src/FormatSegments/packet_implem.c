#include "packet_interface.h"
#include <zlib.h> /* crc32 */
#include <stdlib.h> /* malloc/calloc */
#include <string.h> /* memcpy */
#include <arpa/inet.h> /* htons */
#include <stdio.h> /* fread */

/*
* Code réalisé par :
* Noemie verstraete - 25021500
* Florence Blondiaux - 06521500
* Version du 05.11.17
*/

struct __attribute__((__packed__)) pkt {
	uint8_t window:5;
	uint8_t trFlag:1;
	uint8_t type:2;
	uint8_t seqNum;
	uint16_t length;
	uint32_t timestamp;
	uint32_t crc1;
	char *payload;
	uint32_t crc2;
};


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


pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt)
{

	uLong crc = crc32(0L, Z_NULL, 0);
	int taille_payl = 0;
	int err = 0;

	if(data == NULL || len < 12 || pkt == NULL)
		return E_UNCONSISTENT;

	if( len > 12){
		int temp = len-16;
		if ( temp > 0){
			taille_payl = temp;
		}
		else {
			return E_UNCONSISTENT;
		}
	}

	memcpy(pkt, data, 12); // copie des 12 prem bytes (en network BO) (jusqu'a crc1 y compris)


	// Préparation pour la vérification de crc1
	if(pkt_get_tr(pkt) == 1 )
	{
		return E_TR;
	}
	// mettre length en host byte order dans pkt
	if(pkt_set_length(pkt,ntohs(pkt_get_length(pkt))) != PKT_OK){ // On met length en host byte order
		return err;
	}
	if(pkt_set_crc1(pkt,ntohl(pkt_get_crc1(pkt))) != PKT_OK){ // On met le crc1 en host byte order
		return err;
	}

	if(crc32(crc,(Bytef*) data, 8 ) != pkt_get_crc1(pkt) )
	{
		fprintf(stderr, "Le crc1 calculé n'est pas le meme que celui qui a été reçu\n");
		return E_CRC;
	}
	// Verification length
	if( taille_payl != pkt_get_length(pkt)){
		return E_LENGTH;
	}
	// verification type
	if(pkt->type!=1 && pkt->type!=2 && pkt->type!=3){ //verifie validite type packet
		return E_TYPE;
	}

	if(taille_payl != 0){
		char* payload = (char *) malloc(taille_payl);
		if(payload == NULL) {
			return E_NOMEM;
		}
		memcpy(payload, data+12, taille_payl); // copie du payload dans payload
		uint32_t crc2;
		memcpy(&crc2, data+12+taille_payl, 4);
		crc2 = ntohl(crc2);


		if( crc32(crc, (Bytef *) data+12, taille_payl) != crc2){ // verifie que le payload a la bonne valeur du crc2
			fprintf(stderr, "Attention, le crc2 n'est pas le même que ce que l'on calcule\n" );
			return E_CRC;
		}
		err = pkt_set_payload(pkt,data+12, taille_payl);
		if(err != PKT_OK){
			return err;
		}
		err = pkt_set_crc2(pkt,crc2);
		if(err != PKT_OK){
			return err;
		}

	}
	return PKT_OK;
}

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
	int count = 0;
	if( buf==NULL || *len == 0){
		return E_NOMEM;
	}

	if( pkt == NULL)
		return PKT_OK;

	if(*len < 12){
			return E_NOMEM;
	}

	if(pkt_get_length(pkt) != 0 && *(len) < (size_t) (pkt_get_length(pkt))+4+12 ) // si payload du pkt non nul, il faut verifier qu'il y a assez de place dans buf
	{
		return E_NOMEM;
	}

	memcpy(buf, pkt, 2); // memcpy de wind, ty, type et seqnum
	count = count+2;
	uint16_t l =  htons(pkt_get_length(pkt));
	memcpy(buf+count,& l, 2); // copie de length en network byte order
	count = count +2;
	//copie de timestamp
	uint32_t t = pkt_get_timestamp(pkt);
	memcpy(buf+count,&t, 4);
	count = count +4;

	char buftemp[8];
	// copie du packet avec tr=0 dans un char*
	memcpy(buftemp, buf, 8);
	pkt_t *temp = (pkt_t *)buftemp;
	// copie de char* buf à pkt_t temp pour mettre tr à 0
	if(pkt_set_tr(temp, 0) != PKT_OK){
		return E_TR;
	}


	uLong crc = crc32(0L, Z_NULL, 0);
	//calcul du crc1 du packet en network
	uLong crc1 = crc32(crc,(Bytef*) buftemp, count);

	// crc1 [host] --> crc1[network]
	crc1 = htonl(crc1);

	// mise crc1 en network byte order dans pkt
	memcpy(buf+count, &crc1, 4*sizeof(char));
	count = count+4;
	if (pkt_get_length(pkt) != 0)
	{
		memcpy(buf+count,  pkt_get_payload(pkt), pkt_get_length(pkt));
		count = count+pkt_get_length(pkt);

		uLong crc2 = crc32(crc, (Bytef*) pkt_get_payload(pkt), pkt_get_length(pkt));
		crc2 = htonl(crc2);
		memcpy(buf+count, &crc2, 4);
		count = count+4;


	}
	*len = count;
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
	return pkt->length;
}

uint32_t pkt_get_timestamp   (const pkt_t* pkt)
{
	return pkt->timestamp;
}

uint32_t pkt_get_crc1   (const pkt_t* pkt)
{
	return pkt->crc1;
}
/* Renvoie le CRC2 dans l'endianness native de la machine. Si
* ce field n'est pas present, retourne 0.
*/
uint32_t pkt_get_crc2   (const pkt_t* pkt)
{
	if(pkt_get_tr(pkt) != 0 || pkt_get_length(pkt) == 0){
		return 0;
	}
	return pkt->crc2;
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
		pkt->window = 0;
		return E_WINDOW;
	}
	pkt->window = window;
	return PKT_OK;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum)
{
	if( seqnum > 255)
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
	if(length > 512)
	return E_LENGTH;

	pkt->length = length;
	return PKT_OK;
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp)
{
	pkt->timestamp = timestamp;
	return PKT_OK;
}

pkt_status_code pkt_set_crc1(pkt_t *pkt, const uint32_t crc1)
{
	pkt->crc1 = crc1;
	return PKT_OK;
}

pkt_status_code pkt_set_crc2(pkt_t *pkt, const uint32_t crc2)
{
	if(pkt->payload != NULL && pkt->trFlag == 0)
	{
		pkt->crc2 = crc2;
		return PKT_OK;
	}
	return E_UNCONSISTENT;

}

pkt_status_code pkt_set_payload(pkt_t *pkt, const char *data, const uint16_t length)
{
	if(pkt == NULL)
	return E_UNCONSISTENT; // packet incohérent

	if(pkt->payload != NULL) // il y avait deja un packet
	{
		free(pkt->payload);
		pkt->payload = NULL;
		pkt_status_code err = pkt_set_length(pkt, 0);
		if(err != PKT_OK)
		return E_LENGTH;
	}

	if(data == NULL && length == 0) // payload nul
	{
		pkt->payload = NULL;
		int err = pkt_set_length(pkt, 0);
		return err; // Si tout c'est bien passé : PKT_OK
	}

	if(data == NULL || length == 0){
		return E_UNCONSISTENT;
	}
	else
	{
		if(length <= 512)
		{
			pkt->payload = (char *)malloc(length*sizeof(char));
			if(pkt->payload == NULL)
				return E_NOMEM; // a verifier

			memcpy((void *)pkt->payload, (void *) data, length);
			pkt_status_code err = pkt_set_length(pkt, length);
			return err; // Si tout ok  : PKT_OK
		}
		else // data trop grand
		{
			return E_UNCONSISTENT;
		}
	}
	return E_UNCONSISTENT;
}
