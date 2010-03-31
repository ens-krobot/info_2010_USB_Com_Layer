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

/* On inclut l'impl�mentation correspondante au syst�me utilis� */
#if _WIN32
	#include "msw/usb_com_layer.h"
#else
	#include "unix/usb_com_layer.h"
#endif

/* Note importante : toutes les fonctions doivent �tre thread safe. */

/**
 * Cr�� un handle et alloue la m�moire associ�e
 * @return	handle			handle de la connexion
*/
TUSBHandle* USBCom_Create();

/**
 * D�truit le handle et lib�re la m�moire
 * @param	handle			handle de la connexion
*/
void USBCom_Destroy(TUSBHandle* handle);

/**
 * Ouvre une connexion au p�riph�rique identifi� par un Product ID et un Vendor ID.
 * @param	vid				Vendor ID
 * @param	pid				Product ID
 * @return	result			renvoie TRUE en cas de succ�s, FALSE sinon
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
 * Envoie un message au p�riph�rique distant.
 *
 * @param	handle			handle de la connexion
 * @param	buffer			donn�es � envoyer
 * @param	size			taille des donn�es � envoyer
 * @return	write			nombre d'octets envoy�s
*/
UINT USBCom_InterruptWrite(TUSBHandle* handle, BYTE* const buffer, UINT const size, UINT const timeout);

/**
 * Re�oit un message du p�riph�rique distant.
 * Cette fonction peut prendre comme argument un masque, pour filtrer le type de message que l'on veut recevoir.
 *
 * @note Cette fonction est bloquante, jusqu'� ce qu'un message r�pondant au masque soit disponible.
 * @param	handle			handle de la connexion
 * @param	buffer			donn�es re�ues
 * @param	size			taille des donn�es � recevoir
 * @param	mask			pour les bits � 1 du masque, les octets re�us doivent correspondrent aux octets
 *							d�j� pr�sents dans buffer (exemple : si mask vaut 2, alors le 2�me octet re�us doit
 *							�tre �gal � buffer[1])
 * @return	read			nombre d'octets lus
*/
UINT USBCom_InterruptRead(TUSBHandle* handle, BYTE* buffer, UINT const size, UINT const mask, UINT const timeout);

/**
 * Renvoie la derni�re erreur, ou USBCOM_NOEXCEPTION_ERR si pas d'erreur.
 * L'appel � cette fonction efface l'erreur courante (un 2�me appel cons�cutif produira USBCOM_NOEXCEPTION_ERR).
 * @param	handle			handle de la connexion
 * @return	error			derni�re erreur produite, USBCOM_NOEXCEPTION_ERR si pas d'erreur
*/
TUSBException USBCom_GetLastError(TUSBHandle* handle);

#endif /* _USB_COM_LAYER_H */
