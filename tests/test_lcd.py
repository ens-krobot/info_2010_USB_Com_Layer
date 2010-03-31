#!/usr/bin/env python
# coding=utf-8

# Test test_lcd
# Ce test doit effacer l'écran LCD puis après un bref instant y afficher la chaine de 
# caractère "Hello World!" avec un balayage faiblement perceptible.

import sys
sys.path.append('../src/')
import time
from struct import *
from USB_Com_Layer import *

def main():
    handle = USBCom_Create()
    res = USBCom_Open(handle, USB_VID, USB_PID_USB_DEV_BOARD)

    if res:
        # Efface l'écran LCD
        # Tous les sleep() de ce programme ne sont absolument pas nécessaires et sont
        # présents uniquement pour obtenir un meilleur effet à l'affichage !
        USBCom_InterruptWrite(handle, pack('10b', 0, 0, CMD_SEND, 0, 0, 0, 0, 0, 0x1B, 0x43), 64)
        time.sleep(0.2)

        # On peut bien sûr envoyer tout d'un coup, mais on obtiendrait pas le joli effet
        # de balayage...
        for c in "Hello world!":
            USBCom_InterruptWrite(handle, pack('8b', 0, 0, CMD_SEND, 0, 0, 0, 0, 0) + c, 64)
            time.sleep(0.02)

        # Si une erreur survient dans la boucle, elle reste conservée même si les requêtes
        # suivantes ont réussi.
        errn = USBCom_GetLastError(handle)

        if errn == USBCOM_NOEXCEPTION_ERR:
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
