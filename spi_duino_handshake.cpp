/**********************************************************
 SPI_Raspi_Arduino
   Configures an Raspberry Pi as an SPI master and
   demonstrates a basic bidirectional communication scheme
   with an Arduino slave.  The Raspberry Pi transmits
   commands to perform addition and subtraction on a pair
   of integers and the Ardunio returns the result

Compile String:
g++ -o SPI_Raspi_Arduino SPI_Raspi_Arduino.cpp
***********************************************************/

#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>

#include <stdlib.h>
#include <math.h>

#include "fileio/readfile.h"

#define PI 3.1415926535897932384626433

using namespace std;


struct Instruction{
  char dir;
  int steps;
  void set(int i){
    dir = i < 0 ? 'r' : 'f';
    steps = abs(i);
  }
};

struct Coord{
  float x,y;
  float mag(){ return sqrtf(x*x + y*y); }
  void clone(Coord other){
    x = other.x;
    y = other.y;
  }
  void add(Coord other){ 
    x += other.x;
    y += other.y;
  }
  void subtract(Coord other){
    x -= other.x;
    y -= other.y;
  }
  void times(float f){
    x *= f;
    y *= f;
  }
  void debug(){ cout << "Coord: " << x << ", " << y << endl; }
};

struct MachineConfig{
  float width;
  Coord leftPos;
  Coord rightPos;
  float spoolRadius;
  float stepsPerRev;
  float dist(int steps){
    return PI * 2.0 * spoolRadius * (steps/stepsPerRev);
  }
  float leftAngleRadians(float llen, float rlen){
    float cosAngL = (width*width + llen*llen - rlen*rlen)/(2*width*llen);
    return acosf(cosAngL);
  }
  void debug() { 
    cout << " width: " << width << " spool radius: " << spoolRadius << " steps-per-rev: " << stepsPerRev << endl; 
  } 
};

MachineConfig config;
char configFile[] = "fileio/config.txt";
char pointsFile[] = "fileio/points.txt";
Coord position;

Instruction linst;
Instruction rinst;

/**********************************************************
Housekeeping variables
***********************************************************/
int results;
int fd;

/**********************************************************
Declare Functions
***********************************************************/

int spiTxRx(unsigned char txDat);
int sendMotor(char i, int j, char k, int l);
int sendCommand(char i, int j, int k);
bool updateInstructions();
Coord stringToCoord(const char * instring);
void coordToInstruction(Coord target); 
void setup();
float configValue(const char * instring);

/****
 * BTW: CONVENTION: YPOS points down from motors
 *****/

void setup(){
  initFileToRead(configFile);
  config.width = configValue(nextLine());
  config.rightPos.x = config.width;
  config.spoolRadius = configValue(nextLine());
  config.stepsPerRev = configValue(nextLine());
  position.x = configValue(nextLine());
  position.y = configValue(nextLine());
}

bool updateInstructions(){
  const char * line = nextLine();
  if(line != NULL){
    cout << line << endl;
    Coord inCoord = stringToCoord(line);
    inCoord.debug();
    coordToInstruction(inCoord);
    return true;
  }
  return false;
}

float llen(){
  return position.mag();
}

float rlen(){
  Coord pos;
  pos.clone(position);
  pos.subtract(config.rightPos);
  return pos.mag();
}

float rlenFrom(Coord c){
  c.subtract(config.rightPos);
  return c.mag();
}

void coordToInstruction(Coord target){
  float tarLenL, tarLenR;
  float actualL, actualR;
  int stepsL, stepsR;
  float leftAng;
  tarLenL = target.mag();
  tarLenR = rlenFrom(target);
  stepsL = roundf(tarLenL/config.dist(1));
  stepsR = roundf(tarLenR/config.dist(1));
  linst.set(stepsL);
  rinst.set(stepsR);
  actualL = config.dist(stepsL);
  actualR = config.dist(stepsR);
  leftAng = config.leftAngleRadians(actualL, actualR);
  //set next position
  position.x = actualL * cosf(leftAng);
  position.y = actualL * sinf(leftAng);
}



Coord stringToCoord(const char * instring){
  char * str = strdup(instring); // own strings memory now
  Coord c;
  char * token;
  if(token = strsep(&str, ",")){
    c.x = atof(token); //string to float
  }
  if(token = strsep(&str, ",")){
    c.y = atof(token);
  }
  return c;
}



float configValue(const char * instring){
  char * str = strdup(instring); // own strings memory now
  Coord c;
  char * token;
  if(token = strsep(&str, ":")){
    if(token = strsep(&str, "\0")){
      return atof(token);
    }
  }
  return -1.0;
}

/**********************************************************
Main
***********************************************************/

int main (void)
{
  setup();
  //init read data
  initFileToRead(pointsFile);

/**********************************************************
Setup SPI
Open file spidev0.0 (chip enable 0) for read/write access
with the file descriptor "fd"
Configure transfer speed (1MkHz)
***********************************************************/

   fd = open("/dev/spidev0.0", O_RDWR);

   unsigned int speed = 1000000;
   ioctl (fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

/**********************************************************
An endless loop that repeatedly sends the demonstration
commands to the Arduino and displays the results
***********************************************************/

   while (1)
   {
     if(!updateInstructions()){
       break;
     }
     results = sendMotor(linst.dir, linst.steps, rinst.dir, rinst.steps); 

      cout << "info:" << endl;
      cout << (int)(results) << endl;
      sleep(1);

     }

}


/**********************************************************
spiTxRx
 Transmits one byte via the SPI device, and returns one byte
 as the result.

 Establishes a data structure, spi_ioc_transfer as defined
 by spidev.h and loads the various members to pass the data
 and configuration parameters to the SPI device via IOCTL

 Local variables txDat and rxDat are defined and passed by
 reference.  
***********************************************************/

int spiTxRx(unsigned char txDat)
{
 
  unsigned char rxDat;

  struct spi_ioc_transfer spi;

  memset (&spi, 0, sizeof (spi));

  spi.tx_buf        = (unsigned long)&txDat;
  spi.rx_buf        = (unsigned long)&rxDat;
  spi.len           = 1;

  ioctl (fd, SPI_IOC_MESSAGE(1), &spi);

  return rxDat;
}

/**********************************************************
***********************************************************/


int sendMotor(char jdir, int j, char kdir, int k)
{

unsigned char resultByte;
bool ack;

union p1Buffer_T       
{
  int p1Int;
  unsigned char  p1Char [2];
} p1Buffer;

union p2Buffer_T      
{
  int p2Int;
  unsigned char  p2Char [2];
} p2Buffer;

union resultBuffer_T     
{
  int resultInt;
  unsigned char  resultChar [2];
} resultBuffer;


  p1Buffer.p1Int = j;
  p2Buffer.p2Int = k;
  resultBuffer.resultInt = 0;

  do
  {
    ack = false;

    spiTxRx('c');
    usleep (10);

    resultByte = spiTxRx(jdir); //first dir during handshake
    if (resultByte == 'a')
    {
      ack = true;
    }
    usleep (10);  

   }
  while (ack == false);

/**********************************************************
Send the parameters one byte at a time.
***********************************************************/

  spiTxRx(p1Buffer.p1Char[0]);
  usleep (10);


  spiTxRx(p1Buffer.p1Char[1]);
  usleep (10);

//second dir
spiTxRx(kdir);
usleep(10);


  spiTxRx(p2Buffer.p2Char[0]);
  usleep (10);


  spiTxRx(p2Buffer.p2Char[1]);
  usleep (10);

/**********************************************************
Push two more zeros through so the Arduino can return the
results
***********************************************************/
//pointless. keeping here out of pure paranoia

  resultByte = spiTxRx(0);
  resultBuffer.resultChar[0] = resultByte;
  usleep (10);


  resultByte = spiTxRx(0);
  resultBuffer.resultChar[1] = resultByte;
  return resultBuffer.resultInt;

}
 

int sendCommand(char command, int j, int k)
{

unsigned char resultByte;
bool ack;

/**********************************************************
Unions allow variables to occupy the same memory space
a convenient way to move back and forth between 8-bit and
16-bit values etc.

Here three unions are declared: two for parameters to be 
passed in commands to the Arduino and one to receive
the results
***********************************************************/

union p1Buffer_T       
{
  int p1Int;
  unsigned char  p1Char [2];
} p1Buffer;

union p2Buffer_T      
{
  int p2Int;
  unsigned char  p2Char [2];
} p2Buffer;

union resultBuffer_T     
{
  int resultInt;
  unsigned char  resultChar [2];
} resultBuffer;


  p1Buffer.p1Int = j;
  p2Buffer.p2Int = k;
  resultBuffer.resultInt = 0;

/**********************************************************
An initial handshake sequence sends a one byte start code
('c') and loops endlessly until it receives the one byte 
acknowledgment code ('a') and sets the ack flag to true.
(Note that the loop also sends the command byte while 
still in handshake sequence to avoid wasting a transmit
cycle.)
***********************************************************/

  do
  {
    ack = false;

    spiTxRx('c');
    usleep (10);


    resultByte = spiTxRx(command);
    if (resultByte == 'a')
    {
      ack = true;
    }
    usleep (10);  

   }
  while (ack == false);

/**********************************************************
Send the parameters one byte at a time.
***********************************************************/

  spiTxRx(p1Buffer.p1Char[0]);
  usleep (10);


  spiTxRx(p1Buffer.p1Char[1]);
  usleep (10);


  spiTxRx(p2Buffer.p2Char[0]);
  usleep (10);


  spiTxRx(p2Buffer.p2Char[1]);
  usleep (10);

/**********************************************************
Push two more zeros through so the Arduino can return the
results
***********************************************************/


  resultByte = spiTxRx(0);
  resultBuffer.resultChar[0] = resultByte;
  usleep (10);


  resultByte = spiTxRx(0);
  resultBuffer.resultChar[1] = resultByte;
  return resultBuffer.resultInt;

}
 
