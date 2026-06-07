/*!
 * \file Error.cpp
 * \brief Реализация вывода текстовых сообщений об ошибках.
 */

#include "pch.h"
#include "Error.h"
#include <iostream>

/*!
 * \brief Выводит текстовое описание ошибки в стандартный поток вывода.
 */
void Error::message() const
{
    if (type == Error::emptyTemplate) {
        std::cout << "Empty template";
    }
    else if (type == Error::invalidCharacter) {
        std::cout << "Invalid character";
    }
    else if (type == Error::invalidEscapeSequence) {
        std::cout << "Invalid escape sequence";
    }
    else if (type == Error::insufficientOperands) {
        std::cout << "Insufficient operands";
    }
    else if (type == Error::excessiveOperands) {
        std::cout << "Excessive operands";
    }
    else if (type == Error::unrecognizedSequence) {
        std::cout << "Unrecognized sequence";
    }
    else if (type == Error::invalidArity) {
        std::cout << "Invalid arity";
    }
    else if (type == Error::invalidInterval) {
        std::cout << "Invalid interval";
    }
    else if (type == Error::nfaLimitExceeded) {
        std::cout << "NFA limit exceeded";
    }
    else if (type == Error::inputFileNotExistOrNoAccess) {
        std::cout << "Input file not found or no access";
    }
    else if (type == Error::outputFileCannotBeCreated) {
        std::cout << "Output file cannot be created";
    }
    else {
        std::cout << "Syntax error";
    }
}
