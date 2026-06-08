/*!
 * \file main.cpp
 * \brief Точка входа программы MatchingWithRegEx.
 *
 * \mainpage MatchingWithRegEx
 * Программа находит полные и частичные совпадения анализируемой строки
 * с регулярным выражением в обратной польской записи (ОПЗ).
 *
 * Аргументы командной строки:
 * - файл шаблона (ОПЗ, токены через пробел);
 * - файл проверяемой строки;
 * - выходной HTML-отчёт.
 *
 * \author Филиппова К.Е.
 */

#include "pch.h"
#include "RegExTree.h"
#include "MatchLogic.h"
#include "HtmlReport.h"
#include "Error.h"
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <vector>
#include <string>

namespace {

    const size_t NFA_STATE_LIMIT = 10000;

    bool readTextFile(const std::string& filepath, std::string& content)
    {
        std::ifstream input(filepath.c_str());
        if (!input.is_open()) {
            return false;
        }

        content.clear();
        std::string line;
        bool firstLine = true;
        bool hasMoreLines = static_cast<bool>(std::getline(input, line));
        while (hasMoreLines) {
            if (!firstLine) {
                content.push_back('\n');
            }
            content += line;
            firstLine = false;
            hasMoreLines = static_cast<bool>(std::getline(input, line));
        }
        return true;
    }

    size_t countReachableStates(const std::shared_ptr<NFAState>& startState)
    {
        std::unordered_set<NFAState*> visited;
        std::vector<std::shared_ptr<NFAState>> stack;
        stack.push_back(startState);
        visited.insert(startState.get());

        size_t stackIndex = 0;
        bool stackNotEmpty = stackIndex < stack.size();
        while (stackNotEmpty) {
            std::shared_ptr<NFAState> current = stack[stackIndex];
            stackIndex++;

            size_t transIndex = 0;
            while (transIndex < current->outgoingTransitions.size()) {
                std::shared_ptr<Trans> trans = current->outgoingTransitions[transIndex];
                std::shared_ptr<NFAState> target = trans->getTarget();
                if (target) {
                    NFAState* targetPtr = target.get();
                    if (visited.find(targetPtr) == visited.end()) {
                        visited.insert(targetPtr);
                        stack.push_back(target);
                    }
                }
                transIndex++;
            }

            stackNotEmpty = stackIndex < stack.size();
        }
        return visited.size();
    }
}

/*!
 * \brief Точка входа: разбор шаблона, построение НКА, поиск совпадений, формирование отчёта.
 * \param argc Число аргументов (ожидается 4: имя программы и три пути к файлам).
 * \param argv Массив аргументов: путь к шаблону, путь к строке, путь к отчёту.
 * \return 0 при успешном завершении, 1 при ошибке.
 */
int main(int argc, char* argv[])
{
    // 1: Проверить количество аргументов командной строки
    if (argc < 4) {
        std::cerr << "Usage: MatchingWithRegEx <regex.txt> <string.txt> <report.html>" << std::endl;
        return 1;
    }

    std::string regexFilePath = argv[1];
    std::string inputFilePath = argv[2];
    std::string outputFilePath = argv[3];

    // 2: Считать регулярное выражение из файла шаблона
    std::string regexTemplate;
    if (!readTextFile(regexFilePath, regexTemplate)) {
        std::cerr << "Cannot read regex template file." << std::endl;
        return 1;
    }

    // 3: Построить синтаксическое дерево регулярного выражения
    std::unordered_set<Error> errors;
    std::shared_ptr<RegExNode> tree = buildRegExTreeFromPostfixNotation(regexTemplate, errors);

    // 4: Если при построении возникли ошибки — HTML-отчёт и завершение
    if (!errors.empty()) {
        std::vector<Match> emptyMatches;
        generateHtmlReport(outputFilePath, "", regexTemplate, emptyMatches, errors);
        return 1;
    }

    if (!tree) {
        std::cerr << "Failed to build regex tree." << std::endl;
        return 1;
    }

    // 5: Создать start/end и построить НКА по дереву
    std::shared_ptr<NFAState> startState = std::make_shared<NFAState>();
    std::shared_ptr<NFAState> endState = std::make_shared<NFAState>();
    endState->isFinal = true;

    tree->buildNFA(startState, endState);

    // 5.1: Проверка лимита состояний НКА (10 000)
    size_t stateCount = countReachableStates(startState);
    if (stateCount > NFA_STATE_LIMIT) {
        errors.insert(Error(Error::nfaLimitExceeded, 0));
        std::vector<Match> emptyMatches;
        generateHtmlReport(outputFilePath, "", regexTemplate, emptyMatches, errors);
        return 1;
    }

    // 6: Рассчитать расстояния до допускающего состояния
    calculateDistanceToTerminal(endState);

    // 7: Считать анализируемую строку из файла данных
    std::string inputString;
    if (!readTextFile(inputFilePath, inputString)) {
        std::cerr << "Cannot read input string file." << std::endl;
        return 1;
    }

    // 8: Поиск и отбор итоговых совпадений в строке
    NFA nfa;
    nfa.startState = startState;
    nfa.terminalState = endState;
    std::vector<Match> matches = extractAllMatches(nfa, inputString);

    // 9: Сформировать итоговый HTML-файл
    if (!generateHtmlReport(outputFilePath, inputString, regexTemplate, matches, errors)) {
        return 1;
    }

    return 0;
}
