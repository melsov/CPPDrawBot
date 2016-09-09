/*************************************************************
  SPI_Raspi_Arduino
   Configures an ATMEGA as an SPI slave and demonstrates
   a basic bidirectional communication scheme
   A Raspberry Pi SPI master transmits commands to
   perform addition and subtraction on a pair of integers and
   the Ardunio transmits the result
****************************************************************/

/***************************************************************
  Global Variables
  -receiveBuffer[] and dat are used to capture incoming data
   from the Raspberry Pi
  -marker is used as a pointer to keep track of the current
   position in the incoming data packet
***************************************************************/

unsigned char receiveBuffer[6];
unsigned char dat;
byte marker = 0;


/*************************************************************
  Unions allow variables to occupy the same memory space a
  convenient way to move back and forth between 8-bit and
  16-bit values etc.  Here three unions are declared:
  two for parameters that are passed in commands to the Arduino
  and one to receive  the results
 ***************************************************************/

union
{
  int p1Int;
  unsigned char  p1Char [2];
} p1Buffer;

union
{
  int p2Int;
  unsigned char p2Char [2];
} p2Buffer;


union
{
  int resultInt;
  unsigned char  resultChar [2];
} resultBuffer;


//MMP stepper pins
#define TWON
#ifdef TWO
#define stp 5
#define dir 4
#define stwo 2
#define dtwo 3
#else
#define stp 2
#define dir 3
#define stwo 4
#define dtwo 5
#endif
int x, y;
bool switchDir;

/***************************************************************
  Setup SPI in slave mode (1) define MISO pin as output (2) set
  enable bit of the SPI configuration register
****************************************************************/

void setup (void)
{
  pinMode(MISO, OUTPUT);
  SPCR |= _BV(SPE);

  //Motor pins
  pinMode(stp, OUTPUT);
  pinMode(dir, OUTPUT);
  pinMode(stwo, OUTPUT);
  pinMode(dtwo, OUTPUT);

}


/***************************************************************
  Loop until the SPI End of Transmission Flag (SPIF) is set
  indicating a byte has been received.  When a byte is
  received, call the spiHandler function.
****************************************************************/

void loop (void)
{

  if ((SPSR & (1 << SPIF)) != 0)
  {
    spiHandler();
  }

}

/***************************************************************
  spiHandler
   Uses the marker variable to keep track current position in the
   incoming data packet and execute accordingly
   0   - wait for to receive start byte - once received send
         the acknowledge byte
   1   - the command to add or subtract
   2-5 - two integer parameters to be added or subtracted
       - when the last byte (5) is received, call the
         executeCommand function and load the first byte of the
         result into SPDR
   6   - transmit the first byte of the result and load the
         second byte into SPDR
   7   - transmit the second byte of of the result and reset
         the marker
****************************************************************/
//MMP: wall wart outputs 12v, 2 amps

void spiHandler()
{
  switch (marker)
  {
    case 0:
      dat = SPDR;
      if (dat == 'c')
      {
        SPDR = 'a';
        marker++;
      }
      break;
    case 1:
      receiveBuffer[marker - 1] = SPDR;
      marker++;
      break;
    case 2:
      receiveBuffer[marker - 1] = SPDR;
      marker++;
      break;
    case 3:
      receiveBuffer[marker - 1] = SPDR;
      marker++;
      break;
    case 4:
      receiveBuffer[marker - 1] = SPDR;
      marker++;
      break;
    case 5:
      receiveBuffer[marker - 1] = SPDR;
      marker++;
      break;
    case 6:
      receiveBuffer[marker - 1] = SPDR;
      marker++;
      moveMotors();
//      executeCommand();
//      stepForwardDefault();
      SPDR = resultBuffer.resultChar[0];
      break;
    case 7:
      marker++;
      SPDR = resultBuffer.resultChar[1];
      break;
    case 8:
      dat = SPDR;
      marker = 0;
  }

}



/***************************************************************
  executeCommand
   When the complete 5 byte command sequence has been received
   reconstitute the byte variables from the receiveBuffer
   into integers, parse the command (add or subtract) and perform
   the indicated operation - the result will be in resultBuffer
****************************************************************/

void  moveMotors(void)
{
  digitalWrite(dir, receiveBuffer[0] == 'f' ? HIGH : LOW);
  digitalWrite(dtwo, receiveBuffer[3] == 'f' ? HIGH : LOW);
  p1Buffer.p1Char[0] = receiveBuffer[1];
  p1Buffer.p1Char[1] = receiveBuffer[2];
  p2Buffer.p2Char[0] = receiveBuffer[4];
  p2Buffer.p2Char[1] = receiveBuffer[5];

  int times = p1Buffer.p1Int > p2Buffer.p2Int ? p1Buffer.p1Int : p2Buffer.p2Int;
  for(int i=0; i < times; ++i) {
    if (i<p1Buffer.p1Int) {
      digitalWrite(stp, HIGH);
    }
    if(i<p2Buffer.p2Int){
      digitalWrite(stwo,HIGH);
    }
    delay(1);
    if (i<p1Buffer.p1Int) {
      digitalWrite(stp, LOW);
    }
    if(i<p2Buffer.p2Int){
      digitalWrite(stwo,LOW);
    }
  }

  resultBuffer.resultInt = p2Buffer.p2Int;
}

void stepForwardDefault()
{
  digitalWrite(dir, switchDir ? HIGH : LOW); //Pull direction pin low to move "forward"
  digitalWrite(dtwo, switchDir ? HIGH : LOW);
  switchDir = !switchDir;
  for (x = 1; x < 1600; x++) //Loop the forward stepping enough times for motion to be visible
  {
    digitalWrite(stp, HIGH); //Trigger one step forward
    digitalWrite(stwo, HIGH);
    delay(1);
    digitalWrite(stp, LOW); //Pull step pin low so it can be triggered again
    digitalWrite(stwo, LOW);
    delay(1);
  }

}

//
//void executeCommand(void)
//{
//  p1Buffer.p1Char[0] = receiveBuffer[1];
//  p1Buffer.p1Char[1] = receiveBuffer[2];
//  p2Buffer.p2Char[0] = receiveBuffer[3];
//  p2Buffer.p2Char[1] = receiveBuffer[4];
//
//  if (receiveBuffer[0] == 'a')
//  {
//    resultBuffer.resultInt = p1Buffer.p1Int + p2Buffer.p2Int;
//  }
//  else if (receiveBuffer[0] == 's')
//  {
//    resultBuffer.resultInt = p1Buffer.p1Int - p2Buffer.p2Int;
//  }
//
//}
