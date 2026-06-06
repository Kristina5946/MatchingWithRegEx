/*!
 * \file RegExTreeTests.cpp
 * \brief Модульные тесты для проверки лексического и синтаксического анализа (построения AST дерева).
 */

#include "pch.h"
#include "CppUnitTest.h"
#include "RegExTree.h"
#include "Error.h"
#include "TestHelpers.h"
#include <vector>
#include <string>
#include <unordered_set>
#include <memory>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MatchingWithRegExTests
{

    namespace TreeExpected {

        std::shared_ptr<RegExNode> Char1(char c) {
            std::vector<std::pair<char, char>> ranges;
            ranges.push_back(std::make_pair(c, c));
            return std::make_shared<CharNode>(ranges, 0);
        }

        std::shared_ptr<RegExNode> CharAny() {
            std::vector<std::pair<char, char>> ranges;
            ranges.push_back(std::make_pair((char)1, (char)127));
            return std::make_shared<CharNode>(ranges, 0);
        }

        std::shared_ptr<RegExNode> Concat2(std::shared_ptr<RegExNode> a, std::shared_ptr<RegExNode> b) {
            std::vector<std::shared_ptr<RegExNode>> children;
            children.push_back(a);
            children.push_back(b);
            return std::make_shared<ConcatNode>(children, 0);
        }

        std::shared_ptr<RegExNode> Concat3(std::shared_ptr<RegExNode> a, std::shared_ptr<RegExNode> b, std::shared_ptr<RegExNode> c) {
            std::vector<std::shared_ptr<RegExNode>> children;
            children.push_back(a);
            children.push_back(b);
            children.push_back(c);
            return std::make_shared<ConcatNode>(children, 0);
        }

        std::shared_ptr<RegExNode> ConcatN(const std::vector<std::shared_ptr<RegExNode>>& children) {
            return std::make_shared<ConcatNode>(children, 0);
        }

        std::shared_ptr<RegExNode> Alt2(std::shared_ptr<RegExNode> a, std::shared_ptr<RegExNode> b) {
            std::vector<std::shared_ptr<RegExNode>> children;
            children.push_back(a);
            children.push_back(b);
            return std::make_shared<AlternateNode>(children, 0);
        }

        std::shared_ptr<RegExNode> AltN(const std::vector<std::shared_ptr<RegExNode>>& children) {
            return std::make_shared<AlternateNode>(children, 0);
        }

        std::shared_ptr<RegExNode> Quant(int minOccur, int maxOccur, std::shared_ptr<RegExNode> child) {
            return std::make_shared<QuantifierNode>(child, minOccur, maxOccur, 0);
        }

    }

    /*!
     * \brief Вспомогательная функция для наглядной подсветки позиции ошибки в строке.
     */
    std::wstring HighlightError(const std::string& str, size_t pos) {
        std::string res = str;
        if (pos < res.length()) {
            res.insert(pos + 1, " <<<]");
            res.insert(pos, "[>>> ");
        }
        else {
            res += " [>>> ОШИБКА В КОНЦЕ <<<]";
        }
        return std::wstring(res.begin(), res.end());
    }

    /*!
     * \brief Конвертация строки в std::wstring для вывода в Assert.
     */
    inline std::wstring ToWStr(const std::string& str) {
        return std::wstring(str.begin(), str.end());
    }

    /*!
     * \brief Локальная структура для хранения данных тестового случая AST.
     * expectedTree — эталонное дерево в памяти , nullptr если ожидается ошибка.
     */
    struct TreeTest {
        int id;
        std::string testName;
        std::string inputOPZ;
        std::shared_ptr<RegExNode> expectedTree;
        std::vector<Error> expectedErrors;
    };

    TEST_CLASS(RegExTreeTests)
    {
    public:
        TEST_METHOD(Test_BuildRegExTreeFromPostfixNotation_AllCases)
        {
            // --- Массив тестов (Таблица 1): вход ОПЗ и эталонное дерево в одной строке ---
            std::vector<TreeTest> tests = {
                {1, "Пустая строка", "", nullptr, { {Error::emptyTemplate, 0} }},
                {2, "Строка только из пробелов", " ", nullptr, { {Error::emptyTemplate, 0} }},
                {3, "Одиночный символ", "a", TreeExpected::Char1('a'), {}},
                {4, "Одиночная цифра", "5", TreeExpected::Char1('5'), {}},
                {5, "Мусор в стеке", "a b c", nullptr, { {Error::excessiveOperands, 4} }},
                {6, "Символ табуляции", "\t", TreeExpected::Char1('\t'), {}},
                {7, "Символ переноса строки", "\n", TreeExpected::Char1('\n'), {}},
                {8, "Экранирование *", "\*", TreeExpected::Char1('*'), {}},
                {9, "Экранирование +", "\+", TreeExpected::Char1('+'), {}},
                {10, "Экранирование |", "\|", TreeExpected::Char1('|'), {}},
                {11, "Экранирование слеша", "\\", TreeExpected::Char1('\\'), {}},
                {12, "Экранирование скобки", "\{", TreeExpected::Char1('{'), {}},
                {13, "Экранирование &", "\&", TreeExpected::Char1('&'), {}},
                {14, "Одиночная маска", ".", TreeExpected::CharAny(), {}},
                {15, "Экранированная точка", "\.", TreeExpected::Char1('.'), {}},
                {16, "Конкатенация с маской", "a . &", TreeExpected::Concat2(TreeExpected::Char1('a'), TreeExpected::CharAny()), {}},
                {17, "Любое кол-во любых символов", ". *", TreeExpected::Quant(0, -1, TreeExpected::CharAny()), {}},
                {18, "Простая конкатенация", "a b &", TreeExpected::Concat2(TreeExpected::Char1('a'), TreeExpected::Char1('b')), {}},
                {19, "Простая альтернатива", "a b |", TreeExpected::Alt2(TreeExpected::Char1('a'), TreeExpected::Char1('b')), {}},
                {20, "Вложенная левосторонняя &", "a b & c &", TreeExpected::Concat2(TreeExpected::Concat2(TreeExpected::Char1('a'), TreeExpected::Char1('b')), TreeExpected::Char1('c')), {}},
                {21, "Вложенная правосторонняя &", "a b c & &", TreeExpected::Concat2(TreeExpected::Char1('a'), TreeExpected::Concat2(TreeExpected::Char1('b'), TreeExpected::Char1('c'))), {}},
                {22, "Вложенная альтернатива", "a b | c |", TreeExpected::Alt2(TreeExpected::Alt2(TreeExpected::Char1('a'), TreeExpected::Char1('b')), TreeExpected::Char1('c')), {}},
                {23, "Конкатенация 3 элементов", "a b c &3", TreeExpected::Concat3(TreeExpected::Char1('a'), TreeExpected::Char1('b'), TreeExpected::Char1('c')), {}},
                {24, "Альтернатива 4 элементов", "a b c d |4", TreeExpected::AltN(std::vector<std::shared_ptr<RegExNode>>{ TreeExpected::Char1('a'), TreeExpected::Char1('b'), TreeExpected::Char1('c'), TreeExpected::Char1('d') }), {}},
                {25, "Большая арность (10)", "a b c d e f g h i j &10", TreeExpected::ConcatN(std::vector<std::shared_ptr<RegExNode>>{ TreeExpected::Char1('a'), TreeExpected::Char1('b'), TreeExpected::Char1('c'), TreeExpected::Char1('d'), TreeExpected::Char1('e'), TreeExpected::Char1('f'), TreeExpected::Char1('g'), TreeExpected::Char1('h'), TreeExpected::Char1('i'), TreeExpected::Char1('j') }), {}},
                {26, "Вложенность N-арных в бинарные", "a b c &3 d &", TreeExpected::Concat2(TreeExpected::Concat3(TreeExpected::Char1('a'), TreeExpected::Char1('b'), TreeExpected::Char1('c')), TreeExpected::Char1('d')), {}},
                {27, "Вложенность бинарных в N-арные", "a b & c d | e &3", TreeExpected::Concat3(TreeExpected::Concat2(TreeExpected::Char1('a'), TreeExpected::Char1('b')), TreeExpected::Alt2(TreeExpected::Char1('c'), TreeExpected::Char1('d')), TreeExpected::Char1('e')), {}},
                {28, "N-арные операции подряд", "a b c &3 d e f &3 |2", TreeExpected::Alt2(TreeExpected::Concat3(TreeExpected::Char1('a'), TreeExpected::Char1('b'), TreeExpected::Char1('c')), TreeExpected::Concat3(TreeExpected::Char1('d'), TreeExpected::Char1('e'), TreeExpected::Char1('f'))), {}},
                {29, "Ноль или более", "a *", TreeExpected::Quant(0, -1, TreeExpected::Char1('a')), {}},
                {30, "Одно или более", "a +", TreeExpected::Quant(1, -1, TreeExpected::Char1('a')), {}},
                {31, "Ноль или одно", "a ?", TreeExpected::Quant(0, 1, TreeExpected::Char1('a')), {}},
                {32, "Квантификатор над конкатенацией", "a b & *", TreeExpected::Quant(0, -1, TreeExpected::Concat2(TreeExpected::Char1('a'), TreeExpected::Char1('b'))), {}},
                {33, "Квантификатор над альтернативой", "a b | +", TreeExpected::Quant(1, -1, TreeExpected::Alt2(TreeExpected::Char1('a'), TreeExpected::Char1('b'))), {}},
                {34, "Квантификатор над N-арной", "a b c &3 ?", TreeExpected::Quant(0, 1, TreeExpected::Concat3(TreeExpected::Char1('a'), TreeExpected::Char1('b'), TreeExpected::Char1('c'))), {}},
                {35, "Двойной квантификатор", "a * *", TreeExpected::Quant(0, -1, TreeExpected::Quant(0, -1, TreeExpected::Char1('a'))), {}},
                {36, "Точное количество", "a {3}", TreeExpected::Quant(3, 3, TreeExpected::Char1('a')), {}},
                {37, "Точное количество (нуль)", "a {0}", TreeExpected::Quant(0, 0, TreeExpected::Char1('a')), {}},
                {38, "Диапазон", "a {2,5}", TreeExpected::Quant(2, 5, TreeExpected::Char1('a')), {}},
                {39, "Диапазон от нуля", "a {0,5}", TreeExpected::Quant(0, 5, TreeExpected::Char1('a')), {}},
                {40, "Диапазон без верхней", "a {3,}", TreeExpected::Quant(3, -1, TreeExpected::Char1('a')), {}},
                {41, "Диапазон без нижней", "a {,4}", TreeExpected::Quant(0, 4, TreeExpected::Char1('a')), {}},
                {42, "Большие числа", "a {100,200}", TreeExpected::Quant(100, 200, TreeExpected::Char1('a')), {}},
                {43, "Интервал над маской", ". {2,5}", TreeExpected::Quant(2, 5, TreeExpected::CharAny()), {}},
                {44, "Квантификатор над квантификатором", "a {1,2} {3,4}", TreeExpected::Quant(3, 4, TreeExpected::Quant(1, 2, TreeExpected::Char1('a'))), {}},
                {45, "Поиск email", "a . * @ b . c &5", TreeExpected::ConcatN(std::vector<std::shared_ptr<RegExNode>>{ TreeExpected::Char1('a'), TreeExpected::Quant(0, -1, TreeExpected::CharAny()), TreeExpected::Char1('@'), TreeExpected::Char1('b'), TreeExpected::CharAny(), TreeExpected::Char1('c') }), {}},
                {46, "Недостаточно операндов &", "a &", nullptr, { {Error::insufficientOperands, 2} }},
                {47, "Недостаточно операндов |", "a |", nullptr, { {Error::insufficientOperands, 2} }},
                {48, "Недостаточно операндов N-арная", "a b &3", nullptr, { {Error::insufficientOperands, 4} }},
                {49, "Недостаточно операндов *", "*", nullptr, { {Error::insufficientOperands, 0} }},
                {50, "Некорректная арность (min 2)", "a &1", nullptr, { {Error::invalidArity, 2} }},
                {51, "Некорректная арность (0)", "a &0", nullptr, { {Error::invalidArity, 2} }},
                {52, "Некорректный интервал (min>max)", "a {5,2}", nullptr, { {Error::invalidInterval, 2} }},
                {53, "Мусор в интервале (-)", "a {-1,2}", nullptr, { {Error::unrecognizedSequence, 2} }},
                {54, "Мусор в интервале (.)", "a {1.5, 2}", nullptr, { {Error::unrecognizedSequence, 2} }},
                {55, "Незакрытая скобка", "a {2,5", nullptr, { {Error::unrecognizedSequence, 2} }},
                {56, "Пустой escape (конец строки)", "a \\", nullptr, { {Error::invalidEscapeSequence, 2} }},
                {57, "Несуществующий escape", "\\k", nullptr, { {Error::invalidEscapeSequence, 0} }},
            };

            for (const TreeTest& test : tests)
            {
                std::unordered_set<Error> actualErrors;

                // Вызов тестируемой функции парсинга ОПЗ
                auto actualTree = buildRegExTreeFromPostfixNotation(test.inputOPZ, actualErrors);

                // 1. Проверка количества зафиксированных ошибок парсинга
                std::wstring countMsg = L"Тест " + std::to_wstring(test.id) + L" [" + ToWStr(test.testName) + L"]: Неверное количество синтаксических ошибок.";
                Assert::AreEqual(test.expectedErrors.size(), actualErrors.size(), countMsg.c_str());

                // 2. Двусторонняя проверка множества ошибок (нет лишних и нет недостающих)
                std::vector<std::string> errorSetErrors;
                std::string errorContext = "Тест " + std::to_string(test.id) + " [" + test.testName + "]";
                TestHelpers::compareErrorSetsTwoWay(test.expectedErrors, actualErrors, errorSetErrors, errorContext);
                if (!errorSetErrors.empty()) {
                    std::wstring errMsg = L"Тест " + std::to_wstring(test.id) + L" [" + ToWStr(test.testName) + L"]: Ошибки не совпадают двусторонне.\n";
                    for (const std::string& err : errorSetErrors) {
                        errMsg += ToWStr(err) + L"\n";
                    }
                    Assert::Fail(errMsg.c_str());
                }

                // 3. Сравнение по объектам в памяти 
                if (test.expectedTree == nullptr) {
                    Assert::IsNull(actualTree.get(), (L"Тест " + std::to_wstring(test.id) + L": Дерево должно быть nullptr (ожидалась ошибка синтаксиса)").c_str());
                }
                else {
                    Assert::IsNotNull(actualTree.get(), (L"Тест " + std::to_wstring(test.id) + L": Дерево не построилось (ожидался успех)").c_str());

                    std::vector<std::string> structureErrors;
                    TestHelpers::compareExpressionTrees(test.expectedTree.get(), actualTree.get(), structureErrors, "root");

                    if (!structureErrors.empty()) {
                        std::wstring structMsg = L"Тест " + std::to_wstring(test.id) + L" [" + ToWStr(test.testName) + L"]: Структура дерева не совпадает с ожидаемой.\nОшибки:\n";
                        for (const std::string& err : structureErrors) {
                            structMsg += ToWStr(err) + L"\n";
                        }
                        Assert::Fail(structMsg.c_str());
                    }
                }
            }
        };
    };
}

