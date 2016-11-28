#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
MDNSResponder mdns;
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define SERIAL_DATA_THRESHOLD_MS 500
#define SERIAL_ERROR_TIMEOUT "E: Serial"
#define ETHERNET_ERROR_DHCP "E: DHCP"
#define ETHERNET_ERROR_CONNECT "E: Connect"

String lastCmd;

WiFiClient client;

#define HTTPPORT 88
const int relayPin = D1;
int relayState = 0;
ESP8266WebServer server(HTTPPORT);

void handleRoot();
void openDoor();
void returnStatus();
void handleCmd();
void handleNotFound();

void setup()
{
  pinMode(relayPin, OUTPUT);
  Serial.begin(115200);
  WiFiManager wifiManager;

  wifiManager.autoConnect();

  Serial.println("connected to WiFi");
  if (mdns.begin("garage", WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }

  ArduinoOTA.setHostname("garage");
  ArduinoOTA.setPort(8266);
  
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  /* WebServer stuff */

  server.on("/", handleRoot);
  server.on("/status", returnStatus);
  server.on("/open", openDoor);
  server.on("/cmd", handleCmd);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.print("HTTP server started on ");
  Serial.println(HTTPPORT);
}

void ethConnectError()
{
  Serial.println(ETHERNET_ERROR_CONNECT);
  Serial.println("Wifi died.");
}

void loop()
{
  ArduinoOTA.handle();
  server.handleClient();
}

void handleInput(String cmd)
{
    // Read 2 bytes from serial buffer
    Serial.println("Handling command " + cmd);
    if (cmd == "open")
    {
      openDoor();
    }
    //On
    else if (cmd == "1" || cmd == "on")
    {
      relayState = 1;
      digitalWrite(relayPin, relayState); // turn on relay with voltage HIGH
    }
    //Off
    else if (cmd == "0" || cmd == "off")
    {
      relayState = 0;
      digitalWrite(relayPin, relayState); // turn off relay with voltage LOW
    }
}

/* WebServer Stuff */

void handleRoot() {
  String msg = "<html>\n";
  msg += "<head>\n";
  msg += "<title>ESP8266/Wemos D1 Mini Relay Shield Controller</title>\n";
  msg += "<link rel=\"stylesheet\" type=\"text/css\" href=\"http://joeybabcock.me/iot/hosted/hosted-relay.css\">";
  msg += "<script src=\"https://code.jquery.com/jquery-3.1.1.min.js\"></script>\n";
  msg += "<script src=\"http://joeybabcock.me/iot/hosted/hosted-relay.js\">var relayStatus = "+String(relayState)+";</script>\n";
  msg += "</head>\n";
  msg += "<body>\n";
  msg += "<div id=\"container\">\n";
  msg += "<h1>Esp8266/Wemos D1 Mini Relay Shield Controller!</h1>\n";
  msg += "<p id=\"linkholder\">\n";
  msg += "<div class=\"c"+String(relayState)+"\" id=\"status\"></div>\n";
  msg += "<a href=\"#\" onclick=\"sendCmd('open');\"><img src=\"http://joeybabcock.me/iot/hosted/open.png\"/></a> \n";
  msg += "<a href=\"#\" onclick=\"sendCmd('0');\"><img src=\"http://joeybabcock.me/iot/hosted/o.png\"/></a>\n";
  msg += "<a href=\"#\" onclick=\"sendCmd('1');\"><img src=\"http://joeybabcock.me/iot/hosted/i.png\"/></a></p>\n";
  msg += "<p>Server Response:<div id=\"response\" class=\"response\"></div></p>\n";
  msg += "<p><form action=\"//cmd\" method=\"get\" id=\"console\"><input placeholder=\"Enter a command...\" type=\"text\" id='console_text'/></form></p>\n";
  msg += "<script>\n$('#console').submit(function(){parseCmd($(\"#console_text\").val());\nreturn false;\n});\n</script>\n";
  msg += "</div>\n";
  msg == "</body>\n";
  msg += "</html>\n";
  server.send(200, "text/html", msg);
}

void handleNotFound() {

  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);

}

void handleResponse(String cmd = "none") {
      server.send(200, "text/html", "Worked("+cmd+")");
      Serial.println("Client succesfully executed "+cmd);
}

void returnStatus() {
      server.send(200, "text/html", String(relayState));
      Serial.println("Client succesfully executed Status Command");
}

void handleCmd(){
  for (uint8_t i=0; i<server.args(); i++){
    if(server.argName(i) == "cmd") 
    {
      lastCmd = server.arg(i);
      handleInput(lastCmd);
    }
  }
  handleResponse(lastCmd);
}

void openDoor()
{
  handleResponse("open");
  digitalWrite(relayPin, HIGH); // turn on relay with voltage HIGH
  delay(200);              // pause
  digitalWrite(relayPin, LOW);  // turn off relay with voltage LOW
}

