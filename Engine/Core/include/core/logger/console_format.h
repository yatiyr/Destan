#pragma once

namespace ds::core
{

// Macros for text formatting
#define DS_CONSOLE_TEXT_RESET            "\033[0m"
#define DS_CONSOLE_TEXT_BOLD             "\033[1m"
#define DS_CONSOLE_TEXT_DIM              "\033[2m"
#define DS_CONSOLE_TEXT_ITALIC           "\033[3m"
#define DS_CONSOLE_TEXT_UNDERLINE        "\033[4m"
#define DS_CONSOLE_TEXT_BLINK            "\033[5m"
#define DS_CONSOLE_TEXT_REVERSE          "\033[7m"
#define DS_CONSOLE_TEXT_HIDDEN           "\033[8m"
#define DS_CONSOLE_TEXT_STRIKETHROUGH    "\033[9m"

// Macros for foreground colors
#define DS_CONSOLE_FG_BLACK              "\033[30m"
#define DS_CONSOLE_FG_RED                "\033[31m"
#define DS_CONSOLE_FG_GREEN              "\033[32m"
#define DS_CONSOLE_FG_YELLOW             "\033[33m"
#define DS_CONSOLE_FG_BLUE               "\033[34m"
#define DS_CONSOLE_FG_MAGENTA            "\033[35m"
#define DS_CONSOLE_FG_CYAN               "\033[36m"
#define DS_CONSOLE_FG_WHITE              "\033[37m"
#define DS_CONSOLE_FG_DEFAULT            "\033[39m"

// Macros for bright foreground colors
#define DS_CONSOLE_FG_BRIGHT_BLACK       "\033[90m"
#define DS_CONSOLE_FG_BRIGHT_RED         "\033[91m"
#define DS_CONSOLE_FG_BRIGHT_GREEN       "\033[92m"
#define DS_CONSOLE_FG_BRIGHT_YELLOW      "\033[93m"
#define DS_CONSOLE_FG_BRIGHT_BLUE        "\033[94m"
#define DS_CONSOLE_FG_BRIGHT_MAGENTA     "\033[95m"
#define DS_CONSOLE_FG_BRIGHT_CYAN        "\033[96m"
#define DS_CONSOLE_FG_BRIGHT_WHITE       "\033[97m"

// Macros for background colors
#define DS_CONSOLE_BG_BLACK              "\033[40m"
#define DS_CONSOLE_BG_RED                "\033[41m"
#define DS_CONSOLE_BG_GREEN              "\033[42m"
#define DS_CONSOLE_BG_YELLOW             "\033[43m"
#define DS_CONSOLE_BG_BLUE               "\033[44m"
#define DS_CONSOLE_BG_MAGENTA            "\033[45m"
#define DS_CONSOLE_BG_CYAN               "\033[46m"
#define DS_CONSOLE_BG_WHITE              "\033[47m"
#define DS_CONSOLE_BG_DEFAULT            "\033[49m"

// Macros for bright background colors
#define DS_CONSOLE_BG_BRIGHT_BLACK       "\033[100m"
#define DS_CONSOLE_BG_BRIGHT_RED         "\033[101m"
#define DS_CONSOLE_BG_BRIGHT_GREEN       "\033[102m"
#define DS_CONSOLE_BG_BRIGHT_YELLOW      "\033[103m"
#define DS_CONSOLE_BG_BRIGHT_BLUE        "\033[104m"
#define DS_CONSOLE_BG_BRIGHT_MAGENTA     "\033[105m"
#define DS_CONSOLE_BG_BRIGHT_CYAN        "\033[106m"
#define DS_CONSOLE_BG_BRIGHT_WHITE       "\033[107m"

    // ANSI escape code constants for text formatting
    namespace console_format
    {
        // Text styles
        constexpr const char* RESET = DS_CONSOLE_TEXT_RESET;
        constexpr const char* BOLD = DS_CONSOLE_TEXT_BOLD;
        constexpr const char* DIM = DS_CONSOLE_TEXT_DIM;
        constexpr const char* ITALIC = DS_CONSOLE_TEXT_ITALIC;
        constexpr const char* UNDERLINE = DS_CONSOLE_TEXT_UNDERLINE;
        constexpr const char* BLINK = DS_CONSOLE_TEXT_BLINK;
        constexpr const char* REVERSE = DS_CONSOLE_TEXT_REVERSE;
        constexpr const char* HIDDEN = DS_CONSOLE_TEXT_HIDDEN;
        constexpr const char* STRIKETHROUGH = DS_CONSOLE_TEXT_STRIKETHROUGH;

        // Foreground colors
        constexpr const char* FG_BLACK = DS_CONSOLE_FG_BLACK;
        constexpr const char* FG_RED = DS_CONSOLE_FG_RED;
        constexpr const char* FG_GREEN = DS_CONSOLE_FG_GREEN;
        constexpr const char* FG_YELLOW = DS_CONSOLE_FG_YELLOW;
        constexpr const char* FG_BLUE = DS_CONSOLE_FG_BLUE;
        constexpr const char* FG_MAGENTA = DS_CONSOLE_FG_MAGENTA;
        constexpr const char* FG_CYAN = DS_CONSOLE_FG_CYAN;
        constexpr const char* FG_WHITE = DS_CONSOLE_FG_WHITE;
        constexpr const char* FG_DEFAULT = DS_CONSOLE_FG_DEFAULT;

        // Bright foreground colors
        constexpr const char* FG_BRIGHT_BLACK = DS_CONSOLE_FG_BRIGHT_BLACK;
        constexpr const char* FG_BRIGHT_RED = DS_CONSOLE_FG_BRIGHT_RED;
        constexpr const char* FG_BRIGHT_GREEN = DS_CONSOLE_FG_BRIGHT_GREEN;
        constexpr const char* FG_BRIGHT_YELLOW = DS_CONSOLE_FG_BRIGHT_YELLOW;
        constexpr const char* FG_BRIGHT_BLUE = DS_CONSOLE_FG_BRIGHT_BLUE;
        constexpr const char* FG_BRIGHT_MAGENTA = DS_CONSOLE_FG_BRIGHT_MAGENTA;
        constexpr const char* FG_BRIGHT_CYAN = DS_CONSOLE_FG_BRIGHT_CYAN;
        constexpr const char* FG_BRIGHT_WHITE = DS_CONSOLE_FG_BRIGHT_WHITE;

        // Background colors
        constexpr const char* BG_BLACK = DS_CONSOLE_BG_BLACK;
        constexpr const char* BG_RED = DS_CONSOLE_BG_RED;
        constexpr const char* BG_GREEN = DS_CONSOLE_BG_GREEN;
        constexpr const char* BG_YELLOW = DS_CONSOLE_BG_YELLOW;
        constexpr const char* BG_BLUE = DS_CONSOLE_BG_BLUE;
        constexpr const char* BG_MAGENTA = DS_CONSOLE_BG_MAGENTA;
        constexpr const char* BG_CYAN = DS_CONSOLE_BG_CYAN;
        constexpr const char* BG_WHITE = DS_CONSOLE_BG_WHITE;
        constexpr const char* BG_DEFAULT = DS_CONSOLE_BG_DEFAULT;

        // Bright background colors
        constexpr const char* BG_BRIGHT_BLACK = DS_CONSOLE_BG_BRIGHT_BLACK;
        constexpr const char* BG_BRIGHT_RED = DS_CONSOLE_BG_BRIGHT_RED;
        constexpr const char* BG_BRIGHT_GREEN = DS_CONSOLE_BG_BRIGHT_GREEN;
        constexpr const char* BG_BRIGHT_YELLOW = DS_CONSOLE_BG_BRIGHT_YELLOW;
        constexpr const char* BG_BRIGHT_BLUE = DS_CONSOLE_BG_BRIGHT_BLUE;
        constexpr const char* BG_BRIGHT_MAGENTA = DS_CONSOLE_BG_BRIGHT_MAGENTA;
        constexpr const char* BG_BRIGHT_CYAN = DS_CONSOLE_BG_BRIGHT_CYAN;
        constexpr const char* BG_BRIGHT_WHITE = DS_CONSOLE_BG_BRIGHT_WHITE;
    }

    // Predefined themes for the logger
    enum class Logger_Theme
    {
        DEFAULT,
        DARK,
        LIGHT,
        VIBRANT,
        MONOCHROME,
        PASTEL,
        CUSTOM
    };

    /**
     * Structure defining a logger theme with colors for different log elements
     */
    struct Theme_Struct
    {
        const char* trace_color;     // Color for TRACE level messages
        const char* info_color;      // Color for INFO level messages
        const char* warn_color;      // Color for WARN level messages
        const char* err_color;       // Color for ERR level messages
        const char* fatal_color;     // Color for FATAL level messages
        const char* timestamp_color; // Color for timestamp
        const char* message_color;   // Color for message text
    };

    // Predefined theme definitions
    constexpr Theme_Struct DEFAULT_THEME = {
        console_format::FG_WHITE,     // trace
        console_format::FG_GREEN,     // info
        console_format::FG_YELLOW,    // warn
        console_format::FG_RED,       // err
        console_format::BG_RED,       // fatal
        console_format::FG_CYAN,      // timestamp
        console_format::RESET         // message
    };

    constexpr Theme_Struct DARK_THEME = {
        console_format::FG_BRIGHT_BLACK,  // trace
        console_format::FG_BRIGHT_BLUE,   // info
        console_format::FG_BRIGHT_YELLOW, // warn
        console_format::FG_BRIGHT_RED,    // err
        console_format::BG_RED,           // fatal
        console_format::FG_BRIGHT_BLACK,  // timestamp
        console_format::FG_BRIGHT_WHITE   // message
    };

    constexpr Theme_Struct LIGHT_THEME = {
        console_format::FG_BLACK,     // trace
        console_format::FG_BLUE,      // info
        console_format::FG_YELLOW,    // warn
        console_format::FG_RED,       // err
        console_format::BG_RED,       // fatal
        console_format::FG_BLACK,     // timestamp
        console_format::FG_BLACK      // message
    };

    constexpr Theme_Struct VIBRANT_THEME = {
        console_format::FG_BRIGHT_CYAN,    // trace
        console_format::FG_BRIGHT_GREEN,   // info
        console_format::FG_BRIGHT_YELLOW,  // warn
        console_format::FG_BRIGHT_RED,     // err
        console_format::BG_BRIGHT_RED,     // fatal
        console_format::FG_BRIGHT_MAGENTA, // timestamp
        console_format::FG_WHITE           // message
    };

    constexpr Theme_Struct MONOCHROME_THEME = {
        console_format::DIM,          // trace
        console_format::RESET,        // info
        console_format::BOLD,         // warn
        console_format::UNDERLINE,    // err
        console_format::REVERSE,      // fatal
        console_format::FG_WHITE,     // timestamp
        console_format::RESET         // message
    };

    constexpr Theme_Struct PASTEL_THEME = {
        console_format::FG_CYAN,      // trace
        console_format::FG_GREEN,     // info
        console_format::FG_YELLOW,    // warn
        console_format::FG_MAGENTA,   // err
        console_format::BG_MAGENTA,   // fatal
        console_format::FG_BLUE,      // timestamp
        console_format::FG_WHITE      // message
    };
}

#define DS_STYLED(style, text) style text DS_CONSOLE_TEXT_RESET