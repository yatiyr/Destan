#include <core/destan_pch.h>
#include <core/logger/logger.h>
#include <core/logger/console_format.h>
#include <test_framework.h>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>

// Helper to read log file contents
std::string Read_Log_File()
{
    // Wait a moment for logger thread to complete writing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::ifstream log_file("destan.log");
    if (!log_file.is_open()) {
        return "";
    }

    std::stringstream buffer;
    buffer << log_file.rdbuf();
    return buffer.str();
}

// Helper to check if log contains text
bool Log_Contains(const std::string& log_content, const std::string& text)
{
    return log_content.find(text) != std::string::npos;
}

// Helper to concatenate ANSI format codes
std::string Combine_Formats(std::initializer_list<const char*> formats)
{
    std::string result;
    for (const char* format : formats)
    {
        result += format;
    }
    return result;
}

// Helper to convert Logger_Theme to string
std::string Theme_To_String(destan::core::Logger_Theme theme)
{
    switch (theme)
    {
    case destan::core::Logger_Theme::DEFAULT:
        return "DEFAULT";
    case destan::core::Logger_Theme::DARK:
        return "DARK";
    case destan::core::Logger_Theme::LIGHT:
        return "LIGHT";
    case destan::core::Logger_Theme::VIBRANT:
        return "VIBRANT";
    case destan::core::Logger_Theme::MONOCHROME:
        return "MONOCHROME";
    case destan::core::Logger_Theme::PASTEL:
        return "PASTEL";
    case destan::core::Logger_Theme::CUSTOM:
        return "CUSTOM";
    default:
        return "UNKNOWN";
    }
}

int main(int argc, char** argv)
{
    // Delete previous log file if it exists
    std::remove("destan.log");

    return destan::test::Test_Runner::Run_Tests([]()
    {
        destan::test::Test_Suite logger_tests("Logger Theme and Style Tests");

        // Test basic logging functionality
        DESTAN_TEST(logger_tests, "Basic Logging")
        {
            // Log a simple message
            DESTAN_LOG_INFO("Basic logging test message");

            // Allow time for logging thread to process
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Read log file and check contents
            std::string log_content = Read_Log_File();
            DESTAN_EXPECT(Log_Contains(log_content, "Basic logging test message"));

            return true;
        });

        // Test different log levels
        DESTAN_TEST(logger_tests, "Log Levels")
        {
            // Test each log level
            DESTAN_LOG_TRACE("This is a trace message");
            DESTAN_LOG_INFO("This is an info message");
            DESTAN_LOG_WARN("This is a warning message");
            DESTAN_LOG_ERROR("This is an error message");
            DESTAN_LOG_FATAL("This is a fatal message");

            // Allow time for logging thread to process
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Read log file
            std::string log_content = Read_Log_File();

            // Check that each log level appears in the file
            DESTAN_EXPECT(Log_Contains(log_content, "This is a trace message"));
            DESTAN_EXPECT(Log_Contains(log_content, "This is an info message"));
            DESTAN_EXPECT(Log_Contains(log_content, "This is a warning message"));
            DESTAN_EXPECT(Log_Contains(log_content, "This is an error message"));
            DESTAN_EXPECT(Log_Contains(log_content, "This is a fatal message"));

            // Check level indicators
            DESTAN_EXPECT(Log_Contains(log_content, "[TRACE]"));
            DESTAN_EXPECT(Log_Contains(log_content, "[INFO]"));
            DESTAN_EXPECT(Log_Contains(log_content, "[WARN]"));
            DESTAN_EXPECT(Log_Contains(log_content, "[ERR]"));
            DESTAN_EXPECT(Log_Contains(log_content, "[FATAL]"));

            return true;
        });

        // Test built-in themes
        DESTAN_TEST(logger_tests, "Built-In Themes")
        {
            // Test each theme
            const destan::core::Logger_Theme themes[] =
            {
                destan::core::Logger_Theme::DEFAULT,
                destan::core::Logger_Theme::DARK,
                destan::core::Logger_Theme::LIGHT,
                destan::core::Logger_Theme::VIBRANT,
                destan::core::Logger_Theme::MONOCHROME,
                destan::core::Logger_Theme::PASTEL
            };

            for (int i = 0; i < 6; ++i)
            {
                destan::core::Logger::Apply_Theme(themes[i]);
                std::string theme_name = Theme_To_String(themes[i]);

                // Log with each level to test colors
                DESTAN_LOG_INFO("Theme: {0}", theme_name);
                DESTAN_LOG_TRACE("Trace message with {0} theme", theme_name);
                DESTAN_LOG_INFO("Info message with {0} theme", theme_name);
                DESTAN_LOG_WARN("Warning message with {0} theme", theme_name);
                DESTAN_LOG_ERROR("Error message with {0} theme",theme_name);
                DESTAN_LOG_FATAL("Fatal message with {0} theme" ,theme_name);

                // Verify theme was changed
                // Use our helper function to compare theme enums indirectly
                DESTAN_EXPECT(Theme_To_String(destan::core::Logger::Get_Current_Theme()) == theme_name);

                // Give console time to render
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

            return true;
        });

        // Test custom theme
        DESTAN_TEST(logger_tests, "Custom Theme")
        {
            // Create and apply a custom theme using string concatenation
            auto custom_theme = destan::core::Logger::Create_Custom_Theme(
                DESTAN_CONSOLE_FG_BRIGHT_CYAN,
                DESTAN_CONSOLE_FG_BRIGHT_GREEN DESTAN_CONSOLE_TEXT_BOLD,
                DESTAN_CONSOLE_FG_BRIGHT_YELLOW DESTAN_CONSOLE_TEXT_BOLD,
                DESTAN_CONSOLE_FG_BRIGHT_RED DESTAN_CONSOLE_TEXT_BOLD,
                DESTAN_CONSOLE_FG_BRIGHT_RED DESTAN_CONSOLE_TEXT_BLINK DESTAN_CONSOLE_TEXT_BOLD,
                DESTAN_CONSOLE_FG_BRIGHT_MAGENTA,
                DESTAN_CONSOLE_FG_BRIGHT_WHITE
            );

            destan::core::Logger::Apply_Theme_Struct(custom_theme);

            // Test logging with custom theme
            DESTAN_LOG_INFO("Custom Theme Applied");
            DESTAN_LOG_TRACE("Trace message with custom theme");
            DESTAN_LOG_INFO("Info message with custom theme");
            DESTAN_LOG_WARN("Warning message with custom theme");
            DESTAN_LOG_ERROR("Error message with custom theme");
            DESTAN_LOG_FATAL("Fatal message with custom theme");

            // Verify theme was changed to custom
            DESTAN_EXPECT(Theme_To_String(destan::core::Logger::Get_Current_Theme()) == "CUSTOM");

            // Give console time to render
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            return true;
        });

        // Test text formatting macros
        DESTAN_TEST(logger_tests, "Text Style Formatting")
        {
            // Reset to default theme for clarity
            destan::core::Logger::Apply_Theme(destan::core::Logger_Theme::DEFAULT);

            DESTAN_LOG_INFO("=== Text Style Tests ===");

            // Test different text styles using the formatting macros
            DESTAN_LOG_INFO("Normal text for comparison");

            // Using the text formatting macros
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_TEXT_BOLD "Bold text" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_TEXT_DIM "Dim text" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_TEXT_ITALIC "Italic text" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_TEXT_UNDERLINE "Underlined text" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_TEXT_BLINK "Blinking text" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_TEXT_REVERSE "Reversed text" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_TEXT_STRIKETHROUGH "Strikethrough text" DESTAN_CONSOLE_TEXT_RESET);

            // Test combining multiple styles
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_TEXT_BOLD DESTAN_CONSOLE_TEXT_UNDERLINE "Bold and Underlined" DESTAN_CONSOLE_TEXT_RESET);

            // More complex combinations
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_TEXT_BOLD DESTAN_CONSOLE_TEXT_ITALIC DESTAN_CONSOLE_TEXT_UNDERLINE
                "Bold, Italic and Underlined" DESTAN_CONSOLE_TEXT_RESET);

            // Give console time to render
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            return true;
        });

        // Test foreground colors
        DESTAN_TEST(logger_tests, "Foreground Colors")
        {
            // Reset to default theme
            destan::core::Logger::Apply_Theme(destan::core::Logger_Theme::DEFAULT);

            DESTAN_LOG_INFO("=== Foreground Color Tests ===");

            // Test different foreground colors
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_FG_BLACK "Black text" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_FG_RED "Red text" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_FG_GREEN "Green text" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_FG_YELLOW "Yellow text" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_FG_BLUE "Blue text" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_FG_MAGENTA "Magenta text" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_FG_CYAN "Cyan text" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_FG_WHITE "White text" DESTAN_CONSOLE_TEXT_RESET);

            // Test bright foreground colors
            DESTAN_LOG_INFO("=== Bright Foreground Color Tests ===");
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_FG_BRIGHT_BLACK "Bright Black text" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_FG_BRIGHT_RED "Bright Red text" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_FG_BRIGHT_GREEN "Bright Green text" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_FG_BRIGHT_YELLOW "Bright Yellow text" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_FG_BRIGHT_BLUE "Bright Blue text" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_FG_BRIGHT_MAGENTA "Bright Magenta text" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_FG_BRIGHT_CYAN "Bright Cyan text" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_FG_BRIGHT_WHITE "Bright White text" DESTAN_CONSOLE_TEXT_RESET);

            // Give console time to render
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            return true;
        });

        // Test background colors
        DESTAN_TEST(logger_tests, "Background Colors")
        {
            // Reset to default theme
            destan::core::Logger::Apply_Theme(destan::core::Logger_Theme::DEFAULT);

            DESTAN_LOG_INFO("=== Background Color Tests ===");

            // Test different background colors
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_BG_BLACK "Black background" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_BG_RED "Red background" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_BG_GREEN "Green background" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_BG_YELLOW "Yellow background" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_BG_BLUE "Blue background" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_BG_MAGENTA "Magenta background" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_BG_CYAN "Cyan background" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_BG_WHITE "White background" DESTAN_CONSOLE_TEXT_RESET);

            // Test bright background colors
            DESTAN_LOG_INFO("=== Bright Background Color Tests ===");
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_BG_BRIGHT_BLACK "Bright Black background" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_BG_BRIGHT_RED "Bright Red background" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_BG_BRIGHT_GREEN "Bright Green background" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_BG_BRIGHT_YELLOW "Bright Yellow background" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_BG_BRIGHT_BLUE "Bright Blue background" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_BG_BRIGHT_MAGENTA "Bright Magenta background" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_BG_BRIGHT_CYAN "Bright Cyan background" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_BG_BRIGHT_WHITE "Bright White background" DESTAN_CONSOLE_TEXT_RESET);

            // Give console time to render
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            return true;
        });

        // Test combining styles and colors
        DESTAN_TEST(logger_tests, "Combined Formatting")
        {
            // Reset to default theme
            destan::core::Logger::Apply_Theme(destan::core::Logger_Theme::DEFAULT);

            DESTAN_LOG_INFO("=== Combined Format Tests ===");

            // Test combinations of styles and colors
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_TEXT_BOLD DESTAN_CONSOLE_FG_RED "Bold Red Text" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_TEXT_ITALIC DESTAN_CONSOLE_FG_BLUE "Italic Blue Text" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_TEXT_UNDERLINE DESTAN_CONSOLE_FG_GREEN "Underlined Green Text" DESTAN_CONSOLE_TEXT_RESET);

            // More complex combinations with both foreground and background
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_TEXT_BOLD DESTAN_CONSOLE_FG_CYAN DESTAN_CONSOLE_BG_YELLOW 
                            "Bold Cyan on Yellow" DESTAN_CONSOLE_TEXT_RESET);

            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_TEXT_REVERSE DESTAN_CONSOLE_FG_WHITE DESTAN_CONSOLE_BG_BLUE 
                            "Reversed White on Blue" DESTAN_CONSOLE_TEXT_RESET);

            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_TEXT_BOLD DESTAN_CONSOLE_TEXT_UNDERLINE DESTAN_CONSOLE_FG_MAGENTA 
                            "Bold Underlined Magenta" DESTAN_CONSOLE_TEXT_RESET);

            // Complex combinations
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_TEXT_BOLD DESTAN_CONSOLE_TEXT_ITALIC DESTAN_CONSOLE_FG_BRIGHT_RED 
                            DESTAN_CONSOLE_BG_BLUE "Bold Italic Bright Red on Dark Blue" DESTAN_CONSOLE_TEXT_RESET);

            // Give console time to render
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            return true;
        });

        // Test text alignment and length
        DESTAN_TEST(logger_tests, "Text Formatting with Different Lengths")
        {
            // Reset to default theme
            destan::core::Logger::Apply_Theme(destan::core::Logger_Theme::DEFAULT);

            DESTAN_LOG_INFO("=== Text Length Tests ===");

            // Test short and long messages with formatting
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_FG_RED DESTAN_CONSOLE_TEXT_BOLD "Short" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_FG_GREEN DESTAN_CONSOLE_TEXT_UNDERLINE "Medium length text" DESTAN_CONSOLE_TEXT_RESET);
            DESTAN_LOG_INFO("{}", DESTAN_CONSOLE_FG_YELLOW DESTAN_CONSOLE_TEXT_ITALIC
                "This is a much longer message that will wrap in the console to test how formatting works with line wrapping" 
                DESTAN_CONSOLE_TEXT_RESET);

            // Test mixed formatting in text
            DESTAN_LOG_INFO("Normal text with {} and {} and {}", 
                DESTAN_CONSOLE_TEXT_BOLD DESTAN_CONSOLE_FG_RED "bold red" DESTAN_CONSOLE_TEXT_RESET,
                DESTAN_CONSOLE_TEXT_ITALIC DESTAN_CONSOLE_FG_BLUE "italic blue" DESTAN_CONSOLE_TEXT_RESET,
                DESTAN_CONSOLE_TEXT_UNDERLINE DESTAN_CONSOLE_FG_GREEN "underlined green" DESTAN_CONSOLE_TEXT_RESET);

            // Give console time to render
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            return true;
        });

        // Add test with DESTAN_STYLED macro once implemented
        DESTAN_TEST(logger_tests, "DESTAN_STYLED Macro")
        {
            // Reset to default theme
            destan::core::Logger::Apply_Theme(destan::core::Logger_Theme::DEFAULT);

            DESTAN_LOG_INFO("=== DESTAN_STYLED Macro Tests ===");

            // Basic styling
            DESTAN_LOG_INFO("{}", DESTAN_STYLED(DESTAN_CONSOLE_FG_RED, "This text is red"));

            // Multiple style attributes
            DESTAN_LOG_INFO("{}", DESTAN_STYLED(DESTAN_CONSOLE_TEXT_BOLD DESTAN_CONSOLE_FG_BLUE, "This text is bold and blue"));

            // Multi-variant helper for clarity
            #define DESTAN_STYLED_MULTI(style1, style2, text) style1 style2 text DESTAN_CONSOLE_TEXT_RESET
            
            DESTAN_LOG_INFO("{}", DESTAN_STYLED_MULTI(DESTAN_CONSOLE_TEXT_BOLD, DESTAN_CONSOLE_FG_GREEN, "Bold green using STYLED_MULTI"));

            // Complex styling
            DESTAN_LOG_INFO("{}", DESTAN_STYLED(DESTAN_CONSOLE_TEXT_BOLD DESTAN_CONSOLE_TEXT_UNDERLINE DESTAN_CONSOLE_FG_BRIGHT_CYAN DESTAN_CONSOLE_BG_RED,
                "Bold underlined bright cyan text on red background"));

            // Partial styling in a log message
            DESTAN_LOG_INFO("This is normal text with {} and {} words",
                DESTAN_STYLED(DESTAN_CONSOLE_FG_RED, "red"),
                DESTAN_STYLED(DESTAN_CONSOLE_FG_GREEN, "green"));

            // Nested styling test
            std::string nested = std::string(DESTAN_STYLED(DESTAN_CONSOLE_FG_BLUE, "Blue text with ")) +
                std::string(DESTAN_STYLED(DESTAN_CONSOLE_TEXT_BOLD DESTAN_CONSOLE_FG_RED, "bold red")) +
                std::string(DESTAN_STYLED(DESTAN_CONSOLE_FG_BLUE, " nested inside"));
            DESTAN_LOG_INFO("{}", nested);

            // Give console time to render
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            return true;
        });

        // Run all tests
        return logger_tests.Run_All();
    });
}