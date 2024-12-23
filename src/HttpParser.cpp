#include "HttpParser.hpp"

HttpRequest HttpParser::parse(WiFiClient &client)
{
    HttpRequest request;
    char buffer[1024] = {0};
    int index = 0;

    Serial.println("Parsing HTTP request headers...");

    // Read HTTP headers
    while (client.connected() && client.available())
    {
        char c = client.read();
        if (index < sizeof(buffer) - 1)
        {
            buffer[index++] = c;
        }
        buffer[index] = '\0'; // Null-terminate

        // Log the headers as they are read
        Serial.print(c);

        // Detect end of headers
        if (strstr(buffer, "\r\n\r\n") != nullptr)
        {
            Serial.println("\nHeaders fully read.");
            // Check if it's a POST request
            if (strstr(buffer, "POST /") != nullptr)
            {
                request.isPost = true;
                Serial.println("POST request detected.");

                // Parse Content-Length
                char *contentLengthHeader = strstr(buffer, "Content-Length: ");
                if (contentLengthHeader)
                {
                    request.contentLength = atoi(contentLengthHeader + 16);
                    Serial.print("Content-Length: ");
                    Serial.println(request.contentLength);
                }
                else
                {
                    Serial.println("Content-Length header not found.");
                }
            }
            else
            {
                Serial.println("Non-POST request detected.");
            }
            break;
        }
    }

    // Read the body if it's a POST request
    if (request.isPost && request.contentLength > 0)
    {
        Serial.println("Reading HTTP request body...");
        char bodyBuffer[256] = {0};
        int bytesRead = 0;

        while (bytesRead < request.contentLength && client.connected())
        {
            if (client.available())
            {
                char c = client.read();
                if (bytesRead < sizeof(bodyBuffer) - 1)
                {
                    bodyBuffer[bytesRead++] = c;
                }
                Serial.print(c); // Log each character of the body
            }
        }
        bodyBuffer[bytesRead] = '\0'; // Null-terminate the body
        request.body = String(bodyBuffer);

        Serial.println("\nBody fully read.");
        Serial.print("Body content: ");
        Serial.println(request.body);
    }
    else
    {
        Serial.println("No body to read or Content-Length is 0.");
    }

    return request;
}
