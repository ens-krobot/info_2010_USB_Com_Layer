#ifndef _USB_COM_LAYER_H
#define _USB_COM_LAYER_H

typedef enum {
	USBCOM_NOEXCEPTION_ERR = 0,
	USBCOM_DEVICE_NOT_OPENED_ERR,
	USBCOM_DEVICE_NOT_FOUND_ERR,
	USBCOM_DEVICE_READ_ERR,
	USBCOM_DEVICE_WRITE_ERR,
	USBCOM_DEVICE_OPEN_ERR,
	USBCOM_MEMORY_ALLOC_ERR,
	USBCOM_UNIMPLEMENTED_ERR
} TUSBException;

/* On inclut l'implémentation correspondante au système utilisé */
#if _WIN32
	#include "msw/usb_com_layer.h"
#else
	#include "unix/usb_com_layer.h"
#endif

/* Note importante : toutes les fonctions doivent être thread safe. */

/**
 * Créé un handle et alloue la mémoire associée
 * @return	handle			handle de la connexion
*/
TUSBHandle* USBCom_Create();

/**
 * Détruit le handle et libère la mémoire
 * @param	handle			handle de la connexion
*/
void USBCom_Destroy(TUSBHandle* handle);

/**
 * Ouvre une connexion au périphérique identifié par un Product ID et un Vendor ID.
 * @param	vid				Vendor ID
 * @param	pid				Product ID
 * @return	result			renvoie TRUE en cas de succès, FALSE sinon
*/
BOOL USBCom_Open(TUSBHandle* handle, WORD const vid, WORD const pid);

/**
 * Ferme la connexion.
 * @param	handle			handle de la connexion
*/
void USBCom_Close(TUSBHandle* handle);

/**
 * Etat de la connexion.
 * @param	handle			handle de la connexion
 * @return	result			renvoie TRUE si la connexion est valide, FALSE sinon
*/
BOOL USBCom_IsOpened(TUSBHandle* handle);

/**
 * Envoie un message au périphérique distant.
 *
 * @param	handle			handle de la connexion
 * @param	buffer			données à envoyer
 * @param	size			taille des données à envoyer
 * @return	write			nombre d'octets envoyés
*/
UINT USBCom_InterruptWrite(TUSBHandle* handle, BYTE* const buffer, UINT const size, UINT const timeout);

/**
 * Reçoit un message du périphérique distant.
 * Cette fonction peut prendre comme argument un masque, pour filtrer le type de message que l'on veut recevoir.
 *
 * @note Cette fonction est bloquante, jusqu'à ce qu'un message répondant au masque soit disponible.
 * @param	handle			handle de la connexion
 * @param	buffer			données reçues
 * @param	size			taille des données à recevoir
 * @param	mask			pour les bits à 1 du masque, les octets reçus doivent correspondrent aux octets
 *							déjà présents dans buffer (exemple : si mask vaut 2, alors le 2ème octet reçus doit
 *							être égal à buffer[1])
 * @return	read			nombre d'octets lus
*/
UINT USBCom_InterruptRead(TUSBHandle* handle, BYTE* buffer, UINT const size, UINT const mask, UINT const timeout);

/**
 * Renvoie la dernière erreur, ou USBCOM_NOEXCEPTION_ERR si pas d'erreur.
 * L'appel à cette fonction efface l'erreur courante (un 2ème appel consécutif produira USBCOM_NOEXCEPTION_ERR).
 * @param	handle			handle de la connexion
 * @return	error			dernière erreur produite, USBCOM_NOEXCEPTION_ERR si pas d'erreur
*/
TUSBException USBCom_GetLastError(TUSBHandle* handle);

#endif /* _USB_COM_LAYER_H */
