#ifndef HTTP_PARSER_HPP
#define HTTP_PARSER_HPP

#include <WiFiS3.h>

struct HttpRequest
{
    bool isPost = false;
    int contentLength = 0;
    String body = "";
};

class HttpParser
{
public:
    static HttpRequest parse(WiFiClient &client);
};

#endif // HTTP_PARSER_HPP
