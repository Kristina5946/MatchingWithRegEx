/*!
 * \file ClosureGraphSetup.h
 * \brief Построение тестовых графов НКА для проверки epsilon-замыкания (табл. 2.2).
 */

#pragma once
#include "MatchLogic.h"
#include "NFA.h"
#include <vector>
#include <memory>
#include <unordered_set>

namespace ClosureGraphSetup {

    /*!
     * \brief Собирает граф и начальное множество состояний для теста табл. 2.2.
     * \param testId Номер тестового случая.
     * \param charA Диапазон символов для перехода «a».
     * \param charB Диапазон символов для перехода «b».
     * \param states Выходной список созданных состояний.
     * \param input Выходное начальное множество ActiveState.
     */
    void buildGraph(int testId,
        const std::vector<std::pair<char, char>>& charA,
        const std::vector<std::pair<char, char>>& charB,
        std::vector<std::shared_ptr<NFAState>>& states,
        std::unordered_set<ActiveState>& input);

    /*!
     * \brief Возвращает ожидаемые индексы состояний в activeSet после epsilonClosure.
     * \param testId Номер тестового случая.
     * \return Вектор индексов состояний.
     */
    std::vector<int> expectedIndices(int testId);

}
