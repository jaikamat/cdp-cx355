#ifndef ANSI_CONTROL_CODE_H
#define ANSI_CONTROL_CODE_H

#define CTRL_CODE_SIZE    6

#define BLACK             0
#define RED               1
#define GREEN             2
#define YELLOW            3
#define BLUE              4
#define MAGENTA           5
#define CYAN              6
#define WHITE             7

#define RESET             1
#define BOLD              2
#define UNDERSCORE        4
#define BLINK             8
#define REVERSE           16
#define INVISIBLE         32

#define BLACK_ASC         "\33[30m"
#define RED_ASC           "\33[31m"
#define GREEN_ASC         "\33[32m"
#define YELLOW_ASC        "\33[33m"
#define BLUE_ASC          "\33[34m"
#define MAGENTA_ASC       "\33[35m"
#define CYAN_ASC          "\33[36m"
#define WHITE_ASC         "\33[37m"

#define BG_BLACK_ASC      "\33[40m"
#define BG_RED_ASC        "\33[41m"
#define BG_GREEN_ASC      "\33[42m"
#define BG_YELLOW_ASC     "\33[43m"
#define BG_BLUE_ASC       "\33[44m"
#define BG_MAGENTA_ASC    "\33[45m"
#define BG_CYAN_ASC       "\33[46m"
#define BG_WHITE_ASC      "\33[47m"

#define RESET_ASC         "\33[0m"
#define BOLD_ASC          "\33[1m"
#define UNDERSCORE_ASC    "\33[4m"
#define BLINK_ASC         "\33[5m"
#define REVERSE_ASC       "\33[7m"
#define INVISIBLE_ASC     "\33[8m"

#define FG                "\33[3%dm"
#define BG                "\33[4%dm"

#define FILL              BG CLEAR_ALL

#define SAVE_ATTR         "\337"
#define LOAD_ATTR         "\338"

#define LOC               "\33[%d;%d;H"
#define SAVE_LOC          "\33[s"
#define LOAD_LOC          "\33[u"

#define NEW_LINE          "\33E"
#define REV_LINE          "\33M"

#define SET_TAB           "\33H"
#define CLEAR_TAB         "\33[g"
#define CLEAR_ALL_TABS    "\33[3g"

#define CLEAR_EOL         "\33[K"
#define CLEAR_BOL         "\33[1K"
#define CLEAR_LINE        "\33[2K"
#define CLEAR_EOS         "\33[J"
#define CLEAR_BOS         "\33[1J"
#define CLEAR_ALL         "\33[2J"

#define INS_LINE(XXX)     "\33[" XXX "L"
#define DEL_LINE(XXX)     "\33[" XXX "M"
#define DEL_CHAR(XXX)     "\33[" XXX "P"

#define PRINT_ALL         "\33[i"
#define PRINT_LINE        "\33[?1i"

#define SET_VID_MODE      "\33[=%d;7h"

#define SET_MODE(XXX)     "\33[" XXX "h"
#define RESET_MODE(XXX)   "\33[" XXX "l"

#define REGION            "\33[%d;%dr"

#define UP                "\33[A"
#define DOWN              "\33[B"
#define RIGHT             "\33[C"
#define LEFT              "\33[D"

#define DIV               "\\"
#define MULT              "*"
#define PLUS              "+"
#define MINUS             "-"

#define KEY_LB            "["
#define KEY_RB            "]"
#define KEY_COMMA         ","
#define KEY_PERIOD        "."
#define KEY_FS            "/"
#define KEY_BS            "\\"
#define KEY_SC            ";"
#define KEY_APOS          "'"
#define KEY_REV_APOS      "`"

#define KEY_a             "a"
#define KEY_b             "b"
#define KEY_c             "c"
#define KEY_d             "d"
#define KEY_e             "e"
#define KEY_f             "f"
#define KEY_g             "g"
#define KEY_h             "h"
#define KEY_i             "i"
#define KEY_j             "j"
#define KEY_k             "k"
#define KEY_l             "l"
#define KEY_m             "m"
#define KEY_n             "n"
#define KEY_o             "o"
#define KEY_p             "p"
#define KEY_q             "q"
#define KEY_r             "r"
#define KEY_s             "s"
#define KEY_t             "t"
#define KEY_u             "u"
#define KEY_v             "v"
#define KEY_w             "w"
#define KEY_x             "x"
#define KEY_y             "y"
#define KEY_z             "z"

#define KEY_A             "A"
#define KEY_B             "B"
#define KEY_C             "C"
#define KEY_D             "D"
#define KEY_E             "E"
#define KEY_F             "F"
#define KEY_G             "G"
#define KEY_H             "H"
#define KEY_I             "I"
#define KEY_J             "J"
#define KEY_K             "K"
#define KEY_L             "L"
#define KEY_M             "M"
#define KEY_N             "N"
#define KEY_O             "O"
#define KEY_P             "P"
#define KEY_Q             "Q"
#define KEY_R             "R"
#define KEY_S             "S"
#define KEY_T             "T"
#define KEY_U             "U"
#define KEY_V             "V"
#define KEY_W             "W"
#define KEY_X             "X"
#define KEY_Y             "Y"
#define KEY_Z             "Z"

#define HOME              "\33[1~"     
#define INSERT            "\33[2~"
#define DELETE            "\33[3~"
#define END               "\33[4~"
#define PAGEUP            "\33[5~"
#define PAGEDOWN          "\33[6~"

#define TAB               "\t"
#define ENTER             "\n"
#define SPACE             "\40"
#define BACKSPACE         "\177"
#define ESCAPE            "\33"

#define ALT_TAB           "\33\t"
#define ALT_ENTER         "\33\n"
#define ALT_SPACE         "\33\40"
#define ALT_BACKSPACE     "\33\177"

#define CTRL_A            "\1"
#define CTRL_B            "\2"
#define CTRL_C            "\3"
#define CTRL_D            "\4"
#define CTRL_E            "\5"
#define CTRL_F            "\6"
#define CTRL_G            "\7"
#define CTRL_H            "\8"
#define CTRL_I            "\9"
#define CTRL_J            "\10"
#define CTRL_K            "\11"
#define CTRL_L            "\12"
#define CTRL_M            "\13"
#define CTRL_N            "\14"
#define CTRL_O            "\15"
#define CTRL_P            "\16"
#define CTRL_Q            "\17"
#define CTRL_R            "\18"
#define CTRL_S            "\19"
#define CTRL_T            "\20"
#define CTRL_U            "\21"
#define CTRL_V            "\22"
#define CTRL_W            "\23"
#define CTRL_X            "\24"
#define CTRL_Y            "\25"
#define CTRL_Z            "\26"

#define ALT_A             "\33a"
#define ALT_B             "\33b"
#define ALT_C             "\33c"
#define ALT_D             "\33d"
#define ALT_E             "\33e"
#define ALT_F             "\33f"
#define ALT_G             "\33g"
#define ALT_H             "\33h"
#define ALT_I             "\33i"
#define ALT_J             "\33j"
#define ALT_K             "\33k"
#define ALT_L             "\33l"
#define ALT_M             "\33m"
#define ALT_N             "\33n"
#define ALT_O             "\33o"
#define ALT_P             "\33p"
#define ALT_Q             "\33q"
#define ALT_R             "\33r"
#define ALT_S             "\33s"
#define ALT_T             "\33t"
#define ALT_U             "\33u"
#define ALT_V             "\33v"
#define ALT_W             "\33w"
#define ALT_X             "\33x"
#define ALT_Y             "\33y"
#define ALT_Z             "\33z"

#define KEY_F1            "\33[[A"
#define KEY_F2            "\33[[B"
#define KEY_F3            "\33[[C"
#define KEY_F4            "\33[[D"
#define KEY_F5            "\33[[E"
#define KEY_F6            "\33[17~"
#define KEY_F7            "\33[18~"
#define KEY_F8            "\33[19~"
#define KEY_F9            "\33[20~"
#define KEY_F10           "\33[21~"
#define KEY_F11           "\33[23~"
#define KEY_F12           "\33[24~"

#define PENDING(XXX)      (!strcmp(pending, XXX))
#define HOTKEY(XXX, YYY)  if (PENDING(XXX)) return YYY;

#define IS_LOWER(XXX)     ((XXX[0] > 96) && (XXX[0] < 123))
#define IS_UPPER(XXX)     ((XXX[0] > 64) && (XXX[0] < 91))
#define IS_LETTER(XXX)    (IS_LOWER(XXX) || IS_UPPER(XXX))
#define IS_PRINT(XXX)     ((XXX[0] > 31) && (XXX[0] < 127))

#endif /* ANSI_CONTROL_CODE_H */
