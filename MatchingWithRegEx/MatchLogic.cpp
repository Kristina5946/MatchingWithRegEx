/*!
 * \file MatchLogic.cpp
 * \brief Реализация поиска совпадений, ε-замыкания и симуляции НКА.
 */

#include "pch.h"
#include "MatchLogic.h"
#include <deque>
#include <vector>
#include <set>

void calculateDistanceToTerminal(std::shared_ptr<NFAState> endState)
{
    // 1: Терминальное состояние — расстояние 0
    endState->distanceToTerminal = 0;

    // 2: Собрать все состояния, достижимые назад от end
    std::set<NFAState*> states;
    std::deque<NFAState*> collectQueue;
    collectQueue.push_back(endState.get());
    states.insert(endState.get());

    bool collectNotEmpty = !collectQueue.empty();
    while (collectNotEmpty) {
        NFAState* u = collectQueue.front();
        collectQueue.pop_front();

        size_t inIndex = 0;
        while (inIndex < u->incomingTransitions.size()) {
            std::shared_ptr<Trans> trans = u->incomingTransitions[inIndex].lock();
            if (trans) {
                std::shared_ptr<NFAState> v = trans->getSource();
                if (v && states.find(v.get()) == states.end()) {
                    states.insert(v.get());
                    collectQueue.push_back(v.get());
                }
            }
            inIndex++;
        }

        collectNotEmpty = !collectQueue.empty();
    }

    // 3: Остальным состояниям — «бесконечность»
    for (NFAState* state : states) {
        if (state != endState.get()) {
            state->distanceToTerminal = static_cast<size_t>(-1);
        }
    }

    // 4: Релаксация до стабилизации (нужно для циклов вроде a*)
    bool improved = true;
    while (improved) {
        improved = false;
        for (NFAState* u : states) {
            if (u->distanceToTerminal == static_cast<size_t>(-1)) {
                continue;
            }

            size_t inIndex = 0;
            while (inIndex < u->incomingTransitions.size()) {
                std::shared_ptr<Trans> trans = u->incomingTransitions[inIndex].lock();
                if (trans) {
                    std::shared_ptr<NFAState> v = trans->getSource();
                    if (v) {
                        size_t stepWeight = trans->isEpsilon() ? 0 : 1;
                        size_t newDistance = u->distanceToTerminal + stepWeight;
                        if (newDistance < v->distanceToTerminal) {
                            v->distanceToTerminal = newDistance;
                            improved = true;
                        }
                    }
                }
                inIndex++;
            }
        }
    }
}

std::unordered_set<std::shared_ptr<NFAState>> SimulateNFA(
    const std::unordered_set<std::shared_ptr<NFAState>>& currentStates, char c)
{
    // 1: Создать пустое множество следующих состояний
    std::unordered_set<std::shared_ptr<NFAState>> nextStates;

    std::string inputChar;
    inputChar.push_back(c);

    // 2: Для каждого текущего состояния проверить исходящие переходы
    for (const std::shared_ptr<NFAState>& state : currentStates) {
        size_t transIndex = 0;
        while (transIndex < state->outgoingTransitions.size()) {
            std::shared_ptr<Trans> trans = state->outgoingTransitions[transIndex];

            // 3: Символьный переход применим к c — добавить целевое состояние
            if (!trans->isEpsilon()) {
                bool applicable = trans->isApplicable(inputChar, 0);
                // 3.1: Маска ANY не принимает перенос строки (спецификация маски ".")
                if (applicable && c == '\n' && trans->isApplicable("a", 0)) {
                    applicable = false;
                }
                if (applicable) {
                    std::shared_ptr<NFAState> target = trans->getTarget();
                    if (target) {
                        nextStates.insert(target);
                    }
                }
            }
            transIndex++;
        }
    }

    // 4: eps-замыкание — стек обхода, добавить состояния по пустым переходам
    std::vector<std::shared_ptr<NFAState>> stack;
    for (const std::shared_ptr<NFAState>& state : nextStates) {
        stack.push_back(state);
    }

    size_t stackIndex = 0;
    bool stackNotEmpty = stackIndex < stack.size();
    while (stackNotEmpty) {
        // 4.1: Извлечь состояние из стека
        std::shared_ptr<NFAState> current = stack[stackIndex];
        stackIndex++;

        // 4.2: Для каждого исходящего перехода
        size_t outIndex = 0;
        while (outIndex < current->outgoingTransitions.size()) {
            std::shared_ptr<Trans> trans = current->outgoingTransitions[outIndex];
            if (trans->isEpsilon()) {
                std::shared_ptr<NFAState> target = trans->getTarget();
                if (target) {
                    // 4.2.1: Если целевого состояния ещё нет — добавить в множество и стек
                    if (nextStates.find(target) == nextStates.end()) {
                        nextStates.insert(target);
                        stack.push_back(target);
                    }
                }
            }
            outIndex++;
        }

        stackNotEmpty = stackIndex < stack.size();
    }

    // 5: Вернуть множество после символьного шага и eps-замыкания
    return nextStates;
}

void epsilonClosure(std::unordered_set<ActiveState>& states)
{
    // 1: Создать стек и поместить в него начальные ActiveState из states
    std::vector<ActiveState> stack;
    for (const ActiveState& activeState : states) {
        stack.push_back(activeState);
    }

    // 2: Пока стек не пуст
    size_t stackIndex = 0;
    bool stackNotEmpty = stackIndex < stack.size();
    while (stackNotEmpty) {
        // 2.1: Извлечь активное состояние из стека
        ActiveState current = stack[stackIndex];
        stackIndex++;

        // 2.2: Для каждого исходящего перехода
        size_t transIndex = 0;
        while (transIndex < current.state->outgoingTransitions.size()) {
            std::shared_ptr<Trans> trans = current.state->outgoingTransitions[transIndex];

            // 2.2.1: Если переход пустой (EpsTrans)
            if (trans->isEpsilon()) {
                std::shared_ptr<NFAState> target = trans->getTarget();
                if (target) {
                    // 2.2.1.2: Проверка уникальности целевого состояния во множестве states
                    ActiveState probe;
                    probe.state = target;

                    bool targetAlreadyPresent = states.find(probe) != states.end();
                    if (!targetAlreadyPresent) {
                        ActiveState newActive;
                        newActive.state = target;
                        newActive.currentMatch = current.currentMatch;

                        states.insert(newActive);
                        stack.push_back(newActive);
                    }
                }
            }
            transIndex++;
        }

        stackNotEmpty = stackIndex < stack.size();
    }
}

bool isBetterPartialMatch(const Match& bestMatch, const Match& potentialMatch)
{
    // 1: Меньшее расстояние до терминала — лучше
    bool result = false;
    if (potentialMatch.distanceToTerminal < bestMatch.distanceToTerminal) {
        result = true;
    }
    else if (potentialMatch.distanceToTerminal == bestMatch.distanceToTerminal) {
        // 2: При равном расстоянии — большая длина совпадения лучше
        size_t bestLength = bestMatch.end - bestMatch.start;
        size_t potentialLength = potentialMatch.end - potentialMatch.start;
        if (potentialLength > bestLength) {
            result = true;
        }
        else if (potentialLength == bestLength) {
            // 3: При равной длине — меньшая start лучше 
            if (potentialMatch.start < bestMatch.start) {
                result = true;
            }
        }
    }
    return result;
}

Match determineFinalMatch(const Match& fullMatch, const Match& bestPartialMatch)
{
    Match result;

    // 1: Полное совпадение имеет абсолютный приоритет
    if (fullMatch.isFullMatch) {
        result = fullMatch;
    }
    else {
        // 2: Иначе вернуть лучшее частичное совпадение
        result = bestPartialMatch;
    }
    return result;
}

std::unordered_set<ActiveState> computeNextStates(
    const std::unordered_set<ActiveState>& currentStates,
    const std::string& str, size_t pos)
{
    // 1: Создать пустое множество следующих состояний
    std::unordered_set<ActiveState> nextStates;

    // 2: Для каждого состояния из currentStates проверить исходящие переходы
    for (const ActiveState& active : currentStates) {
        size_t transIndex = 0;
        while (transIndex < active.state->outgoingTransitions.size()) {
            std::shared_ptr<Trans> trans = active.state->outgoingTransitions[transIndex];

            // 3: Символьный переход применим к str[pos] — добавить целевое состояние
            if (!trans->isEpsilon()) {
                bool applicable = trans->isApplicable(str, pos);
                // 3.1: Маска "." не принимает перенос строки 
                if (applicable && pos < str.length() && str[pos] == '\n' && trans->isApplicable("a", 0)) {
                    applicable = false;
                }
                if (applicable) {
                    std::shared_ptr<NFAState> target = trans->getTarget();
                    if (target) {
                        ActiveState probe;
                        probe.state = target;

                        bool targetAlreadyPresent = nextStates.find(probe) != nextStates.end();
                        if (!targetAlreadyPresent) {
                            ActiveState newActive;
                            newActive.state = target;
                            newActive.currentMatch = active.currentMatch;
                            newActive.currentMatch.end = pos + 1;

                            nextStates.insert(newActive);
                        }
                    }
                }
            }
            transIndex++;
        }
    }

    // 4: eps-замыкание для полученного множества
    epsilonClosure(nextStates);

    // 5: Вернуть множество после символьного шага и eps-замыкания
    return nextStates;
}

void simulateNFA(const std::string& str, size_t startPos,
    std::unordered_set<ActiveState>& currentStates,
    Match& fullMatch, Match& bestPartialMatch)
{
    size_t pos = startPos;

    // 1: Пока фронт не пуст и не достигнут конец строки
    bool canContinue = !currentStates.empty() && pos < str.length();
    while (canContinue) {
        // 1.1: Обновить лучшее частичное совпадение по текущему фронту
        for (const ActiveState& active : currentStates) {
            Match potentialPartial;
            potentialPartial.start = active.currentMatch.start;
            potentialPartial.end = active.currentMatch.end;
            potentialPartial.distanceToTerminal = active.state->distanceToTerminal;
            potentialPartial.isValid = true;
            potentialPartial.isFullMatch = false;

            bool zeroLengthPartialOk = (pos >= str.length())
                || (potentialPartial.end == potentialPartial.start && startPos > 0);
            if (potentialPartial.end > potentialPartial.start || zeroLengthPartialOk) {
                if (isBetterPartialMatch(bestPartialMatch, potentialPartial)) {
                    bestPartialMatch = potentialPartial;
                }
            }
        }

        // 1.2: Сдвинуть волну на один символ вперёд
        std::unordered_set<ActiveState> nextStates = computeNextStates(currentStates, str, pos);

        // 1.3: Тупик — дальнейший поиск невозможен
        if (nextStates.empty()) {
            canContinue = false;
        }
        else {
            // 1.4: Следующие состояния становятся текущими
            currentStates = nextStates;

            // 1.5: Если на фронте есть финальное состояние — обновить fullMatch
            for (const ActiveState& active : currentStates) {
                if (active.state->isFinal) {
                    fullMatch.start = active.currentMatch.start;
                    fullMatch.end = active.currentMatch.end;
                    fullMatch.isFullMatch = true;
                    fullMatch.isValid = true;
                    fullMatch.distanceToTerminal = 0;
                }
            }

            pos = pos + 1;
            canContinue = !currentStates.empty() && pos < str.length();
        }
    }

    // 1.1 (финальный фронт): оценить частичное совпадение после последнего шага
    for (const ActiveState& active : currentStates) {
        Match potentialPartial;
        potentialPartial.start = active.currentMatch.start;
        potentialPartial.end = active.currentMatch.end;
        potentialPartial.distanceToTerminal = active.state->distanceToTerminal;
        potentialPartial.isValid = true;
        potentialPartial.isFullMatch = false;

        bool zeroLengthPartialOk = (pos >= str.length())
            || (potentialPartial.end == potentialPartial.start && startPos > 0);
        if (potentialPartial.end > potentialPartial.start || zeroLengthPartialOk) {
            if (isBetterPartialMatch(bestPartialMatch, potentialPartial)) {
                bestPartialMatch = potentialPartial;
            }
        }
    }
}

Match findMatchAtPosition(const NFA& nfa, const std::string& str, size_t startPos)
{
    // 1: Создать currentStates и поместить начальное состояние автомата
    std::unordered_set<ActiveState> currentStates;
    Match initialMatch;
    initialMatch.start = startPos;
    initialMatch.end = startPos;

    ActiveState startActive;
    startActive.state = nfa.startState;
    startActive.currentMatch = initialMatch;
    currentStates.insert(startActive);

    // 2: eps-замыкание — все состояния, достижимые без чтения символов
    epsilonClosure(currentStates);

    // 3: Инициализировать fullMatch и bestPartialMatch
    Match fullMatch;
    Match bestPartialMatch;

    // 4: Если после eps-замыкания есть финальное состояние — полное совпадение нулевой длины
    for (const ActiveState& active : currentStates) {
        if (active.state->isFinal) {
            fullMatch.start = startPos;
            fullMatch.end = startPos;
            fullMatch.isFullMatch = true;
            fullMatch.isValid = true;
            fullMatch.distanceToTerminal = 0;
        }
    }

    // 5: Симуляция автомата по строке
    simulateNFA(str, startPos, currentStates, fullMatch, bestPartialMatch);

    // 6: Вернуть итоговое совпадение
    return determineFinalMatch(fullMatch, bestPartialMatch);
}

std::vector<Match> extractAllMatches(const NFA& nfa, const std::string& str)
{
    // 1: Итоговый список и лучшее частичное совпадение на всей строке
    std::vector<Match> finalMatches;
    Match globalBestPartial;

    // 2: Цикл по каждой позиции строки
    size_t pos = 0;
    bool posInRange = pos < str.length();
    while (posInRange) {
        // 2.1: Поиск совпадения с текущей позиции
        Match resultMatch = findMatchAtPosition(nfa, str, pos);

        // 2.2: Полное совпадение
        if (resultMatch.isValid && resultMatch.isFullMatch) {
            if (resultMatch.end > resultMatch.start) {
                // 2.2.1: Добавить в итоговый список
                finalMatches.push_back(resultMatch);
                // 2.2.2: Сдвинуть pos за конец совпадения
                pos = resultMatch.end;
            }
            else {
                // Пустое совпадение — только сдвиг pos (защита от зацикливания)
                pos = pos + 1;
            }
        }
        else if (resultMatch.isValid && !resultMatch.isFullMatch) {
            // 2.3: Частичное — обновить globalBestPartial
            if (isBetterPartialMatch(globalBestPartial, resultMatch)) {
                globalBestPartial = resultMatch;
            }
            pos = pos + 1;
        }
        else {
            pos = pos + 1;
        }

        posInRange = pos < str.length();
    }

    // 3: Если есть полные совпадения — вернуть их
    if (!finalMatches.empty()) {
        return finalMatches;
    }

    // 4: Иначе вернуть одно лучшее частичное совпадение
    if (globalBestPartial.isValid && globalBestPartial.end > globalBestPartial.start) {
        std::vector<Match> partialResult;
        partialResult.push_back(globalBestPartial);
        return partialResult;
    }

    // 5: Совпадений нет
    return finalMatches;
}