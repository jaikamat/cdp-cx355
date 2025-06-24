#ifndef BIT_H
#define BIT_H

#define BIT_TEST(XXX, YYY)  (XXX & YYY)

#define BIT_ON(XXX, YYY)    (XXX |= YYY)
#define BIT_OFF(XXX, YYY)   (XXX &= ~YYY)
#define BIT_FLIP(XXX, YYY)  ((XXX & YYY) ? BIT_OFF(XXX, YYY) : \
                            BIT_ON(XXX, YYY))

#endif // BIT_H
