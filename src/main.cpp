#include <IRremote.hpp>
#include <map>
#include "WiFiS3.h"
#include "Button.hpp"
#include "Remote.hpp"
#include "WiFiManager.hpp"
#include "HttpParser.hpp"
#include "Secrets.hpp"

#define IR_RECEIVE_PIN 7
#define IR_SEND_PIN 2

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
WiFiManager wifiManager(ssid, password);

void selectDisc(int discNumber)
{
  Remote remote = Remote();

  remote.press(Button::DISC);
  delay(1000);
  remote.sendNumber(discNumber);
  delay(1000);
  remote.press(Button::ENTER);
}

void setDiscMemo(String memo)
{
  Remote remote = Remote();

  remote.press(Button::MEMO_INPUT);
  delay(1000);

  remote.setMode(Remote::Alpha);
  remote.sendAlpha("JusticeLive");
  remote.setMode(Remote::Default);

  remote.press(Button::ENTER);
}

void setup()
{
  Serial.begin(115200);                                  // Establish serial communication
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK); // Start the receiver
  IrSender.begin(IR_SEND_PIN);                           // Start the emitter

  // Connect to WiFi
  wifiManager.connect();
}

void sendForm(WiFiClient &client)
{
  String html =
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/html\r\n"
      "Connection: close\r\n"
      "\r\n"
      "<!DOCTYPE HTML>"
      "<html>"
      "<head><title>CDP-CX355 Controller</title></head>"
      "<body>"
      "<h1>Arduino Uno R4 WiFi Web Server</h1>"
      "<form method='POST'>"
      "  <label for='disc'>Disc:</label><br>"
      "  <input type='number' id='disc' name='disc'><br>"
      "  <label for='message'>Message:</label><br>"
      "  <input type='text' id='message' name='message'><br>"
      "  <input type='submit' value='Submit'>"
      "</form>"
      "</body>"
      "</html>";
  client.print(html);
}

void loop()
{
  WiFiClient client = wifiManager.getServer().available(); // Get the server from WiFiManager

  if (client)
  {
    Serial.println("New client connected.");

    // Use HttpParser to parse the request
    HttpRequest request = HttpParser::parse(client);

    if (request.isPost)
    {
      Serial.println("Processing POST request...");

      // Parse POST body
      String discStr = "";
      String message = "";

      int discIndex = request.body.indexOf("disc=");
      if (discIndex != -1)
      {
        int discEndIndex = request.body.indexOf("&", discIndex);
        if (discEndIndex == -1)
          discEndIndex = request.body.length();
        discStr = request.body.substring(discIndex + 5, discEndIndex);
        Serial.print("Parsed 'disc': ");
        Serial.println(discStr);
      }
      else
      {
        Serial.println("'disc' parameter not found.");
      }

      int messageIndex = request.body.indexOf("message=");
      if (messageIndex != -1)
      {
        int messageEndIndex = request.body.indexOf("&", messageIndex);
        if (messageEndIndex == -1)
          messageEndIndex = request.body.length();
        message = request.body.substring(messageIndex + 8, messageEndIndex);
        Serial.print("Parsed 'message': ");
        Serial.println(message);
      }
      else
      {
        Serial.println("'message' parameter not found.");
      }

      if (!discStr.isEmpty() && !message.isEmpty())
      {
        // Convert disc string to integer
        int disc = discStr.toInt();

        Serial.print("Final Disc value: ");
        Serial.println(disc);
        Serial.print("Final Message value: ");
        Serial.println(message);

        // Perform actions
        selectDisc(disc);
        setDiscMemo(message);
      }
      else
      {
        Serial.println("Error: Missing 'disc' or 'message' parameters.");
      }
    }
    else
    {
      Serial.println("Received a non-POST request.");
    }

    // Send response to client
    Serial.println("Sending response form to client...");
    sendForm(client);
    delay(10); // Allow the client time to read the response

    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
  }

  if (IrReceiver.decode())
  {
    Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX); // Print "old" raw data
    IrReceiver.printIRResultShort(&Serial);                       // Print complete received data in one line
    IrReceiver.printIRSendUsage(&Serial);                         // Print the statement required to send this data
    IrReceiver.resume();                                          // Enable receiving of the next value
  }
}
