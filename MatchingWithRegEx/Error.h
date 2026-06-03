/*!
 * \file Error.h
 * \brief Заголовочный файл с описанием класса синтаксических и системных ошибок.
 */

#pragma once

#include <cstddef>
#include <functional>

 /*!
  * \brief Класс, описывающий ошибку парсинга регулярного выражения или системную ошибку.
  */
class Error {
public:
    /*!
     * \brief Перечисление возможных кодов (типов) ошибок.
     */
    enum ErrorType {
        noError,                     //!< Ошибок не обнаружено
        inputFileNotExistOrNoAccess, //!< Входной файл не существует или нет прав доступа
        emptyTemplate,               //!< Пустой шаблон регулярного выражения
        invalidCharacter,            //!< Обнаружен недопустимый символ
        outputFileCannotBeCreated,   //!< Ошибка при создании выходного HTML-файла
        invalidEscapeSequence,       //!< Некорректная escape-последовательность
        insufficientOperands,        //!< Недостаточно операндов для выполнения операции
        excessiveOperands,           //!< Избыточные операнды (остались лишние элементы в стеке)
        unrecognizedSequence,        //!< Нераспознанная последовательность символов
        invalidArity,                //!< Неверная арность (количество аргументов) операции
        invalidInterval,             //!< Некорректно задан интервал квантификатора
        nfaLimitExceeded             //!< Превышено допустимое количество состояний НКА
    };

    ErrorType type; /*!< Тип ошибки */
    size_t line;    /*!< Номер строки, в которой зафиксирована ошибка */
    size_t pos;     /*!< Индекс символа (позиция) ошибки в строке */

    /*!
     * \brief Конструктор инициализации объекта ошибки.
     * \param _type Код (тип) ошибки из перечисления ErrorType.
     * \param _pos Позиция (индекс) в строке, где была обнаружена ошибка.
     * \param _line Номер строки с ошибкой (по умолчанию 0).
     */
    Error(ErrorType _type, size_t _pos, size_t _line = 0)
        : type(_type), pos(_pos), line(_line) {
    }

    /*!
     * \brief Формирует блок с текстовым сообщением об ошибке и генерирует HTML-разметку.
     * * В реализации данного метода генерируется HTML-код, где символ на позиции pos
     * визуально выделяется красным цветом.
     */
    void message() const;

    /*!
     * \brief Перегрузка оператора проверки на равенство.
     * \param other Сравниваемый объект ошибки.
     * \return true, если типы ошибок и их координаты полностью совпадают, иначе false.
     */
    bool operator==(const Error& other) const {
        return type == other.type && pos == other.pos && line == other.line;
    }
};

namespace std {
    /*!
     * \brief Специализация шаблона std::hash для класса Error.
     * * Позволяет использовать объекты класса Error в качестве ключей в неупорядоченных
     * ассоциативных контейнерах (например, std::unordered_set).
     */
    template <>
    struct hash<Error> {
        /*!
         * \brief Оператор вычисления хэш-значения.
         * \param e Объект ошибки, для которого вычисляется хэш.
         * \return Вычисленное значение хэша (size_t).
         */
        size_t operator()(const Error& e) const {
            return hash<int>()(e.type) ^ (hash<size_t>()(e.pos) << 1);
        }
    };
}