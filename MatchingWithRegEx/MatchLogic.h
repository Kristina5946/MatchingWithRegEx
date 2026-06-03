/*!
 * \file MatchLogic.h
 * \brief Заголовочный файл, содержащий логику поиска совпадений в тексте с использованием недетерминированного конечного автомата (НКА).
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
  * \brief Структура для фиксации параметров найденного совпадения.
  */
struct Match {
    size_t start = 0;              /*!< Индекс начала найденной подстроки */
    size_t end = 0;                /*!< Индекс конца (сразу после последнего совпавшего символа) */
    bool isFullMatch = false;      /*!< Флаг, указывающий на полное совпадение с паттерном */
    size_t distanceToTerminal = static_cast<size_t>(-1); /*!< Минимальное количество оставшихся переходов до допускающего состояния */
    bool isValid = false;          /*!< Флаг успешности поиска (валидности совпадения) */
};

/*!
 * \brief Вспомогательная структура, объединяющая состояние автомата с контекстом текущего совпадения.
 */
struct ActiveState {
    std::shared_ptr<NFAState> state; /*!< Указатель на текущее состояние НКА */
    Match currentMatch;              /*!< Параметры совпадения для данного пути поиска */

    /*!
     * \brief Перегрузка оператора проверки на равенство.
     * \param other Сравниваемое активное состояние.
     * \return true, если состояния НКА идентичны.
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
         * \brief Оператор вычисления хэш-значения.
         * \param as Объект активного состояния.
         * \return Вычисленное значение хэша (size_t) на основе указателя на состояние.
         */
        size_t operator()(const ActiveState& as) const {
            return hash<shared_ptr<NFAState>>()(as.state);
        }
    };
}

/*!
 * \brief Вычисляет кратчайшее расстояние от каждого состояния автомата до терминального.
 * Реализует алгоритм 0-1 BFS (поиск в ширину), двигаясь в обратном направлении от допускающего состояния.
 * \param endState Указатель на терминальное (конечное) состояние автомата.
 */
void calculateDistanceToTerminal(std::shared_ptr<NFAState> endState); 

/*!
 * \brief Осуществляет проход по графу автомата для заданного стартового символа и фиксирует наилучший результат.
 * \param nfa Объект автомата, по которому осуществляется поиск.
 * \param str Анализируемая входная строка.
 * \param startPos Позиция (индекс) в строке, с которой начинается поиск.
 * \return Структура Match с результатами совпадения (границы, флаг полного совпадения, минимальное расстояние до конца).
 */
Match findMatchAtPosition(const NFA& nfa, const std::string& str, size_t startPos);

/*!
 * \brief Симулирует один шаг работы НКА по заданному символу (базовая версия без учета контекста совпадения).
 * \param currentStates Текущее множество состояний.
 * \param c Входной символ для проверки переходов.
 * \return std::unordered_set<std::shared_ptr<NFAState>> Новое множество активных состояний НКА.
 */
std::unordered_set<std::shared_ptr<NFAState>> SimulateNFA(
    const std::unordered_set<std::shared_ptr<NFAState>>& currentStates, char c);

/*!
 * \brief Запускает цикл симуляции НКА для поиска наилучшего совпадения начиная с конкретной позиции.
 * \param str Исходная строка.
 * \param startPos Позиция, с которой начинается проверка.
 * \param currentStates Изначальное множество активных состояний (обычно содержит только стартовое).
 * \param fullMatch Ссылка на объект для записи параметров найденного полного совпадения.
 * \param bestPartialMatch Ссылка на объект для записи параметров наилучшего частичного совпадения.
 */
void simulateNFA(const std::string& str, size_t startPos,
    std::unordered_set<ActiveState>& currentStates,
    Match& fullMatch, Match& bestPartialMatch);

/*!
 * \brief Вычисляет следующее множество активных состояний (шаг автомата).
 * \param currentStates Множество текущих активных состояний.
 * \param str Анализируемая входная строка.
 * \param pos Индекс текущего считываемого символа.
 * \return Множество новых состояний после перехода по символу и выполнения эпсилон-замыкания.
 */
std::unordered_set<ActiveState> computeNextStates(const std::unordered_set<ActiveState>& currentStates,
    const std::string& str, size_t pos);

/*!
 * \brief Сравнивает два частичных совпадения и определяет, какое из них лучше.
 * \param bestMatch Текущее зафиксированное лучшее частичное совпадение.
 * \param potentialMatch Новое потенциальное частичное совпадение для проверки.
 * \return true, если потенциальное совпадение имеет меньшее расстояние до финала или большую длину при равном расстоянии.
 */
bool isBetterPartialMatch(const Match& bestMatch, const Match& potentialMatch);

/*!
 * \brief Определяет итоговое совпадение для конкретной позиции.
 * \param fullMatch Объект зафиксированного полного совпадения.
 * \param bestPartialMatch Объект лучшего частичного совпадения.
 * \return Итоговый объект Match (полное совпадение имеет абсолютный приоритет над частичным).
 */
Match determineFinalMatch(const Match& fullMatch, const Match& bestPartialMatch);

/*!
 * \brief Осуществляет фильтрацию и отбор итоговых непересекающихся совпадений по всей строке.
 * \param nfa Объект автомата.
 * \param str Анализируемая входная строка.
 * \return Список итоговых совпадений для вывода в отчет (либо полные, либо одно лучшее частичное).
 */
std::vector<Match> extractAllMatches(const NFA& nfa, const std::string& str);

/*!
 * \brief Выполняет поиск всех состояний, доступных из переданного множества по пустым (эпсилон) переходам.
 * \param states Ссылка на множество активных состояний, которое будет дополнено новыми достижимыми состояниями.
 */
void epsilonClosure(std::unordered_set<ActiveState>& states);

/*!
 * \brief Генерирует HTML-отчет с цветовой индикацией найденных совпадений и выводом синтаксических ошибок.
 * \param filepath Путь к создаваемому файлу отчета.
 * \param originalStr Исходная проверяемая строка, в которой производился поиск.
 * \param regexTemplate Исходная строка регулярного выражения (шаблон, необходим для подсветки позиций ошибок синтаксиса).
 * \param matches Список отфильтрованных совпадений (полных или одного частичного), полученный из extractAllMatches.
 * \param errors Множество зафиксированных синтаксических ошибок.
 */
void generateHtmlReport(const std::string& filepath,
    const std::string& originalStr,
    const std::string& regexTemplate,
    const std::vector<Match>& matches,
    const std::unordered_set<Error>& errors);