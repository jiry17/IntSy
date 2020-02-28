//
// Created by jiry on 19-11-3.
//

#ifndef FLASHFILL_STRING_RULE_H
#define FLASHFILL_STRING_RULE_H

#include "../lib/specification.h"
#include "../lib/example_space.h"

#include <map>

class StringOperator: public FunctionSemantics {
public:
    StringOperator(const std::vector<Type> inp_type, Type oup, std::string name): FunctionSemantics(inp_type, oup, name) {}
    z3::expr encodeZ3Expr(z3::context &ctx, const std::vector<z3::expr> &inp) {
        LOG(INFO) << "Do not support z3" << std::endl;
        assert(0);
    }
};

class StringConcat: public StringOperator {
public:
    StringConcat(): StringOperator({TSTRING, TSTRING}, TSTRING, "cons") {};
    virtual Data run(const DataList &inp, const DataList &param_value_list);
    virtual WitnessList witnessFunction(const DataList &oup, const DataList &para_list);
};

class StringSubstr: public StringOperator {
public:
    StringSubstr(): StringOperator({TSTRING, TINT, TINT}, TSTRING, "substr") {};
    virtual Data run(const DataList &inp, const DataList &param_value_list);
    virtual WitnessList witnessFunction(const DataList &oup, const DataList &para_list);
};

class StringIndex: public StringOperator {
public:
    StringIndex(): StringOperator({TSTRING, TSTRING, TINT}, TINT, "idx") {}
    virtual Data run(const DataList &inp, const DataList &param_value_list);
    virtual WitnessList witnessFunction(const DataList& oup, const DataList &para_list);
};

class StringIndexMove: public StringOperator {
public:
    StringIndexMove(): StringOperator({TINT, TINT}, TINT, "move") {}
    virtual Data run(const DataList& inp, const DataList &param_value_list);
    virtual WitnessList witnessFunction(const DataList& oup, const DataList &para_list);
};

class StringLen: public StringOperator {
public:
    StringLen(): StringOperator({TSTRING}, TINT, "len") {}
    virtual Data run(const DataList& inp, const DataList &param_value_list);
    virtual WitnessList witnessFunction(const DataList& oup, const DataList &para_list);
};

class StringReplace: public StringOperator {
public:
    StringReplace(): StringOperator({TSTRING, TSTRING, TSTRING}, TSTRING, "replace") {}
    virtual Data run(const DataList& inp, const DataList &param_value_list);
    virtual WitnessList witnessFunction(const DataList& oup, const DataList &para_list);
};

class StringDelete: public StringOperator {
public:
    StringDelete(): StringOperator({TSTRING, TSTRING}, TSTRING, "delete") {}
    virtual Data run(const DataList& inp, const DataList &param_value_list);
    virtual WitnessList witnessFunction(const DataList& oup, const DataList &para_list);
};

extern std::vector<std::string> string_constant_list;
#endif //FLASHFILL_STRING_RULE_H
