/*!
 * \file MatchLogicTests.cpp
 * \brief Модульные тесты для проверки логики поиска совпадений.
 */

#include "pch.h"
#include "CppUnitTest.h"
#include "RegExTree.h"
#include "NFA.h"
#include "MatchLogic.h"
#include "TestHelpers.h"
#include <vector>
#include <string>
#include <unordered_set>
#include <memory>
#include <set>
#include <queue>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MatchingWithRegExTests
{
    // 1. Перечисление типов совпадений 
    enum MatchType {
        NO_MATCH,
        PARTIAL,
        FULL
    };

    // 3. Структура для тестов логики (решает проблему с инициализацией test в таблице 3.4)
    struct MatchLogicTestCase {
        int id;
        std::string testName;
        std::string regexOPZ;
        std::string inputString;
        size_t startPos;
        MatchType expectedMatchType;
        size_t expectedMatchLength;
    };

    Match makeMatch(bool isValid, bool isFull, size_t dist, size_t len, size_t pos) {
        Match m;
        m.isValid = isValid;
        m.isFullMatch = isFull;
        m.distanceToTerminal = dist;
        m.start = pos;
        m.end = pos + len;
        return m;
    }

    inline std::wstring VisualizeMatchForLog(const std::string& input, size_t start, size_t len, MatchType type) {
        std::wstring wInput = TestHelpers::ToWStr(input);
        std::wstring res = L"Вход: [" + wInput + L"], Позиция: " + std::to_wstring(start) + L", Длина: " + std::to_wstring(len) + L" -> ";
        if (type == FULL) res += L"FULL";
        else if (type == PARTIAL) res += L"PARTIAL";
        else res += L"NO_MATCH";
        return res;
    }
    // фабричная функция для настройки графа 
    static void setupSimulateGraph(int testId, std::vector<std::shared_ptr<NFAState>>& s) {
        std::vector<std::pair<char, char>> anyChar = { {0, 127} };

        switch (testId) {
        case 1:
        case 2:
        case 5:
            s[0]->outgoingTransitions.push_back(std::make_shared<CharTrans>(s[1], s[0], std::vector<std::pair<char, char>>{{'a', 'a'}}));
            if (testId == 5) {
                s[1]->outgoingTransitions.push_back(std::make_shared<EpsTrans>(s[2], s[1]));
            }
            break;
        case 3:
        case 4:
            s[0]->outgoingTransitions.push_back(std::make_shared<CharTrans>(s[1], s[0], anyChar));
            break;
        case 6:
            s[0]->outgoingTransitions.push_back(std::make_shared<CharTrans>(s[2], s[0], std::vector<std::pair<char, char>>{{'a', 'a'}}));
            s[1]->outgoingTransitions.push_back(std::make_shared<CharTrans>(s[3], s[1], std::vector<std::pair<char, char>>{{'b', 'b'}}));
            break;
        case 7:
            s[0]->outgoingTransitions.push_back(std::make_shared<CharTrans>(s[1], s[0], std::vector<std::pair<char, char>>{{'a', 'a'}}));
            s[0]->outgoingTransitions.push_back(std::make_shared<CharTrans>(s[2], s[0], std::vector<std::pair<char, char>>{{'a', 'a'}}));
            break;
        case 8:
            s[0]->outgoingTransitions.push_back(std::make_shared<CharTrans>(s[2], s[0], std::vector<std::pair<char, char>>{{'b', 'b'}}));
            s[1]->outgoingTransitions.push_back(std::make_shared<CharTrans>(s[2], s[1], std::vector<std::pair<char, char>>{{'b', 'b'}}));
            break;
        }
    }
    TEST_CLASS(MatchLogicTests)
    {
    public:
        // ТАБЛИЦА 3.1: Расчет дистанции до терминала 
        TEST_METHOD(Test_CalculateDistanceToTerminal_AllCases)
        {
            struct DistTest {
                int id;
                std::string opz;
                std::vector<int> expectedDistances; 
                std::string description;
                bool isManualGraph;
            };

            // Данные  (Таблица 3.1)
            std::vector<DistTest> tests = {
                {1, "a", {0, 1}, "Базовый расчет (один переход)", false},
                {2, "a b c &3", {0, 1, 2, 3}, "Линейное увеличение (abc)", false},
                {3, "a b |", {0, 1}, "Одинаковые ветки (a|b)", false},
                {4, "a b c &3 d |", {0, 1, 2}, "Приоритет кратчайшего пути (abc|d)", false},
                {5, "a *", {0, 1}, "Расстояние из цикла", false},
                {6, "", {-1, 0}, "Искусственно разорванный граф", true}, // -1 обозначает недостижимость
                {7, "a b | c &", {0, 1, 2}, "Сложная развилка и слияние ((a|b)c)", false},
                {8, "b c | a & d | e &", {0, 1, 2}, "Сильная вложенность (((a(b|c))|d)e)", false},
                {9, "a * b & *", {0, 1, 2}, "Цикл внутри цикла ((a*b)*)", false},
            };

            std::vector<std::string> allErrors;

            for (const auto& test : tests) {
                std::shared_ptr<NFAState> startState = std::make_shared<NFAState>();
                std::shared_ptr<NFAState> endState = std::make_shared<NFAState>();
                endState->isFinal = true;

                if (test.isManualGraph) {
                    std::shared_ptr<NFAState> isolated = std::make_shared<NFAState>();
                    // Имитируем разорванный граф: start -> isolated (тупик), end недостижим
                    startState->outgoingTransitions.push_back(std::make_shared<EpsTrans>(isolated, startState));
                }
                else {
                    std::unordered_set<Error> errs;
                    auto tree = buildRegExTreeFromPostfixNotation(test.opz, errs);
                    if (tree.get() == nullptr) {
                        allErrors.push_back("Тест 3.1." + std::to_string(test.id) + ": Синтаксическое дерево не должно быть null");
                        continue;
                    }
                    tree->buildNFA(startState, endState);
                }

                // 1. Запускаем тестируемую функцию расчета
                calculateDistanceToTerminal(endState);

                // 2.Собираю фактические дистанции в графе (BFS-обход)
                std::set<int> uniqueActualDists; 
                std::unordered_set<NFAState*> visited;
                std::queue<NFAState*> q;

                q.push(startState.get());
                visited.insert(startState.get());

                // Для разорванного графа гарантируем, что мы посмотрим и на endState, даже если он недостижим от startState
                if (visited.find(endState.get()) == visited.end()) {
                    q.push(endState.get());
                    visited.insert(endState.get());
                }
                //Пока в очереди есть узлы, достаем первый узел и удаляем его из очереди.
                while (!q.empty()) {
                    NFAState* curr = q.front();
                    q.pop();

                    // Конвертируем size_t(-1) в int(-1) для удобства проверки
                    int dist = (curr->distanceToTerminal == static_cast<size_t>(-1)) ? -1 : static_cast<int>(curr->distanceToTerminal);
                    uniqueActualDists.insert(dist);
                    //Обход соседей
                    for (const auto& trans : curr->outgoingTransitions) {
                        NFAState* target = trans->getTarget().get();
                        if (visited.find(target) == visited.end()) {
                            visited.insert(target);
                            q.push(target);
                        }
                    }
                }

                // Перегоняем собранный set в vector для  функции-хелпера
                std::vector<int> actualDistances(uniqueActualDists.begin(), uniqueActualDists.end());

                // 3. двусторонняя проверка множеств
                std::vector<std::string> testErrors;
                std::string context = "Тест 3.1." + std::to_string(test.id) + " [" + test.description + "]";

                // нет ли лишних дистанций или не потерялись ли нужные
                TestHelpers::compareSetsTwoWay(test.expectedDistances, actualDistances, testErrors, context);
                TestHelpers::appendErrors(allErrors, testErrors);
            }

            if (!allErrors.empty()) {
                Assert::Fail(TestHelpers::formatErrors(allErrors).c_str());
            }
        }

        // ТАБЛИЦА 3.2: Тестирование шага симуляции автомата SimulateNFA
        TEST_METHOD(Test_SimulateNFA_StepByStep)
        {
            struct SimTest {
                int id;
                char inputChar;
                std::vector<int> initialActiveIndices;
                std::vector<int> expectedActiveIndices;
                std::string description;
            };

            std::vector<SimTest> tests = {
                {1, 'a', {0}, {1}, "Успешный переход по 'a'"},
                {2, 'b', {0}, {}, "Пустое множество, ветка отмерла"},
                {3, 'x', {0}, {1}, "Маска принимает любой символ"},
                {4, '\n', {0}, {}, "Маска игнорирует перенос строки (эмуляция)"},
                {5, 'a', {0}, {1, 2}, "Автоматическое ε-замыкание после шага"},
                {6, 'a', {0, 1}, {2}, "Из нескольких состояний выжило только одно"},
                {7, 'a', {0}, {1, 2}, "Развилка по одному символу"},
                {8, 'b', {0, 1}, {2}, "Слияние: два разных в одно"}
            };

            std::vector<std::string> allErrors;

            for (const auto& test : tests) {
                std::vector<std::shared_ptr<NFAState>> states;
                for (int i = 0; i < 5; ++i) states.push_back(std::make_shared<NFAState>());

                // Использование фабрики графов
                setupSimulateGraph(test.id, states);

                std::unordered_set<std::shared_ptr<NFAState>> activeStates;
                for (int idx : test.initialActiveIndices) {
                    activeStates.insert(states[idx]);
                }

                // Вызов тестируемой функции
                auto resultStates = SimulateNFA(activeStates, test.inputChar);

                // Трансформируем указатели в индексы
                std::vector<int> actualIndices;
                for (const auto& resState : resultStates) {
                    auto it = std::find(states.begin(), states.end(), resState);
                    if (it != states.end()) {
                        actualIndices.push_back(static_cast<int>(std::distance(states.begin(), it)));
                    }
                    else {
                        actualIndices.push_back(-1); // Отлов узлов, не принадлежащих графу
                    }
                }

                // Контейнер для сбора логов
                std::vector<std::string> errors;
                std::string context = "Тест 3.2." + std::to_string(test.id);

                // Двустороннюю проверку множеств 
                TestHelpers::compareSetsTwoWay(test.expectedActiveIndices, actualIndices, errors, context);
                TestHelpers::appendErrors(allErrors, errors);
            }

            if (!allErrors.empty()) {
                Assert::Fail(TestHelpers::formatErrors(allErrors).c_str());
            }
        }

        // ТАБЛИЦА 3.3: Тестирование функции determineFinalMatch
        TEST_METHOD(Test_DetermineFinalMatch_Priorities)
        {
            struct DetTest {
                int id;
                Match fullM;
                Match partM;
                bool expectFull;
                bool expectValid;
                size_t expectLen;
                std::string desc;
            };

            // Таблица 3.3 : вход fullMatch / bestPartialMatch, ожидаемый тип и длина
            std::vector<DetTest> tests = {
                {1, makeMatch(true, true, 0, 3, 0), makeMatch(false, false, 0, 0, 0), true, true, 3, "Однозначный успех"},
                {2, makeMatch(true, true, 0, 4, 0), makeMatch(true, false, 2, 2, 0), true, true, 4, "Приоритет полного над частичным"},
                {3, makeMatch(false, false, 0, 0, 0), makeMatch(true, false, 1, 3, 0), false, true, 3, "Оборвалось в графе (PARTIAL)"},
                {4, makeMatch(false, false, 0, 0, 0), makeMatch(false, false, 0, 0, 0), false, false, 0, "NO_MATCH"},
                {5, makeMatch(true, true, 0, 2, 0), makeMatch(true, false, 1, 10, 0), true, true, 2, "Полное короткое побеждает длинное частичное"},
                };

            // Тесты 6–7 по документу: проверка выбора лучшего частичного
            Match partialA6 = makeMatch(true, false, 2, 3, 0); // len = 3
            Match partialB6 = makeMatch(true, false, 2, 5, 0); // len = 5
            Assert::IsTrue(isBetterPartialMatch(partialA6, partialB6), L"Тест 3.3.6: должно победить более длинное частичное");

            Match partialA7 = makeMatch(true, false, 2, 5, 2); // start = 2
            Match partialB7 = makeMatch(true, false, 2, 5, 0); // start = 0
            Assert::IsTrue(isBetterPartialMatch(partialA7, partialB7), L"Тест 3.3.7: должно победить совпадение с меньшей start");

            std::vector<std::string> allErrors;

            for (const auto& test : tests) {
                Match result = determineFinalMatch(test.fullM, test.partM);
                TestHelpers::validateDetermineFinalMatch(test.id, test.expectValid, test.expectFull,
                    test.expectLen, test.fullM, test.partM, result, allErrors);
            }

            if (!allErrors.empty()) {
                Assert::Fail(TestHelpers::formatErrors(allErrors).c_str());
            }
        }

        // ТАБЛИЦА 3.4: Комплексное тестирование ядра findMatchAtPosition
        TEST_METHOD(Test_FindMatchAtPosition_Comprehensive)
        {
            std::vector<MatchLogicTestCase> tests = {
                // --- Базовый поиск и маски ---
                {1, "ExactMatch", "c a t &3", "cat", 0, FULL, 3},
                {2, "NoMatch", "c a t &3", "dog", 0, NO_MATCH, 0},
                {3, "PrefixMatch", "c a t &3", "caterpillar", 0, FULL, 3},
                {4, "OffsetMatch", "c a t &3", "tomcat", 3, FULL, 3},
                {5, "EarlyEOF", "c a t &3", "ca", 0, PARTIAL, 2},
                {6, "WildcardOK", "c . t &3", "cut", 0, FULL, 3},
                {7, "WildcardNL", "c . t &3", "c\nt", 0, PARTIAL, 1},
                {8, "BreakBeforeDot", "c . t &3", "c", 0, PARTIAL, 1},
                {9, "BreakAtDot", "c . t &3", "ca", 0, PARTIAL, 2},

                // --- Квантификаторы (*, +, ?) ---
                {10, "GreedyStar", "a *", "aaa", 0, FULL, 3},
                {11, "EmptyStar", "a *", "b", 0, FULL, 0},
                {12, "GreedyPlus", "a +", "aaa", 0, FULL, 3},
                {13, "RequiredPlus", "a +", "b", 0, NO_MATCH, 0},
                {14, "QuestionOnce", "a ?", "aab", 0, FULL, 1},
                {15, "QuestionZero", "a ?", "b", 0, FULL, 0},
                {16, "QuantSeq", "a + b &", "aaab", 0, FULL, 4},
                {17, "QuantPartial", "a + b &", "aaa", 0, PARTIAL, 3},
                {18, "CatchAll", ". * b &", "xyzxyzb", 0, FULL, 7},
                {181, "GreedyCatchAll", ". * b &", "xyzbxyzb", 0, FULL, 8},
                {19, "NoDelimiter", ". * b &", "xyz", 0, PARTIAL, 3},

                // --- Интервалы {n,m} ---
                {20, "ExactN", "a {3}", "aaaa", 0, FULL, 3},
                {21, "UnderN", "a {3}", "aa", 0, PARTIAL, 2},
                {22, "RangeMax", "a {2,4}", "aaaaa", 0, FULL, 4},
                {23, "RangeMin", "a {2,4}", "aa", 0, FULL, 2},
                {24, "RangeUnder", "a {2,4}", "a", 0, PARTIAL, 1},
                {25, "RangeOpen", "a {2,}", "aaaaa", 0, FULL, 5},
                {26, "ZeroRange", "a {0,2}", "b", 0, FULL, 0},

                // --- Ветвления и приоритеты ---
                {27, "AltMatch", "c a t &3 c a r &3 |", "car", 0, FULL, 3},
                {28, "AltPartial", "c a t &3 c a r &3 |", "ca", 0, PARTIAL, 2},
                {29, "FullPriority", "a b c &3 a |", "ab", 0, FULL, 1},
                {30, "N-aryAlt", "a b c |3", "c", 0, FULL, 1},
                {36, "LongestMatch", "a a a & |", "aa", 0, FULL, 2},
                {37, "Leftmost Longest", "a a b & |", "ab", 0, FULL, 2},
                {38, "Greedy Priority", "a * a {2,4} |", "aaaaa", 0, FULL, 5},
                {39, "Mask Priority", "a a . * & |", "ab", 0, FULL, 2},
                {40, "NFA Resolution", "a b | + a &", "aba", 0, FULL, 3},

                // --- Борьба за лучшее частичное ---
                {41, "DistPriority", "a b c d &4 a x y &3 |", "a", 0, PARTIAL, 1},
                {42, "FixedPriority", "a . * b &3 a c & |", "a", 0, PARTIAL, 1},

                // --- Граничные условия строки ---
                {43, "EmptyStart", "a", "", 0, PARTIAL, 0},
                {44, "EmptyFull", "a ?", "", 0, FULL, 0},
                {45, "OutOfBounds", "c a t &3", "cat\n", 3, PARTIAL, 0}
            };

            std::vector<std::string> allErrors;

            for (const auto& test : tests) {
                std::unordered_set<Error> errs;
                auto tree = buildRegExTreeFromPostfixNotation(test.regexOPZ, errs);
                if (!errs.empty()) {
                    allErrors.push_back("Тест 3.4." + std::to_string(test.id) + ": Ошибка парсинга ОПЗ");
                    continue;
                }
                if (tree.get() == nullptr) {
                    allErrors.push_back("Тест 3.4." + std::to_string(test.id) + ": Дерево пустое");
                    continue;
                }

                auto startState = std::make_shared<NFAState>();
                auto endState = std::make_shared<NFAState>();
                endState->isFinal = true;

                tree->buildNFA(startState, endState);
                calculateDistanceToTerminal(endState);

                std::vector<std::string> distanceErrors;
                TestHelpers::assertAllReachableNodesHaveDistance(startState.get(), distanceErrors);
                if (!distanceErrors.empty()) {
                    allErrors.push_back("Тест 3.4." + std::to_string(test.id) + ": невалидные расстояния в автомате");
                    continue;
                }

                NFA nfa = { startState, endState, {} };
                Match actualMatch = findMatchAtPosition(nfa, test.inputString, test.startPos);

                bool expectValid = (test.expectedMatchType != NO_MATCH);
                bool expectFull = (test.expectedMatchType == FULL);
                TestHelpers::validateMatchResult(test.id, test.inputString, test.startPos,
                    expectValid, expectFull, test.expectedMatchLength, actualMatch, allErrors);

                size_t actualLenLog = actualMatch.isValid ? (actualMatch.end - actualMatch.start) : 0;
                MatchType actualTypeLog = !actualMatch.isValid ? NO_MATCH : (actualMatch.isFullMatch ? FULL : PARTIAL);

                std::wstring msg = L"Тест 3.4." + std::to_wstring(test.id);
                std::wstring vis = VisualizeMatchForLog(test.inputString, test.startPos, actualLenLog, actualTypeLog);
                std::wstring wTestName(test.testName.begin(), test.testName.end());
                Logger::WriteMessage((msg + L" [" + wTestName + L"] -> " + vis + L"\n").c_str());
            }

            if (!allErrors.empty()) {
                Assert::Fail(TestHelpers::formatErrors(allErrors).c_str());
            }
        }
    };
}