/*
  threaded tcp port scanner 1.1 - september 1999
  by mirage <mirage@hackers-pt.org> - .\\idgard Security Services

  linux, du: gcc tscan.c -o tscan -lpthread
  bsd:       gcc tscan.c -o tscan -pthread
  other platforms are untested.

  ./tscan [options] [host:startport-endport | host:port (...)]
  options are:
    -t <threads to use> (default 20)
    -s <host> scan this host for the ports in /etc/services

  open ports are printed on stdout, while errors go to stderr.
  note: scans are made in inverse order from the input.

  ./tscan `cat list` > list.open 2> /dev/null
  is a good bet.

  greets to #hackers & #coders (PTlink/PTnet)
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define THREADS_MAX 255

struct target {
  unsigned long ip;
  unsigned short port;
  struct target *next;
} *targets = NULL;

char *progname;
pthread_mutex_t lock;

void
error (char *err)
{
  fprintf (stderr, "%s: %s\n", progname, err);
  exit (1);
}

unsigned long
host_resolve (char *host)
{
  struct in_addr addr;
  struct hostent *host_ent;

  addr.s_addr = inet_addr (host);
  if (addr.s_addr == -1)
    {
      host_ent = gethostbyname (host);
      if (host_ent == NULL)
        addr.s_addr = 0;
      else
        bcopy (host_ent->h_addr, (char *)&addr.s_addr, host_ent->h_length);
    }
  return addr.s_addr;
}

/* put a host:port(s) in the stack of targets */
void
put_target (char *str)
{
  struct target *t;
  char *host, *startport, *endport;
  unsigned long ip;
  int port;

  /* parse */
  host = strtok (str, ":");
  startport = strtok (NULL, "-");
  if (startport == NULL)
    error ("invalid host:port");
  endport = strtok (NULL, "-");
  if (endport == NULL)
    endport = startport;

  ip = host_resolve (host);
  if (ip == 0)
    fprintf (stderr, "can't resolve %s\n", host);
  else
    for (port = atoi (startport); port <= atoi (endport); port++)
      {
        /* build target */
        t = (struct target *)malloc (sizeof (struct target));
        if (t == NULL)
          error ("malloc()");
        t->ip = ip;
        t->port = port;
        t->next = targets;
        targets = t;
      }
}

/* return and delete a target from the stack */
struct target *
get_target (void)
{
  struct target *tmp;
  pthread_mutex_lock (&lock);
  if (targets == NULL)
    tmp = NULL;
  else
    {
      tmp = targets;
      targets = targets->next;	/* note that the memory is never freed */
    }
  pthread_mutex_unlock (&lock);
  return tmp;
}

/* simple connect() scan */
int
can_connect (unsigned long ip, unsigned short port)
{
  int fd, result;
  struct sockaddr_in addr;
  fd = socket (AF_INET, SOCK_STREAM, 0);
  if (fd == -1)
    {
      perror ("socket()");
      return 0;
    }
  addr.sin_addr.s_addr = ip;
  addr.sin_port = htons (port);
  addr.sin_family = AF_INET;
  result = connect (fd, (struct sockaddr *)&addr, sizeof (struct sockaddr));
  close (fd);
  return (result == 0);
}

/* the scan routine of each thread */
void *scan (void *data)
{
  struct target *t;
  struct in_addr addr;
  while ((t = get_target ()) != NULL)
    if (can_connect (t->ip, t->port))
      {
        addr.s_addr = t->ip;
        printf ("%s:%d\n", inet_ntoa (addr), t->port);
      }
  return NULL;
}

int main (int argc, char *argv[])
{
  int thread_num = 20, i, optnum = 0;
  char buf[1000];
  pthread_t threads[THREADS_MAX];
  void *retval;
  struct servent *se;
  progname = argv[0];

  if (argc == 1)
    {
      printf ("threaded tcp port scanner - <mirage@hackers-pt.org>\n");
      printf ("usage: %s [options] [host:startport-endport | host:port (...)]\n", argv[0]);
      printf ("options are:\n");
      printf ("  -t <threads to use> (default 20)\n");
      printf ("  -s <host> scan this host for the ports in /etc/services\n");
      return 1;
    }

  while ((i = getopt(argc, argv, "t:s:")) != EOF)
    switch (i)
      {
        case 't':
          thread_num = atoi (optarg);
          if (thread_num > THREADS_MAX)
            error ("too many threads requested");
          optnum += 2;
          break;
        case 's':
          setservent (1);
          while ((se = getservent ()) != NULL)
            if (strcasecmp ("tcp", se->s_proto) == 0)
              {
                if (strlen (optarg) > 64)
                  optarg[64] = 0;
                sprintf (buf, "%s:%d", optarg, ntohs (se->s_port));
                put_target (buf);
              }
          endservent ();
          optnum += 2;
          break;
        default:
          return 1;
      }

  for (i = optnum+1; i < argc; i++)
    put_target (argv[i]);

  /* initialize the mutex lock */
  pthread_mutex_init (&lock, NULL);

  /* create threads */
  for (i = 0; i < thread_num; i++)
    if (pthread_create (&threads[i], NULL, scan, 0) != 0)
      error ("pthread_create()");

  /* wait for all of them to end */
  for (i = 0; i < thread_num; i++)
    pthread_join (threads[i], &retval);

  return 0;
}

