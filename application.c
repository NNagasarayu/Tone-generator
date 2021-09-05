#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include "stm32f4xx_dac.h"
#include <stdlib.h>
#include <stdio.h>


char length[32] = {'a',  'a',  'a',  'a',  'a',  'a',  'a',  'a',  'a',  'a',  'b', 'a',  'a',  'b',  'c',  'c',  'c',  'c',  'a',  'a',  'c',  'c',  'c',  'c',  'a',  'a',  'a',  'a',  'b',  'a',  'a',  'b'}; 
typedef struct {
    Object super;
    char c;
	int i;
    int volupdate;
    int num;
    char n[20];
	int key;
	int tempo;
    char buf[20];
} App;

typedef struct {
    Object super;
    int volume;
    int val;
    int count;
    int p;
    int s;
    int u_key;
    int period[25];
    int Freqi[32];
      
    
} Volume;

typedef struct {
    Object super;
    int u_tempo;
    int c_key;
    int ap;
    int bp;
    int cp;
    int j;

} load;




//Method
void tonegen(Volume *, int);
void reader(App*, int);
void receiver(App*, int);
void silence(Volume *, int);
void updateperiod(Volume *, int);
void silence(Volume *, int);
void nsilence(Volume *, int);
void clength(load *, int);
void utempo(load *, int);
void ukey(load *, int);
void csilence(load *, int);



//Initialization Macro
#define dacreg ((char*)(0x4000741C))
#define initVolume() {initObject()}
#define initApp() {initObject(), 0}
#define initload() {initObject()}



//Object
Volume Task1 = { initObject(),5,5,0,0,0,0};
load Task2 = { initObject(),120,0,450,950,200,0};
App app = { initObject(),'X',0,5};
Serial sci0 = initSerial(SCI_PORT0, &app, reader);
Can can0 = initCan(CAN_PORT0, &app, receiver);


//volume
void tonegen(Volume *self, int value){

 
 if( self->count % 2 == 0)
     {
        *dacreg = self->volume;

    }
    else
    {
    
	    *dacreg = 0;
    }
    self->count++; 
 
    self->p = self->u_key + self->Freqi[self->s]+10;
    AFTER(USEC(self->period[self->p]), self, tonegen, value); 
}
    
    
void silence(Volume *self, int unused)
{
    self->volume = 0;

   
}
void nsilence(Volume *self, int value1)
{   
    

    self->volume = self->val;
    self->s = value1;
}

void updatevolume(Volume *self, int value5)
{ char lv[32];
  self->val =value5;
  snprintf(lv, 6, "%d\n", self->val);
  SCI_WRITE(&sci0, lv);
}

    
void updateperiod(Volume *self, int value2)
{  
        self->u_key = value2;
    }


   





//load
void utempo(load *self, int value3)
{
     self->u_tempo = value3;
     self-> ap= (60000/self->u_tempo)-50;
     self-> bp= (120000/self->u_tempo)-50;
     self-> cp= (30000/self->u_tempo)-50; 
        
}

void ukey(load *self, int value4){    
       self->c_key = value4;
       ASYNC(&Task1, updateperiod, self->c_key);
}

void clength(load *self, int unused){
        ASYNC(&Task1, nsilence, self->j);
       
 if (self->j<32)
     
     {

         if (length[self->j] == 'a')       
        {  
        
        SEND(MSEC(self->ap),MSEC(100), self, csilence, 0);   
         self->j++;
        }
        else if (length[self->j] == 'b')       
        {  
       
        SEND(MSEC(self->bp),MSEC(100), self, csilence, 0);  
         self->j++;
        }
        else if (length[self->j] == 'c')       
        {  
            
         SEND(MSEC(self->cp),MSEC(100), self, csilence, 0);  
         self->j++;
        }
 }
        else{
            self->j = 0;
            SEND(MSEC(self->ap),MSEC(100), self, csilence, 0);  
            self->j++;
            }
 }


void csilence(load *self, int unused)
{       
        ASYNC(&Task1, silence, 0);
        SEND(MSEC(50),MSEC(100), self, clength, 0);
        
    }




//app
void receiver(App *self, int unused) {
    CANMsg msg;
    CAN_RECEIVE(&can0, &msg);
    SCI_WRITE(&sci0, "Can msg received: ");
    SCI_WRITE(&sci0, msg.buff);
}


void reader(App *self, int c) {
	
   SCI_WRITE(&sci0, "Rcv: \'");
   SCI_WRITECHAR(&sci0, c);
   SCI_WRITE(&sci0, "\'\n");
   
   
   if(c == 'i' && self->volupdate >= 1 && self->volupdate <= 19){                                                    //Increase the volume - i  
		self->volupdate++;
		ASYNC(&Task1, updatevolume, self->volupdate);
	}
	else if(c == 'r'&& self->volupdate > 1 && self->volupdate <= 20 ){                                               //Reduce the volume - r 
		self->volupdate--;
		ASYNC(&Task1, updatevolume, self->volupdate);
	}
	else if(c == 'm'){                                              //Mute the volume - m

		ASYNC(&Task1, updatevolume, 0);
	}
	else if(c == 'l'){                                              //Unmute the volume - l 
  
		ASYNC(&Task1, updatevolume, self->volupdate);
	}
   else if(c== 'i' && self->volupdate >= 20){
        self->volupdate = 20;
		ASYNC(&Task1, updatevolume, self->volupdate);
	}
    else if(c == 'r'&& self->volupdate <= 1 )
        
    {
        self->volupdate = 1;
		ASYNC(&Task1, updatevolume, self->volupdate);
	
	}

   
   	if(c != 'e' && c != 'i' && c != 'r' && c != 'm' && c != 'l' )
        {
		self->buf[self->i] = c;
		self->i++;
       	}
    
        
     //Enter the key and type 'e' to end the input integer 
	else if (c == 'e' && c != 'i' && c != 'r' && c != 'm' && c != 'l')
        {  
		self->buf[self->i] = '\0';
		self->num = atoi(self->buf);
        if (self->num >=-5 && self->num <=5)
        {
            
			self->key = self->num;
             
            SCI_WRITE(&sci0, "key ");
            snprintf(self->n, 6, "%d\n", self->key);
            SCI_WRITE(&sci0, self->n);
            self->i = 0;
            ASYNC(&Task2, ukey, self->key);
             
		}
		else if (self->num >=60 && self->num <=240)
		{  char t[32];
            
			self->tempo = self->num;
            SCI_WRITE(&sci0, "tempo: ");
            snprintf(t, 6, "%d\n", self->tempo);
            SCI_WRITE(&sci0, t);
            self->i = 0;
            ASYNC(&Task2, utempo, self->tempo );
		}
        
	     
         
        }
}

void startApp(App *self, int arg) {
    CANMsg msg;

    CAN_INIT(&can0);
    SCI_INIT(&sci0);
    SCI_WRITE(&sci0, "Hello, hello...\n");
    int period_1[25] ={2024, 1911, 1803, 1702, 1607, 1516, 1431, 1351, 1275, 1203, 1136, 1072, 1012, 955, 901, 851, 803, 758, 715, 675, 637, 601, 568, 536, 506};
	for (int k=0; k<25; k++)
	{
		Task1.period[k]= period_1[k];
	}
    int index[32] = { 0, 2, 4, 0, 0, 2, 4, 0, 4, 5, 7, 4, 5, 7, 7, 9, 7, 5, 4, 0, 7, 9, 7, 5, 4, 0, 0, -5, 0, 0, -5, 0};
    for (int l=0; l<32; l++)
	{
		Task1.Freqi[l]=index[l];
	}
    msg.msgId = 1;
    msg.nodeId = 1;
    msg.length = 6;
    msg.buff[0] = 'H';
    msg.buff[1] = 'e';
    msg.buff[2] = 'l';
    msg.buff[3] = 'l';
    msg.buff[4] = 'o';
    msg.buff[5] = 0;
    CAN_SEND(&can0, &msg);
	ASYNC(&Task1, tonegen, 0);
    ASYNC(&Task2, clength, 0);
}

int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
	INSTALL(&can0, can_interrupt, CAN_IRQ0);
    TINYTIMBER(&app, startApp, 0);
    return 0;
}