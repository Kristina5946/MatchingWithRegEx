/*!
 * \file RegExTree.h
 * \brief Заголовочный файл, содержащий классы узлов синтаксического дерева регулярного выражения и функции парсинга.
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
  * \brief Базовый абстрактный класс узла синтаксического дерева регулярного выражения.
  */
class RegExNode {
public:
    size_t startPosInStr; /*!< Стартовая позиция узла в исходной строке с выражением */

    /*!
     * \brief Виртуальный деструктор по умолчанию.
     */
    virtual ~RegExNode() = default;

    /*!
     * \brief Построение фрагмента недетерминированного конечного автомата (НКА) для текущего узла.
     * \param start Указатель на начальное состояние, предоставляемое родительским узлом.
     * \param end Указатель на конечное состояние, предоставляемое родительским узлом.
     */
    virtual void buildNFA(std::shared_ptr<NFAState> start, std::shared_ptr<NFAState> end) = 0;
};

/*!
 * \brief Класс узла для представления конкретных символов и символьных масок (диапазонов).
 */
class CharNode : public RegExNode {
public:
    std::vector<std::pair<char, char>> charRanges; /*!< Список допустимых отрезков символов таблицы ASCII */

    /*!
     * \brief Конструктор узла символов.
     * \param ranges Вектор пар символов, задающих допустимые диапазоны.
     * \param pos Позиция токена в исходной строке.
     */
    CharNode(const std::vector<std::pair<char, char>>& ranges, size_t pos)
        : charRanges(ranges) {
        startPosInStr = pos;
    }

    /*!
     * \brief Построение фрагмента НКА для символьного узла.
     * \param start Указатель на начальное состояние фрагмента.
     * \param end Указатель на конечное состояние фрагмента.
     */
    void buildNFA(std::shared_ptr<NFAState> start, std::shared_ptr<NFAState> end) override;
};

/*!
 * \brief Класс узла конкатенации (последовательного соединения элементов).
 */
class ConcatNode : public RegExNode {
public:
    std::vector<std::shared_ptr<RegExNode>> children; /*!< Список упорядоченных дочерних узлов */

    /*!
     * \brief Конструктор узла конкатенации.
     * \param _children Вектор указателей на дочерние синтаксические узлы.
     * \param pos Позиция операции в исходной строке.
     */
    ConcatNode(const std::vector<std::shared_ptr<RegExNode>>& _children, size_t pos)
        : children(_children) {
        startPosInStr = pos;
    }

    /*!
     * \brief Построение фрагмента НКА для последовательности дочерних узлов.
     * \param start Указатель на начальное состояние фрагмента.
     * \param end Указатель на конечное состояние fragment.
     */
    void buildNFA(std::shared_ptr<NFAState> start, std::shared_ptr<NFAState> end) override;
};

/*!
 * \brief Класс узла альтернативы (выбора из нескольких вариантов).
 */
class AlternateNode : public RegExNode {
public:
    std::vector<std::shared_ptr<RegExNode>> children; /*!< Список дочерних узлов-ветвей альтернативы */

    /*!
     * \brief Конструктор узла альтернативы.
     * \param _children Вектор указателей на возможные варианты ветвления.
     * \param pos Позиция операции в исходной строке.
     */
    AlternateNode(const std::vector<std::shared_ptr<RegExNode>>& _children, size_t pos)
        : children(_children) {
        startPosInStr = pos;
    }

    /*!
     * \brief Построение фрагмента НКА для параллельных ветвей альтернативы.
     * \param start Указатель на начальное состояние фрагмента.
     * \param end Указатель на конечное состояние фрагмента.
     */
    void buildNFA(std::shared_ptr<NFAState> start, std::shared_ptr<NFAState> end) override;
};

/*!
 * \brief Класс узла универсального квантификатора повторений.
 */
class QuantifierNode : public RegExNode {
public:
    std::shared_ptr<RegExNode> child; /*!< Указатель на единственный повторяющийся дочерний узел */
    int minOccur;                     /*!< Минимальное требуемое количество повторений */
    int maxOccur;                     /*!< Максимальное количество повторений (значение -1 означает бесконечность) */

    /*!
     * \brief Конструктор узла квантификатора.
     * \param _child Повторяемый дочерний узел.
     * \param _min Минимальный порог повторений.
     * \param _max Максимальный порог повторений.
     * \param pos Позиция квантификатора в исходной строке.
     */
    QuantifierNode(std::shared_ptr<RegExNode> _child, int _min, int _max, size_t pos)
        : child(_child), minOccur(_min), maxOccur(_max) {
        startPosInStr = pos;
    }

    /*!
     * \brief Построение фрагмента НКА для циклической структуры квантификатора.
     * \param start Указатель на начальное состояние фрагмента.
     * \param end Указатель на конечное состояние фрагмента.
     */
    void buildNFA(std::shared_ptr<NFAState> start, std::shared_ptr<NFAState> end) override;
};

/*!
 * \brief Главная функция-фасад для построения синтаксического дерева из строки в обратной польской записи (ОПЗ).
 * \param str Исходная строка регулярного выражения в формате ОПЗ.
 * \param errorMessages Множество для фиксации обнаруженных синтаксических ошибок.
 * \return Корневой указатель на построенное дерево RegExNode (nullptr в случае критической ошибки).
 */
std::shared_ptr<RegExNode> buildRegExTreeFromPostfixNotation(
    const std::string& str,
    std::unordered_set<Error>& errorMessages
);

/*!
 * \brief Разбивает исходную строку на отдельные текстовые токены с фиксацией их позиций.
 * \param str Исходная строка регулярного выражения.
 * \param delim Строка-разделитель (например, пробел).
 * \return Вектор пар, где каждый элемент содержит текст токена и его смещение от начала строки.
 */
std::vector<std::pair<std::string, size_t>> splitStringIntoTokens(
    const std::string& str,
    const std::string& delim
);

/*!
 * \brief Трансформирует плоский список текстовых токенов в изолированные объекты узлов дерева.
 * \param tokens Вектор пар токенов (текст и позиция).
 * \param errorMessages Множество для фиксации ошибок при валидации токенов.
 * \return Вектор указателей на созданные независимые узлы RegExNode.
 */
std::vector<std::shared_ptr<RegExNode>> createRegExNodesFromTokens(
    const std::vector<std::pair<std::string, size_t>>& tokens,
    std::unordered_set<Error>& errorMessages
);

/*!
 * \brief Сверяет арность операций и собирает итоговое дерево из массива узлов с помощью стекового алгоритма.
 * \param nodes Вектор указателей на ранее созданные узлы.
 * \param errorMessages Множество для фиксации ошибок (недостаток операндов, избыток операндов).
 * \return Указатель на корневой узел собранного дерева.
 */
std::shared_ptr<RegExNode> createRegExTreeFromRegExNodes(
    std::vector<std::shared_ptr<RegExNode>>& nodes,
    std::unordered_set<Error>& errorMessages
);