#pragma once
#include "MatchLogic.h"
#include "NFA.h"
#include <vector>
#include <memory>
#include <unordered_set>

namespace ClosureGraphSetup {

    void buildGraph(int testId,
        const std::vector<std::pair<char, char>>& charA,
        const std::vector<std::pair<char, char>>& charB,
        std::vector<std::shared_ptr<NFAState>>& states,
        std::unordered_set<ActiveState>& input);

    std::vector<int> expectedIndices(int testId);

}
