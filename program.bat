avrdude -v -c avrispmkII -p m32u2 -P usb -b 19200 -U "hfuse:w:0xd9:m"
avrdude -v -c avrispmkII -p m32u2 -P usb -b 19200 -U "lfuse:w:0xef:m"
avrdude -v -c avrispmkII -p m32u2 -P usb -b 19200 -U "flash:w:.\program\Argb\Argb\Release\Argb.hex"