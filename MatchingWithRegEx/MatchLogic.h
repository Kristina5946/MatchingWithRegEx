/*!
 * \file MatchLogic.h
 * \brief Заголовочный файл, описывающий структуры данных и логику поиска совпадений в строке (НКА).
 */

#pragma once

#include "NFA.h"
#include "Error.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <memory>
#include <cstddef>

/*!
 * \brief Структура для хранения информации найденного совпадения.
 */
struct Match {
    size_t start = 0;              /*!< Позиция начала найденного фрагмента */
    size_t end = 0;                /*!< Позиция конца (сразу после последнего совпавшего символа) */
    bool isFullMatch = false;      /*!< Флаг, является ли совпадение полным */
    size_t distanceToTerminal = static_cast<size_t>(-1); /*!< Минимальное количество переходов от обрыва до допускающего состояния */
    bool isValid = false;          /*!< Флаг валидности совпадения (найдено ли оно) */
};

/*!
 * \brief Вспомогательная структура, объединяющая состояние автомата и параметры текущего совпадения.
 */
struct ActiveState {
    std::shared_ptr<NFAState> state; /*!< Указатель на текущее состояние НКА */
    Match currentMatch;              /*!< Структура совпадения для текущей ветви поиска */

    /*!
     * \brief Сравнивает состояния автомата на равенство.
     * \param other Сравниваемое активное состояние.
     * \return true, если состояния автомата совпадают.
     */
    bool operator==(const ActiveState& other) const {
        return state == other.state;
    }
};

namespace std {
    /*!
     * \brief Специализация шаблона std::hash для структуры ActiveState.
     */
    template <>
    struct hash<ActiveState> {
        /*!
         * \brief Функция вычисления хэш-значения.
         * \param as Объект активного состояния.
         * \return Вычисленное хэш-значение (size_t) на основе указателя на состояние.
         */
        size_t operator()(const ActiveState& as) const {
            return hash<shared_ptr<NFAState>>()(as.state);
        }
    };
}

/*!
 * \brief Вычисляет расстояние до терминального состояния для каждого состояния автомата.
 * Использует алгоритм 0-1 BFS (поиск в ширину), двигаясь в обратном направлении от допускающего состояния.
 * \param endState Указатель на терминальное (конечное) состояние автомата.
 */
void calculateDistanceToTerminal(std::shared_ptr<NFAState> endState); 

/*!
 * \brief Осуществляет поиск совпадения (полного или частичного) в строке с заданной позиции.
 * \param nfa Объект автомата, по которому осуществляется поиск.
 * \param str Анализируемая входная строка.
 * \param startPos Позиция (индекс) в строке, с которой начинается поиск.
 * \return Структура Match с результатами совпадения.
 */
Match findMatchAtPosition(const NFA& nfa, const std::string& str, size_t startPos);

/*!
 * \brief Симулирует один шаг работы НКА по заданному символу (функция для тестов).
 * \param currentStates Текущее множество состояний.
 * \param c Входной символ для проверки переходов.
 * \return std::unordered_set<std::shared_ptr<NFAState>> Новое множество активных состояний.
 */
std::unordered_set<std::shared_ptr<NFAState>> SimulateNFA(
    const std::unordered_set<std::shared_ptr<NFAState>>& currentStates, char c);

/*!
 * \brief Выполняет симуляцию работы автомата (обход графа волной) для строки.
 * \param str Входная строка.
 * \param startPos Позиция, с которой начинается проверка.
 * \param currentStates Текущее множество активных состояний.
 * \param fullMatch Ссылка на объект для записи наилучшего полного совпадения.
 * \param bestPartialMatch Ссылка на объект для записи наилучшего частичного совпадения.
 */
void simulateNFA(const std::string& str, size_t startPos,
    std::unordered_set<ActiveState>& currentStates,
    Match& fullMatch, Match& bestPartialMatch);

/*!
 * \brief Вычисляет следующее множество активных состояний (шаг автомата).
 * \param currentStates Множество текущих активных состояний.
 * \param str Анализируемая входная строка.
 * \param pos Индекс текущего проверяемого символа.
 * \return Множество новых активных состояний после перехода по символу и выполнения эпсилон-замыкания.
 */
std::unordered_set<ActiveState> computeNextStates(const std::unordered_set<ActiveState>& currentStates,
    const std::string& str, size_t pos);

/*!
 * \brief Сравнивает два частичных совпадения и возвращает, какое из них лучше.
 * \param bestMatch Текущее наилучшее найденное частичное совпадение.
 * \param potentialMatch Новое потенциальное частичное совпадение для проверки.
 * \return true, если потенциальное совпадение лучше (ближе к финалу или длиннее).
 */
bool isBetterPartialMatch(const Match& bestMatch, const Match& potentialMatch);

/*!
 * \brief Определяет итоговое совпадение для конкретной позиции.
 * \param fullMatch Найденное полное совпадение.
 * \param bestPartialMatch Найденное лучшее частичное совпадение.
 * \return Итоговый объект Match (полное совпадение имеет абсолютный приоритет).
 */
Match determineFinalMatch(const Match& fullMatch, const Match& bestPartialMatch);

/*!
 * \brief Извлекает все отфильтрованные совпадения в строке.
 * \param nfa Объект автомата.
 * \param str Анализируемая входная строка.
 * \return Вектор итоговых совпадений.
 */
std::vector<Match> extractAllMatches(const NFA& nfa, const std::string& str);

/*!
 * \brief Выполняет эпсилон-замыкание для заданного множества.
 * \param states Множество активных состояний, которое будет дополнено новыми достижимыми состояниями.
 */
void epsilonClosure(std::unordered_set<ActiveState>& states);

/*!
 * \brief Генерирует HTML-отчет с цветовой индикацией найденных совпадений и синтаксических ошибок.
 * \param filepath Путь к выходному файлу отчета.
 * \param originalStr Исходная проверяемая строка.
 * \param regexTemplate Исходная строка регулярного выражения (шаблон).
 * \param matches Список отфильтрованных совпадений.
 * \param errors Множество выявленных синтаксических ошибок.
 */
void generateHtmlReport(const std::string& filepath,
    const std::string& originalStr,
    const std::string& regexTemplate,
    const std::vector<Match>& matches,
    const std::unordered_set<Error>& errors);