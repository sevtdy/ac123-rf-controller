#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <RCSwitch.h>
#include <uri/UriRegex.h>

WiFiClient espClient;
PubSubClient mqttClient;

// WIFI_SSID, WIFI_PASSWD, MQTT_SERVER inject from env

const unsigned long REMOTE_6_13 = 0xa35073f3;
const unsigned long REMOTE_6_14 = 0xa329b02d;
const unsigned long REMOTE_6_15 = 0xa3507273;
const unsigned long REMOTE_6_16 = 0xa3514e0f;

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
  byte idByte3 = deviceId >> 8;
  byte idByte4 = deviceId;
  byte checksum = idByte2 + idByte3 + idByte4 + channel + CHANNEL_PADDING + action;
  byte arr[] = {idByte1, idByte2, idByte3, idByte4, channel, CHANNEL_PADDING, action, checksum};
  char code[65];
  code[64] = '\0';
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

void handleTask(String remote, String channel, String action)
{
  Serial.printf("remote:%s, channel:%s, action:%s \n", remote.c_str(), channel.c_str(), action.c_str());
  Serial.println("-----------------------");
  byte channelByte = channel == "all" ? 0b11111111 : 0b1 << (channel.toInt() - 1);
  unsigned long remoteHex;
  switch (remote.toInt())
  {
  case 13:
    remoteHex = REMOTE_6_13;
    break;
  case 14:
    remoteHex = REMOTE_6_14;
    break;
  case 15:
    remoteHex = REMOTE_6_15;
    break;
  case 16:
    remoteHex = REMOTE_6_16;
    break;
  default:
    return;
  }
  Serial.print(remoteHex);
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
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  char command[12];
  for (unsigned int i = 0; i < length; i++)
  {
    command[i] = (char)payload[i];
  }
  command[length] = '\0';
  // e.g. "14,3,up"
  Serial.print(command);
  Serial.println();
  Serial.println("-----------------------");

  byte tokenIndex = 0;
  char *tokens[3];
  char *token = strtok(command, ",");
  while (token != NULL)
  {
    tokens[tokenIndex] = token;
    tokenIndex++;
    token = strtok(NULL, ",");
  }

  handleTask(String(tokens[0]), String(tokens[1]), String(tokens[2]));
}

void connectMqtt()
{
  // Loop until we're reconnected
  while (!mqttClient.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect("ESP8266Client"))
    {
      Serial.println("connected");
      mqttClient.subscribe("ac123");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
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
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.begin(WIFI_SSID, WIFI_PASSWD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Mac address: ");
  Serial.println(WiFi.macAddress());

  // setting up mqtt client
  mqttClient.setClient(espClient);
  mqttClient.setServer(MQTT_SERVER, 3001);
  mqttClient.setCallback(mqttCallback);
}

void loop()
{
  // put your main code here, to run repeatedly:
  if (!mqttClient.connected())
  {
    connectMqtt();
  }
  mqttClient.loop();
}