# Génère l'interface Python avec SWIG
bin:= bin/Release
obj:= obj/Release/src
src:= src
LIBUSB := $(shell pkg-config --cflags libusb-1.0)

msw :
	swig -python $(src)/usb_com_layer.i; \
	gcc -Wall -std=c99 -pedantic -shared $(src)/msw/usb_com_layer.c $(src)/usb_com_layer_wrap.c -o $(bin)/_USB_Com_Layer.pyd \
		-I C:/Python26/include \
		-L C:/Python26/libs \
		-l python26 \
		-l setupapi; \
	cp $(bin)/_USB_Com_Layer.pyd $(src)/_USB_Com_Layer.pyd

unix-libhid :
	mkdir -p $(obj)/unix
	sed -i -e 's:unix/usb_com_layer.h:unix-libhid/usb_com_layer.h:' $(src)/usb_com_layer.h
	swig -python $(src)/usb_com_layer.i; \
	gcc -Wall -fpic -c $(src)/unix-libhid/usb_com_layer.c -o $(obj)/unix/usb_com_layer.o; \
	gcc -fpic -c $(src)/usb_com_layer_wrap.c -o $(obj)/usb_com_layer_wrap.o \
		-I/usr/include/python2.6 \
		-I/usr/lib/python2.6/config; \
	ld -shared $(obj)/unix/usb_com_layer.o $(obj)/usb_com_layer_wrap.o -o $(bin)/_USB_Com_Layer.so \
		-l hid; \
	cp $(bin)/_USB_Com_Layer.so $(src)/_USB_Com_Layer.so

unix :
	# Fonctionne après avoir installé libusb-1.0 et exécuté la commande :
	# ln -s /usr/local/lib/libusb-1.0.so.0 /usr/lib/libusb-1.0.so.0
	sed -i -e 's:unix-libhid/usb_com_layer.h:unix/usb_com_layer.h:' $(src)/usb_com_layer.h
	swig -python $(src)/usb_com_layer.i; \
	gcc -Wall -std=c99 -pedantic -fpic -shared $(src)/unix/usb_com_layer.c $(src)/usb_com_layer_wrap.c -o $(bin)/_USB_Com_Layer.so \
		$(LIBUSB) \
		-I/usr/include/python2.6 \
		-I/usr/lib/python2.6/config \
		-l usb-1.0; \
	cp $(bin)/_USB_Com_Layer.so $(src)/_USB_Com_Layer.so

unix-install :
	cp usbcomlayer.rules /lib/udev/rules.d/99-usbcomlayer.rules

unix-uninstall :
	unlink /lib/udev/rules.d/99-usbcomlayer.rules

clean :
	rm -f $(obj)/*.o $(obj)/msw/*.o $(obj)/unix/*.o
	rm -f $(src)/*.so $(src)/*.pyd
	rm -f $(src)/usb_com_layer_wrap.c $(src)/USB_Com_Layer.py $(src)/USB_Com_Layer.pyc
	rm -Rf $(obj)/unix
