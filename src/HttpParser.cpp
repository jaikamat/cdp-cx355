#include "HttpParser.hpp"

HttpRequest HttpParser::parse(WiFiClient &client)
{
    HttpRequest request;
    char buffer[1024] = {0};
    int index = 0;

    // --- Step A: Read the first line (method + path + HTTP version) ---
    // We'll read until we get "\r\n" or fill buffer.
    while (client.connected() && client.available())
    {
        char c = client.read();
        if (index < (int)sizeof(buffer) - 1)
        {
            buffer[index++] = c;
        }
        buffer[index] = '\0';

        // Stop at end of line
        if (strstr(buffer, "\r\n"))
        {
            // First line is in buffer. Something like:
            // "GET /?page=2 HTTP/1.1\r\n"
            break;
        }
    }

    // Parse out method and url from that first line
    // Typically: METHOD [space] URL [space] HTTP/...
    // e.g.: "GET /?page=2 HTTP/1.1"
    char *methodPtr = strtok(buffer, " "); // "GET"
    char *urlPtr = strtok(NULL, " ");      // "/?page=2"
    // We'll ignore the HTTP version for now.

    if (methodPtr)
        request.method = String(methodPtr);
    if (urlPtr)
        request.url = String(urlPtr);

    // If the request was GET, we keep reading the rest of the headers,
    // but we know we won't expect a body. If it's POST, same as before.
    index = 0; // re-use buffer for reading the rest of the headers
    memset(buffer, 0, sizeof(buffer));

    // Keep reading headers until we see "\r\n\r\n"
    while (client.connected() && client.available())
    {
        char c = client.read();
        if (index < (int)sizeof(buffer) - 1)
        {
            buffer[index++] = c;
        }
        buffer[index] = '\0';

        if (strstr(buffer, "\r\n\r\n"))
        {
            break;
        }
    }

    // Check if POST
    if (request.method == "POST")
    {
        request.isPost = true;
        // Parse Content-Length if any
        char *contentLengthHeader = strstr(buffer, "Content-Length: ");
        if (contentLengthHeader)
        {
            request.contentLength = atoi(contentLengthHeader + 16);
        }
    }

    // If it's POST, read the body
    if (request.isPost && request.contentLength > 0)
    {
        int remaining = request.contentLength;
        String bodyString;
        // read exactly contentLength bytes
        while (remaining > 0 && client.connected())
        {
            while (client.available())
            {
                char c = client.read();
                bodyString += c;
                remaining--;
                if (remaining <= 0)
                    break;
            }
        }
        request.body = bodyString;
    }

    return request;
}