Recently I try to use ciscocrack to reveal some password protected with
CISCO xor algorithm, and I see that some long long password can not be
uncipher correctly.
So I update the xlat xor table from the original C file, and now it's Ok
to uncipher good PSK in CISCO WIFI router  :-)

Remind tha it only work on :
  password 7,
  password-enable 7,

  ascii 7,
  key 7

The original table was :
char xlat[] = {
        0x64, 0x73, 0x66, 0x64, 0x3b, 0x6b, 0x66, 0x6f,
        0x41, 0x2c, 0x2e, 0x69, 0x79, 0x65, 0x77, 0x72,   
        0x6b, 0x6c, 0x64, 0x4a, 0x4b, 0x44
};

can be found at PacketStorm
http://packetstorm.linuxsecurity.com/Exploit_Code_Archive/ciscocrack.c

Now the new was :
char xlat[] = {
        0x64, 0x73, 0x66, 0x64, 0x3b, 0x6b, 0x66, 0x6f,
        0x41, 0x2c, 0x2e, 0x69, 0x79, 0x65, 0x77, 0x72,
        0x6b, 0x6c, 0x64, 0x4a, 0x4b, 0x44, 0x48, 0x53,
        0x55, 0x42, 0x73, 0x67, 0x76, 0x63, 0x61, 0x36,
        0x39, 0x38, 0x33, 0x34, 0x6e, 0x63, 0x78, 0x76,
        0x39, 0x38, 0x37, 0x33, 0x32, 0x35, 0x34, 0x6b,
        0x3b, 0x66, 0x67, 0x38, 0x37,
        0x64, 0x73, 0x66, 0x64, 0x3b, 0x6b, 0x66, 0x6f,
        0x41, 0x2c, 0x2e, 0x69, 0x79, 0x65, 0x77, 0x72,
        0x6b, 0x6c, 0x64, 0x4a, 0x4b, 0x44, 0x48, 0x53,
        0x55, 0x42, 0x73, 0x67, 0x76, 0x63, 0x61, 0x36,
        0x39, 0x38, 0x33, 0x34, 0x6e, 0x63, 0x78, 0x76,
        0x39, 0x38, 0x37, 0x33, 0x32, 0x35, 0x34, 0x6b,
        0x3b, 0x66, 0x67, 0x38, 0x37
};

It was extract from an uncompressed binary image of IOS 12.2(8)

0df4a70:                     6473 6664 3b6b 666f          dsfd;kfo
0df4a80: 412c 2e69 7965 7772 6b6c 644a 4b44 4853  A,.iyewrkldJKDHS
0df4a90: 5542 7367 7663 6136 3938 3334 6e63 7876  UBsgvca69834ncxv
0df4aa0: 3938 3733 3235 346b 3b66 6738 3700 0000  9873254k;fg87...

You can find the modified ciscocrack.c file in attached piece.

I extend also some buffer ... ;-)

--------------------------------------------------------------------
#include <stdio.h>
#include <ctype.h>

char xlat[] = {
        0x64, 0x73, 0x66, 0x64, 0x3b, 0x6b, 0x66, 0x6f,
        0x41, 0x2c, 0x2e, 0x69, 0x79, 0x65, 0x77, 0x72,
        0x6b, 0x6c, 0x64, 0x4a, 0x4b, 0x44, 0x48, 0x53,
        0x55, 0x42, 0x73, 0x67, 0x76, 0x63, 0x61, 0x36,
	0x39, 0x38, 0x33, 0x34, 0x6e, 0x63, 0x78, 0x76,
	0x39, 0x38, 0x37, 0x33, 0x32, 0x35, 0x34, 0x6b,
	0x3b, 0x66, 0x67, 0x38, 0x37,
        0x64, 0x73, 0x66, 0x64, 0x3b, 0x6b, 0x66, 0x6f,
        0x41, 0x2c, 0x2e, 0x69, 0x79, 0x65, 0x77, 0x72,
        0x6b, 0x6c, 0x64, 0x4a, 0x4b, 0x44, 0x48, 0x53,
        0x55, 0x42, 0x73, 0x67, 0x76, 0x63, 0x61, 0x36,
	0x39, 0x38, 0x33, 0x34, 0x6e, 0x63, 0x78, 0x76,
	0x39, 0x38, 0x37, 0x33, 0x32, 0x35, 0x34, 0x6b,
	0x3b, 0x66, 0x67, 0x38, 0x37
};

/* Extract from an IOS 12.2(8)
0df4a70:                     6473 6664 3b6b 666f          dsfd;kfo
0df4a80: 412c 2e69 7965 7772 6b6c 644a 4b44 4853  A,.iyewrkldJKDHS
0df4a90: 5542 7367 7663 6136 3938 3334 6e63 7876  UBsgvca69834ncxv
0df4aa0: 3938 3733 3235 346b 3b66 6738 3700 0000  9873254k;fg87...
*/

char pw_str1[] = "password 7 ";
char pw_str2[] = "enable-password 7 ";

char *pname;

cdecrypt(enc_pw, dec_pw)
unsigned char *enc_pw;
unsigned char *dec_pw;
{
        unsigned int seed, i, val = 0;

        if(strlen(enc_pw) & 1)
                return(-1);

        seed = (enc_pw[0] - '0') * 10 + enc_pw[1] - '0';

        if (seed > 15 || !isdigit(enc_pw[0]) || !isdigit(enc_pw[1]))
                return(-2);

        for (i = 2 ; i <= strlen(enc_pw); i++) {
                if(i !=2 && !(i & 1)) {
                        dec_pw[i / 2 - 2] = val ^ xlat[seed++];
                        val = 0;
                }

                val *= 16;

                if(isdigit(enc_pw[i] = toupper(enc_pw[i]))) {
                        val += enc_pw[i] - '0';
                        continue;
                }

                if(enc_pw[i] >= 'A' && enc_pw[i] <= 'F') {
                        val += enc_pw[i] - 'A' + 10;
                        continue;
                }

                if(strlen(enc_pw) != i)
                        return(-4);
        }

        dec_pw[++i / 2] = 0;

        return(0);
}

usage()
{
        fprintf(stdout, "Usage: %s -p <encrypted password>\n", pname);
        fprintf(stdout, "       %s <router config file> <output file>\n", pname);

        return(0);
}

main(argc,argv)
int argc;
char **argv;

{
        FILE *in = stdin, *out = stdout;
        char line[512];
        char passwd[512];
        unsigned int i, pw_pos;

        pname = argv[0];

        if(argc > 1)
        {
                if(argc > 3) {
                        usage();
                        exit(1);
                }

                if(argv[1][0] == '-')
                {
                        switch(argv[1][1]) {
                                case 'h':
                                usage();
                                break;

                                case 'p':
                                if(cdecrypt(argv[2], passwd)) {
                                        fprintf(stderr, "Error.\n");
                                        exit(1);
                                }
                                fprintf(stdout, "password: %s\n", passwd);
                                break;

                                default:
                                fprintf(stderr, "%s: unknow option.", pname);
                        }

                        return(0);
                }

                if((in = fopen(argv[1], "rt")) == NULL)
                        exit(1);
                if(argc > 2)
                        if((out = fopen(argv[2], "wt")) == NULL)
                                exit(1);
        }

        while(1) {
                for(i = 0; i < 256; i++) {
                        if((line[i] = fgetc(in)) == EOF) {
                                if(i)
                                        break;

                                fclose(in);
                                fclose(out);
                                return(0);
                        }
                        if(line[i] == '\r')
                                i--;

                        if(line[i] == '\n')
                                break;
                }
                pw_pos = 0;
                line[i] = 0;

                if(!strncmp(line, pw_str1, strlen(pw_str1)))
                        pw_pos = strlen(pw_str1);

                if(!strncmp(line, pw_str2, strlen(pw_str2)))
                        pw_pos = strlen(pw_str2);

                if(!pw_pos) {
                        fprintf(stdout, "%s\n", line);
                        continue;
                }

                if(cdecrypt(&line[pw_pos], passwd)) {
                        fprintf(stderr, "Error. N�%02d\n");
                        exit(1);
                }
                else { 
                        if(pw_pos == strlen(pw_str1)) {
                                fprintf(out, "%s", pw_str1); }
                        else {
                                fprintf(out, "%s", pw_str2);
			}
                        fprintf(out, "%s\n", passwd);
                }
        }

}
