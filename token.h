#ifndef LLVM_SAMPLE_TOKEN_H
#define LLVM_SAMPLE_TOKEN_H

// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.
enum class Token {
    eof = -1,

    // commands
    def = -2,
    ext = -3,

    // primary
    identifier = -4,
    number = -5
};

#endif //LLVM_SAMPLE_TOKEN_H
