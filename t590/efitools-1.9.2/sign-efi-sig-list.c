/*
 * Copyright 2012 <James.Bottomley@HansenPartnership.com>
 *
 * see COPYING file
 */
#include <stdint.h>
#define __STDC_VERSION__ 199901L
#include <efi.h>
#ifdef CONFIG_arm
/* FIXME:
 * arm efi leaves a visibilit pragma pushed that won't work for
 * non efi programs, so eliminate it */
#pragma GCC visibility pop
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

#include <variables.h>
#include <guid.h>
#include <version.h>
#include <openssl_sign.h>

static void
usage(const char *progname)
{
	printf("Usage: %s [-r] [-m] [-a] [-g <guid>] [-o] [-t <timestamp>] [-i <infile>] [-c <crt file>] [-k <key file>] [-e <engine>] <var> <efi sig list file> <output file>\n", progname);
}

static void
help(const char *progname)
{
	usage(progname);
	printf("Produce an output file with an authentication header for direct\n"
	       "update to a secure variable.  This output may be signed by the usual keys directly\n"
	       "or may be split for external signing using the -o and -i options.\n\n"
	       "Options:\n"
	       "\t-r               the certificate is rsa2048 rather than x509 [UNIMPLEMENTED]\n"
	       "\t-m               Use a monotonic count instead of a timestamp [UNIMPLEMENTED]\n"
	       "\t-a               Prepare the variable for APPEND_WRITE rather than replacement\n"
	       "\t-o               Do not sign, but output a file of the exact bundle to be signed\n"
	       "\t-t <timestamp>   Use <timestamp> as the timestamp of the timed variable update\n"
	       "\t                 If not present, then the timestamp will be taken from system\n"
	       "\t                 time.  Note you must use this option when doing detached\n"
	       "\t                 signing otherwise the signature will be incorrect because\n"
	       "\t                 of timestamp mismatches.\n"
	       "\t-i <infile>        take a detached signature (in PEM format) of the bundle\n"
	       "\t                 produced by -o and complete the creation of the update\n"
	       "\t-g <guid>        Use <guid> as the signature owner GUID\n"
	       "\t-c <crt>         <crt> is the file containing the signing certificate in PEM format\n"
	       "\t-k <key>         <key> is the file containing the key for <crt> in PEM format\n"
	       "\t-e <engine>      Use openssl engine <engine> for the private key\n"
	       );
}

int
main(int argc, char *argv[])
{
	char *certfile = NULL, *efifile, *keyfile = NULL, *outfile,
		*str, *signedinput = NULL, *timestampstr = NULL;
	void *out;
	const char *progname = argv[0];
	char *engine = NULL;
	unsigned char *sigbuf;
	int rsasig = 0, monotonic = 0, varlen, i, outputforsign = 0, outlen,
		sigsize;
	EFI_GUID vendor_guid;
	struct stat st;
	wchar_t var[256];
	UINT32 attributes = EFI_VARIABLE_NON_VOLATILE
		| EFI_VARIABLE_RUNTIME_ACCESS
		| EFI_VARIABLE_BOOTSERVICE_ACCESS
		| EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;
	EFI_TIME timestamp = { 0 };

	while (argc > 1) {
		if (strcmp("--version", argv[1]) == 0) {
			version(progname);
			exit(0);
		} else if (strcmp("--help", argv[1]) == 0) {
			help(progname);
			exit(0);
		} else if (strcmp("-g", argv[1]) == 0) {
			str_to_guid(argv[2], &vendor_guid);
			argv += 2;
			argc -= 2;
		} else if (strcmp("-r", argv[1]) == 0) {
			rsasig = 1;
			argv += 1;
			argc -= 1;
		} else if (strcmp("-t", argv[1]) == 0) {
			timestampstr = argv[2];
			argv += 2;
			argc -= 2;
		} else if (strcmp("-m", argv[1]) == 0)  {
			monotonic = 1;
			argv += 1;
			argc -= 1;
			attributes &= ~EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;
			attributes |= EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS;

		} else if (strcmp("-o", argv[1]) == 0) {
			outputforsign = 1;
			argv += 1;
			argc -= 1;
		} else if (strcmp("-i", argv[1]) == 0) {
			signedinput = argv[2];
			argv += 2;
			argc -= 2;
		} else if (strcmp("-a", argv[1]) == 0) {
			attributes |= EFI_VARIABLE_APPEND_WRITE;
			argv += 1;
			argc -= 1;
		} else if (strcmp("-k", argv[1]) == 0) {
			keyfile = argv[2];
			argv += 2;
			argc -= 2;
		} else if (strcmp("-c", argv[1]) == 0) {
			certfile = argv[2];
			argv += 2;
			argc -= 2;
		} else if (strcmp("-e", argv[1]) == 0) {
			engine = argv[2];
			argv += 2;
			argc -= 2;
		} else  {
			break;
		}
	}

	if (argc != 4) {
		usage(progname);
		exit(1);
	}

	if (rsasig || monotonic) {
		fprintf(stderr, "FIXME: rsa signatures and monotonic payloads are not implemented\n");
		exit(1);
	}

	str = argv[1];
	efifile = argv[2];
	outfile = argv[3];

	/* Specific GUIDs for special variables */
	if (strcmp(str, "PK") == 0 || strcmp(str, "KEK") == 0) {
		vendor_guid = (EFI_GUID)EFI_GLOBAL_VARIABLE;
	} else if (strcmp(str, "db") == 0 || strcmp(str, "dbx") == 0) {
		vendor_guid = (EFI_GUID){ 0xd719b2cb, 0x3d3a, 0x4596, {0xa3, 0xbc, 0xda, 0xd0,  0xe, 0x67, 0x65, 0x6f }};
	}

	memset(&timestamp, 0, sizeof(timestamp));
	time_t t;
	struct tm *tm, tms;

	memset(&tms, 0, sizeof(tms));

	if (timestampstr) {
		strptime(timestampstr, "%Y-%m-%d %H:%M:%S", &tms);
		tm = &tms;
		/* timestamp.Year is from 0 not 1900 as tm year is */
		tm->tm_year += 1900;
		tm->tm_mon += 1; /* tm_mon is 0-11 not 1-12 */
	} else if (attributes & EFI_VARIABLE_APPEND_WRITE) {
		/* for append update timestamp should be zero */
		memset(&tms, 0, sizeof(tms));
		tm = &tms;
	} else {
		time(&t);
		tm = localtime(&t);
		/* timestamp.Year is from 0 not 1900 as tm year is */
		tm->tm_year += 1900;
		tm->tm_mon += 1; /* tm_mon is 0-11 not 1-12 */
	}

	timestamp.Year = tm->tm_year;
	timestamp.Month = tm->tm_mon;
	timestamp.Day = tm->tm_mday;
	timestamp.Hour = tm->tm_hour;
	timestamp.Minute = tm->tm_min;
	timestamp.Second = tm->tm_sec;

	printf("Timestamp is %d-%d-%d %02d:%02d:%02d\n", timestamp.Year,
	       timestamp.Month, timestamp.Day, timestamp.Hour, timestamp.Minute,
	       timestamp.Second);

	/* Warning: don't use any glibc wchar functions.  We're building
	 * with -fshort-wchar which breaks the glibc ABI */
	i = 0;
	do {
		var[i] = str[i];
	} while (str[i++] != '\0');

	varlen = (i - 1)*sizeof(wchar_t);

	int fdefifile = open(efifile, O_RDONLY);
	if (fdefifile == -1) {
		fprintf(stderr, "failed to open file %s: ", efifile);
		perror("");
		exit(1);
	}
	fstat(fdefifile, &st);

	/* signature is over variable name (no null), the vendor GUID, the
	 * attributes, the timestamp and the contents */
	int signbuflen = varlen + sizeof(EFI_GUID) + sizeof(UINT32) + sizeof(EFI_TIME) + st.st_size;
	char *signbuf = malloc(signbuflen);
	char *ptr = signbuf;
	memcpy(ptr, var, varlen);
	ptr += varlen;
	memcpy(ptr, &vendor_guid, sizeof(vendor_guid));
	ptr += sizeof(vendor_guid);
	memcpy(ptr, &attributes, sizeof(attributes));
	ptr += sizeof(attributes);
	memcpy(ptr, &timestamp, sizeof(timestamp));
	ptr += sizeof(timestamp);
	read(fdefifile, ptr, st.st_size);

	printf("Authentication Payload size %d\n", signbuflen);

	if (outputforsign) {
		out = signbuf;
		outlen = signbuflen;
		goto output;
	}

	if (signedinput) {
		struct stat sti;
		int infile = open(signedinput, O_RDONLY);
		if (infile == -1) {
			fprintf(stderr, "failed to open file %s: ", signedinput);
			perror("");
			exit(1);
		}
		fstat(infile, &sti);
		sigbuf = malloc(sti.st_size);
		sigsize = sti.st_size;
		read(infile, sigbuf, sigsize);
	} else {
		if (!keyfile || !certfile) {
			fprintf(stderr, "Doing signing, need certificate and key\n");
			exit(1);
		}
		if (sign_efi_var(signbuf, signbuflen, keyfile, certfile,
				 &sigbuf, &sigsize, engine))
			exit(1);
	}
	printf("Signature of size %d\n", sigsize);

	EFI_VARIABLE_AUTHENTICATION_2 *var_auth = malloc(sizeof(EFI_VARIABLE_AUTHENTICATION_2) + sigsize);

	var_auth->TimeStamp = timestamp;
	var_auth->AuthInfo.CertType = EFI_CERT_TYPE_PKCS7_GUID;
	var_auth->AuthInfo.Hdr.dwLength = sigsize + OFFSET_OF(WIN_CERTIFICATE_UEFI_GUID, CertData);
	var_auth->AuthInfo.Hdr.wRevision = 0x0200;
	var_auth->AuthInfo.Hdr.wCertificateType = WIN_CERT_TYPE_EFI_GUID;

	memcpy(var_auth->AuthInfo.CertData, sigbuf, sigsize);
	sigbuf = var_auth->AuthInfo.CertData;
	if (!signedinput) {
		printf("Signature at: %ld\n", sigbuf - (unsigned char *)var_auth);
	}

	out = var_auth;
	outlen = OFFSET_OF(EFI_VARIABLE_AUTHENTICATION_2, AuthInfo.CertData) + sigsize;

 output:
	;
	int fdoutfile = open(outfile, O_CREAT|O_WRONLY|O_TRUNC, S_IWUSR|S_IRUSR);
	if (fdoutfile == -1) {
		fprintf(stderr, "failed to open %s: ", outfile);
		perror("");
		exit(1);
	}
	/* first we write the authentication header */
	write(fdoutfile, out, outlen);
	if (!outputforsign)
		/* Then we write the payload */
		write(fdoutfile, ptr, st.st_size);
	/* so now the file is complete and can be fed straight into
	 * SetVariable() as an authenticated variable update */
#if 0
	write (fdoutfile, var_auth->AuthInfo.CertData, sigsize);
#endif
	close(fdoutfile);

	return 0;
}
