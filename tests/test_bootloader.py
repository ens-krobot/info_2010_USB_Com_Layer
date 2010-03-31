#!/usr/bin/env python
# coding=utf-8

# Test test_bootloader
# Tente de faire un reset du PIC en mode bootloader. Pour que le test soit réussi, il
# faut que seule la première LED sur la carte clignote et la carte doit être
# inaccessible depuis l'ordinateur.
# Appuyez sur le bouton de RESET pour réinitialiser la carte et quitter le mode bootloader.

import sys
sys.path.append('../src/')
import time
from struct import *
from USB_Com_Layer import *

def main():
    handle = USBCom_Create()
    res = USBCom_Open(handle, USB_VID, USB_PID_USB_DEV_BOARD)

    if res:
        # Envoie la commande de RESET en mode bootloader
        USBCom_InterruptWrite(handle, pack('4b', 0, 0, CMD_BOOTLOADER, 0), 64)
        errn = USBCom_GetLastError(handle)

        if errn == USBCOM_NOEXCEPTION_ERR:
            # Attends un peu pour être sûr que le périphérique ait eu le temps de se déconnecter
            time.sleep(1)

            # Puisque l'on est sensé être en mode bootloader, le requête suivante doit échouer !
            USBCom_InterruptWrite(handle, pack('4b', 0, 0, CMD_RESET, 0), 64)
            errn = USBCom_GetLastError(handle)

        # On DOIT avoir une erreur d'écriture !
        if errn == USBCOM_DEVICE_NOT_OPENED_ERR:
            print("===> Test OK")
        else:
            print("===> Echec test")
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
