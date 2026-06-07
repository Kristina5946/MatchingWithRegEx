/*!
 * \file HtmlReport.h
 * \brief Формирование HTML-отчёта с подсветкой совпадений и синтаксических ошибок.
 */

#pragma once

#include "MatchLogic.h"
#include "Error.h"
#include <string>
#include <vector>
#include <unordered_set>

/*!
 * \brief Генерирует HTML-отчёт с цветовой индикацией найденных совпадений и ошибок шаблона.
 *
 * При наличии ошибок в \p errors выводится описание и шаблон с красной подсветкой
 * позиции \c pos. Иначе в \p originalStr подсвечиваются:
 * - полные совпадения — зелёный фон (\c mark);
 * - лучшее частичное совпадение — жёлтый фон (\c span);
 * - несовпавшие фрагменты — красный текст.
 *
 * \param filepath Путь к выходному файлу отчёта.
 * \param originalStr Исходная проверяемая строка.
 * \param regexTemplate Исходная строка регулярного выражения (шаблон ОПЗ).
 * \param matches Список отфильтрованных совпадений.
 * \param errors Множество выявленных синтаксических ошибок.
 */
void generateHtmlReport(const std::string& filepath,
    const std::string& originalStr,
    const std::string& regexTemplate,
    const std::vector<Match>& matches,
    const std::unordered_set<Error>& errors);
