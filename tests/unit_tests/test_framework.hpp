#pragma once
// Mini framework de test simple pour les tests unitaires Gomoku
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace test_framework {

// Compteurs globaux de tests
static int tests_passed = 0;
static int tests_failed = 0;

// Registre des tests
static std::vector<std::pair<std::string, std::function<void()>>>* test_registry = nullptr;

inline std::vector<std::pair<std::string, std::function<void()>>>& get_test_registry()
{
    if (!test_registry) {
        test_registry = new std::vector<std::pair<std::string, std::function<void()>>>();
    }
    return *test_registry;
}

// Macros pour définir les tests
#define TEST(name)                                                                 \
    void test_##name();                                                            \
    static struct TestRegistrar_##name {                                           \
        TestRegistrar_##name()                                                     \
        {                                                                          \
            test_framework::get_test_registry().push_back({ #name, test_##name }); \
        }                                                                          \
    } registrar_##name;                                                            \
    void test_##name()

// Macros d'assertion
#define ASSERT_TRUE(expr)                                                            \
    do {                                                                             \
        if (!(expr)) {                                                               \
            std::cerr << "  x FAILED: " << #expr << " (line " << __LINE__ << ")\n"; \
            test_framework::tests_failed++;                                          \
            return;                                                                  \
        }                                                                            \
    } while (0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))
#define ASSERT_EQ(a, b) ASSERT_TRUE((a) == (b))
#define ASSERT_NE(a, b) ASSERT_TRUE((a) != (b))

#define TEST_PASSED()                                   \
    do {                                                \
        test_framework::tests_passed++;                 \
        std::cout << "  ✓ " << __func__ << " passed\n"; \
    } while (0)

// Fonction utilitaire pour afficher un résumé
inline void print_summary(const std::string& test_suite_name)
{
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════\n";
    std::cout << "  Test Results: " << test_suite_name << "\n";
    std::cout << "═══════════════════════════════════════════════\n";
    std::cout << "  Total passed: " << tests_passed << "\n";
    std::cout << "  Total failed: " << tests_failed << "\n";
    std::cout << "═══════════════════════════════════════════════\n";

    if (tests_failed == 0) {
        std::cout << "  All tests passed!\n";
    } else {
        std::cout << "  Some tests failed.\n";
    }
    std::cout << "\n";
}

// Fonction pour exécuter tous les tests enregistrés
inline void run_all_tests(const std::string& test_suite_name)
{
    std::cout << "Running " << test_suite_name << " Tests...\n";

    for (const auto& [name, test_func] : get_test_registry()) {
        test_func();
    }

    print_summary(test_suite_name);
}

} // namespace test_framework
