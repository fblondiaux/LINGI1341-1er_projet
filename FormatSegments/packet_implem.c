#include "packet_interface.h"
# include <zlib.h> /* crc32 */
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

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt)
{
	// packet recu en network byte order ! ->est ce qu'il faut mettre des ntohs un peu partout?

	size_t err = 0;
	pkt_status_code ret = 0;

	memcpy(pkt, data, 1);

	if(pkt->type!=1 && pkt->type!=2 && pkt->type!=3) //verifie validite type packet
		return E_TYPE;

	uint8_t seqnum = 0;
	uint16_t length = 0;
	uint32_t timestamp = 0;
	uint32_t crc1 = 0;
	uint32_t crc2 = 0;

	memcpy(&seqnum, data, 1);
	ret = pkt_set_seqnum(pkt, seqnum);
	if(ret!= PKT_OK)
		return E_UNCONSISTENT;

	memcpy(&length, data, 2);
	ret = pkt_set_length(pkt, length);
	if(ret != PKT_OK)
		return E_UNCONSISTENT;

	memcpy(&timestamp, data, 4);
	ret = pkt_set_timestamp(pkt, timestamp);
	if(ret!= PKT_OK)
		return E_UNCONSISTENT;

	// pas fini




}

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
	int count = 0;
	if( *len < 12) // verifie qu'il y a assez de place dans buffer pour mettre tout jusqu'au crc1
	{
		return E_NOMEM;
	}
	//Tous les commentaires par la suite sont à supprimer quand tu les auras lu :)
	// Quand tu fait ça, il reprends la ou il en était, il ne recopie pas tout depuis
	// le début du coup on aurait window tr type window tr type ?
	// Et buf il serait réécrasé non ?
	//memcpy(buf, pkt, sizeof(uint8_t)); // copie de window,tr,type
	//memcpy(buf, pkt, sizeof(uint8_t)); // copie seqnum
	// Ma proposition :
	memcpy(buf,pkt,sizeof(uint16_t)); // Copie de window, tr, type, seqnum.
	count = count + 2;

	uint16_t length = htons(pkt_get_length(pkt));
	//F: On peux faire buf+count pour etre au bon endroit ?
	memcpy(buf+count, &length, sizeof(uint16_t)); // copie de lentgh
	count = count+2;
	//F: Pareil, pkt+count pour être au bon endroit dans la structure ?
	memcpy(buf+count, pkt+count, sizeof(uint32_t)); // sans endianness particuliere
	count = count+4;


	// N: pas certaine que c'est juste
	// F= Ça ne compile pas car pkt est const. En soi c'est logique mais comme tr
	// Est un des premiers trucs a mettre dans le buffer, on le met quand à 0
	pkt_set_tr(pkt, 0); //Vérifier la valeur de retour ?
// char *inter = NULL;
// F: CA fait pas une segfalt ça , un memcpy sur NULL?
//N	memcpy(inter, pkt, 4);
//N 	uLong crc1 = crc32(pkt_get_crc1(pkt), inter, 4);
		// De ce que j'ai compris il calcule un crc sur les donnees passées dans le
		// Deuxiemme argument.
				//http://netbsd.gw.com/cgi-bin/man-cgi?zlib+3+NetBSD-current
				//uLong crc32(uLong crc, const Bytef *buf, uInt len);
				//The crc32() function updates a running CRC with the bytes
				//buf[0..len-1] and returns the updated CRC.  If buf is NULL, this
				//function returns the required initial value for the CRC.  Pre-
				//and post-conditioning (one's complement) is performed within this
				//function so it shouldn't be done by the application.
	uLong crc1 = crc32(pkt_get_crc1(pkt),(Bytef*) buf, count);

	/* erreur à la compilation : "expected ‘const Bytef *
	{aka const unsigned char *}’ but argument is of
	type ‘const pkt_t * {aka const struct pkt *}’"
	si je ne caste pas en char* */

	// pas fini
	// F: pour la suite je dirais qu'il faut mettre le payload ( de nouveau comment)
	// changer tr si on s'appercoit qu'il n'y à pas assez de place. Et quand payload est ajouté
	// Alors on fait crc
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
