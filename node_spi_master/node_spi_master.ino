#include <SPI.h>
#include <ESP8266WiFi.h>
#include <string.h>

#define MSG_SIZE 100

const char *ssid = "<your wifi SSID>";
const char *password = "<your wifi password>";
char responseMsg[MSG_SIZE+1];
char requestMsg[MSG_SIZE+1];

bool isLedOn;
bool isLedChanged;

WiFiServer server(80);

SPISettings spi_settings(100000, MSBFIRST, SPI_MODE0);
//100 kHz legyen a sebesseg, a Node tud 80MHzt de az Arduino csak 16MHzt

void setRequestMsg(char msg[MSG_SIZE])
{
  // biztos ami biztos, lenullazzuk a request buffert
  strncpy(requestMsg, msg, MSG_SIZE);
}

void showUI(WiFiClient client)
{
  // HTML weboldal megjelenitese
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("");
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");

  client.print("Led is now: ");

  if (isLedOn)
  {
    client.print("On");
  }
  else
  {
    client.print("Off");
  }
  client.println("<br><br>");
  client.println(responseMsg);
  client.println("<br><br>");

  client.println("<a href=\"/LED=ON\"\"><button>On</button></a>");
  client.println("<a href=\"/LED=OFF\"\"><button>Off</button></a><br />");
  client.println("</html>");

  Serial.println("Web UI was refreshed");
  delay(1);
}

void checkButtonPressed(String request)
{
  // Ellenorizzuk, hogy a user milyen gombot nyomott meg
  if (request.indexOf("/LED=ON") != -1)
  {
    isLedChanged = isLedOn == false;
    isLedOn = true;
  } 
  else if (request.indexOf("/LED=OFF") != -1)
  {
    isLedChanged = isLedOn == true;
    isLedOn = false;
  }
  else
  {
    isLedChanged = false;
  }
}

void writeToSlave()
{
  // Elkuldjuk a parancsot az Arduinonak
  Serial.print("Sending to slave: ");
  Serial.println(requestMsg);

  for (int i = 0; i < MSG_SIZE; i++)
  {
    responseMsg[i] = i < strlen(requestMsg)
      ? SPI.transfer(requestMsg[i])
      : SPI.transfer(' ');
    delay(1);
  }
  delay(100);
  
  setRequestMsg("");
}

void readFromSlave()
{
  // Fogadjuk az uzenetet az Arduinotol
  for(int i=0; i<MSG_SIZE; i++)
  {
    responseMsg[i] = SPI.transfer(' ');
    delay(1);
  }

  Serial.print("Received from slave: ");
  Serial.println(responseMsg);
}

void handleAction()
{
  // Ha van parancs modosulas, akkor beallitjuk
  // Ez lesz majd elkuldve az Arduinonak
  if(isLedChanged)
  {
    if (isLedOn)
    {
      setRequestMsg("LED=ON");
    }
    else
    {
      setRequestMsg("LED=OFF");
    }
  }
}

void setup()
{
  isLedOn = false;
  isLedChanged = false;

  Serial.begin(9600);
  SPI.begin();
  Serial.print("Server starting..");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1);
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("Use this URL to connect: http://");
  Serial.print(WiFi.localIP());
}

void loop()
{
  WiFiClient client = server.available();

  if (client)
  {
    Serial.println("new client");
    bool isClientAvailable = client.available();
    while (!isClientAvailable)
    {
      isClientAvailable = client.available();
      delay(1);
    }

    String request = client.readStringUntil('\r');
    checkButtonPressed(request);
    handleAction();
        
    // Kuldes utan egybol fogadunk uzenetet
    SPI.beginTransaction(spi_settings);
    writeToSlave();    
    readFromSlave();
    SPI.endTransaction();
    
    // Uzenet megjelenitese a weboldalon
    client.flush();
    showUI(client);
  }

  delay(100);
}
