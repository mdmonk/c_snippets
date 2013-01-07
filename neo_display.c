/* quick hack to display the positions of asteroids and planets */
/* Code here is Copyrighted by Scott Manley and licensed under the terms 
   of the Gnu Public License V2  - look at www.gnu.org */

/* The file astorb.dat should be acquired from 
   ftp://ftp.lowell.edu/pub/elgb/astorb.html */

/* command line for compilation : */
/* gcc -lm -L/usr/X11R6/lib -lX11 -lXext -O3 neo_display.c -o neo_display */

#include <math.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/XShm.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>

#define BPP 3
#define PI 3.14159265359


/* yes I'm lazy - this is all global data */

typedef struct ast{
float p,e,m0,md;
float xx,xy,yx,yy;
int type;
} adat;

int num_asteroids;
adat a_list[60000];  /* 60,000 asteroid slots should be enough for the near future */
int AU_SCALE=220;


Display *display;
int screen,depth,width,height,xshmevent=-1,xshm=0,buffered=0,xdpm=0;
Window rootwin,win;
GC gc;
XImage *image=0;
int redshift,greenshift,blueshift,nodraw=0,verbose=0;

double jd_conv(double day,double month,double year){
double m,y,a,b,c,d,jd;
m=month;
y=year;
if (m < 3 ) {
  y=y-1;
  m=m+12;
}	
a=floor(y/100.0);
b=2-a+floor(a/4);
c=floor(365.25*y);
d=floor(30.6001*(m+1));
jd=b+c+d+day+1720994.5;
return jd;
}

double true_anomaly (double ma,double ec){
double e,diff,f;
e=ma;
diff=1.0;
 while(diff>0.00001) {
   e = ma + ec*sin(e);
   diff= abs(e-ec*sin(e)-ma);
 }
f=2*atan(sqrt((1+ec)/(1-ec))*tan(e*0.5));
return f;
}

void get_position(double a,double e,double i,double lo,double so,double ma,double *x,double *y){
double inc,l,s,p,m;
double xx,xy,yx,yy,tx,ty;
double ang,rad;

inc=i*PI/180.0;
l=lo*PI/180.0;
s=so*PI/180.0;
p=a*(1-e*e);
m=ma*PI/180.0;

xx=  cos(s)*cos(l) - sin(s)*sin(l);
xy= -sin(s)*cos(l) - cos(s)*sin(l);
yx=  cos(s)*sin(l) + sin(s)*cos(inc)*cos(l);
yy=  cos(s)*cos(inc)*cos(l) - sin(s)*sin(l);

ang=true_anomaly(m,e);
rad=p/(1+e*cos(ang));
tx=rad*cos(ang);
ty=rad*sin(ang);

*x=tx*xx+ty*xy;
*y=tx*yx+ty*yy;

}

void blank_bitmap(unsigned char *image,int sx,int sy){
long i;
 for(i=0;i<sx*sy*BPP;i++)
   image[i]=0;
}

/* this is wrong...... */
save_targa(unsigned char *image,int sx,int sy,char *filename){
long i,ref;
int lstep=sx*BPP;
short srt;
unsigned char hi,lo;
unsigned char header[18]={0x0,0x0, 0x2,0x0, 0x0,0x0, 0x0,0x0, 0x0,0x0, 0x0,0x0, sx%256,sx>>8, sy%256,sy>>8, 0x18,0x20};
FILE *out;
out=fopen(filename,"wb");
fwrite(header,1,18,out);
for(i=0;i<sx*sy;i++){
ref=i*BPP;
fputc(image[ref],out);
fputc(image[ref+1],out);
fputc(image[ref+2],out);
}
fclose(out);
}


void blur_bitmap(unsigned char *image,int sx,int sy){
int x,y;
unsigned long ref1,ref2;
unsigned char *copy;

copy=malloc((sx+2)*(sy*2)*BPP*sizeof(char));

for(ref1=0;ref1<(sx+2)*(sy+2)*BPP;ref1++) 
  copy[ref1]=0;

 for(x=0;x<sx;x++){
   for(y=0;y<sy;y++){
     ref1=((x+1) + (y+1)*(sx+2))*BPP;
     ref2=(x+y*sx)*BPP;
     copy[ref1]=image[ref2];
     copy[ref1+1]=image[ref2+1];
     copy[ref1+2]=image[ref2+2];
     copy[ref1+3]=image[ref2+3];
   }
 }

 for(x=0;x<sx;x++){
   for(y=0;y<sy;y++){
     ref1=((x+1) + (y+1)*(sx+2))*BPP;
     ref2=(x+y*sx)*BPP;
     image[ref2]=(copy[ref1]+copy[ref1-(sx+3)*BPP]+copy[ref1-(sx+2)*BPP]+copy[ref1-(sx+1)*BPP]+copy[ref1-BPP]+copy[ref1+BPP]+copy[ref1+(sx+1)*BPP]+copy[ref1+(sx+2)*BPP]+copy[ref1+(sx+3)*BPP])/9;
     ref1++;
     ref2++;
     image[ref2]=(copy[ref1]+copy[ref1-(sx+3)*BPP]+copy[ref1-(sx+2)*BPP]+copy[ref1-(sx+1)*BPP]+copy[ref1-BPP]+copy[ref1+BPP]+copy[ref1+(sx+1)*BPP]+copy[ref1+(sx+2)*BPP]+copy[ref1+(sx+3)*BPP])/9;
     ref1++;
     ref2++;
     image[ref2]=(copy[ref1]+copy[ref1-(sx+3)*BPP]+copy[ref1-(sx+2)*BPP]+copy[ref1-(sx+1)*BPP]+copy[ref1-BPP]+copy[ref1+BPP]+copy[ref1+(sx+1)*BPP]+copy[ref1+(sx+2)*BPP]+copy[ref1+(sx+3)*BPP])/9;
     ref1++;
     ref2++;
     image[ref2]=(copy[ref1]+copy[ref1-(sx+3)*BPP]+copy[ref1-(sx+2)*BPP]+copy[ref1-(sx+1)*BPP]+copy[ref1-BPP]+copy[ref1+BPP]+copy[ref1+(sx+1)*BPP]+copy[ref1+(sx+2)*BPP]+copy[ref1+(sx+3)*BPP])/9;
   }
 }
free(copy);
}

int draw_pixel(char *bm,int sx,int sy,int x,int y,int col){
long ref;
unsigned char r,g,b,a;

  if(x<0 || x>=sx) 
    return 0;
  if(y<0 || y>=sy)
    return 0;
  ref=(x+y*sx)*BPP;
  r=col & 0xff;
  g=(col>>8) & 0xff;
  b=(col>>16) & 0xff;
  if (b>bm[ref])   bm[ref]=b;
  if (g>bm[ref+1]) bm[ref+1]=g;
  if (r>bm[ref+2]) bm[ref+2]=r;
  return col;
}

int  draw_orbit(char *bm,int sx,int sy,double a,double e,double i,double lo,double so,double scale){
double inc,l,s,p;
double xx,xy,yx,yy,tx,ty,x,y;
double ang,rad,astep;

inc=i*PI/180.0;
l=lo*PI/180.0;
s=so*PI/180.0;
p=a*(1-e*e);

xx=  cos(s)*cos(l) - sin(s)*sin(l);
xy= -sin(s)*cos(l) - cos(s)*sin(l);
yx=  cos(s)*sin(l) + sin(s)*cos(inc)*cos(l);
yy=  cos(s)*cos(inc)*cos(l) - sin(s)*sin(l);
astep=3/(a*scale);

 for(ang=0;ang<2*PI;ang+=astep){
   rad=p/(1+e*cos(ang));
   tx=rad*cos(ang);
   ty=rad*sin(ang);
   x=sx/2+scale*(tx*xx+ty*xy);
   y=sy/2-scale*(tx*yx+ty*yy);
   draw_pixel(bm,sx,sy,x,y,0xff00);
 }
}


int draw_square(char *bm,int sx, int sy, int x,int y, int s,int col){
int i,j;

  if(((x+s)<0) || ((x-s)>sx) || ((y+s)<0) || ((y-s)>sy) )
    return 0;
  
  for(i=(x-s);i<=(x+s);i++){
    draw_pixel(bm,sx,sy,i,y-s,col);
    draw_pixel(bm,sx,sy,i,y+s,col);
  }
  
  for(i=(y-s);i<=(y+s);i++){
    draw_pixel(bm,sx,sy,x-s,i,col);
    draw_pixel(bm,sx,sy,x+s,i,col);
  }
  return 1;
}

int fill_square(char *bm,int sx, int sy, int x,int y, int s,int col){
int i,j;

  if(((x+s)<0) || ((x-s)>sx) || ((y+s)<0) || ((y-s)>sy) )
    return 0;
  for(i=(x-s);i<=(x+s);i++){
    for(j=(y-s);j<=(y+s);j++){
      draw_pixel(bm,sx,sy,i,j,col);
    }
  }

  return 1;
}

int fill_circle(char *bm,int sx,int sy,int x,int y, int s,int col){
int i,j;

  if(((x+s)<0) || ((x-s)>sx) || ((y+s)<0) || ((y-s)>sy) )
    return 0;
  for(i=(x-s);i<=(x+s);i++){
    for(j=(y-s);j<=(y+s);j++){
      if(sqrt((x-i)*(x-i)+(y-j)*(y-j))<=s){
	draw_pixel(bm,sx,sy,i,j,col);
      }
    }
  }

  return 1;
}


/* loads the asteroid file and calculate the rotation matrix */
/* objects which are too far out won't be included */
void load_asteroids(char *file,double jd,double min){
FILE *inp;
double y,m,d,epoch,period,m_now;
double a,e,i,lo,so,ma,q;
char blah[256];
char date[16];
num_asteroids=0;
inp=fopen(file,"r");
 while(!feof(inp)){   
   fscanf(inp,"%104c %s %lf %lf %lf %lf %lf %lf %[^\n]s",blah,date,&ma,&so,&lo,&i,&e,&a,blah);
   fgetc(inp);
   q=a*(1-e);
   if(q<min){
     d=atof(&date[6]);
     date[6]=0;
     m=atof(&date[4]);
     date[4]=0;
     y=atof(date);
     epoch=jd_conv(d,m,y);
     period=pow(a,1.5)*365.25;
     a_list[num_asteroids].e=e;
     i=i*PI/180.0;
     so=so*PI/180.0;
     lo=lo*PI/180.0;
     a_list[num_asteroids].md=2*PI/period;
     ma=ma*PI/180.0;
     a_list[num_asteroids].m0=ma + (jd-epoch)*a_list[num_asteroids].md;
     a_list[num_asteroids].xx=  cos(so)*cos(lo) - sin(so)*sin(lo);
     a_list[num_asteroids].xy= -sin(so)*cos(lo) - cos(so)*sin(lo);
     a_list[num_asteroids].yx=  cos(so)*sin(lo) + sin(so)*cos(i)*cos(lo);
     a_list[num_asteroids].yy=  cos(so)*cos(i)*cos(lo) - sin(so)*sin(lo);
     a_list[num_asteroids].p=a*(1-e*e);
     if(q<1)
       a_list[num_asteroids].type=2;       
     else if(q<1.3)
       a_list[num_asteroids].type=1;
     else 
       a_list[num_asteroids].type=0;
     num_asteroids++;
   }
 }
fclose(inp);
}


void draw_asteroids_fast(char *bm,int sx,int sy,double  date,double scale){
int i,x,y;
double ang,rad,tx,ty;

 for(i=0;i<num_asteroids;i++){
   ang=true_anomaly(a_list[i].m0+date*a_list[i].md,a_list[i].e);
   rad=a_list[i].p/(1+a_list[i].e*cos(ang));
   tx=rad*cos(ang);
   ty=rad*sin(ang);
   x=(sx/2)+scale*(tx*a_list[i].xx+ty*a_list[i].xy);
   y=(sy/2)-scale*(tx*a_list[i].yx+ty*a_list[i].yy);
   if(a_list[i].type==2)
     fill_square(bm,sx,sy,x,y,1,0xff0000);
   else if(a_list[i].type==1)
     draw_square(bm,sx,sy,x,y,1,0xe0e000);
   else
     draw_square(bm,sx,sy,x,y,0,0xc000);
 }

}


/* draws the terrestrial planet */
void draw_solar_system(char *bm,int sx,int sy,double jd,double scale){
double x,y,m_now;

  /* draw sun */
  fill_circle(bm,sx,sy,sx/2,sy/2,8,0xffff00);
  /* earth */
  draw_orbit(bm,sx,sy,1.0,0.0167,0.00035,357.8,105.245,scale);
  m_now= 23.05797 +  (jd - 2450840.5 ) * 0.985606 ;
  get_position(1.0,0.0167,0.00035,357.8,105.245,m_now,&x,&y);
  x=sx/2+scale*x;
  y=sy/2-scale*y;
  fill_circle(bm,sx,sy,x,y,5,0x00ffff);

  /* test earth */
  /*
    draw_orbit(bm,sx,sy,1.0,0.0167,0.0,0.0,102.252,scale);
    m_now= 100.15 +  (jd - jd_conv(1.5,1,1960) ) * 0.985606 ;
    get_position(1.0,0.0167,0.0,0.0,102.252,m_now,&x,&y);
    x=sx/2+scale*x;
    y=sy/2-scale*y;
    fill_circle(bm,sx,sy,x,y,5,0x00ffff);
  */

  /* mercury */
  draw_orbit(bm,sx,sy,0.3870972,0.2056321,7.00507,48.3333,29.1192,scale);
  m_now = 171.74808 +  (jd - 2450840.5 ) *4.092362;
  get_position(0.3870972,0.2056321,7.00507,48.3333,29.1192,m_now,&x,&y);
  x=sx/2+scale*x;
  y=sy/2-scale*y;
  fill_circle(bm,sx,sy,x,y,4,0xffff);

  /* venus */
  draw_orbit(bm,sx,sy,0.7233459,0.0067929,3.39460,76.6870,54.871,scale);
  m_now = 1.72101 +  (jd - 2450840.5 ) * 1.602085;
  get_position(0.7233459,0.0067929,3.39460,76.6870,54.871,m_now,&x,&y);
  x=sx/2+scale*x;
  y=sy/2-scale*y;
  fill_circle(bm,sx,sy,x,y,4,0xffff);

  /*mars*/
  draw_orbit(bm,sx,sy,1.5237118,0.0934472,1.84990,49.5629,286.4653,scale);
  m_now = 10.2393 +  (jd - 2450840.5 ) * 0.5240224;
  get_position(1.5237118,0.0934472,1.84990,49.5629,286.4653,m_now,&x,&y);
  x=sx/2+scale*x;
  y=sy/2-scale*y;
  fill_circle(bm,sx,sy,x,y,4,0xffff); 

}




int init(int w, int h)
{
  char *window_name = "NEO";
  char *icon_name = "NEO";
  XSizeHints size_hints;
  Window rootwin;
  
  display = XOpenDisplay(NULL);
  
  if (display == NULL)
    {
      printf("NEO: Failed to open display\n");
      return 1;
    }
  
  screen = DefaultScreen(display);
  depth = DefaultDepth(display, screen);
  rootwin = RootWindow(display, screen);
  gc=DefaultGC(display,screen);
  win = XCreateSimpleWindow(display, rootwin, 10, 10, w, h, 5,
			    BlackPixel(display, screen), BlackPixel(display, screen));
  
  size_hints.flags = PSize | PMinSize | PMaxSize;
  size_hints.min_width = w;
  size_hints.max_width = w;
  size_hints.min_height = h;
  size_hints.max_height = h;
  
  XSetStandardProperties(display, win, window_name, icon_name, None,
			 0, 0, &size_hints); 
  
  XSelectInput(display, win, ExposureMask|StructureNotifyMask);
  XMapWindow(display, win);
  if(verbose)
    printf("NEO: %ix%i window opened\n",w,h);
  return 0;
}

void eventloop()
{
  XEvent xev;
  int num_events;

  while(1)
    {
      XFlush(display);
      num_events = XPending(display);
      if(num_events==0) return;
      while((num_events != 0))
	{
	  num_events--;
	  XNextEvent(display, &xev);
	  if(xev.type==DestroyNotify)
	    exit(0);
	  if(xev.type==Expose && image!=0)
	    {
	      if(xshm)
		XShmPutImage(display,win,gc,image,0,0,0,0,image->width,
			     image->height,0);
	      else if(!xdpm)
		XPutImage(display,win,gc,image,0,0,0,0,image->width,image->height);
	    }
	}
    }
}


inline int compose(int i, int shift)
{ if(shift<0) return (i>>(-shift)); else return (i<<shift); }

void putlinegeneric8(unsigned char *buf, int y, int y2)
{
  int j,a;
  unsigned char *dest;

  dest=(unsigned char *)(image->data+y*image->bytes_per_line);
  a=0;
  while(y<y2)
    {
      for(j=0;j<width;j++,a+=3)
	{
	  dest[j]=(compose(buf[a],redshift)&image->red_mask)+
	    (compose(buf[a+1],blueshift)&image->blue_mask)+
	    (compose(buf[a+2],greenshift)&image->green_mask);
	}
      dest+=image->bytes_per_line; y++;
    }
}

void putlinegeneric16(unsigned char *buf, int y, int y2)
{
  int j,a;
  unsigned short *dest;

  a=0;
  while(y<y2)
    {
      dest=(unsigned short *)(image->data+y*image->bytes_per_line);
      y++;

      for(j=0;j<width;j++,a+=3)
	{
	  dest[j]=(compose(buf[a],redshift)&image->red_mask)+
	    (compose(buf[a+1],blueshift)&image->blue_mask)+
	    (compose(buf[a+2],greenshift)&image->green_mask);
	}
    }
}

void putline16RGB565(unsigned char *buf, int y, int y2)
{
  int j,a;
  unsigned short *dest;
  a=0;
  while(y<y2)
    {
      dest=(unsigned short *)(image->data+y*image->bytes_per_line);
      y++;
      for(j=0;j<width;j++,a+=3)
	{
	  dest[j]=((((buf[a]<<8)|(buf[a+2]>>3))&0xf81f)|((buf[a+1]<<3)&0x7e0));
	}
    }
}

void putlinegeneric32(unsigned char *buf, int y, int y2)
{
  int j,a;
  unsigned int *dest;

  a=0;
  while(y<y2)
    {
      dest=(unsigned int *)(image->data+y*image->bytes_per_line);
      y++;
      for(j=0;j<width;j++,a+=3)
	{
	  dest[j]=(compose(buf[a],redshift)&image->red_mask)+
	    (compose(buf[a+1],blueshift)&image->blue_mask)+
	    (compose(buf[a+2],greenshift)&image->green_mask);
	}
    }
}

void putline32RGB888(unsigned char *buf, int y, int y2)
{
  int j,a;
  unsigned int *dest;

  a=0;
  while(y<y2)
    {
      dest=(unsigned int *)(image->data+y*image->bytes_per_line);
      y++;
      
      for(j=0;j<width;j++,a+=3)
	{
	  dest[j]=((buf[a]<<16)|(buf[a+1]<<8)|(buf[a+2]));
	}
    }
}


void create_image(char *buf,int time){
	int i,j;
	long ref;
	for(i=0;i<height;i++){
		for(j=0;j<width;j++){
			ref=(i*width + j)*3;
			buf[ref]=time%255;
			buf[ref+1]=time%255;
			buf[ref+2]=time%255;
		}
	}
}

void coreloop(){
	int i,k;
	unsigned char *buf;
	void (*f)(unsigned char *buf, int y, int y2);
	int tlast, fcount;
	long ftime=0;
	buf=malloc(height*width*3);
	fprintf(stderr,"%d\n",image->bits_per_pixel);
	switch(image->bits_per_pixel)
	{
	case 8: f=&putlinegeneric8; 
		break;
	case 16:
		if(image->red_mask==0xf800 &&
		   image->green_mask==0x7e0 &&
		   image->blue_mask==0x1f)
		{
			f=&putline16RGB565;
		}
		else
		{
			f=&putlinegeneric16;
		}
		break;
	case 32:
		if(image->red_mask==0xff0000 &&
		   image->green_mask==0xff00 &&
		   image->blue_mask==0xff)
		{
			f=&putline32RGB888;
		}
		else
		{
			f=&putlinegeneric32;
		}
		break;
	}
	eventloop();
	while(1){
		blank_bitmap(buf,width,height);
		draw_asteroids_fast(buf,width,height,ftime,AU_SCALE);
		draw_solar_system(buf,width,height,ftime+2450000.0,AU_SCALE);
		ftime++;
		fprintf(stderr,"%d \r",ftime+2450000);
		f(buf,0,height);
		if(xshm) {
			XShmPutImage(display,win,gc,image,0,0,0,0,image->width,
				     image->height,0);
		} else {
 			XPutImage(display,win,gc,image,0,0,0,0,
				  image->width,image->height);
		}
		eventloop();
	}
}

void mainloop_noshm()
{
  int i,j;
  XColor color;
  if(verbose) puts("NEO: mainloop_noshm()");
  while(1)
    {
      for(i=0;i<height;i++)
	for(j=0;j<width;j++)
	  {
	    eventloop();
	    color.red=getchar()*256;
	    if(feof(stdin)) return;
	    color.green=getchar()*256;
	    if(feof(stdin)) return;
	    color.blue=getchar()*256;
	    if(feof(stdin)) return;
	    XAllocColor(display,DefaultColormap(display,screen),&color);
	    XSetForeground(display,gc,color.pixel);
	    XDrawPoint(display,win,gc,j,i);
	  }
    }
}


int initimage()
{
  int i;

  if(!image)
    {
      puts("NEO: XCreateImage failed");
      return 1;
    }

  if(image->red_mask==0 ||
     image->green_mask==0 ||
     image->blue_mask==0)
    {
      printf("NEO: could not get an RGB pixmap");
      return 1;
    }
  i=image->red_mask; redshift=-8;
  while(i) { i/=2; redshift++; }
  i=image->green_mask; greenshift=-8;
  while(i) { i/=2; greenshift++; }
  i=image->blue_mask; blueshift=-8;
  while(i) { i/=2; blueshift++; }
  if(verbose)
    printf("NEO: RGB masks are (%X,%X,%X), shifts are (%i,%i,%i)\n",
	   image->red_mask,image->green_mask,image->blue_mask,
	   redshift,greenshift,blueshift);
  if((image->bits_per_pixel!=16) && (image->bits_per_pixel!=32) &&
     (image->bits_per_pixel!=8))
    {
      printf("NEO: image has %i bits per pixel, only supported values are "
	     "8, 16 and 32\n",image->bits_per_pixel);
      return 1;
    }
  return 0;
}

void mainloop_shm()
{
  XShmSegmentInfo shminfo;

  if(verbose)
    puts("NEO: mainloop_shm()");

  image=XShmCreateImage(display,DefaultVisual(display,screen),
			DefaultDepth(display,screen),ZPixmap,
			NULL,&shminfo,width,height);
  if(initimage(image))
    return;

  shminfo.shmid=shmget(IPC_PRIVATE,image->bytes_per_line*image->height,
		       IPC_CREAT|0777);
  if(shminfo.shmid<0)
    {
      puts("NEO: shmget failed");
      perror("NEO");
      return;
    }
  else
    {
      if(verbose)
	printf("NEO: shmget returned shmid=%i\n",shminfo.shmid);
    }

  shminfo.shmaddr = image->data = shmat(shminfo.shmid, 0, 0);
  if(shminfo.shmaddr == (char *)-1)
    {
      puts("NEO: shmat failed");
      return;
    }
  shminfo.readOnly = 0;
  XShmAttach(display, &shminfo);
  if(shmctl(shminfo.shmid,IPC_RMID,0))
    {
      printf("NEO: error in shmctl; you may need to delete shared memory\n"
	     "NEO: resource %i manually after exit (see ipcrm(8))\n",
	     shminfo.shmid);
      perror("NEO: error message was");
    }
 
  coreloop(&putlinegeneric16);
}

void mainloop_buffered()
{
  char *buf;
  int bpl,nbits;

  if(verbose)
    puts("NEO: mainloop_buffered()");

  nbits=DefaultDepth(display,screen);
  if(nbits!=8 && nbits!=16 && nbits!=32)
    {
      printf("NEO: Error: bits_per_rgb is %i, only 8, 16 or 32 is"
	     " supported.\n",
	     nbits);
      return;
    }

  bpl=width*nbits/8;
  if(nbits==16 && (bpl&1)) bpl++;
  if(nbits==32 && (bpl&3)) bpl+=4-(bpl&3);
  
  buf=malloc(height*bpl);
  image=XCreateImage(display,DefaultVisual(display,screen),
		     DefaultDepth(display,screen),ZPixmap,
		     0,buf,width,height,nbits,bpl);
  if(initimage(image)) return;
  coreloop(&putlinegeneric16);
}


int main(int argc, char *argv[])
{
  int ignore, major, minor;
  Bool pixmaps;
  int noshm=0,sleeploop=0,ppm=0;
  int i,j,k;
  char s[1000];
  width=height=800; j=0;

  if(init(width,height))
    {
      puts("NEO: Could not initialize window");
      return -1;
    }
  load_asteroids("astorb.dat",2450000,
		 sqrt(width*width+height*height)*0.5/AU_SCALE);
  printf("%d asteroids used\n",num_asteroids);

  if(!xdpm)
    {
      if(noshm==0)
	{
	  if( XQueryExtension(display, "MIT-SHM", &ignore, &ignore, &ignore) )
	    {
	      if(verbose)
		puts("NEO: Found MIT-SHM; looking for version number");
	      if(XShmQueryVersion(display, &major, &minor, &pixmaps) == True)
		{
		  if(verbose)
		    printf("NEO: XShm extention version "
			   "%d.%d %s shared pixmaps\n",
			 major, minor, (pixmaps==True) ? "with" : "without");
		  xshm=1;
		  xshmevent=XShmGetEventBase(display)+ShmCompletion;
		} 
	    }
	  else
	    {
	      if(verbose)
		puts("NEO: MIT-SHM not found");
	    }
	}
      else
	{
	  if(verbose)
	    puts("NEO: MIT-SHM disabled");
	}
    }
  else
    {
      if(verbose)
	puts("NEO: XDrawPoint mode");
    }
  if(buffered)
    {
      if(verbose)
	puts("NEO: screen-buffered mode");
    }
  if(verbose && nodraw)
    puts("NEO: nodraw mode");
  if(xshm)
    mainloop_shm();
  else 
    mainloop_buffered();
  if(sleeploop)
    {
      while(1) sleep(10);
    }
  return 0;
}



