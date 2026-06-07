#include "pch.h"
#include "CppUnitTest.h"
#include "RegExTree.h"
#include "NFA.h"
#include "MatchLogic.h"
#include "TestHelpers.h"
#include <vector>
#include <string>
#include <unordered_set>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MatchingWithRegExTests
{
    TEST_CLASS(NFAIntegrationTests)
    {
    public:
        struct IntegrationTestCase {
            int id;
            std::string testName;
            std::string regexOPZ;
            std::string input;
            std::string expectedResult; 
        };

        TEST_METHOD(Test_ExtractAllMatches_FullSuite)
        {
            // [текст] - полное совпадение
            // {текст} - частичное совпадение
            // текст без скобок - несовпавший мусор
            std::vector<IntegrationTestCase> tests = {
                // --- Базовый поиск и маски ---
                {1, "FullOnly", "a", "a", "[a]"},
                {2, "GarbageOnly", "a", "b", "b"},
                {3, "MidMatch", "a", "xax", "x[a]x"},
                {4, "MultiMatch", "a", "xaaax", "x[a][a][a]x"},
                {5, "PrefixGarbage", "c a t &3", "the cat", "the [cat]"},

                // --- Изолированные частичные совпадения ---
                {6, "PartialEOF", "c a t &3", "ca", "{ca}"},
                {7, "GarbageThenPartial", "c a t &3", "xca", "x{ca}"},
                {8, "WildcardBreak", "c . t &3", "car", "{ca}r"},
                {9, "DoublePartial", "a b c &3", "abxab", "{ab}xab"},

                // --- Влияние квантификаторов ---
                {10, "MaxQuant", "a {1,2}", "aaa", "[aa][a]"},
                {11, "StarGreedyScan", "a *", "baaab", "b[aaa]b"},
                {12, "CatchAllMatch", ". *", "xyz", "[xyz]"},
                {13, "MultiBranchScan", "a b & c d & |", "xabxcdx", "x[ab]x[cd]x"},

                // --- Защита от зацикливаний ---
                {14, "AntiLoopQuestion", "a ?", "b", "b"},
                {15, "EmptyTemplate", "a {0}", "xyz", "xyz"},
                {16, "EmptyInputProtect", "a", "", ""},

                // --- Комбинированные совпадения ---
                {17, "GlobalAltMatch", "c a t s &4 b a t s &4 |", "Do cats eat bats?", "Do [cats] eat [bats]?"},
                {18, "GlobalGroupMatch", "c b | a t s &3 &", "Do bats eat cats?", "Do [bats] eat [cats]?"},

                // --- Фильтрация совпадений ---
                {19, "IgnorePartialIfFull", "c a t &3", "ca then cat", "ca then [cat]"},
                {20, "BestGlobalPartial", "c a t &3", "c and ca", "c and {ca}"},

                // --- Смежные совпадения ---
                {21, "ConsecutiveMatch", "a b &", "ababab", "[ab][ab][ab]"}
            };

            std::vector<std::string> allErrors;

            for (const auto& test : tests) {
                // 1. Сборка NFA
                std::unordered_set<Error> errs;
                auto tree = buildRegExTreeFromPostfixNotation(test.regexOPZ, errs);

                auto startState = std::make_shared<NFAState>();
                auto endState = std::make_shared<NFAState>();
                endState->isFinal = true;

                if (tree) {
                    tree->buildNFA(startState, endState);
                }

                calculateDistanceToTerminal(endState);

                std::vector<std::string> distanceErrors;
                TestHelpers::assertAllReachableNodesHaveDistance(startState.get(), distanceErrors);
                if (!distanceErrors.empty()) {
                    allErrors.push_back("Тест " + std::to_string(test.id) + ": ошибки дистанций в автомате");
                    continue;
                }

                NFA nfa = { startState, endState, {} };

                // 2. Выполнение поиска
                std::vector<Match> matches = extractAllMatches(nfa, test.input);

                // 3. Формирование простой строки результата 
                std::string actualResult = "";
                size_t currentPos = 0;

                for (const auto& match : matches) {
                    // Добавляем обычный текст (мусор), который был до совпадения
                    if (match.start > currentPos) {
                        actualResult += test.input.substr(currentPos, match.start - currentPos);
                    }

                    // Добавляем само совпадение, обернутое в скобки
                    std::string matchText = test.input.substr(match.start, match.end - match.start);
                    if (match.isFullMatch) {
                        actualResult += "[" + matchText + "]"; // Полное
                    }
                    else {
                        actualResult += "{" + matchText + "}"; // Частичное
                    }

                    currentPos = match.end;
                }

                // Добавляем хвост строки, если после последнего совпадения остался текст
                if (currentPos < test.input.length()) {
                    actualResult += test.input.substr(currentPos);
                }

                // 4. Проверка
                if (test.expectedResult != actualResult) {
                    allErrors.push_back("Ошибка в тесте " + std::to_string(test.id) + " (" + test.testName
                        + "): ожидалось [" + test.expectedResult + "], получено [" + actualResult + "]");
                }
            }

            if (!allErrors.empty()) {
                Assert::Fail(TestHelpers::formatErrors(allErrors).c_str());
            }
        }
    };
}