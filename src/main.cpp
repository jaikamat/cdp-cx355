#include <IRremote.hpp>
#include <map>
#include "WiFiS3.h"
#include "Button.hpp"
#include "Remote.hpp"
#include "WiFiManager.hpp"
#include "HttpParser.hpp"
#include "Secrets.hpp"
#include "Arduino_LED_Matrix.h"
#include <functional>
#include <map>

#define IR_RECEIVE_PIN 7
#define IR_SEND_PIN 2

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
ArduinoLEDMatrix matrix;
WiFiManager wifiManager(ssid, password);
Remote remote = Remote();

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

void selectDisc(int discNumber)
{
  remote.reset();
  remote.press(Button::DISC);
  delay(1000);
  remote.sendNumber(discNumber);
  delay(1000);
  remote.press(Button::ENTER);
  delay(100);
}

void setDiscMemo(String memo)
{
  remote.reset();
  remote.press(Button::MEMO_INPUT);
  delay(1000);

  remote.setMode(Remote::Alpha);
  remote.sendAlpha(memo);
  remote.setMode(Remote::Default);

  remote.press(Button::ENTER);
  delay(100);
}

// Define a type for your command functions
using CommandHandler = std::function<void(const String &)>;

// Create a map to store command handlers
std::map<String, CommandHandler> commandHandlers;

// Create individual handlers
void handlePowerCommand(const String &)
{
  remote.reset();
  remote.press(Button::POWER);
}

void handleSelectDiscCommand(const String &args)
{
  int discNumber = args.toInt();
  selectDisc(discNumber);
}

void handleSetDiscMemoCommand(const String &args)
{
  setDiscMemo(args);
}

void handleSelectDiscAndMemoCommand(const String &args)
{
  // Extract `disc` and `message` parameters
  int discIndex = args.indexOf("disc=");
  int messageIndex = args.indexOf("message=");
  String discValue = "";
  String messageValue = "";

  if (discIndex != -1)
  {
    int discEnd = args.indexOf("&", discIndex);
    if (discEnd == -1)
      discEnd = args.length();
    discValue = urlDecode(args.substring(discIndex + 5, discEnd));
  }

  if (messageIndex != -1)
  {
    int messageEnd = args.indexOf("&", messageIndex);
    if (messageEnd == -1)
      messageEnd = args.length();
    messageValue = urlDecode(args.substring(messageIndex + 8, messageEnd));
  }

  // Log and process
  Serial.print("Disc Value: ");
  Serial.println(discValue);
  Serial.print("Message Value: ");
  Serial.println(messageValue);

  if (!discValue.isEmpty() && !messageValue.isEmpty())
  {
    int discNumber = discValue.toInt();
    selectDisc(discNumber);
    setDiscMemo(messageValue);
  }
  else
  {
    Serial.println("Error: Missing disc or message parameters.");
  }
}

// Initialize the handlers in setup
void setupCommandHandlers()
{
  commandHandlers["power"] = handlePowerCommand;
  commandHandlers["selectDisc"] = handleSelectDiscCommand;
  commandHandlers["setDiscMemo"] = handleSetDiscMemoCommand;
  commandHandlers["selectDiscAndMemo"] = handleSelectDiscAndMemoCommand;
}

void setup()
{
  setupCommandHandlers();

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
        "  <input type='hidden' name='command' value='selectDiscAndMemo'>"
        "  <input type='submit' value='Submit Disc and Memo'><br>"
        "</form>"
        "<br>"
        "<form method='POST'>"
        "  <input type='hidden' name='command' value='power'>"
        "  <input type='submit' value='Power'><br>"
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

      String command = "";

      // Extract 'command' parameter from the POST body
      int commandIndex = request.body.indexOf("command=");
      if (commandIndex != -1)
      {
        int commandEndIndex = request.body.indexOf("&", commandIndex);
        if (commandEndIndex == -1)
          commandEndIndex = request.body.length();
        command = urlDecode(request.body.substring(commandIndex + 8, commandEndIndex));
      }

      Serial.print("Extracted Command: ");
      Serial.println(command);

      // Execute the command if it exists in the map
      auto handler = commandHandlers.find(command);
      if (handler != commandHandlers.end())
      {
        handler->second(request.body); // Pass the entire body to the handler
      }
      else
      {
        Serial.println("Error: Unknown command.");
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
