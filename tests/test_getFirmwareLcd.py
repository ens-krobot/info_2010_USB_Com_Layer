#!/usr/bin/env python
# coding=utf-8

# Test test_getFirmwareLcd
# Ce test doit effacer l'écran LCD puis après un bref instant y afficher les
# infos du firmware de la carte.

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

        USBCom_InterruptWrite(handle, pack('9b', 1, 0, CMD_GET, 0, 0, 0, 0, 0, GET_BOARD_INFO), 64)
        nbytes, buff1 = USBCom_InterruptRead(handle, pack('4b', 1, 0, CMD_RESPOND, 0), 64, 0xF)

        if nbytes > 0:
            # On n'affiche pas le premier octet qui est l'octet de réponse (CMD_RESPOND)
            res = buff1[8:len(buff1)]
            res = res[0:res.find("\0")].split("\n")

            USBCom_InterruptWrite(handle, pack('9b', 0, 0, CMD_SEND, 0, 0, 0, 0, 0, 0x01) + res[0], 64)
            USBCom_InterruptWrite(handle, pack('9b', 0, 0, CMD_SEND, 0, 0, 0, 0, 0, 0x02) + res[1], 64)

            USBCom_InterruptWrite(handle, pack('9b', 1, 0, CMD_GET, 0, 0, 0, 0, 0, GET_FIRMWARE_BUILD), 64)
            nbytes, buff2 = USBCom_InterruptRead(handle, pack('4b', 1, 0, CMD_RESPOND, 0), 64, 0xF)

            if nbytes > 0:
                res = buff2[8:len(buff2)]
                res = res[0:res.find("\0")]

                USBCom_InterruptWrite(handle, pack('9b', 0, 0, CMD_SEND, 0, 0, 0, 0, 0, 0x03) + res, 64)
                # On positionne le curseur sur la dernière ligne
                USBCom_InterruptWrite(handle, pack('9b', 0, 0, CMD_SEND, 0, 0, 0, 0, 0, 0x04), 64)

            errn = USBCom_GetLastError(handle)

            # Si le nom de la carte apparait dans le résultat, c'est forcément qu'on
            # a pu récupérer les infos du firmware, donc a priori ça fonctionne
            if errn == USBCOM_NOEXCEPTION_ERR and buff1.find("Carte d'essais") == 8:
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
