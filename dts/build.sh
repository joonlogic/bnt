dtc -@ -I dts -O dtb -o spi-gpio-cs.dtbo spi-gpio-cs-overlay-joon.dts           
sudo cp spi-gpio-cs.dtbo /boot/overlays
