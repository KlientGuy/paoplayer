#ifndef MUSIC_WIDGET_LEXER_H
#define MUSIC_WIDGET_LEXER_H

#include <unordered_set>

namespace config
{
    struct lexer {
        std::unordered_set<char> special_tokens;
    };
}

#endif //MUSIC_WIDGET_LEXER_H
