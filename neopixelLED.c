#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <fcntl.h>

#include <malloc.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <signal.h>
#include <sched.h>

#define NbLeds 1


typedef struct{
  unsigned char R,G,B,W;
}ledStruct;


union ledUnion {
  unsigned long dw;
  ledStruct RGBW;
};



union ledUnion leds[NbLeds];



int fd;


void refreshDisplay(void);

static void pabort(const char *s)
{
   perror(s);
   abort();
}


static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 3000000;
static uint16_t delay;


void writeSPI(unsigned char * array, int length)
{
   int ret;
   struct spi_ioc_transfer tr = {
   .tx_buf = (unsigned long) array,
   .rx_buf = (unsigned long) array,
   .len = length,
   .delay_usecs = 0,
   .speed_hz= speed,
   .bits_per_word = bits,
  };

  ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
  if(ret < 1)
    pabort("Can't send spi message");
}

void   closeSPI(void)
 {
    if(fd >=0)
     {
       // clear screen
       memset(leds,0,sizeof(leds));
       refreshDisplay();
       close(fd);
       fd=-1;
     }
}

void Ctrl_C_Handler(int value)
  {
    closeSPI();
    exit(0);
  }



void fillColor(unsigned char * colorPt,int RGBWvalue)
{
   int loop;
   unsigned char _temp;

   for(loop=0;loop < 8;loop+=2)
   {
        _temp = (RGBWvalue & 0x80) ? 0xC0 : 0X80;
        _temp |= (RGBWvalue & 0x40) ? 0xC : 0X8;
        *(colorPt++)=_temp;
        RGBWvalue <<= 2;
   }
}


void refreshDisplay(void)
{
   int loop;
   if(fd < 0) return;

   // set the reset 50 us minimum then at 3Mhz we will used 20 ~60us
   int ResetCount=50;

   unsigned char  * bufferSPI;
   unsigned char  * bufferPtr;
   bufferSPI = malloc(ResetCount + NbLeds*32);

   memset(bufferSPI,0,ResetCount);
   bufferPtr= &bufferSPI[ResetCount];
   for(loop=0;loop<NbLeds;loop++)
     {
        fillColor(bufferPtr,leds[loop].RGBW.G);
        bufferPtr+=4;
        fillColor(bufferPtr,leds[loop].RGBW.R);
        bufferPtr+=4;
        fillColor(bufferPtr,leds[loop].RGBW.B);
        bufferPtr+=4;
        fillColor(bufferPtr,leds[loop].RGBW.W);
        bufferPtr+=4;
     }

   writeSPI(bufferSPI, ResetCount+NbLeds*32);
   free(bufferSPI);
 }


unsigned long  randomLed(void)
{
   int r,g,b,w;
   union ledUnion Rled;

   // set all  colors to 0
   Rled.dw=0;

// ok only pure color R,G,B,or W

   unsigned char level = (rand() % 192) + 64;
   switch(rand() % 4)
   {
     case 0 : // RED
              Rled.RGBW.R = level;
              break;
     case 1 : // GREEN
              Rled.RGBW.G = level;
              break;
     case 2 : // BLUE
              Rled.RGBW.B = level;
              break;
     default : // white
              Rled.RGBW.W = level;
   }

   return Rled.dw;
}


void set_max_priority(void) {
  struct sched_param sched;
  memset(&sched, 0, sizeof(sched));
  // Use FIFO scheduler with highest priority for the lowest chance of the kernel context switching.
  sched.sched_priority = sched_get_priority_max(SCHED_FIFO);
  sched_setscheduler(0, SCHED_FIFO, &sched);
}

void set_default_priority(void) {
  struct sched_param sched;
  memset(&sched, 0, sizeof(sched));
  // Go back to default scheduler with default 0 priority.
  sched.sched_priority = 0;
  sched_setscheduler(0, SCHED_OTHER, &sched);
}



int main(int argc, char *argv[])
{
   int ret = 0;
   int loop;



   unsigned char Table[NbLeds*2];


   // initialize random
    time_t t;
    srand((unsigned) time(&t));

   //create table
   for(loop=0;loop<NbLeds;loop++)
    {
      Table[loop]=loop;
      Table[loop+NbLeds]= NbLeds - loop -1;
    }

   // clear Leds
   memset(leds,0,sizeof(leds));

   fd = open("/dev/spidev0.0",O_RDWR);
   if(fd <0)
     pabort("Can't open device\n");

   mode = 0;

   ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
   if (ret == -1)
      pabort("can't set spi mode");

   ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
   if (ret == -1)
      pabort("can't get spi mode");

   /*
    * bits per word
    */
   ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
   if (ret == -1)
      pabort("can't set bits per word");

   ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
   if (ret == -1)
      pabort("can't get bits per word");

    /*
    * max speed hz
    */
   ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
   if (ret == -1)
      pabort("can't set max speed hz");

   ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
   if (ret == -1)
      pabort("can't get max speed hz");


   atexit(closeSPI);
   signal(SIGINT,Ctrl_C_Handler);

   printf("speed = %d\n",speed);
   printf("bits  = %d\n",bits);
   printf("mode  = %d\n",mode);

   set_max_priority();

   while(1)
    {
     unsigned c1,c2,c3;
     c1 = randomLed();
     c2 = randomLed();
     c3 = randomLed();

     for(loop=0;loop<(NbLeds*2);loop++)
      {
       memset(leds,0,sizeof(leds));
       leds[Table[loop]].dw= c1;
       leds[Table[((NbLeds*2/3) +loop) % (NbLeds *2)]].dw|=c2;
       leds[Table[((NbLeds*4/3) +loop) % (NbLeds *2)]].dw|=c3;
       refreshDisplay();
       usleep(50000);
      }
   }


return 0;
}
