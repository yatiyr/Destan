#pragma once
#include <core/destan_pch.h>
#include <core/logger/logger.h>
#include <functional>
#include <string>
#include <vector>

namespace destan::test
{
    // Initialize testing environment
    inline void Initialize_Test_Environment()
    {
        // Initialize the logger
        destan::core::Logger::Get_Instance().Start();
        destan::core::Logger::Get_Instance().Set_Synchronous_Mode(true);
        destan::core::Logger::Get_Instance().Set_File_Output_Mode(false);
    }

    // Shutdown testing environment
    inline void Shutdown_Test_Environment()
    {
        // Shutdown the logger
        destan::core::Logger::Get_Instance().Stop();
    }

    class Test_Case
    {
    public:
        using Test_Function = std::function<bool()>;

        Test_Case(const std::string& name, Test_Function func)
            : m_name(name), m_test_func(func) {
        }

        bool Run() const
        {
            bool result = m_test_func();
            if (result)
            {
                DESTAN_LOG_INFO("TEST PASSED: {0}", m_name);
            }
            else
            {
                DESTAN_LOG_ERROR("TEST FAILED: {0}", m_name);
            }
            return result;
        }

        const std::string& Get_Name() const { return m_name; }

    private:
        std::string m_name;
        Test_Function m_test_func;
    };

    class Test_Suite
    {
    public:
        explicit Test_Suite(const std::string& name) : m_name(name) {}

        void Add_Test(const std::string& name, Test_Case::Test_Function func)
        {
            m_tests.emplace_back(name, func);
        }

        bool Run_All()
        {
            // Make sure the testing environment is initialized
            Initialize_Test_Environment();

            DESTAN_LOG_INFO("========== Running Test Suite: {0} ==========", m_name);

            size_t passed = 0;
            size_t total = m_tests.size();

            for (const auto& test : m_tests)
            {
                if (test.Run())
                {
                    passed++;
                }
            }

            DESTAN_LOG_INFO("========== Results: {0}/{1} tests passed ==========", passed, total);

            // Allow time for logs to be processed before returning
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            return (passed == total);
        }

    private:
        std::string m_name;
        std::vector<Test_Case> m_tests;
    };

    // Test runner utility
    class Test_Runner
    {
    public:
        static int Run_Tests(std::function<bool()> test_func)
        {

            bool success = false;

            try
            {
                // Run the tests
                success = test_func();
            }
            catch (const std::exception& e)
            {
                DESTAN_LOG_ERROR("Exception caught during tests: {0}", e.what());
                success = false;
            }
            catch (...)
            {
                DESTAN_LOG_ERROR("Unknown exception caught during tests");
                success = false;
            }

            // Allow logger to finish writing
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Shutdown test environment
            Shutdown_Test_Environment();

            return success ? 0 : 1;
        }
    };
}

// Convenient macros for test creation
#define DESTAN_TEST(suite, name) \
    suite.Add_Test(name, []() -> bool

#define DESTAN_EXPECT(condition) \
    if (!(condition)) { \
        DESTAN_LOG_ERROR("  Assertion failed: " #condition); \
        return false; \
    }

#define DESTAN_EXPECT_EQ(a, b) \
    if (!((a) == (b))) { \
        DESTAN_LOG_ERROR("  Assertion failed: " #a " == " #b " (values: {0} != {1})", a, b); \
        return false; \
    }

#define DESTAN_EXPECT_NE(a, b) \
    if (!((a) != (b))) { \
        DESTAN_LOG_ERROR("  Assertion failed: " #a " != " #b " (values: {0} == {1})", a, b); \
        return false; \
    }

#define DESTAN_EXPECT_LT(a, b) \
    if (!((a) < (b))) { \
        DESTAN_LOG_ERROR("  Assertion failed: " #a " < " #b " (values: {0} >= {1})", a, b); \
        return false; \
    }

#define DESTAN_EXPECT_LE(a, b) \
    if (!((a) <= (b))) { \
        DESTAN_LOG_ERROR("  Assertion failed: " #a " <= " #b " (values: {0} > {1})", a, b); \
        return false; \
    }

#define DESTAN_EXPECT_GT(a, b) \
    if (!((a) > (b))) { \
        DESTAN_LOG_ERROR("  Assertion failed: " #a " > " #b " (values: {0} <= {1})", a, b); \
        return false; \
    }

#define DESTAN_EXPECT_GE(a, b) \
    if (!((a) >= (b))) { \
        DESTAN_LOG_ERROR("  Assertion failed: " #a " >= " #b " (values: {0} < {1})", a, b); \
        return false; \
    }

#define DESTAN_EXPECT_FLOAT_EQ(a, b) \
    if (std::abs((a) - (b)) > std::numeric_limits<float>::epsilon()) { \
        DESTAN_LOG_ERROR("  Assertion failed: " #a " == " #b " (values: {0} != {1}, diff: {2}, epsilon: {3})", \
            (a), (b), std::abs((a) - (b)), std::numeric_limits<float>::epsilon()); \
        return false; \
    }

#define DESTAN_EXPECT_DOUBLE_EQ(a, b) \
    if (std::abs((a) - (b)) > std::numeric_limits<double>::epsilon()) { \
        DESTAN_LOG_ERROR("  Assertion failed: " #a " == " #b " (values: {0} != {1}, diff: {2}, epsilon: {3})", \
            (a), (b), std::abs((a) - (b)), std::numeric_limits<double>::epsilon()); \
        return false; \
    }

#define DESTAN_EXPECT_NEAR(a, b, epsilon) \
    if (std::abs((a) - (b)) > (epsilon)) { \
        DESTAN_LOG_ERROR("  Assertion failed: |" #a " - " #b "| <= " #epsilon " (values: {0} and {1}, diff: {2}, epsilon: {3})", \
            (a), (b), std::abs((a) - (b)), (epsilon)); \
        return false; \
    }

#define DESTAN_EXPECT_THROW(expression, exception_type) \
    try { \
        expression; \
        DESTAN_LOG_ERROR("  Assertion failed: Expected exception " #exception_type " was not thrown"); \
        return false; \
    } catch (const exception_type&) { \
        /* Expected exception - test passes */ \
    } catch (...) { \
        DESTAN_LOG_ERROR("  Assertion failed: Wrong exception type caught"); \
        return false; \
    }