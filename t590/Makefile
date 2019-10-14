
all: keys
clean:
	rm -f PK.crt PK.cer PK.esl PK.auth KEK.crt KEK.cer KEK.esl KED.auth db.crt db.cer db.esl db.auth
	rm -f KEK.auth KEK.key PK.key db.key
install: keys
	sudo efi-updatevar -f PK.auth -g `cat GUID.txt` PK
	sudo efi-updatevar -f KEK.auth -g `cat GUID.txt` KEK
	sudo efi-updatevar -f db.auth -g `cat GUID.txt` db

keys: PK.auth KEK.auth db.auth
GUID.txt: MSGUID.txt
	#uuidgen --random > GUID.txt
	cp MSGUID.txt GUID.txt


PK.crt:
	openssl req -newkey rsa:2048 -nodes -keyout PK.key -new -x509 -sha256 -days 3650 -subj "/CN=my Platform Key/" -out PK.crt

PK.cer: PK.crt
	openssl x509 -outform DER -in PK.crt -out PK.cer

PK.esl: PK.crt GUID.txt
	cert-to-efi-sig-list -g "$(< GUID.txt)" PK.crt PK.esl

PK.auth: PK.crt PK.esl GUID.txt 
	sign-efi-sig-list -g "$(< GUID.txt)" -k PK.key -c PK.crt PK PK.esl PK.auth

KEK.crt:
	openssl req -newkey rsa:2048 -nodes -keyout KEK.key -new -x509 -sha256 -days 3650 -subj "/CN=my Key Exchange Key/" -out KEK.crt
KEK.cer: KEK.crt
	openssl x509 -outform DER -in KEK.crt -out KEK.cer
KEK.esl: KEK.crt GUID.txt
	cert-to-efi-sig-list -g "$(< GUID.txt)" KEK.crt KEK.esl
KEK.auth: GUID.txt PK.crt KEK.esl
	sign-efi-sig-list -g "$(< GUID.txt)" -k PK.key -c PK.crt KEK KEK.esl KEK.auth


db.crt:
	openssl req -newkey rsa:2048 -nodes -keyout db.key -new -x509 -sha256 -days 3650 -subj "/CN=my Signature Database key/" -out db.crt

db.cer: db.crt
	openssl x509 -outform DER -in db.crt -out db.cer
db.esl: db.crt GUID.txt
	cert-to-efi-sig-list -g "$(< GUID.txt)" db.crt db.esl
db.auth: GUID.txt KEK.crt db.esl
	sign-efi-sig-list -g "$(< GUID.txt)" -k KEK.key -c KEK.crt db db.esl db.auth
#sudo mkdir -p /etc/secureboot/keys/{PK,KEK,db,dbx}
#sudo cp *.PK.signed /etc/secureboot/keys/PK/
#sudo cp *.KEK.signed /etc/secureboot/keys/KEK/
#sudo cp *.db.signed /etc/secureboot/keys/db/
#sudo sbkeysync --verbose --pk --dry-run
#sudo sbkeysync --pk
