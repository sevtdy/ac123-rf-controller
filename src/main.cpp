#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <RCSwitch.h>
#include <uri/UriRegex.h>

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWD;
ESP8266WebServer server(80);

const unsigned long REMOTE_6_14 = 0xa329b02d;
const unsigned long REMOTE_6_15 = 0xa3507273;

const byte CHANNEL_PADDING = 0b00000000;
const byte UP = 0b00001011;
const byte STOP = 0b00100011;
const byte DOWN = 0b01000011;
static const RCSwitch::Protocol customprotocol = {15, 132, 50, {105, 13}, {3, 13}, {11, 5}, {11, 17}, false};
RCSwitch rfTransmitter = RCSwitch();

void sendRfSignal(unsigned long deviceId, byte channel, byte action)
{
  byte idByte1 = deviceId >> 24;
  byte idByte2 = deviceId >> 16;
  byte idbyte3 = deviceId >> 8;
  byte idByte4 = deviceId;
  byte checksum = idByte2 + idbyte3 + idByte4 + channel + CHANNEL_PADDING + action;
  byte arr[] = {idByte1, idByte2, idbyte3, idByte4, channel, CHANNEL_PADDING, action, checksum};
  char code[64];
  for (int i = 0; i < 8; i++)
  {
    for (int j = 0; j < 8; j++)
    {
      if (bitRead(arr[i], 7 - j))
      {
        code[i * 8 + j] = '1';
      }
      else
      {
        code[i * 8 + j] = '0';
      }
    }
  }
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
  }
  rfTransmitter.send(code);
  Serial.printf("sended code: %s \n", code);
}

void handleRoot()
{
  server.send(200, "text/plain", "hello from esp8266!\r\n");
}

void redirectToNewPath()
{
  String channel = server.pathArg(0);
  String action = server.pathArg(1);
  String newPath = "/floor/6/remote/14/channel/" + channel + "/" + action;
  Serial.printf("redirect to: %s \n", newPath.c_str());
  server.sendHeader("Location", newPath, true);
  server.send(301, "text/plain", "");
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

void handleTask()
{
  String remote = server.pathArg(0);
  String channel = server.pathArg(1);
  String action = server.pathArg(2);
  Serial.printf("remote:%s, channel:%s, action:%s \n", remote.c_str(), channel.c_str(), action.c_str());
  byte channelByte = channel == "all" ? 0b11111111 : 0b1 << (channel.toInt() - 1);
  unsigned long remoteHex = remote == "14" ? REMOTE_6_14 : REMOTE_6_15;
  if (action == "up")
  {
    sendRfSignal(remoteHex, channelByte, UP);
  }
  else if (action == "stop")
  {

    sendRfSignal(remoteHex, channelByte, STOP);
  }
  else
  {
    sendRfSignal(remoteHex, channelByte, DOWN);
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
  WiFi.setPhyMode(WIFI_PHY_MODE_11N);
  WiFi.setAutoReconnect(true);
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
  server.on(UriRegex("^\\/channel\\/([1-3])\\/(up|stop|down)$"), redirectToNewPath);
  server.on(UriRegex("^\\/floor\\/6\\/remote\\/(14|15)\\/channel\\/([1-3]|all)\\/(up|stop|down)$"), handleTask);
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