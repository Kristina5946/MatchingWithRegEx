#include "pch.h"
#include "MatchLogic.h"
#include "Error.h"
#include <fstream>
#include <string>

namespace {

    std::string escapeHtml(const std::string& text)
    {
        std::string result;
        size_t index = 0;
        while (index < text.length()) {
            char symbol = text[index];
            if (symbol == '&') {
                result += "&amp;";
            }
            else if (symbol == '<') {
                result += "&lt;";
            }
            else if (symbol == '>') {
                result += "&gt;";
            }
            else {
                result.push_back(symbol);
            }
            index++;
        }
        return result;
    }

    void writeRedText(std::ofstream& out, const std::string& text)
    {
        out << "<span style=\"color:red\">";
        out << escapeHtml(text);
        out << "</span>";
    }

    void writeRegexTemplateWithError(std::ofstream& out,
        const std::string& regexTemplate, size_t errorPos)
    {
        size_t index = 0;
        while (index < regexTemplate.length()) {
            if (index == errorPos) {
                out << "<span style=\"color:red\">";
                out << escapeHtml(regexTemplate.substr(index, 1));
                out << "</span>";
                index++;
            }
            else {
                out << escapeHtml(regexTemplate.substr(index, 1));
                index++;
            }
        }
    }

    std::string errorDescription(const Error& error)
    {
        std::string description;
        if (error.type == Error::emptyTemplate) {
            description = "Empty template";
        }
        else if (error.type == Error::invalidCharacter) {
            description = "Invalid character";
        }
        else if (error.type == Error::invalidEscapeSequence) {
            description = "Invalid escape sequence";
        }
        else if (error.type == Error::insufficientOperands) {
            description = "Insufficient operands";
        }
        else if (error.type == Error::excessiveOperands) {
            description = "Excessive operands";
        }
        else if (error.type == Error::unrecognizedSequence) {
            description = "Unrecognized sequence";
        }
        else if (error.type == Error::invalidArity) {
            description = "Invalid arity";
        }
        else if (error.type == Error::invalidInterval) {
            description = "Invalid interval";
        }
        else if (error.type == Error::nfaLimitExceeded) {
            description = "NFA limit exceeded";
        }
        else if (error.type == Error::inputFileNotExistOrNoAccess) {
            description = "Input file not found or no access";
        }
        else if (error.type == Error::outputFileCannotBeCreated) {
            description = "Output file cannot be created";
        }
        else {
            description = "Syntax error";
        }
        return description;
    }
}

void generateHtmlReport(const std::string& filepath,
    const std::string& originalStr,
    const std::string& regexTemplate,
    const std::vector<Match>& matches,
    const std::unordered_set<Error>& errors)
{
    // 1: Открыть или создать выходной файл отчёта
    std::ofstream out(filepath.c_str());

    // 2: Записать базовую структуру HTML-документа
    out << "<!DOCTYPE html>\n";
    out << "<html><head><meta charset=\"UTF-8\"><title>MatchingWithRegEx Report</title></head><body>\n";

    bool hasErrors = !errors.empty();
    if (hasErrors) {
        // 3: Ошибки синтаксиса при разборе шаблона
        for (const Error& error : errors) {
            // 3.1: Блок сообщения об ошибке и подсветка шаблона
            error.message();
            out << "<p>" << errorDescription(error) << "</p>\n";
            out << "<p>";
            writeRegexTemplateWithError(out, regexTemplate, error.pos);
            out << "</p>\n";
        }

        // 3.2: Закрыть документ и завершить
        out << "</body></html>\n";
    }
    else {
        // 4.1: Позиция начала необработанного текста
        size_t unprocessedPos = 0;

        // 4.2: Для каждого совпадения из списка
        for (const Match& match : matches) {
            // 4.2.1: Несовпавший текст перед совпадением — красный
            if (match.start > unprocessedPos) {
                writeRedText(out, originalStr.substr(unprocessedPos, match.start - unprocessedPos));
            }

            std::string matchText = originalStr.substr(match.start, match.end - match.start);

            // 4.2.2: Текст совпадения
            if (match.isFullMatch) {
                // 4.2.2.1: Полное — зелёный маркер
                out << "<mark style=\"background-color:lightgreen\">";
                out << escapeHtml(matchText);
                out << "</mark>";
            }
            else {
                // 4.2.2.2: Частичное — жёлтый фон
                out << "<span class=\"partial-box\" style=\"background-color:yellow\">";
                out << escapeHtml(matchText);
                out << "</span>";
            }

            // 4.2.3: Сдвинуть необработанную позицию
            unprocessedPos = match.end;
        }

        // 4.3: Хвост строки — красный
        if (unprocessedPos < originalStr.length()) {
            writeRedText(out, originalStr.substr(unprocessedPos));
        }

        // 5: Закрыть теги HTML-документа
        out << "</body></html>\n";
    }
}
