/*!
 * \file NFA.h
 * \brief Заголовочный файл, описывающий структуры и классы недетерминированного конечного автомата (НКА).
 */

#pragma once

#include <memory>
#include <vector>
#include <string>
#include <utility>
#include <cstddef>

class Trans;

/*!
 * \brief Структура, описывающая состояние (узел) двунаправленного графа НКА.
 */
struct NFAState {
    std::vector<std::shared_ptr<Trans>> outgoingTransitions; /*!< Список исходящих путей (переходов) в другие состояния */
    std::vector<std::weak_ptr<Trans>> incomingTransitions;   /*!< Список входящих путей (для обратного прохода графа) */
    bool isFinal = false;                                    /*!< Флаг, обозначающий допускающее (конечное) состояние */
    size_t distanceToTerminal = static_cast<size_t>(-1);     /*!< Минимальное число переходов до допускающего состояния (-1 = недостижимо) */
};

/*!
 * \brief Абстрактный базовый класс перехода между состояниями НКА.
 */
class Trans {
protected:
    std::shared_ptr<NFAState> target; /*!< Целевое состояние, в которое осуществляется переход */
    std::weak_ptr<NFAState> source;   /*!< Исходное состояние, из которого осуществляется переход */

public:
    /*!
     * \brief Базовый конструктор перехода.
     * \param _target Указатель на целевое состояние.
     * \param _source Указатель на исходное состояние.
     */
    Trans(std::shared_ptr<NFAState> _target, std::shared_ptr<NFAState> _source)
        : target(_target), source(_source) {
    }

    /*!
     * \brief Виртуальный деструктор по умолчанию.
     */
    virtual ~Trans() = default;

    /*!
     * \brief Проверяет, применим ли данный переход к текущему символу строки.
     * \param str Исходная строка, по которой осуществляется поиск.
     * \param pos Индекс текущего проверяемого символа в строке.
     * \return true, если по данному символу можно совершить переход, иначе false.
     */
    virtual bool isApplicable(const std::string& str, size_t pos) const = 0;

    /*!
     * \brief Возвращает количество символов строки, которые поглощаются при этом переходе.
     * \return Количество символов (0 для эпсилон-перехода, 1 для символьного).
     */
    virtual size_t charsConsumed() const = 0;

    /*!
     * \brief Проверяет, является ли переход пустым (эпсилон-переходом).
     * \return true, если это эпсилон-переход, иначе false.
     */
    virtual bool isEpsilon() const { return false; }

    /*!
     * \brief Возвращает целевое состояние перехода.
     * \return std::shared_ptr<NFAState> Указатель на состояние назначения.
     */
    std::shared_ptr<NFAState> getTarget() const { return target; }

    /*!
     * \brief Возвращает исходное состояние перехода.
     * \return std::shared_ptr<NFAState> Указатель на состояние отправления.
     */
    std::shared_ptr<NFAState> getSource() const { return source.lock(); }
};

/*!
 * \brief Класс пустого (эпсилон) перехода, не потребляющего символы входной строки.
 */
class EpsTrans : public Trans {
public:
    /*!
     * \brief Конструктор эпсилон-перехода.
     * \param target Указатель на целевое состояние.
     * \param source Указатель на исходное состояние.
     */
    EpsTrans(std::shared_ptr<NFAState> target, std::shared_ptr<NFAState> source)
        : Trans(target, source) {
    }

    /*!
     * \brief Эпсилон-переход применим всегда, независимо от текущего символа.
     * \param str Исходная строка.
     * \param pos Индекс текущего символа.
     * \return Всегда true.
     */
    bool isApplicable(const std::string& str, size_t pos) const override { return true; }

    /*!
     * \brief Идентифицирует переход как пустой.
     * \return Всегда true.
     */
    bool isEpsilon() const override { return true; }

    /*!
     * \brief Возвращает количество поглощаемых символов.
     * \return Всегда 0.
     */
    size_t charsConsumed() const override { return 0; }
};

/*!
 * \brief Класс символьного перехода, осуществляемого по заданному диапазону (или массиву диапазонов) символов.
 */
class CharTrans : public Trans {
protected:
    std::vector<std::pair<char, char>> charRanges; /*!< Список допустимых отрезков символов таблицы ASCII */

public:
    /*!
     * \brief Конструктор символьного перехода.
     * \param target Указатель на целевое состояние.
     * \param source Указатель на исходное состояние.
     * \param ranges Вектор пар символов, задающих допустимые диапазоны (например, 'a'-'z').
     */
    CharTrans(std::shared_ptr<NFAState> target, std::shared_ptr<NFAState> source,
        const std::vector<std::pair<char, char>>& ranges)
        : Trans(target, source), charRanges(ranges) {
    }

    /*!
     * \brief Проверяет вхождение символа строки в заданные допустимые диапазоны.
     * \param str Исходная строка.
     * \param pos Индекс текущего символа.
     * \return true, если символ попадает хотя бы в один из диапазонов charRanges.
     */
    bool isApplicable(const std::string& str, size_t pos) const override;

    /*!
     * \brief Возвращает количество поглощаемых символов.
     * \return Всегда 1.
     */
    size_t charsConsumed() const override { return 1; }
};

/*!
 * \brief Структура, инкапсулирующая недетерминированный конечный автомат целиком.
 */
struct NFA {
    std::shared_ptr<NFAState> startState;            /*!< Указатель на начальную точку (состояние) автомата */
    std::shared_ptr<NFAState> terminalState;         /*!< Указатель на допускающую (конечную) точку автомата */
    std::vector<std::shared_ptr<NFAState>> states;   /*!< Полный список всех состояний графа для централизованного управления памятью */
};