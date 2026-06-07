/*!
 * \file TestHelpers.h
 * \brief Вспомогательные функции сравнения AST и графов НКА в модульных тестах.
 */

#pragma once
#include "RegExTree.h"
#include "NFA.h"
#include "MatchLogic.h"
#include "Error.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_set>

namespace TestHelpers {

    /*!
     * \brief Преобразует UTF-8 строку в std::wstring для вывода в Assert.
     * \param str Исходная строка.
     * \return Строка в формате wide.
     */
    inline std::wstring ToWStr(const std::string& str) {
        if (str.empty()) {
            return L"";
        }
        int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        if (len <= 0) {
            return std::wstring(str.begin(), str.end());
        }
        std::wstring result(static_cast<size_t>(len - 1), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], len);
        return result;
    }

    /*!
     * \brief Склеивает список сообщений об ошибках в одну строку для Assert::Fail.
     * \param errors Список текстов ошибок.
     * \return Объединённое сообщение с переводами строк.
     */
    inline std::wstring formatErrors(const std::vector<std::string>& errors) {
        std::wstring msg;
        size_t i = 0;
        while (i < errors.size()) {
            msg += ToWStr(errors[i]);
            msg += L"\n";
            i++;
        }
        return msg;
    }

    /*!
     * \brief Добавляет новые ошибки в общий список теста.
     * \param allErrors Накопленный список ошибок.
     * \param newErrors Ошибки текущего шага.
     */
    void appendErrors(std::vector<std::string>& allErrors, const std::vector<std::string>& newErrors);

    /*!
     * \brief Рекурсивное сравнение ожидаемого и фактического AST по типам и узлам.
     * \param expected Эталонное дерево.
     * \param actual Фактическое дерево.
     * \param errors Список для записи найденных расхождений.
     * \param path Путь в дереве (для сообщения об ошибке).
     */
    void compareExpressionTrees(const RegExNode* expected, const RegExNode* actual, std::vector<std::string>& errors, std::string path = "root");

    /*!
     * \brief Сравнение двух графов НКА (BFS + epsilon-closure) для проверки эквивалентности поведения автоматов.
     */
    void areNFAsEquivalent(const NFA& expected, const NFA& actual, std::vector<std::string>& errors);

    /*!
     * \brief Двустороннее сравнение числовых множеств (для поиска недостающих и лишних элементов).
     */
    void compareSetsTwoWay(const std::vector<int>& expected, const std::vector<int>& actual, std::vector<std::string>& errors, const std::string& context);

    /*!
     * \brief Двустороннее сравнение множеств ошибок синтаксического анализа.
     */
    void compareErrorSetsTwoWay(const std::vector<Error>& expected, const std::unordered_set<Error>& actual, std::vector<std::string>& errors, const std::string& context);

    /*!
     * \brief Извлекает индексы состояний из allStates для проверки содержимого activeSet после вызова epsilonClosure.
     */
    std::vector<int> collectStateIndicesFromActiveSet(const std::vector<std::shared_ptr<NFAState>>& allStates, const std::unordered_set<ActiveState>& activeSet);

    /*!
     * \brief Проверяет, что после calculateDistanceToTerminal у всех достижимых узлов заполнено поле distanceToTerminal.
     */
    void assertAllReachableNodesHaveDistance(NFAState* startState, std::vector<std::string>& errors);

    /*!
     * \brief Табл. 3.4: Сверяет итог совпадения, длину и позицию start (игнорируя внутренние индексы).
     */
    void validateMatchResult(int testId, const std::string& inputString, size_t queryStartPos,
        bool expectValid, bool expectFullMatch, size_t expectedLength,
        const Match& actualMatch, std::vector<std::string>& errors);

    /*!
     * \brief Проверяет корректность выбора финального совпадения между полным и частичным.
     */
    void validateDetermineFinalMatch(int testId, bool expectValid, bool expectFull, size_t expectLen,
        const Match& expectedFull, const Match& expectedPartial,
        const Match& result, std::vector<std::string>& errors);

    /*!
     * \brief Считает число состояний НКА, достижимых из начального.
     * \param startState Начальное состояние автомата.
     * \return Количество достижимых состояний.
     */
    size_t countReachableNFAStates(const std::shared_ptr<NFAState>& startState);

    /*!
     * \brief Проверяет, превысит ли дерево лимит 10 000 состояний при построении НКА.
     * \param node Корень синтаксического дерева.
     * \return true, если лимит будет превышен.
     */
    bool regexTreeExceedsNfaLimit(const RegExNode* node);

} // namespace TestHelpers