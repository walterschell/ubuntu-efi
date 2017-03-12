#!/bin/bash
guid=$(uuidgen)
echo $guid
sbsiglist --owner $guid --type x509 --output test-cert.der.siglist test-cert.der
for n in PK KEK db
do
	sbvarsign --key test-key.rsa --cert test-cert.pem \
	--output test-cert.der.siglist.$n.signed \
	$n test-cert.der.siglist
done

sudo mkdir -p /etc/secureboot/keys/{PK,KEK,db,dbx}
sudo cp *.PK.signed /etc/secureboot/keys/PK/
sudo cp *.KEK.signed /etc/secureboot/keys/KEK/
sudo cp *.db.signed /etc/secureboot/keys/db/
sudo sbkeysync --verbose --pk --dry-run
sudo sbkeysync
