#!/usr/bin/env python
# coding=utf-8

# Test test_getFirmware
# Teste la communication USB avec la carte d'essais et affiche les infos du firmware
# en cas de succès.

import sys
sys.path.append('../src/')
from struct import *
from USB_Com_Layer import *

def main():
    handle = USBCom_Create()
    res = USBCom_Open(handle, USB_VID, USB_PID_USB_DEV_BOARD)

    if res:
        USBCom_InterruptWrite(handle, pack('9b', 1, 0, CMD_GET, 0, 0, 0, 0, 0, GET_BOARD_INFO), 64)
        nbytes, buff1 = USBCom_InterruptRead(handle, pack('4b', 1, 0, CMD_RESPOND, 0), 64, 0xF)

        if nbytes > 0:
            res = buff1[8:len(buff1)]
            res = res[0:res.find("\0")]
            print(res)

            USBCom_InterruptWrite(handle, pack('9b', 1, 0, CMD_GET, 0, 0, 0, 0, 0, GET_FIRMWARE_BUILD), 64)
            nbytes, buff2 = USBCom_InterruptRead(handle, pack('4b', 1, 0, CMD_RESPOND, 0), 64, 0xF)

            if nbytes > 0:
                res = buff2[8:len(buff2)];
                res = res[0:res.find("\0")]
                print(res)

            print("")

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
