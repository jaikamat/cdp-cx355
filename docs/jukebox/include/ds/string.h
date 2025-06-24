#ifndef STRING_CLASS_H
#define STRING_CLASS_H

#ifndef EOS
#define EOS 0
#endif

#include <string.h>
#include <ctype.h>
#include <std/ansi.h>

class String {
  private:
    char *str;
    int size;
    int fg, bg;
    
  public:
    String(void);
    String(char *);
    String(const String&);
    ~String(void);

    String& operator= (const String&);
    String& operator= (char *);

    int operator== (const String&) const;
    int operator== (char *) const;
    friend int operator== (char *, const String&);

    int operator!= (const String&) const;
    int operator!= (char *) const;
    friend int operator!= (char *, const String&);

    int operator< (const String&) const;
    int operator< (char *) const;
    friend int operator< (char *, const String&);

    int operator<= (const String&) const;
    int operator<= (char *) const;
    friend int operator<= (char *, const String&);

    int operator> (const String&) const;
    int operator> (char *) const;
    friend int operator> (char *, const String&);

    int operator>= (const String&) const;
    int operator>= (char *) const;
    friend int operator>= (char *, const String&);

    String operator+ (const String&) const;
    String operator+ (char *) const;
    friend String operator+ (char *, const String&);

    String operator+= (const String&);
    String operator+= (char *);
    friend String operator+= (char *, const String&);

    String operator- (const String&);
    String operator- (char *);
    friend String operator- (char *, const String&);

    String operator-= (const String&);
    String operator-= (char *);
    friend String operator-= (char *, const String&);

    int Find(char);
    int Find(char, int);
    int Find(char, int, int);

    String Extract(int);
    String Extract(int, int);
    int Extract(char *, int, int);

    int Insert(const String&);
    int Insert(const String&, int);
    int Insert(char *);
    int Insert(char *, int);

    int Delete(int);
    int Delete(int, int);

    char& operator[] (int);

    int QueryLength(void);
    int SizeOf(void);
    void Clear(void);
    int IsEmpty(void);
    char *Text(void);
    char Capitalize(void);
    char *ToUpper(void);
    char *ToLower(void);

    void SetColor(int f, int b) { fg = f; bg = b; }
    int QueryForeground(void) { return fg; }
    int QueryBackground(void) { return bg; }

}; /* class String */

#endif /* STRING_CLASS_H */
