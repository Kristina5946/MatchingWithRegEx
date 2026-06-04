/*!
 * \file RegExTree.h
 * \brief Заголовочный файл, описывающий классы узлов дерева регулярного выражения и функции построения.
 */

#pragma once

#include "NFA.h"
#include "Error.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_set>
#include <utility>
#include <cstddef>

/*!
 * \brief Базовый абстрактный класс узла дерева регулярного выражения.
 */
class RegExNode {
public:
    size_t startPosInStr; /*!< Позиция начала узла в исходной строке с выражением */

    /*!
     * \brief Виртуальный деструктор по умолчанию.
     */
    virtual ~RegExNode() = default;

    /*!
     * \brief Строит фрагмент недетерминированного конечного автомата (НКА) для данного узла.
     * \param start Указатель на начальное состояние, соответствующее началу фрагмента.
     * \param end Указатель на конечное состояние, соответствующее концу фрагмента.
     */
    virtual void buildNFA(std::shared_ptr<NFAState> start, std::shared_ptr<NFAState> end) = 0;
};

/*!
 * \brief Класс узла для одиночного символа и символьной маски (литерал).
 */
class CharNode : public RegExNode {
public:
    std::vector<std::pair<char, char>> charRanges; /*!< Набор допустимых диапазонов символов таблицы ASCII */

    /*!
     * \brief Конструктор узла символа.
     * \param ranges Вектор пар символов, задающий допустимые диапазоны.
     * \param pos Позиция токена в исходной строке.
     */
    CharNode(const std::vector<std::pair<char, char>>& ranges, size_t pos)
        : charRanges(ranges) {
        startPosInStr = pos;
    }

    /*!
     * \brief Реализует построение НКА для символьного узла.
     * \param start Указатель на начальное состояние фрагмента.
     * \param end Указатель на конечное состояние фрагмента.
     */
    void buildNFA(std::shared_ptr<NFAState> start, std::shared_ptr<NFAState> end) override;
};

/*!
 * \brief Класс узла конкатенации (последовательное соединение операндов).
 */
class ConcatNode : public RegExNode {
public:
    std::vector<std::shared_ptr<RegExNode>> children; /*!< Список последовательно соединяемых операндов */
    int operandArity; /*!< Число операндов операции (по умолчанию бинарная; иначе равно children.size()) */

    /*!
     * \brief Конструктор узла конкатенации.
     * \param _children Вектор указателей на дочерние узлы конкатенации.
     * \param pos Позиция оператора в исходной строке.
     * \param pendingArity Заданная арность операции (-1 если не указана).
     */
    ConcatNode(const std::vector<std::shared_ptr<RegExNode>>& _children, size_t pos, int pendingArity = -1)
        : children(_children),
        operandArity(pendingArity >= 0 ? pendingArity : static_cast<int>(_children.size())) {
        startPosInStr = pos;
    }

    /*!
     * \brief Реализует построение НКА для последовательного соединения фрагментов.
     * \param start Указатель на начальное состояние фрагмента.
     * \param end Указатель на конечное состояние фрагмента.
     */
    void buildNFA(std::shared_ptr<NFAState> start, std::shared_ptr<NFAState> end) override;
};

/*!
 * \brief Класс узла альтернативы (выбор из нескольких операндов).
 */
class AlternateNode : public RegExNode {
public:
    std::vector<std::shared_ptr<RegExNode>> children; /*!< Список ветвей альтернативы */
    int operandArity; /*!< Число операндов операции (по умолчанию бинарная; иначе равно children.size()) */

    /*!
     * \brief Конструктор узла альтернативы.
     * \param _children Вектор указателей на дочерние узлы альтернативы.
     * \param pos Позиция оператора в исходной строке.
     * \param pendingArity Заданная арность операции (-1 если не указана).
     */
    AlternateNode(const std::vector<std::shared_ptr<RegExNode>>& _children, size_t pos, int pendingArity = -1)
        : children(_children),
        operandArity(pendingArity >= 0 ? pendingArity : static_cast<int>(_children.size())) {
        startPosInStr = pos;
    }

    /*!
     * \brief Реализует построение НКА для параллельных ветвей альтернативы.
     * \param start Указатель на начальное состояние фрагмента.
     * \param end Указатель на конечное состояние фрагмента.
     */
    void buildNFA(std::shared_ptr<NFAState> start, std::shared_ptr<NFAState> end) override;
};

/*!
 * \brief Класс узла универсального квантификатора.
 */
class QuantifierNode : public RegExNode {
public:
    std::shared_ptr<RegExNode> child; /*!< Указатель на квантифицируемый дочерний узел */
    int minOccur;                     /*!< Минимальное требуемое число повторений */
    int maxOccur;                     /*!< Максимальное число повторений (значение -1 означает бесконечность) */

    /*!
     * \brief Конструктор узла квантификатора.
     * \param _child Квантифицируемый дочерний узел.
     * \param _min Минимальное число повторений.
     * \param _max Максимальное число повторений.
     * \param pos Позиция квантификатора в исходной строке.
     */
    QuantifierNode(std::shared_ptr<RegExNode> _child, int _min, int _max, size_t pos)
        : child(_child), minOccur(_min), maxOccur(_max) {
        startPosInStr = pos;
    }

    /*!
     * \brief Реализует построение НКА для повторения дочернего фрагмента.
     * \param start Указатель на начальное состояние фрагмента.
     * \param end Указатель на конечное состояние фрагмента.
     */
    void buildNFA(std::shared_ptr<NFAState> start, std::shared_ptr<NFAState> end) override;
};

/*!
 * \brief Строит синтаксическое дерево регулярного выражения из обратной польской записи (ОПЗ).
 * \param str Входная строка регулярного выражения в формате ОПЗ.
 * \param errorMessages Множество для записи обнаруженных ошибок построения дерева.
 * \return Умный указатель на корень дерева RegExNode (nullptr при ошибке построения).
 */
std::shared_ptr<RegExNode> buildRegExTreeFromPostfixNotation(
    const std::string& str,
    std::unordered_set<Error>& errorMessages
);

/*!
 * \brief Разбивает строку на токены и возвращает пары (токен, позиция начала).
 * \param str Входная строка регулярного выражения.
 * \param delim Строка-разделитель (например, пробел).
 * \return Вектор пар: текст токена и позиция его начала в исходной строке.
 */
std::vector<std::pair<std::string, size_t>> splitStringIntoTokens(
    const std::string& str,
    const std::string& delim
);

/*!
 * \brief Создаёт упорядоченный список узлов по токенам ОПЗ.
 * \param tokens Упорядоченный список пар (токен, позиция начала в строке).
 * \param errorMessages Множество для записи ошибок лексического и синтаксического разбора токенов.
 * \return Линейный список узлов RegExNode; при ошибках возвращается пустой список.
 */
std::vector<std::shared_ptr<RegExNode>> createRegExNodesFromTokens(
    const std::vector<std::pair<std::string, size_t>>& tokens,
    std::unordered_set<Error>& errorMessages
);

/*!
 * \brief Создаёт дерево вычисления из линейного списка узлов с проверкой арности операций.
 * \param nodes Упорядоченный список узлов, полученный из токенов.
 * \param errorMessages Множество для записи ошибок (недостаток или избыток операндов).
 * \return Указатель на корень итогового дерева выражения.
 */
std::shared_ptr<RegExNode> createRegExTreeFromRegExNodes(
    std::vector<std::shared_ptr<RegExNode>>& nodes,
    std::unordered_set<Error>& errorMessages
);