#!/bin/bash
ls /boot/vmlinuz* | xargs -I{} -n1 sudo sbsign --key test-key.rsa --cert test-cert.pem --output {} {}
sudo sbsign --key test-key.rsa --cert test-cert.pem \
        --output grubx64.efi /boot/efi/efi/ubuntu/grubx64.efi
sudo cp /boot/efi/efi/ubuntu/grubx64.efi{,.bak}
sudo cp grubx64.efi /boot/efi/efi/ubuntu/

