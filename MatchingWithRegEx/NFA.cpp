/*!
 * \file NFA.cpp
 * \brief Реализация символьных переходов НКА.
 */

#include "pch.h"
#include "NFA.h"

bool CharTrans::isApplicable(const std::string& str, size_t pos) const
{
    // 1: Проверка границ — pos должна быть в пределах строки
    bool result = false;
    if (pos < str.length()) {
        char currentChar = str[pos];

        // 2: Цикл по charRanges — проверка вхождения символа в диапазон
        size_t rangeIndex = 0;
        while (rangeIndex < charRanges.size() && !result) {
            char rangeStart = charRanges[rangeIndex].first;
            char rangeEnd = charRanges[rangeIndex].second;
            if (currentChar >= rangeStart && currentChar <= rangeEnd) {
                result = true;
            }
            rangeIndex++;
        }
    }
    return result;
}
