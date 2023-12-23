sudo dd if=/dev/zero of=/dev/ram0 bs=512 count=1048576
sudo ln -s /dev/ram0 /dev/ramdisk
