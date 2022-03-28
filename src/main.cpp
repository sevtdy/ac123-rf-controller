#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <RCSwitch.h>
#include <uri/UriRegex.h>

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWD;
ESP8266WebServer server(80);

const byte REMOTE_BYTE_1 = 0b10100011;
const byte REMOTE_BYTE_2 = 0b00101001;
const byte REMOTE_BYTE_3 = 0b10110000;
const byte REMOTE_BYTE_4 = 0b00101101;
const byte CHANNEL_PADDING = 0b00000000;

const byte UP = 0b00001011;
const byte STOP = 0b00100011;
const byte DOWN = 0b01000011;
static const RCSwitch::Protocol customprotocol = {15, 132, 50, {105, 13}, {3, 13}, {11, 5}, {11, 17}, false};
RCSwitch rfTransmitter = RCSwitch();

void sendRfSignal(byte channel, byte action)
{
  byte checksum = REMOTE_BYTE_2 + REMOTE_BYTE_3 + REMOTE_BYTE_4 + channel + CHANNEL_PADDING + action;
  byte arr[] = {REMOTE_BYTE_1, REMOTE_BYTE_2, REMOTE_BYTE_3, REMOTE_BYTE_4, channel, CHANNEL_PADDING, action, checksum};
  char sendCodeChar[64];
  for (int i = 0; i < 8; i++)
  {
    for (int j = 0; j < 8; j++)
    {
      if (bitRead(arr[i], 7 - j))
      {
        sendCodeChar[i * 8 + j] = '1';
      }
      else
      {
        sendCodeChar[i * 8 + j] = '0';
      }
    }
  }
  rfTransmitter.send(sendCodeChar);
  Serial.printf("sended code: %s \n", sendCodeChar);
};
void handleRoot()
{
  server.send(200, "text/plain", "hello from esp8266!\r\n");
}
void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}
void handleAction()
{
  String channel = server.pathArg(0);
  String action = server.pathArg(1);
  Serial.printf("channel:%s, action:%s \n", channel.c_str(), action.c_str());
  int offset = channel.toInt();
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
  }
  if (action == "up")
  {

    sendRfSignal(0b1 << (offset - 1), UP);
  }
  else if (action == "stop")
  {

    sendRfSignal(0b1 << (offset - 1), STOP);
  }
  else
  {
    sendRfSignal(0b1 << (offset - 1), DOWN);
  }
  server.send(200, "text/plain", "ok");
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println("");
  Serial.println("setting up rf transmitter");
  rfTransmitter.enableTransmit(10);
  rfTransmitter.setProtocol(customprotocol);

  Serial.println("setting up WiFi");
  WiFi.mode(WIFI_AP);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Mac address: ");
  Serial.println(WiFi.macAddress());

  // setting up server
  server.on("/", handleRoot);
  server.on(UriRegex("^\\/channel\\/([1-3])\\/(up|stop|down)$"), handleAction);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.print("HTTP server started. Runing on: http://");
  Serial.print(WiFi.localIP());
  Serial.println(":80");
}

void loop()
{
  // put your main code here, to run repeatedly:
  server.handleClient();
}