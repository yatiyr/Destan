#include <core/ds_pch.h>
#include <core/logger/logger.h>
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

    std::ifstream log_file("ds.log");
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
std::string Theme_To_String(ds::core::Logger_Theme theme)
{
    switch (theme)
    {
    case ds::core::Logger_Theme::DEFAULT:
        return "DEFAULT";
    case ds::core::Logger_Theme::DARK:
        return "DARK";
    case ds::core::Logger_Theme::LIGHT:
        return "LIGHT";
    case ds::core::Logger_Theme::VIBRANT:
        return "VIBRANT";
    case ds::core::Logger_Theme::MONOCHROME:
        return "MONOCHROME";
    case ds::core::Logger_Theme::PASTEL:
        return "PASTEL";
    case ds::core::Logger_Theme::CUSTOM:
        return "CUSTOM";
    default:
        return "UNKNOWN";
    }
}

int main(int argc, char** argv)
{
    // Delete previous log file if it exists
    std::remove("ds.log");

    return ds::test::Test_Runner::Run_Tests([]()
    {
        ds::test::Test_Suite logger_tests("Logger Theme and Style Tests");

        // Test basic logging functionality
        DS_TEST(logger_tests, "Basic Logging")
        {
            // Log a simple message
            ds::core::Logger::Get_Instance().Set_File_Output_Mode(true);
            DS_LOG_INFO("Basic logging test message");

            // Allow time for logging thread to process
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Read log file and check contents
            std::string log_content = Read_Log_File();
            DS_EXPECT(Log_Contains(log_content, "Basic logging test message"));
            ds::core::Logger::Get_Instance().Set_File_Output_Mode(false);
            return true;
        });

        // Test different log levels
        DS_TEST(logger_tests, "Log Levels")
        {
            // Test each log level
            ds::core::Logger::Get_Instance().Set_File_Output_Mode(true);
            DS_LOG_TRACE("This is a trace message");
            DS_LOG_INFO("This is an info message");
            DS_LOG_WARN("This is a warning message");
            DS_LOG_ERROR("This is an error message");
            DS_LOG_FATAL("This is a fatal message");

            // Allow time for logging thread to process
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Read log file
            std::string log_content = Read_Log_File();

            // Check that each log level appears in the file
            DS_EXPECT(Log_Contains(log_content, "This is a trace message"));
            DS_EXPECT(Log_Contains(log_content, "This is an info message"));
            DS_EXPECT(Log_Contains(log_content, "This is a warning message"));
            DS_EXPECT(Log_Contains(log_content, "This is an error message"));
            DS_EXPECT(Log_Contains(log_content, "This is a fatal message"));

            // Check level indicators
            DS_EXPECT(Log_Contains(log_content, "[TRACE]"));
            DS_EXPECT(Log_Contains(log_content, "[INFO]"));
            DS_EXPECT(Log_Contains(log_content, "[WARN]"));
            DS_EXPECT(Log_Contains(log_content, "[ERR]"));
            DS_EXPECT(Log_Contains(log_content, "[FATAL]"));
            ds::core::Logger::Get_Instance().Set_File_Output_Mode(false);
            return true;
        });

        // Test built-in themes
        DS_TEST(logger_tests, "Built-In Themes")
        {
            // Test each theme
            const ds::core::Logger_Theme themes[] =
            {
                ds::core::Logger_Theme::DEFAULT,
                ds::core::Logger_Theme::DARK,
                ds::core::Logger_Theme::LIGHT,
                ds::core::Logger_Theme::VIBRANT,
                ds::core::Logger_Theme::MONOCHROME,
                ds::core::Logger_Theme::PASTEL
            };

            for (int i = 0; i < 6; ++i)
            {
                ds::core::Logger::Apply_Theme(themes[i]);
                std::string theme_name = Theme_To_String(themes[i]);

                // Log with each level to test colors
                DS_LOG_INFO("Theme: {0}", theme_name);
                DS_LOG_TRACE("Trace message with {0} theme", theme_name);
                DS_LOG_INFO("Info message with {0} theme", theme_name);
                DS_LOG_WARN("Warning message with {0} theme", theme_name);
                DS_LOG_ERROR("Error message with {0} theme",theme_name);
                DS_LOG_FATAL("Fatal message with {0} theme" ,theme_name);

                // Verify theme was changed
                // Use our helper function to compare theme enums indirectly
                DS_EXPECT(Theme_To_String(ds::core::Logger::Get_Current_Theme()) == theme_name);

                // Give console time to render
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

            return true;
        });

        // Test custom theme
        DS_TEST(logger_tests, "Custom Theme")
        {
            // Create and apply a custom theme using string concatenation
            auto custom_theme = ds::core::Logger::Create_Custom_Theme(
                DS_CONSOLE_FG_BRIGHT_CYAN,
                DS_CONSOLE_FG_BRIGHT_GREEN DS_CONSOLE_TEXT_BOLD,
                DS_CONSOLE_FG_BRIGHT_YELLOW DS_CONSOLE_TEXT_BOLD,
                DS_CONSOLE_FG_BRIGHT_RED DS_CONSOLE_TEXT_BOLD,
                DS_CONSOLE_FG_BRIGHT_RED DS_CONSOLE_TEXT_BLINK DS_CONSOLE_TEXT_BOLD,
                DS_CONSOLE_FG_BRIGHT_MAGENTA,
                DS_CONSOLE_FG_BRIGHT_WHITE
            );

            ds::core::Logger::Apply_Theme_Struct(custom_theme);

            // Test logging with custom theme
            DS_LOG_INFO("Custom Theme Applied");
            DS_LOG_TRACE("Trace message with custom theme");
            DS_LOG_INFO("Info message with custom theme");
            DS_LOG_WARN("Warning message with custom theme");
            DS_LOG_ERROR("Error message with custom theme");
            DS_LOG_FATAL("Fatal message with custom theme");

            // Verify theme was changed to custom
            DS_EXPECT(Theme_To_String(ds::core::Logger::Get_Current_Theme()) == "CUSTOM");

            // Give console time to render
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            return true;
        });

        // Test text formatting macros
        DS_TEST(logger_tests, "Text Style Formatting")
        {
            // Reset to default theme for clarity
            ds::core::Logger::Apply_Theme(ds::core::Logger_Theme::DEFAULT);

            DS_LOG_INFO("=== Text Style Tests ===");

            // Test different text styles using the formatting macros
            DS_LOG_INFO("Normal text for comparison");

            // Using the text formatting macros
            DS_LOG_INFO("{}", DS_CONSOLE_TEXT_BOLD "Bold text" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_TEXT_DIM "Dim text" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_TEXT_ITALIC "Italic text" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_TEXT_UNDERLINE "Underlined text" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_TEXT_BLINK "Blinking text" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_TEXT_REVERSE "Reversed text" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_TEXT_STRIKETHROUGH "Strikethrough text" DS_CONSOLE_TEXT_RESET);

            // Test combining multiple styles
            DS_LOG_INFO("{}", DS_CONSOLE_TEXT_BOLD DS_CONSOLE_TEXT_UNDERLINE "Bold and Underlined" DS_CONSOLE_TEXT_RESET);

            // More complex combinations
            DS_LOG_INFO("{}", DS_CONSOLE_TEXT_BOLD DS_CONSOLE_TEXT_ITALIC DS_CONSOLE_TEXT_UNDERLINE
                "Bold, Italic and Underlined" DS_CONSOLE_TEXT_RESET);

            // Give console time to render
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            return true;
        });

        // Test foreground colors
        DS_TEST(logger_tests, "Foreground Colors")
        {
            // Reset to default theme
            ds::core::Logger::Apply_Theme(ds::core::Logger_Theme::DEFAULT);

            DS_LOG_INFO("=== Foreground Color Tests ===");

            // Test different foreground colors
            DS_LOG_INFO("{}", DS_CONSOLE_FG_BLACK "Black text" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_FG_RED "Red text" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_FG_GREEN "Green text" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_FG_YELLOW "Yellow text" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_FG_BLUE "Blue text" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_FG_MAGENTA "Magenta text" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_FG_CYAN "Cyan text" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_FG_WHITE "White text" DS_CONSOLE_TEXT_RESET);

            // Test bright foreground colors
            DS_LOG_INFO("=== Bright Foreground Color Tests ===");
            DS_LOG_INFO("{}", DS_CONSOLE_FG_BRIGHT_BLACK "Bright Black text" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_FG_BRIGHT_RED "Bright Red text" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_FG_BRIGHT_GREEN "Bright Green text" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_FG_BRIGHT_YELLOW "Bright Yellow text" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_FG_BRIGHT_BLUE "Bright Blue text" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_FG_BRIGHT_MAGENTA "Bright Magenta text" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_FG_BRIGHT_CYAN "Bright Cyan text" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_FG_BRIGHT_WHITE "Bright White text" DS_CONSOLE_TEXT_RESET);

            // Give console time to render
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            return true;
        });

        // Test background colors
        DS_TEST(logger_tests, "Background Colors")
        {
            // Reset to default theme
            ds::core::Logger::Apply_Theme(ds::core::Logger_Theme::DEFAULT);

            DS_LOG_INFO("=== Background Color Tests ===");

            // Test different background colors
            DS_LOG_INFO("{}", DS_CONSOLE_BG_BLACK "Black background" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_BG_RED "Red background" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_BG_GREEN "Green background" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_BG_YELLOW "Yellow background" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_BG_BLUE "Blue background" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_BG_MAGENTA "Magenta background" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_BG_CYAN "Cyan background" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_BG_WHITE "White background" DS_CONSOLE_TEXT_RESET);

            // Test bright background colors
            DS_LOG_INFO("=== Bright Background Color Tests ===");
            DS_LOG_INFO("{}", DS_CONSOLE_BG_BRIGHT_BLACK "Bright Black background" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_BG_BRIGHT_RED "Bright Red background" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_BG_BRIGHT_GREEN "Bright Green background" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_BG_BRIGHT_YELLOW "Bright Yellow background" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_BG_BRIGHT_BLUE "Bright Blue background" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_BG_BRIGHT_MAGENTA "Bright Magenta background" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_BG_BRIGHT_CYAN "Bright Cyan background" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_BG_BRIGHT_WHITE "Bright White background" DS_CONSOLE_TEXT_RESET);

            // Give console time to render
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            return true;
        });

        // Test combining styles and colors
        DS_TEST(logger_tests, "Combined Formatting")
        {
            // Reset to default theme
            ds::core::Logger::Apply_Theme(ds::core::Logger_Theme::DEFAULT);

            DS_LOG_INFO("=== Combined Format Tests ===");

            // Test combinations of styles and colors
            DS_LOG_INFO("{}", DS_CONSOLE_TEXT_BOLD DS_CONSOLE_FG_RED "Bold Red Text" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_TEXT_ITALIC DS_CONSOLE_FG_BLUE "Italic Blue Text" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_TEXT_UNDERLINE DS_CONSOLE_FG_GREEN "Underlined Green Text" DS_CONSOLE_TEXT_RESET);

            // More complex combinations with both foreground and background
            DS_LOG_INFO("{}", DS_CONSOLE_TEXT_BOLD DS_CONSOLE_FG_CYAN DS_CONSOLE_BG_YELLOW 
                            "Bold Cyan on Yellow" DS_CONSOLE_TEXT_RESET);

            DS_LOG_INFO("{}", DS_CONSOLE_TEXT_REVERSE DS_CONSOLE_FG_WHITE DS_CONSOLE_BG_BLUE 
                            "Reversed White on Blue" DS_CONSOLE_TEXT_RESET);

            DS_LOG_INFO("{}", DS_CONSOLE_TEXT_BOLD DS_CONSOLE_TEXT_UNDERLINE DS_CONSOLE_FG_MAGENTA 
                            "Bold Underlined Magenta" DS_CONSOLE_TEXT_RESET);

            // Complex combinations
            DS_LOG_INFO("{}", DS_CONSOLE_TEXT_BOLD DS_CONSOLE_TEXT_ITALIC DS_CONSOLE_FG_BRIGHT_RED 
                            DS_CONSOLE_BG_BLUE "Bold Italic Bright Red on Dark Blue" DS_CONSOLE_TEXT_RESET);

            // Give console time to render
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            return true;
        });

        // Test text alignment and length
        DS_TEST(logger_tests, "Text Formatting with Different Lengths")
        {
            // Reset to default theme
            ds::core::Logger::Apply_Theme(ds::core::Logger_Theme::DEFAULT);

            DS_LOG_INFO("=== Text Length Tests ===");

            // Test short and long messages with formatting
            DS_LOG_INFO("{}", DS_CONSOLE_FG_RED DS_CONSOLE_TEXT_BOLD "Short" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_FG_GREEN DS_CONSOLE_TEXT_UNDERLINE "Medium length text" DS_CONSOLE_TEXT_RESET);
            DS_LOG_INFO("{}", DS_CONSOLE_FG_YELLOW DS_CONSOLE_TEXT_ITALIC
                "This is a much longer message that will wrap in the console to test how formatting works with line wrapping" 
                DS_CONSOLE_TEXT_RESET);

            // Test mixed formatting in text
            DS_LOG_INFO("Normal text with {} and {} and {}", 
                DS_CONSOLE_TEXT_BOLD DS_CONSOLE_FG_RED "bold red" DS_CONSOLE_TEXT_RESET,
                DS_CONSOLE_TEXT_ITALIC DS_CONSOLE_FG_BLUE "italic blue" DS_CONSOLE_TEXT_RESET,
                DS_CONSOLE_TEXT_UNDERLINE DS_CONSOLE_FG_GREEN "underlined green" DS_CONSOLE_TEXT_RESET);

            // Give console time to render
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            return true;
        });

        // Add test with DS_STYLED macro once implemented
        DS_TEST(logger_tests, "DS_STYLED Macro")
        {
            // Reset to default theme
            ds::core::Logger::Apply_Theme(ds::core::Logger_Theme::DEFAULT);

            DS_LOG_INFO("=== DS_STYLED Macro Tests ===");

            // Basic styling
            DS_LOG_INFO("{}", DS_STYLED(DS_CONSOLE_FG_RED, "This text is red"));

            // Multiple style attributes
            DS_LOG_INFO("{}", DS_STYLED(DS_CONSOLE_TEXT_BOLD DS_CONSOLE_FG_BLUE, "This text is bold and blue"));

            // Multi-variant helper for clarity
            #define DS_STYLED_MULTI(style1, style2, text) style1 style2 text DS_CONSOLE_TEXT_RESET
            
            DS_LOG_INFO("{}", DS_STYLED_MULTI(DS_CONSOLE_TEXT_BOLD, DS_CONSOLE_FG_GREEN, "Bold green using STYLED_MULTI"));

            // Complex styling
            DS_LOG_INFO("{}", DS_STYLED(DS_CONSOLE_TEXT_BOLD DS_CONSOLE_TEXT_UNDERLINE DS_CONSOLE_FG_BRIGHT_CYAN DS_CONSOLE_BG_RED,
                "Bold underlined bright cyan text on red background"));

            // Partial styling in a log message
            DS_LOG_INFO("This is normal text with {} and {} words",
                DS_STYLED(DS_CONSOLE_FG_RED, "red"),
                DS_STYLED(DS_CONSOLE_FG_GREEN, "green"));

            // Nested styling test
            std::string nested = std::string(DS_STYLED(DS_CONSOLE_FG_BLUE, "Blue text with ")) +
                std::string(DS_STYLED(DS_CONSOLE_TEXT_BOLD DS_CONSOLE_FG_RED, "bold red")) +
                std::string(DS_STYLED(DS_CONSOLE_FG_BLUE, " nested inside"));
            DS_LOG_INFO("{}", nested);

            // Give console time to render
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            return true;
        });

        // Run all tests
        return logger_tests.Run_All();
    });
}