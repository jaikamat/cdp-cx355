#include <IRremote.hpp>
#include <map>
#include "WiFiS3.h"
#include "Button.hpp"
#include "Remote.hpp"
#include "WiFiManager.hpp"
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
  ///////////////////////////////////////////////////////////////////// wifi stuff
  WiFiClient client = wifiManager.getServer().available(); // Get the server from WiFiManager

  if (client)
  {
    Serial.println("New client connected.");
    char request[1024] = {0}; // Buffer for storing the request
    int reqIndex = 0;
    int contentLength = 0;
    bool isPost = false;

    // TODO: pull the read and parsing stuff out to the wifi class?
    // Read the HTTP request
    while (client.connected() && client.available())
    {
      char c = client.read();
      if (reqIndex < sizeof(request) - 1)
      {
        request[reqIndex++] = c;
      }
      request[reqIndex] = '\0'; // Null-terminate the request

      // Detect the end of headers
      if (strstr(request, "\r\n\r\n") != nullptr)
      {
        // Check if POST request
        if (strstr(request, "POST /") != nullptr)
        {
          isPost = true;
          char *contentLengthHeader = strstr(request, "Content-Length: ");
          if (contentLengthHeader)
          {
            contentLength = atoi(contentLengthHeader + 16);
          }
        }
        break;
      }
    }

    // Handle POST request body
    char body[256] = {0}; // Buffer for POST body
    if (isPost && contentLength > 0)
    {
      int bytesRead = 0;
      while (bytesRead < contentLength && client.connected())
      {
        if (client.available())
        {
          char c = client.read();
          if (bytesRead < sizeof(body) - 1)
          {
            body[bytesRead++] = c;
          }
        }
      }
      body[bytesRead] = '\0'; // Null-terminate the body
      Serial.println("Form Data Received:");

      // Parse the POST body
      String postData = String(body);
      String discStr = "";
      String message = "";

      int discIndex = postData.indexOf("disc=");
      if (discIndex != -1)
      {
        int discEndIndex = postData.indexOf("&", discIndex);
        if (discEndIndex == -1)
          discEndIndex = postData.length();                        // If no '&', go to end of string
        discStr = postData.substring(discIndex + 5, discEndIndex); // Extract disc value
      }

      int messageIndex = postData.indexOf("message=");
      if (messageIndex != -1)
      {
        int messageEndIndex = postData.indexOf("&", messageIndex);
        if (messageEndIndex == -1)
          messageEndIndex = postData.length();
        message = postData.substring(messageIndex + 8, messageEndIndex); // Extract message value
      }

      if (!discStr.isEmpty() && !message.isEmpty())
      {
        // Convert disc string to integer
        int disc = discStr.toInt();

        // Print extracted values
        Serial.print("Disc: ");
        Serial.println(disc);
        Serial.print("Message: ");
        Serial.println(message);

        // Select disc
        selectDisc(disc);

        // Set disc memo
        setDiscMemo(message);
      }
      else
      {
        Serial.println("Error: Missing 'disc' or 'message'.");
      }
    }

    // Send response to client
    sendForm(client);
    delay(10); // Allow the client time to read the response

    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
  }
  ///////////////////////////////////////////////////////////////////// end wifi stuff

  if (IrReceiver.decode())
  {
    Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX); // Print "old" raw data
    IrReceiver.printIRResultShort(&Serial);                       // Print complete received data in one line
    IrReceiver.printIRSendUsage(&Serial);                         // Print the statement required to send this data
    IrReceiver.resume();                                          // Enable receiving of the next value
  }
}
