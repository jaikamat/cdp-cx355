#include <IRremote.hpp>
#include <map>
#include "WiFiS3.h"
#include "Button.hpp"
#include "Remote.hpp"
#include "WiFiManager.hpp"
#include "HttpParser.hpp"
#include "Secrets.hpp"
#include <LedMatrixController.hpp>
#include "DiscStorage.hpp"
#include <functional>
#include <map>

#define IR_RECEIVE_PIN 7
#define IR_SEND_PIN 2

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
ArduinoLEDMatrix matrix;
LedMatrixController ledController(matrix);
WiFiManager wifiManager(ssid, password);
Remote remote = Remote();
DiscStorage storage;

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
  int discIndex = args.indexOf("disc=");
  String discValue = "";

  if (discIndex != -1)
  {
    int discEnd = args.indexOf("&", discIndex);
    if (discEnd == -1)
      discEnd = args.length();
    discValue = urlDecode(args.substring(discIndex + 5, discEnd));
  }

  Serial.print("Disc Value: ");
  Serial.println(discValue);

  if (!discValue.isEmpty())
  {
    int discNumber = discValue.toInt();
    selectDisc(discNumber);
  }
  else
  {
    Serial.println("Error: Missing disc value.");
  }
}

void handleSetDiscMemoCommand(const String &args)
{
  setDiscMemo(args);
}

void handleSelectDiscAndMemoCommand(const String &args)
{
  // Extract `disc` and `memo` parameters
  int discIndex = args.indexOf("disc=");
  int memoIndex = args.indexOf("memo=");
  String discValue = "";
  String memoValue = "";

  if (discIndex != -1)
  {
    int discEnd = args.indexOf("&", discIndex);
    if (discEnd == -1)
      discEnd = args.length();
    discValue = urlDecode(args.substring(discIndex + 5, discEnd));
  }

  if (memoIndex != -1)
  {
    int memoEnd = args.indexOf("&", memoIndex);
    if (memoEnd == -1)
      memoEnd = args.length();
    memoValue = urlDecode(args.substring(memoIndex + 5, memoEnd));
  }

  // Log and process
  Serial.print("Disc Value: ");
  Serial.println(discValue);
  Serial.print("Memo Value: ");
  Serial.println(memoValue);

  if (!discValue.isEmpty() && !memoValue.isEmpty())
  {
    int discNumber = discValue.toInt();

    // Save the memo locally to EEPROM
    Serial.print("Saving memo locally for Disc ");
    Serial.print(discNumber);
    Serial.println("...");
    storage.writeDiscWithNumber(discNumber, memoValue);

    // Check if the disc is a Data CD
    if (storage.isDataDisc(discNumber))
    {
      // Log a message and skip sending IR commands
      Serial.print("Disc ");
      Serial.print(discNumber);
      Serial.println(" is marked as a Data CD. Memo saved locally, but no IR commands sent.");
      return; // Exit the function
    }

    // Send commands to the jukebox via IR for non-data discs
    Serial.print("Sending memo to jukebox for Disc ");
    Serial.print(discNumber);
    Serial.println("...");
    selectDisc(discNumber);
    setDiscMemo(memoValue);

    Serial.println("Memo successfully set.");
  }
  else
  {
    Serial.println("Error: Missing disc or memo message.");
  }
}

void handleSetDiscAsDataCDCommand(const String &args)
{
  int discIndex = args.indexOf("disc=");
  int dataCDIndex = args.indexOf("isDataCD=");
  String discValue = "";
  bool isDataCD = false;

  if (discIndex != -1)
  {
    int discEnd = args.indexOf("&", discIndex);
    if (discEnd == -1)
      discEnd = args.length();
    discValue = urlDecode(args.substring(discIndex + 5, discEnd));
  }

  if (dataCDIndex != -1)
  {
    int dataCDEnd = args.indexOf("&", dataCDIndex);
    if (dataCDEnd == -1)
      dataCDEnd = args.length();
    String isDataCDValue = urlDecode(args.substring(dataCDIndex + 9, dataCDEnd));
    isDataCD = (isDataCDValue == "true");
  }

  if (!discValue.isEmpty())
  {
    int discNumber = discValue.toInt();
    storage.setDiscAsDataCD(discNumber, isDataCD);
    Serial.print("Disc ");
    Serial.print(discNumber);
    Serial.println(isDataCD ? " marked as Data CD." : " unmarked as Data CD.");
  }
  else
  {
    Serial.println("Error: Missing disc value.");
  }
}

// Initialize the handlers in setup
void setupCommandHandlers()
{
  commandHandlers["power"] = handlePowerCommand;
  commandHandlers["selectDisc"] = handleSelectDiscCommand;
  commandHandlers["setDiscMemo"] = handleSetDiscMemoCommand;
  commandHandlers["selectDiscAndMemo"] = handleSelectDiscAndMemoCommand;
  commandHandlers["setDiscAsDataCD"] = handleSetDiscAsDataCDCommand;
}

void setup()
{
  Serial.begin(115200); // Establish serial communication
  matrix.begin();

  setupCommandHandlers();

  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK); // Start the receiver
  IrSender.begin(IR_SEND_PIN);                           // Start the emitter

  // Play WiFi search animation
  ledController.playAnimation(MatrixAnimation::WifiSearch, true);

  // Connect to WiFi
  wifiManager.connect();

  // Indicate WiFi connection
  ledController.displayText("wifi connected");
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
      "<head>"
      "<title>CDP-CX355 Controller</title>"
      "<style>"
      "  body { font-family: Arial, sans-serif; margin: 20px; }"
      "  table { width: 100%; border-collapse: collapse; margin-bottom: 20px; }"
      "  th, td { border: 1px solid #ccc; padding: 10px; text-align: left; }"
      "  th { background-color: #f2f2f2; }"
      "  input[type='number'], input[type='text'], input[type='submit'], input[type='checkbox'] { margin-top: 5px; padding: 5px; font-size: 14px; }"
      "</style>"
      "</head>"
      "<body>"
      "<h1>CDP-CX355 Jukebox Controller</h1>"
      "<table>"
      "<tr><th>Disc Number</th><th>Memo</th><th>Data CD</th><th>Action</th></tr>";

  client.print(html);

  // Add rows to the table for each disc with an inline form
  for (int i = 0; i < storage.getMaxDiscs(); i++)
  {
    DiscInfo disc = storage.readDisc(i);

    // Memo input field and update button (editable for all discs)
    String memoField =
        "<form method='POST' style='display: inline;'>"
        "  <input type='hidden' name='disc' value='" +
        String(disc.discNumber) + "'>"
                                  "  <input type='hidden' name='command' value='selectDiscAndMemo'>"
                                  "  <input type='text' name='memo' value='" +
        String(disc.memo) + "' maxlength='13' pattern='^[a-zA-Z0-9 ]*$'>"
                            "  <input type='submit' value='Update'>"
                            "</form>";

    // Data CD checkbox form
    String dataCDField =
        "<form method='POST' style='display: inline;'>"
        "  <input type='hidden' name='disc' value='" +
        String(disc.discNumber) + "'>"
                                  "  <input type='hidden' name='command' value='setDiscAsDataCD'>"
                                  "  <input type='checkbox' name='isDataCD' value='true' " +
        (disc.isDataCD ? "checked" : "") + "> Data CD"
                                           "  <input type='submit' value='Update'>"
                                           "</form>";

    // Construct the row HTML
    String rowHtml =
        "<tr>"
        "<td>" +
        String(disc.discNumber) + "</td>"
                                  "<td>" +
        memoField + "</td>"
                    "<td>" +
        dataCDField + "</td>"
                      "<td>"
                      "<form method='POST' style='display: inline;'>"
                      "  <input type='hidden' name='disc' value='" +
        String(disc.discNumber) + "'>"
                                  "  <input type='hidden' name='command' value='selectDisc'>"
                                  "  <input type='submit' value='Select'>"
                                  "</form>"
                                  "</td>"
                                  "</tr>";

    client.print(rowHtml);
  }

  client.print("</table>"
               "</body>"
               "</html>");
}

void loop()
{
  WiFiClient client = wifiManager.getServer().available(); // Get the server from WiFiManager

  if (client)
  {
    Serial.println("New client connected.");

    // Play loading state
    ledController.playAnimation(MatrixAnimation::Loading, true);

    // Use HttpParser to parse the request
    HttpRequest request = HttpParser::parse(client);

    if (request.isPost)
    {
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

    // Send response to client
    Serial.println("Sending response form to client...");
    sendForm(client);
    delay(10); // Allow the client time to read the response

    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");

    ledController.clearDisplay();
  }

  if (IrReceiver.decode())
  {
    Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX); // Print "old" raw data
    IrReceiver.printIRResultShort(&Serial);                       // Print complete received data in one line
    IrReceiver.printIRSendUsage(&Serial);                         // Print the statement required to send this data
    IrReceiver.resume();                                          // Enable receiving of the next value
  }
}
