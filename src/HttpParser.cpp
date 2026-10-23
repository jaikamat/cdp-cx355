#include "HttpParser.hpp"

// Upper bound on the time spent parsing one request, measured from the
// moment the client was accepted. A client that connects and then stalls,
// or that announces a Content-Length it never sends, is abandoned after this
// instead of freezing loop() (and with it S-Link processing). Whatever was
// parsed by then is returned as-is; main.cpp validates it like any request.
// Must stay below WATCHDOG_TIMEOUT_MS in main.cpp.
static const unsigned long REQUEST_DEADLINE_MS = 2000;

// Request line plus headers. Browsers routinely send 600-1200 bytes of
// headers (User-Agent, Accept, sec-ch-ua, Origin, Referer...). Bytes beyond
// the buffer are read and discarded so the body still lines up.
static const size_t HEADER_BUFFER_SIZE = 2048;

// Largest body we will read. The biggest legitimate command is
// "command=saveDiscTitle&disc=300&title=" plus a 13-character URL-encoded
// title, well under 128 bytes. Anything larger is truncated.
static const int MAX_BODY_SIZE = 512;

// Read one byte, waiting for data until `deadline`. Returns -1 if the
// deadline passes or the client disconnects before a byte arrives.
static int readByte(WiFiClient &client, unsigned long deadline)
{
    while (!client.available())
    {
        if (!client.connected() || (long)(millis() - deadline) >= 0)
            return -1;
    }
    return client.read();
}

HttpRequest HttpParser::parse(WiFiClient &client)
{
    HttpRequest request;
    const unsigned long deadline = millis() + REQUEST_DEADLINE_MS;

    // --- Step A: read the request line and headers ---
    // They end at the first blank line ("\r\n\r\n"). The terminator is
    // tracked with a small match counter rather than by searching the
    // buffer, so it is still detected if the headers overflow the buffer.
    char buffer[HEADER_BUFFER_SIZE];
    size_t index = 0;
    static const char terminator[] = "\r\n\r\n";
    int matched = 0;
    int c;
    while ((c = readByte(client, deadline)) >= 0)
    {
        if (index < HEADER_BUFFER_SIZE - 1)
            buffer[index++] = (char)c;

        matched = (c == terminator[matched]) ? matched + 1 : (c == '\r' ? 1 : 0);
        if (matched == 4)
            break;
    }
    buffer[index] = '\0';

    // --- Step B: split "METHOD URL HTTP/x" from the headers that follow ---
    char *headers = strstr(buffer, "\r\n");
    if (headers)
    {
        *headers = '\0';
        headers += 2;
    }
    else
    {
        headers = buffer + index; // No line ending seen: no headers
    }

    char *methodPtr = strtok(buffer, " ");
    char *urlPtr = strtok(NULL, " ");
    if (methodPtr)
        request.method = String(methodPtr);
    if (urlPtr)
        request.url = String(urlPtr);

    // --- Step C: for POST, read exactly Content-Length bytes of body ---
    if (request.method == "POST")
    {
        request.isPost = true;
        char *contentLengthHeader = strstr(headers, "Content-Length: ");
        if (contentLengthHeader)
        {
            request.contentLength = atoi(contentLengthHeader + 16);
        }
    }

    if (request.isPost && request.contentLength > 0)
    {
        int toRead = min(request.contentLength, MAX_BODY_SIZE);
        request.body.reserve(toRead);
        while (toRead > 0 && (c = readByte(client, deadline)) >= 0)
        {
            request.body += (char)c;
            toRead--;
        }
    }

    return request;
}
