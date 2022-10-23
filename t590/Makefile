
all: keys signed-shim
clean:
	rm -f PK.crt PK.cer PK.esl PK.auth KEK.crt KEK.cer KEK.esl KED.auth db.crt db.cer db.esl db.auth
	rm -f KEK.auth KEK.key PK.key db.key shimx64-signed.efi mmx64-signed.efi 

install: install-shim install-keys
install-keys: keys binaries
	sudo efi-updatevar -f PK.auth -g `cat GUID.txt` PK
	sudo efi-updatevar -f KEK.auth -g `cat GUID.txt` KEK
	sudo efi-updatevar -f db.auth -g `cat GUID.txt` db

install-shim: signed-shim signed-mokmgr
	sudo cp shimx64-signed.efi /boot/efi/efi/ubuntu/shimx64.efi
	sudo cp mmx64-signed.efi /boot/efi/efi/ubuntu/mmx64.efi

keys: PK.auth KEK.auth db.auth
GUID.txt: MSGUID.txt
	#uuidgen --random > GUID.txt
	cp MSGUID.txt GUID.txt
/boot/efi/efi/ubuntu/shimx64.efi.bak:
	sudo cp /boot/efi/efi/ubuntu/shimx64.efi /boot/efi/efi/ubuntu/shimx64.efi.bak

/boot/efi/efi/ubuntu/mmx64.efi.bak:
	sudo cp /boot/efi/efi/ubuntu/mmx64.efi /boot/efi/efi/ubuntu/mmx64.efi.bak



signed-shim: shimx64-signed.efi
shimx64-signed.efi: db.crt db.key binaries /boot/efi/efi/ubuntu/shimx64.efi.bak
	sudo sbsign --key db.key --cert db.crt --output shimx64-signed.efi /boot/efi/efi/ubuntu/shimx64.efi.bak


signed-mokmgr: mmx64-signed.efi
mmx64-signed.efi: db.crt db.key binaries /boot/efi/efi/ubuntu/mmx64.efi.bak
	sudo sbsign --key db.key --cert db.crt --output mmx64-signed.efi /boot/efi/efi/ubuntu/mmx64.efi.bak


PK.crt:
	openssl req -newkey rsa:2048 -nodes -keyout PK.key -new -x509 -sha256 -days 3650 -subj "/CN=my Platform Key/" -out PK.crt

PK.cer: PK.crt
	openssl x509 -outform DER -in PK.crt -out PK.cer

PK.esl: PK.crt GUID.txt binaries
	cert-to-efi-sig-list -g "$(< GUID.txt)" PK.crt PK.esl

PK.auth: PK.crt PK.esl GUID.txt binaries
	sign-efi-sig-list -g "$(< GUID.txt)" -k PK.key -c PK.crt PK PK.esl PK.auth

KEK.crt:
	openssl req -newkey rsa:2048 -nodes -keyout KEK.key -new -x509 -sha256 -days 3650 -subj "/CN=my Key Exchange Key/" -out KEK.crt
KEK.cer: KEK.crt
	openssl x509 -outform DER -in KEK.crt -out KEK.cer
KEK.esl: KEK.crt GUID.txt binaries
	cert-to-efi-sig-list -g "$(< GUID.txt)" KEK.crt KEK.esl
KEK.auth: GUID.txt PK.crt KEK.esl binaries
	sign-efi-sig-list -g "$(< GUID.txt)" -k PK.key -c PK.crt KEK KEK.esl KEK.auth


db.crt:
	openssl req -newkey rsa:2048 -nodes -keyout db.key -new -x509 -sha256 -days 3650 -subj "/CN=my Signature Database key/" -out db.crt

db.cer: db.crt
	openssl x509 -outform DER -in db.crt -out db.cer
db.esl: db.crt GUID.txt binaries
	cert-to-efi-sig-list -g "$(< GUID.txt)" db.crt db.esl
db.auth: GUID.txt KEK.crt db.esl binaries
	sign-efi-sig-list -g "$(< GUID.txt)" -k KEK.key -c KEK.crt db db.esl db.auth
binaries:
	$(MAKE) -C efitools-1.9.2 sign-efi-sig-list cert-to-efi-sig-list efi-updatevar
