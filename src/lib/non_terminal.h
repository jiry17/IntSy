//
// Created by pro on 2019/10/15.
//

#ifndef FLASHFILL_NONTERMINAL_H
#define FLASHFILL_NONTERMINAL_H

#include <string>

enum Type {
    TINT, TBOOL, TSTRING
};

class NonTerminal {
public:
    std::string name;
    Type type;
    int symbol_id;
    NonTerminal(std::string _name, Type _type, int _symbol_id): name(_name), type(_type), symbol_id(_symbol_id) {}
};

#endif //FLASHFILL_NONTERMINAL_H
