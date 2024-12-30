#include <IRremote.hpp>
#include <map>
#include "WiFiS3.h"
#include "Button.hpp"
#include "Remote.hpp"
#include "WiFiManager.hpp"
#include "HttpParser.hpp"
#include "Secrets.hpp"
#include "Arduino_LED_Matrix.h"

#define IR_RECEIVE_PIN 7
#define IR_SEND_PIN 2

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
ArduinoLEDMatrix matrix;
WiFiManager wifiManager(ssid, password);
Remote remote = Remote();

void selectDisc(int discNumber)
{
  remote.press(Button::DISC);
  delay(1000);
  remote.sendNumber(discNumber);
  delay(1000);
  remote.press(Button::ENTER);
}

void setDiscMemo(String memo)
{
  remote.press(Button::MEMO_INPUT);
  delay(1000);

  remote.setMode(Remote::Alpha);
  remote.sendAlpha(memo);
  remote.setMode(Remote::Default);

  remote.press(Button::ENTER);
}

// Decode URL strings in the POST request
String urlDecode(const String &encoded)
{
  String decoded = "";
  for (size_t i = 0; i < encoded.length(); i++)
  {
    char c = encoded[i];
    if (c == '+')
    {
      decoded += ' '; // Replace '+' with space
    }
    else if (c == '%' && i + 2 < encoded.length())
    {
      // Decode %XX into the corresponding character
      char hex[3] = {encoded[i + 1], encoded[i + 2], '\0'};
      decoded += (char)strtol(hex, nullptr, 16);
      i += 2; // Skip the next two characters
    }
    else
    {
      decoded += c; // Add other characters as-is
    }
  }
  return decoded;
}

void setup()
{
  Serial.begin(115200); // Establish serial communication
  matrix.begin();

  matrix.loadSequence(LEDMATRIX_ANIMATION_STARTUP);
  matrix.play(false);
  delay(4800);

  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK); // Start the receiver
  IrSender.begin(IR_SEND_PIN);                           // Start the emitter

  matrix.loadSequence(LEDMATRIX_ANIMATION_WIFI_SEARCH);
  matrix.play(true);

  // Connect to WiFi
  wifiManager.connect();

  // Indicate we have connected to wifi
  matrix.loadFrame(LEDMATRIX_CLOUD_WIFI);
  delay(1000);
  matrix.clear();
}

void sendForm(WiFiClient &client)
{
  String html =
      F("HTTP/1.1 200 OK\r\n"
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
        "</html>");
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
      matrix.loadSequence(LEDMATRIX_ANIMATION_SPINNING_COIN);
      matrix.play(true);

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
        discStr = urlDecode(request.body.substring(discIndex + 5, discEndIndex));
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
        message = urlDecode(request.body.substring(messageIndex + 8, messageEndIndex));
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

    // clear the matrix
    matrix.clear();
  }

  if (IrReceiver.decode())
  {
    Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX); // Print "old" raw data
    IrReceiver.printIRResultShort(&Serial);                       // Print complete received data in one line
    IrReceiver.printIRSendUsage(&Serial);                         // Print the statement required to send this data
    IrReceiver.resume();                                          // Enable receiving of the next value
  }
}
