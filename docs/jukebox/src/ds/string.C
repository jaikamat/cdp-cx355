#ifndef STRING_CLASS
#define STRING_CLASS

#include "ds/string.h"

String::String(void) {
  str = new char[1];
  size = 0;
  str[0] = EOS;
  fg = WHITE;
  bg = BLACK;
} /* String() */

String::String(char *s) {
  size = strlen(s);

  if (size < 1)
    size = 0;

  str = new char[size + 1];

  if (!size)
    str[0] = EOS;
  else
    strcpy(str,s);

  fg = WHITE;
  bg = BLACK;
} /* String() */

String::String(const String& s) {
  size = s.size;
  str = new char[size + 1];
  strcpy(str, s.str);
  fg = WHITE;
  bg = BLACK;
} /* String() */

String::~String(void) {
  if (str != NULL)
    delete [] str;
} /* ~String() */

String& String::operator= (const String& s) {

  if (&s != this) {

    if (str != NULL)
      delete [] str;

    size = s.size;
    str = new char[size + 1];
    strcpy(str, s.str);
  }

  return *this;
} /* operator=() */

String& String::operator= (char *s) {

  if (str != NULL)
    delete [] str;

  size = strlen(s);

  if (size < 1)
    size = 0;

  str = new char[size + 1];

  if (!size)
    str[0] = EOS;
  else
    strcpy(str, s);

  return *this;
} /* operator=() */

int String::operator== (const String& s) const {
  return strcmp(str, s.str) == 0;
} /* operator==() */

int String::operator== (char *s) const {
  return strcmp(str, s) == 0;
} /* operator==() */

int operator== (char *str, const String& s) {
  return (strcmp(str, s.str) == 0);
} /* operator==() */

int String::operator!= (const String& s) const {
  return strcmp(str, s.str) != 0;
} /* operator!=() */

int String::operator!= (char *s) const {
  return strcmp(str, s) != 0;
} /* operator!=() */

int operator!= (char *str, const String& s) {
  return strcmp(str, s.str) != 0;
} /* operator!=() */

int String::operator< (const String& s) const {
  return strcmp(str, s.str) < 0;
} /* operator<() */

int String::operator< (char *s) const {
  return strcmp(str, s) < 0;
} /* operator<() */

int operator< (char *str, const String& s) {
  return strcmp(str, s.str) < 0;
} /* operator<() */

int String::operator<= (const String& s) const {
  return strcmp(str, s.str) <= 0;
} /* operator<=() */

int String::operator<= (char *s) const {
  return strcmp(str, s) <= 0;
} /* operator<=() */

int operator<= (char *str, const String& s) {
  return strcmp(str, s.str) <= 0;
} /* operator<=() */

int String::operator> (const String& s) const {
  return strcmp(str, s.str) > 0;
} /* operator>() */

int String::operator> (char *s) const {
  return strcmp(str, s) > 0;
} /* operator>() */

int operator> (char *str, const String& s) {
  return strcmp(str, s.str) > 0;
} /* operator>() */

int String::operator>= (const String& s) const {
  return strcmp(str, s.str) >= 0;
} /* operator>=() */

int String::operator>= (char *s) const {
  return strcmp(str, s) >= 0;
} /* operator>=() */

int operator>= (char *str, const String& s) {
  return strcmp(str, s.str) >= 0;
} /* operator>=() */

String String::operator+ (const String& s) const {
  String NewString;

  NewString.str = new char[size + s.size + 1];
  NewString.size = size + s.size;
  strcpy(NewString.str, str);
  strcat(NewString.str, s.str);

  return NewString;
} /* operator+() */

String String::operator+ (char *s) const {
  String NewString;

  NewString.str = new char[size + strlen(s) + 1];
  NewString.size = size + strlen(s);
  strcpy(NewString.str, str);
  strcat(NewString.str, s);

  return NewString;
} /* operator+() */

String operator+ (char *str, const String& s) {
  String NewString;

  NewString.str = new char[strlen(str) + s.size + 1];
  NewString.size = strlen(str) + s.size;
  strcpy(NewString.str, str);
  strcat(NewString.str, s.str);

  return NewString;
} /* operator+() */

String String::operator+= (const String& s) {
  String NewString;

  NewString.size = size + s.size;
  NewString.str = new char[size + s.size + 1];
  strcpy(NewString.str, str);
  strcat(NewString.str, s.str);

  size = NewString.size;
  delete [] str;
  str = new char[size + 1];
  strcpy(str, NewString.str);

  return NewString;
} /* operator+=() */

String String::operator+= (char *s) {
  String NewString; 

  NewString.size = size + strlen(s);
  NewString.str = new char[size + strlen(s) + 1];
  strcpy(NewString.str, str);
  strcat(NewString.str, s);

  size = NewString.size;
  delete [] str;
  str=new char[size + 1];
  strcpy(str, NewString.str);
  return NewString;
} /* operator+=() */

String operator+= (char *str, const String& s) {
  String NewString;

  NewString.size = strlen(str) + s.size;
  NewString.str = new char[strlen(str) + s.size + 1];
  strcpy(NewString.str,str);
  strcat(NewString.str,s.str);

  delete [] str;
  str = new char[NewString.size + 1];
  strcpy(str, NewString.str);
  return NewString;
} /* operator+=() */

String String::operator- (const String& s) {
  String NewString;

  if (!size)
    return NULL;

  NewString.str = new char[size + 1];
  strcpy(NewString.str, str);
  NewString.size = size;

  for (int StartPoint = 0; StartPoint < size; StartPoint++)
    if ((NewString.str[StartPoint] == s.str[0]) &&
        (strncmp(&NewString.str[StartPoint], s.str, s.size) == 0))
      NewString.Delete(StartPoint, s.size);

  return NewString;
} /* operator-() */

String String::operator- (char *s) {
  String NewString;

  if (!size)
    return NULL;

  NewString.str = new char[size + 1];
  strcpy(NewString.str, str);
  NewString.size = size;

  for (int StartPoint = 0; StartPoint < size; StartPoint++)
    if ((NewString.str[StartPoint] == s[0]) &&
        (strncmp(&NewString.str[StartPoint], s, strlen(s)) == 0))
      NewString.Delete(StartPoint, strlen(s));

  return NewString;
} /* operator-() */

String operator- (char *str, const String& s) {
  String NewString;

  if (strlen(str) == 0)
    return NULL;

  NewString = str;

  for (int StartPoint = 0; StartPoint < NewString.size; StartPoint++)
    if ((NewString.str[StartPoint] == s.str[0]) &&
        (strncmp(&NewString.str[StartPoint], s.str, s.size) == 0))
      NewString.Delete(StartPoint, s.size);

  return NewString;
} /* operator-() */

String String::operator-= (const String& s) {
  String TempString, NewString;

  if (size==0)
    return NULL;

  NewString.str = str;
  NewString.size = size;
  TempString = NewString - s;
  size = TempString.size;
  delete [] str;
  str = new char[size + 1];
  strcpy(str, TempString.str);

  return TempString;
} /* operator-=() */

String String::operator-= (char *s) {
  String TempString, NewString;

  if (size==0)
    return NULL;

  NewString.str = str;
  NewString.size = size;
  TempString = NewString - s;
  size = TempString.size;
  delete [] str;
  str = new char[size + 1];
  strcpy(str, TempString.str);

  return TempString;
} /* operator-=() */

String operator-= (char *str, const String& s) {
  String TempString;

  TempString = str - s;
  delete [] str;
  str = new char[TempString.size + 1];
  strcpy(str, TempString.str);

  return TempString;
} /* operator-=() */

#if 0
ostream& operator<< (ostream& ostr, const String& s) {
  ostr << s.str;
  return ostr;
} /* operator<<() */

istream& operator>> (istream& istr, String& s) {
  char temp[255];

  istr >> setw(255) >> temp;
  s = temp;

  return istr;
} /* operator>>() */
#endif

char& String::operator[](int subscript) {
  return str[subscript];
} /* operator[]() */

int String::Find(char c) {

  for (int i = 0; i <= size; i++)
    if (c == str[i])
      return i;

  return -1;
} /* Find() */

int String::Find(char c, int start) {

  if (start > size)
    return -1;

  if (start < 0)
    return -1;

  for (int i=start; i <= size; i++)
    if (c == str[i])
      return i;

  return -1;
} /* Find() */

int String::Find(char c, int start, int flag) {

  if (start > size)
    return -1;

  if (start < 0)
    return -1;

  if (flag == 0)
    return Find(c, start);

  for (int i = start; i >= 0; i--)
    if (c == str[i])
      return i;

  return -1;
} /* Find() */

String String::Extract(int index) {
  String ReturnClass;

  if (index > size)
    return NULL;

  if (index < 0)
    index = 0;

  ReturnClass = &str[index];
  return ReturnClass;
} /* Extract() */

String String::Extract(int index, int length) {
  String ReturnClass;

  if ((index > size) || (length < 1))
    return NULL;

  if (index < 0)
    index = 0;

  if ((index + length) > size)
    length = size - index;

  ReturnClass.size = length;
  delete [] ReturnClass.str;
  ReturnClass.str = new char[length + 1];
  strncpy(ReturnClass.str, &str[index], length);

  return ReturnClass;
} /* Extract() */

int String::Extract(char *buf, int index, int length) {

  if ((index > size) || (length < 1)) {
    buf[0] = '\0';
    return 0;
  }

  if (index < 0)
    index = 0;

  if ((index + length) > size)
    length = size - index;

  strncpy(buf, &str[index], length);
  buf[length] = '\0';

  return strlen(buf);
} /* Extract() */

int String::Insert(const String& s) {
  String Temporarystring, Me;

  if (s == NULL)
    return -1;

  Me.str = str;
  Me.size = size;
  Temporarystring = Extract(0);
  Delete(0);
  Me += s;
  Me += Temporarystring;
  strcpy(str, Me.str);

  return (size = Me.size);
} /* Insert() */

int String::Insert(const String& s, int index) {
  String Temporarystring, Me;

  if (index > size)
    return -1;

  if (index < 0)
    return -1;

  Me.str = str;
  Me.size = size;
  Temporarystring = Extract(index);
  Delete(index);
  Me += s;
  Me += Temporarystring;
  strcpy(str, Me.str);

  return (size = Me.size);
} /* Insert() */

int String::Insert(char *s) {
  String Temporarystring, Me;

  if (s == NULL)
    return -1;

  Me.str = str;
  Me.size = size;
  Temporarystring = Extract(0);
  Delete(0);
  Me += s;
  Me += Temporarystring;
  strcpy(str, Me.str);

  return (size = Me.size);
} /* Insert() */

// Available (Other inserts wiggy at best)
int String::Insert(char *s, int index) {
  String old;

  if (s == NULL)
    return -1;

  if (index > size)
    return -1;

  if (index < 0)
    return -1;

  old = str;
  delete [] str;

  size = size + strlen(s);
  str = new char[size + 1];

  strncpy(str, old.str, index);
  strncpy(str + index, s, strlen(s));
  strncpy(str + index + strlen(s), old.str + index, old.size - index);
  str[size] = EOS;

  return size;
} /* Insert() */

int String::Delete(int index) {

  if (index > size)
    return -1;

  if (index < 0)
    return -1;

  if (!size)
    return 0;

  char *NewStr = new char[index + 1];
  str[index] = EOS;
  strcpy(NewStr, str);
  delete [] str;
  str = NewStr;
  size = index;

  return size;
} /* Delete() */

int String::Delete(int index, int length) {

  if (index > size)
    return -1;

  if (index < 0)
    return -1;

  if (!size)
    return 0;

  if (length > size)
    length = size;

  char *NewStr = new char[size - length + 1];
  strncpy(NewStr, str, index);
  strcpy(&NewStr[index], &str[index + length]);
  delete [] str;
  str = NewStr;
  size = strlen(str);

  return size;
} /* Delete() */

int String::QueryLength(void) { return size; }
int String::SizeOf(void) { return (size + sizeof(str)); }

void String::Clear(void) {
  size = 0;
  delete [] str;
  str = new char[1];
  str[0] = EOS;
} /* Clear() */

int String::IsEmpty(void) {

  if (!size)
    return 1;

  return 0;
} /* IsEmpty */

char* String::Text(void) {
  return str;
} /* Text() */

char String::Capitalize(void) {
  str[0] = (char)toupper(str[0]);
  return str[0];
} /* Capitalize() */

char* String::ToUpper(void) {

  for (int i = 0; i < size; i++)
    str[i] = (char)toupper(str[i]);

  return str;
} /* ToUpper() */

char* String::ToLower(void) {
  
  for (int i = 0; i < size; i++)
    str[i] = (char)tolower(str[i]);

  return str;
} /* ToLower() */

#endif /* STRING_CLASS */
