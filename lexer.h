#ifndef LLVM_SAMPLE_LEXER_H
#define LLVM_SAMPLE_LEXER_H

#include <string>

namespace Lexer {
    static std::string IdentifierStr; // Filled in if identifier
    static double NumVal;             // Filled in if number

    int getToken();
}

#endif //LLVM_SAMPLE_LEXER_H
