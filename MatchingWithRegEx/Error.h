/*!
 * \file Error.h
 * \brief Заголовочный файл с описанием класса лексических и синтаксических ошибок.
 */

#pragma once

#include <cstddef>
#include <functional>

/*!
 * \brief Класс, описывающий ошибку синтаксического анализа или построения автомата.
 */
class Error {
public:
    /*!
     * \brief Перечисление возможных типов (кодов) ошибок.
     */
    enum ErrorType {
        noError,                     /*!< Ошибка не обнаружена */
        inputFileNotExistOrNoAccess, /*!< Входной файл не существует или нет прав доступа */
        emptyTemplate,               /*!< Пустой шаблон регулярного выражения */
        invalidCharacter,            /*!< Обнаружен недопустимый символ */
        outputFileCannotBeCreated,   /*!< Ошибка при создании выходного HTML-файла */
        invalidEscapeSequence,       /*!< Некорректная escape-последовательность */
        insufficientOperands,        /*!< Недостаточно операндов для выполнения операции */
        excessiveOperands,           /*!< Избыточные операнды (остались лишние элементы в стеке) */
        unrecognizedSequence,        /*!< Нераспознанная последовательность символов */
        invalidArity,                /*!< Неверная арность (количество операндов) операции */
        invalidInterval,             /*!< Некорректный интервал квантификатора (например, min > max) */
        nfaLimitExceeded             /*!< Превышен лимит состояний при генерации НКА */
    };

    ErrorType type; /*!< Тип ошибки */
    size_t line;    /*!< Номер строки, в которой зафиксирована ошибка */
    size_t pos;     /*!< Индекс символа (позиция) ошибки в строке */

    /*!
     * \brief Конструктор инициализации объекта ошибки.
     * \param _type Тип (код) ошибки из перечисления ErrorType.
     * \param _pos Позиция (индекс) в строке, где была обнаружена ошибка.
     * \param _line Номер строки в файле (по умолчанию 0).
     */
    Error(ErrorType _type, size_t _pos, size_t _line = 0)
        : type(_type), pos(_pos), line(_line) {
    }

    /*!
     * \brief Выводит текстовое сообщение об ошибке в стандартный поток вывода.
     */
    void message() const;

    /*!
     * \brief Оператор сравнения ошибок на равенство.
     * \param other Сравниваемый объект ошибки.
     * \return true, если типы ошибок и их координаты совпадают, иначе false.
     */
    bool operator==(const Error& other) const {
        return type == other.type && pos == other.pos && line == other.line;
    }
};

namespace std {
    /*!
     * \brief Специализация шаблона std::hash для класса Error.
     *
     * Позволяет использовать объекты класса Error в качестве ключей
     * в неупорядоченных ассоциативных контейнерах (например, std::unordered_set).
     */
    template <>
    struct hash<Error> {
        /*!
         * \brief Функция вычисления хэш-значения.
         * \param e Объект ошибки, для которого вычисляется хэш.
         * \return Вычисленное значение хэша (size_t).
         */
        size_t operator()(const Error& e) const {
            return hash<int>()(e.type) ^ (hash<size_t>()(e.pos) << 1);
        }
    };
}