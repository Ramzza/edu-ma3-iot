#include <SPI.h>
#include <string.h>

#define MSG_SIZE 100

SPISettings spi_settings(100000, MSBFIRST, SPI_MODE0);

char requestMsg[MSG_SIZE+1];
char responseMsg[MSG_SIZE+1];

volatile byte spi_index;
volatile bool fogadunk;
volatile bool kuldunk;
volatile bool isMsgReceived;


void cleanRequestMsg()
{
  // Lenullazzuk a fogadott uzenet buffert
  memset(requestMsg,0,sizeof(requestMsg));
}

void setResponse()
{
  // Frissitjuk az aktualis valasz buffert
  int feny = analogRead(A1);
  
  memset(responseMsg,0,sizeof(responseMsg));
  strcpy(responseMsg, String("Feny=" + String(feny)).c_str());
  Serial.print("Response: ");
  Serial.println(responseMsg);
}

void handleLedChange(String cmd)
{  
  // Vegrehajtjuk a LED kapcsolas parancsot
  if (cmd == "ON")
  {    
    digitalWrite(3, 1);
  }
  
  if (cmd == "OFF")
  {
    digitalWrite(3, 0);
  }  
}

void handleCommand()
{  
  if (NULL == strchr(requestMsg, '='))
  {
    // A nem "key=value" formaju uzenet nem erdekel
    return;
  }

  String cmd = String(requestMsg);  
  cmd.trim();  

  Serial.print("cmd: ");
  Serial.println(cmd);
  
  String key = cmd.substring(0, cmd.indexOf('='));
  String value = cmd.substring(cmd.indexOf('=') + 1, cmd.length());

  // Elvegezzuk a parancsnak megfelelo akciot
  if (key.equals("LED"))
  {
    handleLedChange(value);
  } 
}

void setup()
{
  fogadunk = false;
  kuldunk = true;
  isMsgReceived = false;
  spi_index = 0;  
  setResponse();

  Serial.begin(9600);
  SPCR |= bit(SPE); 
  pinMode(MISO, OUTPUT);

  SPI.attachInterrupt();
  pinMode(3, OUTPUT);
}

void loop()
{  
  if (isMsgReceived)
  {
    // Vegrehajtuk a fogadott parancsot
    isMsgReceived = false;
    handleCommand();
  }

  if (!kuldunk)
  {
    // Ha eppen nem kuldunk, akkor frissitjuk a fenyero uzenetet
    setResponse();
  }

  delay(100);
}

ISR(SPI_STC_vect)
{
  char c = SPDR;
  
  if (spi_index == 0)
  {
    // Kuldes utan mindig fogadunk es forditva
    kuldunk = !kuldunk;
    fogadunk = !fogadunk;
  }

  if (fogadunk)
  {
    requestMsg[spi_index] = c;
  }
  
  if (kuldunk && spi_index < strlen(responseMsg))
  {
    SPDR = responseMsg[spi_index];
  }

  spi_index++;

  if(spi_index == MSG_SIZE)
  {
    // Kuldesi/fogadasi ciklus vegen nullazzuk a pointert
    isMsgReceived = fogadunk;
    spi_index = 0;
  }
}
