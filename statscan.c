/* Statd scanner....... 		*/

#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <signal.h>
#include <rpc/rpc.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <rpc/pmap_prot.h>
#include <rpc/pmap_clnt.h>

#define ERROR -1

void woopy(int s);
void usage(char *s);
void scan(char *i, char *o);

int statd(char *host);
unsigned long int res(char *p);

void usage(char *s)
{
  printf("Usage: %s <inputfile> <outputfile>\n",s);
  exit(ERROR);
}

void main(int argc, char **argv)
{
  if(argc < 3) usage(argv[0]);
  scan(argv[1], argv[2]);
}

void scan(char *i, char *o)
{
  FILE *iff, *of;
  char buf[512];
  
  if((iff=fopen(i,"r")) == NULL)
    return;
  while(fgets(buf,512,iff) != NULL) 
  {
    if(buf[strlen(buf)-1]=='\n')
      buf[strlen(buf)-1]=0;
    if(statd(buf) && (of=fopen(o,"a")) != NULL) {
      buf[strlen(buf)+1]=0;
      buf[strlen(buf)]='\n';
      
      fputs(buf,of);
      fclose(of);
    }  
  }
  fclose(iff);
}

void woopy(int s)
{
  return;
}

int statd(char *host)
{
  struct sockaddr_in server_addr;
  struct pmaplist *head = NULL;
  int sockett = RPC_ANYSOCK;
  struct timeval minutetimeout;
  register CLIENT *client;
  struct rpcent *rpc;

  server_addr.sin_addr.s_addr=res(host);
  server_addr.sin_family=AF_INET;
  server_addr.sin_port = htons(PMAPPORT);
  minutetimeout.tv_sec = 15;
  minutetimeout.tv_usec = 0;
  
  /* cause clnttcp_create uses connect() */
  signal(SIGALRM,woopy);
  alarm(15);
  
  if ((client = clnttcp_create(&server_addr, PMAPPROG,
  	PMAPVERS, &sockett, 50, 500)) == NULL) {
    alarm(0);
    signal(SIGALRM,SIG_DFL);
    return 0;
  }
  alarm(0);
  signal(SIGALRM,SIG_DFL);
  
  if (clnt_call(client, PMAPPROC_DUMP, (xdrproc_t) xdr_void, NULL, 
  	(xdrproc_t) xdr_pmaplist, &head, minutetimeout) != RPC_SUCCESS)
    return 0;
  if (head != NULL) 
    for (; head != NULL; head = head->pml_next)
      if((rpc = getrpcbynumber(head->pml_map.pm_prog)))
        if(strcmp(rpc->r_name,"rstatd") == 0)
          return 1;              
   
  return 0;
}

unsigned long int res(char *p)
{
   struct hostent *h;
   unsigned long int rv;
    
   h=gethostbyname(p);
   if(h!=NULL)
     memcpy(&rv,h->h_addr,h->h_length);
   else
     rv=inet_addr(p);
   return rv;
}
                    
