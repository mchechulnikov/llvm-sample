#include "lexer.h"
#include "token.h"

namespace Lexer {
    int getToken() {
        static int lastChar = ' ';

        // Skip any whitespace.
        while (isspace(lastChar))
            lastChar = getchar();

        if (isalpha(lastChar)) { // identifier: [a-zA-Z][a-zA-Z0-9]*
            IdentifierStr = lastChar;
            while (isalnum((lastChar = getchar())))
                IdentifierStr += lastChar;

            if (IdentifierStr == "def")
                return static_cast<int>(Token::def);
            if (IdentifierStr == "extern")
                return static_cast<int>(Token::ext);
            return static_cast<int>(Token::identifier);
        }

        if (isdigit(lastChar) || lastChar == '.') { // Number: [0-9.]+
            std::string numStr;
            do {
                numStr += lastChar;
                lastChar = getchar();
            } while (isdigit(lastChar) || lastChar == '.');

            NumVal = strtod(numStr.c_str(), nullptr);
            return static_cast<int>(Token::number);
        }

        if (lastChar == '#') {
            do {
                lastChar = getchar();
            } while (lastChar != EOF && lastChar != '\n' && lastChar != '\r');

            if (lastChar != EOF) {
                return getToken();
            }
        }

        // Check for end of file.  Don't eat the EOF.
        if (lastChar == EOF)
            return static_cast<int>(Token::eof);

        // Otherwise, just return the character as its ascii value.
        int ThisChar = lastChar;
        lastChar = getchar();
        return ThisChar;
    }
}
