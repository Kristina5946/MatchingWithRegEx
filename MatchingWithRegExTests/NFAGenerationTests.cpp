/*!
 * \file NFAGenerationTests.cpp
 * \brief Модульные тесты для проверки генерации НКА и алгоритма epsilon-замыкания.
 */

#include "pch.h"
#include "CppUnitTest.h"
#include "RegExTree.h"
#include "NFA.h"
#include "MatchLogic.h"
#include "TestHelpers.h"
#include "ClosureGraphSetup.h"
#include <memory>
#include <vector>
#include <string>
#include <unordered_set>
#include <utility>
#include <queue>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MatchingWithRegExTests
{
    inline std::wstring ToWStr(const std::string& str) {
        return std::wstring(str.begin(), str.end());
    }

    /*!
     * \brief Эталонные НКА для табл. 2.1.
     * возвращают готовый объект NFA.
     */
    namespace NFAExpected {

        static void addEps(const std::shared_ptr<NFAState>& from, const std::shared_ptr<NFAState>& to) {
            from->outgoingTransitions.push_back(std::make_shared<EpsTrans>(to, from));
        }

        static void addChar(const std::shared_ptr<NFAState>& from, const std::shared_ptr<NFAState>& to, char c) {
            std::vector<std::pair<char, char>> r = { {c, c} };
            from->outgoingTransitions.push_back(std::make_shared<CharTrans>(to, from, r));
        }

        static void addAny(const std::shared_ptr<NFAState>& from, const std::shared_ptr<NFAState>& to) {
            std::vector<std::pair<char, char>> r = { {1, 127} };
            from->outgoingTransitions.push_back(std::make_shared<CharTrans>(to, from, r));
        }

        static NFA makeChar(char c) {
            auto s = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addChar(s, e, c);
            return { s, e, {} };
        }

        NFA Char(char c) { return makeChar(c); }

        NFA Any() {
            auto s = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addAny(s, e);
            return { s, e, {} };
        }

        NFA Concat(char a, char b) {
            auto s = std::make_shared<NFAState>();
            auto m = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addChar(s, m, a);
            addChar(m, e, b);
            return { s, e, {} };
        }

        NFA Concat4(char a, char b, char c, char d) {
            auto s = std::make_shared<NFAState>();
            auto n1 = std::make_shared<NFAState>();
            auto n2 = std::make_shared<NFAState>();
            auto n3 = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addChar(s, n1, a);
            addChar(n1, n2, b);
            addChar(n2, n3, c);
            addChar(n3, e, d);
            return { s, e, {} };
        }

        NFA ConcatCharAny(char a) {
            auto s = std::make_shared<NFAState>();
            auto m = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addChar(s, m, a);
            addAny(m, e);
            return { s, e, {} };
        }

        NFA Alt2(char a, char b) {
            auto s = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addChar(s, e, a);
            addChar(s, e, b);
            return { s, e, {} };
        }

        NFA Alt4(char a, char b, char c, char d) {
            auto s = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addChar(s, e, a);
            addChar(s, e, b);
            addChar(s, e, c);
            addChar(s, e, d);
            return { s, e, {} };
        }

        /*! ОПЗ: a a | — две одинаковые ветки сливаются в одно ребро a */
        NFA AltSame(char a) {
            return makeChar(a);
        }

        /*! ОПЗ: a b | c | — вложенная альтернатива из трёх символов. */
        NFA AltNested3() {
            auto s = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addChar(s, e, 'a');
            addChar(s, e, 'b');
            addChar(s, e, 'c');
            return { s, e, {} };
        }

        /*! ОПЗ: a b & c |   */
        NFA AltConcat_ab_c() {
            auto s = std::make_shared<NFAState>();
            auto m = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addChar(s, m, 'a');
            addChar(m, e, 'b');
            addChar(s, e, 'c');
            return { s, e, {} };
        }

        /*! ОПЗ: a b c | &    */
        NFA Concat_a_Alt_bc() {
            auto s = std::make_shared<NFAState>();
            auto m = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addChar(s, m, 'a');
            addChar(m, e, 'b');
            addChar(m, e, 'c');
            return { s, e, {} };
        }

        /*! ОПЗ: a ?     */
        NFA Quant01(char a) {
            auto s = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addChar(s, e, a);
            addEps(s, e);
            return { s, e, {} };
        }

        /*! ОПЗ: a +      */
        NFA Quant1Inf(char a) {
            auto s = std::make_shared<NFAState>();
            auto n1 = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addChar(s, n1, a);
            addChar(n1, n1, a);
            addEps(n1, e);
            return { s, e, {} };
        }

        /*! ОПЗ: a *     */
        NFA Quant0Inf(char a) {
            auto s = std::make_shared<NFAState>();
            auto n1 = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addEps(s, e);
            addChar(s, n1, a);
            addChar(n1, n1, a);
            addEps(n1, e);
            return { s, e, {} };
        }

        /*! ОПЗ: a {3} — три символа подряд. */
        NFA QuantExact3(char a) {
            auto s = std::make_shared<NFAState>();
            auto n1 = std::make_shared<NFAState>();
            auto n2 = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addChar(s, n1, a);
            addChar(n1, n2, a);
            addChar(n2, e, a);
            return { s, e, {} };
        }

        /*! ОПЗ: a {2,4}. */
        NFA Quant24(char a) {
            auto s = std::make_shared<NFAState>();
            auto n1 = std::make_shared<NFAState>();
            auto n2 = std::make_shared<NFAState>();
            auto n3 = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addChar(s, n1, a);
            addChar(n1, n2, a);
            addEps(n2, e);
            addChar(n2, n3, a);
            addEps(n3, e);
            addChar(n3, e, a);
            return { s, e, {} };
        }

        /*! ОПЗ: a {2,}. */
        NFA Quant2Open(char a) {
            auto s = std::make_shared<NFAState>();
            auto n1 = std::make_shared<NFAState>();
            auto n2 = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addChar(s, n1, a);
            addChar(n1, n2, a);
            addChar(n2, n2, a);
            addEps(n2, e);
            return { s, e, {} };
        }

        /*! ОПЗ: a {,3}. */
        NFA Quant0to3(char a) {
            auto s = std::make_shared<NFAState>();
            auto n1 = std::make_shared<NFAState>();
            auto n2 = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addEps(s, e);
            addChar(s, n1, a);
            addEps(n1, e);
            addChar(n1, n2, a);
            addEps(n2, e);
            addChar(n2, e, a);
            return { s, e, {} };
        }

        /*! ОПЗ: a {0} — только epsilon S -> E. */
        NFA Quant00() {
            auto s = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addEps(s, e);
            return { s, e, {} };
        }

        /*! ОПЗ: a b & * — (ab)*. */
        NFA Star_ab() {
            auto s = std::make_shared<NFAState>();
            auto n1 = std::make_shared<NFAState>();
            auto n2 = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addEps(s, e);
            addChar(s, n1, 'a');
            addChar(n1, n2, 'b');
            addChar(n2, n1, 'a');
            addEps(n2, e);
            return { s, e, {} };
        }

        /*! ОПЗ: a b | +      */
        NFA Plus_Alt_ab() {
            auto s = std::make_shared<NFAState>();
            auto n1 = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addChar(s, n1, 'a');
            addChar(s, n1, 'b');
            addChar(n1, n1, 'a');
            addChar(n1, n1, 'b');
            addEps(n1, e);
            return { s, e, {} };
        }

        /*! ОПЗ: a b & + a &           */
        NFA AbPlus_a() {
            auto s = std::make_shared<NFAState>();
            auto n1 = std::make_shared<NFAState>();
            auto n2 = std::make_shared<NFAState>();
            auto n3 = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addChar(s, n1, 'a');
            addChar(n1, n2, 'b');
            addChar(n2, n1, 'a');
            addEps(n2, n3);
            addChar(n3, e, 'a');
            return { s, e, {} };
        }

        /*! ОПЗ: a b & + a b a b &4 &        */
        NFA AbPlus_abab() {
            auto s = std::make_shared<NFAState>();
            auto n1 = std::make_shared<NFAState>();
            auto n2 = std::make_shared<NFAState>();
            auto n3 = std::make_shared<NFAState>();
            auto n4 = std::make_shared<NFAState>();
            auto n5 = std::make_shared<NFAState>();
            auto n6 = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addChar(s, n1, 'a');
            addChar(n1, n2, 'b');
            addChar(n2, n1, 'a');
            addEps(n2, n3);
            addChar(n3, n4, 'a');
            addChar(n4, n5, 'b');
            addChar(n5, n6, 'a');
            addChar(n6, e, 'b');
            return { s, e, {} };
        }

        /*! ОПЗ: a b c &3 ? a b c &3 + |           */
        NFA Alt_abcOpt_abcPlus() {
            auto s = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            auto n1 = std::make_shared<NFAState>();
            auto n2 = std::make_shared<NFAState>();
            addEps(s, e);
            addChar(s, n1, 'a');
            addChar(n1, n2, 'b');
            addChar(n2, e, 'c');
            auto t3 = std::make_shared<NFAState>();
            auto t4 = std::make_shared<NFAState>();
            auto t5 = std::make_shared<NFAState>();
            addChar(s, t3, 'a');
            addChar(t3, t4, 'b');
            addChar(t4, t5, 'c');
            addEps(t5, t3);
            addEps(t5, e);
            return { s, e, {} };
        }

        /*! ОПЗ: . *            */
        NFA StarAny() {
            auto s = std::make_shared<NFAState>();
            auto n1 = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addEps(s, e);
            addAny(s, n1);
            addAny(n1, n1);
            addEps(n1, e);
            return { s, e, {} };
        }

        /*! ОПЗ: . * a &             */
        NFA ConcatDotStar_a() {
            auto s = std::make_shared<NFAState>();
            auto n1 = std::make_shared<NFAState>();
            auto n2 = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addEps(s, n1);
            addAny(n1, n1);
            addEps(n1, n2);
            addChar(n2, e, 'a');
            return { s, e, {} };
        }

        /*! ОПЗ: . {2,5}           */
        NFA QuantAny_2_5() {
            auto s = std::make_shared<NFAState>();
            auto n1 = std::make_shared<NFAState>();
            auto n2 = std::make_shared<NFAState>();
            auto n3 = std::make_shared<NFAState>();
            auto n4 = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addAny(s, n1);
            addAny(n1, n2);
            addEps(n2, e);
            addAny(n2, n3);
            addEps(n3, e);
            addAny(n3, n4);
            addEps(n4, e);
            addAny(n4, e);
            return { s, e, {} };
        }

        /*! ОПЗ: a b {0} |              */
        NFA Alt_a_emptyB() {
            auto s = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addChar(s, e, 'a');
            addEps(s, e);
            return { s, e, {} };
        }

        /*! ОПЗ: a b | c d | | — четыре параллельных символа. */
        NFA Alt_ab_cd_nested() {
            return Alt4('a', 'b', 'c', 'd');
        }

        /*! ОПЗ: a b c d | & | e &                    */
        NFA Concat_AltComplex_e() {
            auto s = std::make_shared<NFAState>();
            auto afterAlt = std::make_shared<NFAState>();
            auto n1 = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addChar(s, afterAlt, 'a');
            addChar(s, n1, 'b');
            addChar(n1, afterAlt, 'c');
            addChar(n1, afterAlt, 'd');
            addChar(afterAlt, e, 'e');
            return { s, e, {} };
        }

        /*! ОПЗ: a + * — вложенные квантификаторы */
        NFA Nest_plus_star() {
            auto s = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            auto n1 = std::make_shared<NFAState>();
            auto n2 = std::make_shared<NFAState>();
            auto n3 = std::make_shared<NFAState>();
            auto n4 = std::make_shared<NFAState>();
            addEps(s, e);
            addEps(s, n1);
            addEps(n1, n2);
            addChar(n2, n3, 'a');
            addEps(n3, n2);
            addEps(n3, n4);
            addEps(n4, n1);
            addEps(n4, e);
            return { s, e, {} };
        }

        /*! ОПЗ: a {2,3} {1,2} — вложенные интервальные  */
        NFA Nest_intervals() {
            auto s = std::make_shared<NFAState>();
            auto n1 = std::make_shared<NFAState>();
            auto n2 = std::make_shared<NFAState>();
            auto n3 = std::make_shared<NFAState>();
            auto n4 = std::make_shared<NFAState>();
            auto n5 = std::make_shared<NFAState>();
            auto n6 = std::make_shared<NFAState>();
            auto n7 = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addChar(s, n1, 'a');
            addChar(n1, n2, 'a');
            addEps(n2, n3);
            addChar(n2, n4, 'a');
            addEps(n4, n3);
            addEps(n3, e);
            addChar(n3, n5, 'a');
            addChar(n5, n6, 'a');
            addEps(n6, e);
            addChar(n6, n7, 'a');
            addEps(n7, e);
            return { s, e, {} };
        }

        /*! ОПЗ: a b & {10} — десять повторений «ab». */
        NFA Ab_repeat10() {
            auto s = std::make_shared<NFAState>();
            std::shared_ptr<NFAState> cur = s;
            for (int i = 0; i < 10; ++i) {
                auto na = std::make_shared<NFAState>();
                auto nb = std::make_shared<NFAState>();
                addChar(cur, na, 'a');
                addChar(na, nb, 'b');
                cur = nb;
            }
            cur->isFinal = true;
            return { s, cur, {} };
        }

        /*! ОПЗ: a b | {2,3}                  */
        NFA Alt_ab_quant23() {
            auto s = std::make_shared<NFAState>();
            auto n1 = std::make_shared<NFAState>();
            auto n2 = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addChar(s, n1, 'a');
            addChar(s, n1, 'b');
            addChar(n1, n2, 'a');
            addChar(n1, n2, 'b');
            addEps(n2, e);
            addChar(n2, e, 'a');
            addChar(n2, e, 'b');
            return { s, e, {} };
        }

        NFA EmptyAltEpsilon() {
            auto s = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addEps(s, e);
            return { s, e, {} };
        }

        NFA EmptyConcatEpsilon() {
            return EmptyAltEpsilon();
        }

        /*! ОПЗ: a {0} b {0} & — два epsilon подряд. */
        NFA DoubleEpsilon_ab0() {
            auto s = std::make_shared<NFAState>();
            auto n1 = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addEps(s, n1);
            addEps(n1, e);
            return { s, e, {} };
        }

        /*! ОПЗ: a {0} * — внешняя звезда над «пустым» подавтоматом. */
        NFA NestedEmptyStar() {
            auto s = std::make_shared<NFAState>();
            auto n1 = std::make_shared<NFAState>();
            auto e = std::make_shared<NFAState>();
            e->isFinal = true;
            addEps(s, e);
            addEps(s, n1);
            addEps(n1, n1);
            addEps(n1, e);
            return { s, e, {} };
        }

        /*! Пустышка для строк таблицы с expectException == true. */
        NFA InvalidPlaceholder() {
            return { nullptr, nullptr, {} };
        }

    } // namespace NFAExpected

    struct BuildNFATest {
        int id;
        std::string testName;
        std::string inputOPZ;
        NFA expectedNFA;
        bool expectException;
    };

    struct ClosureTest {
        int id;
        std::string testName;
        std::vector<int> expectedStateIndices;
    };

    TEST_CLASS(NFAGenerationTests)
    {
    public:

        TEST_METHOD(Test_BuildNFA_Logic)
        {
            const NFA emptyFail = NFAExpected::InvalidPlaceholder();

            std::vector<BuildNFATest> tests = {
                { 1, "Обычный видимый символ", "a", NFAExpected::Char('a'), false },
                { 2, "Спецсимвол переноса строки", "\\n", NFAExpected::Char('\n'), false },
                { 3, "Спецсимвол табуляции", "\\t", NFAExpected::Char('\t'), false },
                { 4, "Масочный символ любой", ".", NFAExpected::Any(), false },
                { 5, "Конкатенация 2 узлов", "a b &", NFAExpected::Concat('a', 'b'), false },
                { 6, "Вложенность N-арных узлов", "a b c &3 d &", NFAExpected::Concat4('a', 'b', 'c', 'd'), false },
                { 7, "Конкатенация символа и маски", "a . &", NFAExpected::ConcatCharAny('a'), false },
                { 8, "Альтернатива 2 узлов", "a b |", NFAExpected::Alt2('a', 'b'), false },
                { 9, "Альтернатива 4 узлов", "a b c d |4", NFAExpected::Alt4('a', 'b', 'c', 'd'), false },
                { 10, "Одинаковые ветки", "a a |", NFAExpected::AltSame('a'), false },
                { 11, "Вложенная альтернатива", "a b | c |", NFAExpected::AltNested3(), false },
                { 12, "Конкатенация внутри альтернативы", "a b & c |", NFAExpected::AltConcat_ab_c(), false },
                { 13, "Альтернатива внутри конкатенации", "a b c | &", NFAExpected::Concat_a_Alt_bc(), false },
                { 14, "Ноль или одно", "a ?", NFAExpected::Quant01('a'), false },
                { 15, "Одно или более", "a +", NFAExpected::Quant1Inf('a'), false },
                { 16, "Ноль или более", "a *", NFAExpected::Quant0Inf('a'), false },
                { 17, "Точное количество", "a {3}", NFAExpected::QuantExact3('a'), false },
                { 18, "Диапазон", "a {2,4}", NFAExpected::Quant24('a'), false },
                { 19, "Диапазон без верха", "a {2,}", NFAExpected::Quant2Open('a'), false },
                { 20, "Диапазон до максимума", "a {,3}", NFAExpected::Quant0to3('a'), false },
                { 21, "Удаление", "a {0}", NFAExpected::Quant00(), false },
                { 22, "Цикл внутри скобок", "a b & *", NFAExpected::Star_ab(), false },
                { 23, "Плюс альтернативы", "a b | +", NFAExpected::Plus_Alt_ab(), false },
                { 24, "Сложная конкатенация с циклом", "a b & + a &", NFAExpected::AbPlus_a(), false },
                { 25, "Длинная развернутая цепочка", "a b & + a b a b &4 &", NFAExpected::AbPlus_abab(), false },
                { 26, "Квантификаторы внутри альтернативы", "a b c &3 ? a b c &3 + |", NFAExpected::Alt_abcOpt_abcPlus(), false },
                { 27, "Любые символы", ". *", NFAExpected::StarAny(), false },
                { 28, "Поиск с конца", ". * a &", NFAExpected::ConcatDotStar_a(), false },
                { 29, "Квантификатор над маской", ". {2,5}", NFAExpected::QuantAny_2_5(), false },
                { 30, "Альтернатива с пустой веткой", "a b {0} |", NFAExpected::Alt_a_emptyB(), false },
                { 31, "Глубокое ветвление", "a b | c d | |", NFAExpected::Alt_ab_cd_nested(), false },
                { 32, "Сложное вложенное разветвление", "a b c d | & | e &", NFAExpected::Concat_AltComplex_e(), false },
                { 33, "Вложенный квантификатор базовый", "a + *", NFAExpected::Nest_plus_star(), false },
                { 34, "Вложенный квантификатор интервальный", "a {2,3} {1,2}", NFAExpected::Nest_intervals(), false },
                { 35, "Длинный интервал от строки", "a b & {10}", NFAExpected::Ab_repeat10(), false },
                { 36, "Альтернатива плюс интервал", "a b | {2,3}", NFAExpected::Alt_ab_quant23(), false },
                { 37, "Лимит точного интервала", "a {10001}", emptyFail, true },
                { 38, "Лимит диапазона", "a {1,10000}", emptyFail, true },
                { 39, "Узел без потомков (Alt)", "", NFAExpected::EmptyAltEpsilon(), false },
                { 40, "Узел без потомков (Concat)", "", NFAExpected::EmptyConcatEpsilon(), false },
                { 41, "Неверные параметры минимума", "a {5,2}", emptyFail, true },
                { 42, "Отрицательные параметры", "a {-1,5}", emptyFail, true },
                { 43, "Экранирование спецсимвола", "\\*", NFAExpected::Char('*'), false },
                { 44, "Двойной эпсилон", "a {0} b {0} &", NFAExpected::DoubleEpsilon_ab0(), false },
                { 45, "Пустой эпсилон-цикл", "a {0} *", NFAExpected::NestedEmptyStar(), false }
            };

            for (const auto& test : tests) {
                std::unordered_set<Error> errs;
                auto tree = buildRegExTreeFromPostfixNotation(test.inputOPZ, errs);

                if (test.expectException) {
                    Assert::IsTrue(errs.size() > 0 || tree == nullptr,
                        (L"Тест " + std::to_wstring(test.id) + L": Ожидалась ошибка лимита памяти или неверных параметров!").c_str());
                    continue;
                }

                Assert::IsNotNull(tree.get(),
                    (L"Тест " + std::to_wstring(test.id) + L": Дерево не построилось (вернулся nullptr)").c_str());

                auto startState = std::make_shared<NFAState>();
                auto endState = std::make_shared<NFAState>();
                endState->isFinal = true;

                tree->buildNFA(startState, endState);

                NFA actualNFA{ startState, endState, {} };

                std::vector<std::string> equivalenceErrors;
                TestHelpers::areNFAsEquivalent(test.expectedNFA, actualNFA, equivalenceErrors);
                if (!equivalenceErrors.empty()) {
                    std::wstring msg = L"Тест " + std::to_wstring(test.id) + L" [" + ToWStr(test.testName) + L"]: Автоматы не эквивалентны!\n";
                    for (const std::string& err : equivalenceErrors) {
                        msg += ToWStr(err) + L"\n";
                    }
                    Assert::Fail(msg.c_str());
                }

                // ? Чтобы доказать, что построенный buildNFA граф связный и позволяет алгоритму дистанций дойти от конца до начала...
                calculateDistanceToTerminal(endState);

                std::vector<std::string> distanceErrors;
                TestHelpers::assertAllReachableNodesHaveDistance(startState.get(), distanceErrors);
                if (!distanceErrors.empty()) {
                    std::wstring distMsg = L"Тест " + std::to_wstring(test.id) + L" [" + ToWStr(test.testName) + L"]: ";
                    for (const std::string& err : distanceErrors) {
                        distMsg += ToWStr(err) + L"\n";
                    }
                    Assert::Fail(distMsg.c_str());
                }
            }
        }

        TEST_METHOD(Test_EpsilonClosure_GraphTopology)
        {
            std::vector<std::pair<char, char>> dummyChar;
            dummyChar.push_back(std::make_pair('a', 'a'));
            std::vector<std::pair<char, char>> dummyCharB;
            dummyCharB.push_back(std::make_pair('b', 'b'));

            std::vector<ClosureTest> tests = {
                { 1, "Изолированное состояние", {0} },
                { 2, "Один символьный переход", {0} },
                { 3, "Один epsilon-переход", {0, 1} },
                { 4, "Цепочка переходов 5 узлов", {0, 1, 2, 3, 4} },
                { 5, "Цепочка прерванная символом", {0, 1} },
                { 6, "Запуск после прерывания", {2, 3} },
                { 7, "Простое ветвление", {0, 1, 2} },
                { 8, "Глубокое бинарное ветвление", {0, 1, 2, 3, 4} },
                { 9, "Слияние путей", {0, 1, 2, 3} },
                { 10, "Петля на себя", {0} },
                { 11, "Цикл из двух узлов", {0, 1} },
                { 12, "Длинный цикл кольцо", {0, 1, 2, 3} },
                { 13, "Цикл с выходом лассо", {0, 1, 2} },
                { 14, "Пересекающиеся циклы восьмерка", {0, 1, 2} },
                { 15, "Два независимых компонента", {0, 1, 3, 4} },
                { 16, "Стартовые внутри цикла", {0, 1, 2} },
                { 17, "Вся цепочка на входе", {0, 1, 2} },
                { 18, "Пустое входное множество", {} },
                { 19, "Переход вперед и возврат", {0, 1, 3} },
                { 20, "Оценка замыкания из центра цикла", {1, 2, 3} },
                { 21, "Сложное ветвление", {0, 1, 3} },
                { 22, "Замыкание в конце ветвления", {2, 4, 5} },
                { 23, "Полный граф каждый с каждым", {0, 1, 2, 3} },
                { 24, "Огромная линейная цепь тест стека", {} }, //ниже заполнила
                { 25, "Тупиковые ветви", {0, 1, 2} }
            };
            // Динамическое заполнение 
            for (int i = 0; i < 10000; ++i) {
                tests[23].expectedStateIndices.push_back(i);
            }

            for (const ClosureTest& test : tests) {
                std::vector<std::shared_ptr<NFAState>> allStates;
                std::unordered_set<ActiveState> activeSet;

                // 1. вход: Строим граф (скрыто в фабрике в файле ClosureGraphSetup.сpp)
                ClosureGraphSetup::buildGraph(test.id, dummyChar, dummyCharB, allStates, activeSet);

                // 2. Вычисляем эпсилон-замыкание
                epsilonClosure(activeSet);

                // 3. Извлекаем фактические индексы и берем ожидаемые из таблицы
                std::vector<int> actualIndices = TestHelpers::collectStateIndicesFromActiveSet(allStates, activeSet);
                const std::vector<int>& expectedIndices = test.expectedStateIndices;

                // 4. Двустороннее сравнение множеств (проверка на лишние/недостающие)
                std::vector<std::string> setErrors;
                std::string context = "epsilon-closure test " + std::to_string(test.id);
                TestHelpers::compareSetsTwoWay(expectedIndices, actualIndices, setErrors, context);

                // Если ошибки есть — тест падает и выводит все рассинхроны разом
                if (!setErrors.empty()) {
                    std::wstring failMsg = L"Тест " + std::to_wstring(test.id) + L" (" + std::wstring(test.testName.begin(), test.testName.end()) + L"):\n";
                    for (const std::string& err : setErrors) {
                        failMsg += std::wstring(err.begin(), err.end()) + L"\n";
                    }
                    Assert::Fail(failMsg.c_str());
                }
            }
        }
    };
}

