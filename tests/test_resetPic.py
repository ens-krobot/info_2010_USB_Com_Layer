#!/usr/bin/env python
# coding=utf-8

# Test test_resetPic
# Tente de faire un reset du PIC puis de récupérer la connexion. On doit observer
# une discontinuité dans le clignotement des LEDs de la carte (qui repasse en fait
# la phase d'énumération).

import sys
sys.path.append('../src/')
import time
from struct import *
from USB_Com_Layer import *

def main():
    handle = USBCom_Create()
    res = USBCom_Open(handle, USB_VID, USB_PID_USB_DEV_BOARD)

    if res:
        # Envoie la commande de RESET et ferme la connexion
        USBCom_InterruptWrite(handle, pack('4b', 0, 0, CMD_RESET, 0), 64)
        USBCom_Close(handle)

        # Attends un peu pour être sûr que le périphérique ait eu le temps de se déconnecter
        time.sleep(1)

        # On tente de se reconnecter tant que l'on ne trouve pas le périphérique
        # Si on obtient une erreur différente de USBCOM_DEVICE_NOT_FOUND_ERR, c'est que
        # la connexion ne peut pas se faire pour une autre raison => on abandonne
        res = 0
        while res != 1:
            res = USBCom_Open(handle, USB_VID, USB_PID_USB_DEV_BOARD)
            errn = USBCom_GetLastError(handle)

            if errn != USBCOM_DEVICE_NOT_FOUND_ERR:
                break

    if res:
        USBCom_InterruptWrite(handle, pack('9b', 1, 0, CMD_GET, 0, 0, 0, 0, 0, GET_RESET_SOURCE), 64)
        nbytes, buff = USBCom_InterruptRead(handle, pack('4b', 1, 0, CMD_RESPOND, 0), 64, 0xF)

        if nbytes > 0:
            errn = USBCom_GetLastError(handle)

            # La réponse RESET_SOURCE_RI qui signifie que le RESET est software
            if errn == USBCOM_NOEXCEPTION_ERR and buff[8] == pack('b', RESET_SOURCE_RI):
                print("===> Test OK")
            else:
                print("===> Echec test")
        else:
            print("Réponse de taille nulle.")
    else:
        errn = USBCom_GetLastError(handle)

        if errn == USBCOM_DEVICE_NOT_FOUND_ERR:
            print("Périphérique non connecté.")
        else:
            print("Erreur N°%d." % errn)

    USBCom_Close(handle)
    USBCom_Destroy(handle)

if __name__ == '__main__':
    main()
