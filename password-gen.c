//Yeah, I know it's riddiculous..

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>

int main(void){

 int slump,tal,counter=0;
 time_t tid;

 slump=time(&tid);
 printf("Press CTRL-C to abort.\n\n");
 printf("password: ");

 while(1){  
  srandom(slump); 
  slump=random();
  tal=1+(int) (250.0*random()/(RAND_MAX+1.0));
  if(((tal>47)&&(tal<58))||((tal>64)&&(tal<91))||((tal>96)&&(tal<123))){
   if(counter<8){
    printf("%c",(char)tal);
    counter++;
   } else {
    printf("\nPassword generated.");
    getchar();
    printf("\nPassword: ");
    counter=0;
   }
  }
 }  

return(0);
}     
