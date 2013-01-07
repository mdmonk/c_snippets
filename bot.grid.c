#include <time.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <curses.h>

#define NONE  -1
#define ERROR -1

/* genders     */
#define FEMALE 0
#define MALE   1
/* ----------- */

/* direction */
#define NORTH 1
#define SOUTH 2
#define EAST  3
#define WEST  4
#define MAXDIRS 4
/* --------- */

#define NUMBOTS 6          /* # of bots.. (needs to be an even #) */

int first = 1;

int colors;                /* use color or not       */

int minX = 0,  minY = 0;   /* minimum values on grid */
int maxX = 38*2, maxY = 22;  /* largest values on grid */

/* structure for all bots */
struct bot {
  int sex;         /* 0 = female, 1 = male           */

  int dir;         /* direction (n,s,e,w)            */
  int speed;       /* speed (# of X, Y)              */
  int stop;        /* stop if someone is in the way. */
                   /* 0 = no, 1 = stop and wait,     */
                   /* 2 = change directions          */

  int posX, posY, nextX, nextY;  /* positions on grid */

} *bots[NUMBOTS];
/* ---------------------- */

/* all current points */
int start = 0;
int curPoint = 0; /* current point in points[] */

struct point {
  int x, y;
} *points[1024];
/* -------------------------------- */

void init();
void moveBots();
void printGrid();
void printStats();

void checkProbs();
void fixProbs(struct bot *bot1, struct bot *bot2);

void terminate(int status);

int main()
{
   signal(SIGINT,  terminate);
   signal(SIGTERM, terminate);
   signal(SIGQUIT, terminate);

   if (NUMBOTS % 2 != 0)
   {
      fprintf(stderr, "ERROR: the # of bots must be an even number. "
                      "Recompile\n");
      exit(ERROR);
   }

   init(), clear();
   printStats(), printGrid();

   while(1) 
   {
      moveBots(), printGrid();
      /* refresh(); */
   }

   clear(); refresh();
   terminate(0);
}

void init()
{
   int i;

   initscr();

   if (has_colors() == TRUE) 
   {
      start_color(), colors = 1;

      init_pair(COLOR_BLACK, COLOR_BLACK, COLOR_BLACK);
      init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
      init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
      init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
      init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
      init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
      init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
      init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);

      wattrset(stdscr, COLOR_PAIR(COLOR_BLUE));
   }
 
   else 
   {
      colors = 0;

      init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
      wattrset(stdscr, COLOR_PAIR(COLOR_WHITE));
   }
 
   cbreak(), noecho(), nonl();
   curs_set(0); /* show no cursor */

   intrflush(stdscr, FALSE);
   keypad(stdscr, TRUE);

   /* -------------------------- */

   srand(getpid() * time(NULL));

   for (i = 0; i <= NUMBOTS-1; i++) 
   {
      bots[i] = (struct bot *)malloc(sizeof(struct bot));
      if (bots[i] == NULL) 
      {
         perror("malloc");
         exit(ERROR);
      }

      bots[i]->posX = rand() % maxX, bots[i]->posY = rand() % maxY;

      /* MAXDIRS = 4 (N, S, E, W) */
      bots[i]->dir   = rand() % MAXDIRS + 1;
      bots[i]->speed = rand() % 2 + 1; /* 1 or 2 units a move */

      /* 0 = no stop, 1 = wait, 2 = turn around */
      bots[i]->stop  = rand() % 3;
   }
}

void printStats()
{
   int i;

   wattrset(stdscr, COLOR_PAIR(COLOR_WHITE));

   for (i = 0; bots[i]; i++)
      printw("bot%d's position (on grid) = (%d, %d)\n",
              i+1, bots[i]->posX, bots[i]->posY);
 
   addch('\n');

   for (i = 0; bots[i]; i++) 
   {
      printw("bot%d's direction = ", i+1);

      if (bots[i]->dir == NORTH) printw("north\n");
      else if (bots[i]->dir == SOUTH) printw("south\n");
      else if (bots[i]->dir == EAST) printw("east\n");
      else if (bots[i]->dir == WEST) printw("west\n");
      else printw("%d\n", bots[i]->dir);
   }

   addch('\n');

   for (i = 0; bots[i]; i++)
      printw("bot%d's speed = %d\n", i+1, bots[i]->speed);

   addch('\n');

   for (i = 0; bots[i]; i++) 
   {
      printw("bot%d will ", i+1);

      if (bots[i]->stop == 0) printw("not stop ");
      else if (bots[i]->stop == 1) printw("stop and wait ");
      else if (bots[i]->stop == 2) printw("change directions ");
      else printw("[unknown].. bots[%]->stop = %d\n", i, bots[i]);

      printw("if something is in its way\n");
   }

   addch('\n');

   printw("Hit Enter to continue..."), getch();
   wattrset(stdscr, COLOR_PAIR(COLOR_BLUE));
   clear(), refresh();
}

void printGrid()
{
   int curX, curY;
   int i = 0, j = 0, k = 0;

   curX = minX, curY = maxY;

   /* print the edge of grid */
   if (colors) wattrset(stdscr, COLOR_PAIR(COLOR_WHITE));

   move(24, 0), printw("(0, 0)");
   move(24, 72), printw("(%d, 0)", maxX);

   move(0, 0), printw("(0, %d)",  maxY);
   move(0, 71), printw("(%d, %d)", maxX, maxY); 

   if (colors) wattrset(stdscr, COLOR_PAIR(COLOR_BLUE));

   /* ------------------ */

   /* Just print title and author */
   if (colors) wattrset(stdscr, COLOR_PAIR(COLOR_CYAN));
   move(0,  35), printw("B O T S");

   if (colors) wattrset(stdscr, COLOR_PAIR(COLOR_MAGENTA));
   move(24, 31); printw("By Matt Conover");

   if (colors) wattrset(stdscr, COLOR_PAIR(COLOR_BLUE));
   refresh();

   /* ------------------ */

   /* Make horizontal lines */
   for (i = 1; i < 24; i++, curY--) 
   {
      curX = minX;

      for (j = 1; j < 79; j++, curX++) 
         for (k = 0; bots[k]; k++) 
            if ((curX == bots[k]->posX) && (curY == bots[k]->posY)) 
            {
               if ((curY+1 > maxY) || (curY < minY)) continue;

               points[curPoint] = (struct point *)malloc(sizeof(struct point));
               if (points[curPoint] == NULL) 
               {
                  perror("malloc");
                  exit(ERROR);
               }

               points[curPoint]->x = j, points[curPoint]->y = i;
               curPoint++;

               move(i, j);
               if (colors) wattrset(stdscr, COLOR_PAIR(COLOR_RED));
               hline('*', 1);
               if (colors) wattrset(stdscr, COLOR_PAIR(COLOR_BLUE));

               refresh();
            } 
   }

   if (first != 1) usleep(350000);
   else first = 0;

   curX = minX, curY = maxY;
   for (i = 1; i < 24; i++, curY--) 
   {
      curX = minX;

      for (j = 1; j < 79; j++, curX++)
         for (k = 0; k <= curPoint && points[k]; k++)
             if ((points[k]->x != j) || (points[k]->y != i)) 
             {
                move(i, j), hline('.', 1);
                /* refresh(); */
             }
   }

   refresh();

   for (i = 0; i < curPoint; i++)
       free(points[i]);

   curPoint = 0, start = 1;
}

void moveBots()
{
   int i;
   int donext;

   for (i = 0; bots[i]; i++) 
   {
      if (bots[i]->dir == NORTH) 
      {
         bots[i]->posY += bots[i]->speed;
         bots[i]->nextY = bots[i]->posY + bots[i]->speed;
      } 

      else if (bots[i]->dir == SOUTH)
      {
         bots[i]->posY -= bots[i]->speed;
         bots[i]->nextY = bots[i]->posY - bots[i]->speed;
      }
 
      else if (bots[i]->dir == EAST) 
      {
         bots[i]->posX += bots[i]->speed;
         bots[i]->nextX = bots[i]->posX + bots[i]->speed;
      } 

      else if (bots[i]->dir == WEST) 
      { 
         bots[i]->posX -= bots[i]->speed;
         bots[i]->nextX = bots[i]->posX - bots[i]->speed;
      } 

      else 
      {
         clear();

         if (colors) wattrset(stdscr, COLOR_PAIR(COLOR_RED));
         printw("moveBots(): bots[%d]->dir is %d\n", i, bots[i]->dir);
         if (colors) wattrset(stdscr, COLOR_PAIR(COLOR_BLUE));

         refresh();
      }

      if (bots[i]->posX > maxX)  bots[i]->posX = minX;
      else if (bots[i]->posX < minX)  bots[i]->posX = maxX;

      if (bots[i]->nextX > maxX) bots[i]->nextX = minX;
      else if (bots[i]->nextX < minX) bots[i]->nextX = maxX;

      if (bots[i]->posY > maxY)  bots[i]->posY = minY;
      else if (bots[i]->posY < minY)  bots[i]->posY = maxY;

      if (bots[i]->nextY > maxY) bots[i]->nextY = minY;
      else if (bots[i]->nextY < minY) bots[i]->nextY = maxY;

      if (bots[i]->stop > 0) donext = 1; 
   }

   if (donext == 1) checkProbs();
}

void checkProbs()
{
   int i, j;

   struct {
     int nextX;
     int nextY;
   } botpos[NUMBOTS];

   /* make sure there are no conflicts.. if so.. call fixProbs() */
   for (i = 0; bots[i]; i++) 
   {      
      botpos[i].nextX = bots[i]->nextX;
      botpos[i].nextY = bots[i]->nextY;
   }

   for (i = 0; bots[i]; i++)
      for (j = 0; j < NUMBOTS; j++)
         if ((botpos[i].nextX == botpos[j].nextX) &&
             (botpos[i].nextY == botpos[j].nextY))
             fixProbs(bots[i], bots[j]);

}

void fixProbs(struct bot *bot1, struct bot *bot2)
{
   int i;

   for (i = 0; bots[i]; i++) 
   {      
      if (bots[i]->stop == 1) 
      {
         if (bots[i]->dir == NORTH)
            bots[i]->posY += bots[i]->speed; 

         else if (bots[i]->dir == SOUTH) 
            bots[i]->posY -= bots[i]->speed; 

         else if (bots[i]->dir == EAST) 
            bots[i]->posX += bots[i]->speed; 

         else if (bots[i]->dir == WEST)
            bots[i]->posX -= bots[i]->speed; 

         else 
         {
            printf("bots[%d]->dir = %d\n", i, bots[i]->dir);
            refresh();
         }
      }

      if (bots[i]->stop == 2) 
      {
         if (bots[i]->dir == NORTH) 
         { 
            bots[i]->dir = SOUTH;
            bots[i]->posY -= bots[i]->speed; 
         } 

         else if (bots[i]->dir == SOUTH)
         {
            bots[i]->dir = NORTH;
            bots[i]->posY += bots[i]->speed; 
         } 

         else if (bots[i]->dir == EAST) 
         { 
            bots[i]->dir = WEST;
            bots[i]->posX -= bots[i]->speed; 
         } 

         else if (bots[i]->dir == WEST) 
         {
            bots[i]->dir = EAST;
            bots[i]->posX += bots[i]->speed; 
         } 

         else 
         {
            printf("bots[%d]->dir = %d\n", i, bots[i]->dir);
            refresh();
         }
      }

      bots[i]->nextX = bots[i]->nextY = ERROR;
   }
}

void terminate(int status)
{
   int i;

   usleep(50000);
  
   clear(), refresh(), endwin();

   for (i = 0; bots[i]; i++) free(bots[i]);
   for (i = 0; i < curPoint; i++) free(points[i]);

   exit(0);
}
