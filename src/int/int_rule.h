//
// Created by jiry on 19-10-26.
//

#ifndef FLASHFILL_INT_RULE_H
#define FLASHFILL_INT_RULE_H

#include "../lib/rule.h"

class IntBinary: public FunctionSemantics {
protected:
public:
    IntBinary(Type oup, std::string name): FunctionSemantics({TINT, TINT}, oup, name) {}
    virtual WitnessList witnessFunction(const DataList& inp, const DataList &para_list);
};

class IntCompare: public FunctionSemantics {
public:
    IntCompare(std::string name): FunctionSemantics({TINT, TINT}, TBOOL, name) {}
    virtual WitnessList witnessFunction(const DataList& inp, const DataList &para_list);
};

class IntAdd: public IntBinary {
public:
    IntAdd(): IntBinary(TINT, "+") {}
    virtual Data run(const DataList &inp, const DataList &param_value_list) {
#ifdef DEBUG
        check(inp);
#endif
        return Data(new IntValue(inp[0].getInt() + inp[1].getInt()));
    }
    virtual z3::expr encodeZ3Expr(z3::context &ctx, const std::vector<z3::expr> &inp) {
        return inp[0] + inp[1];
    }
};

class IntMinus: public IntBinary {
public:
    IntMinus(): IntBinary(TINT, "-") {}
    virtual Data run(const DataList &inp, const DataList &param_value_list) {
#ifdef DEBUG
        check(inp);
#endif
        return Data(new IntValue(inp[0].getInt() - inp[1].getInt()));
    }
    virtual z3::expr encodeZ3Expr(z3::context &ctx, const std::vector<z3::expr> &inp) {
        return inp[0] - inp[1];
    }
};

class IntLq: public IntCompare {
public:
    IntLq(): IntCompare("<") {}
    virtual Data run(const DataList &inp, const DataList &param_value_list) {
#ifdef DEBUG
        check(inp);
#endif
        return Data(new BoolValue(inp[0].getInt() < inp[1].getInt()));
    }
    virtual z3::expr encodeZ3Expr(z3::context &ctx, const std::vector<z3::expr> &inp) {
        return inp[0] < inp[1];
    }
};

class IntLe: public IntCompare {
public:
    IntLe(): IntCompare("<=") {}
    virtual Data run(const DataList &inp, const DataList &param_value_list) {
#ifdef DEBUG
        check(inp);
#endif
        return Data(new BoolValue(inp[0].getInt() <= inp[1].getInt()));
    }
    virtual z3::expr encodeZ3Expr(z3::context &ctx, const std::vector<z3::expr> &inp) {
        return inp[0] <= inp[1];
    }
};

class IntGe: public IntCompare {
public:
    IntGe(): IntCompare(">=") {}
    virtual Data run(const DataList &inp, const DataList &param_value_list) {
#ifdef DEBUG
        check(inp);
#endif
        return Data(new BoolValue(inp[0].getInt() >= inp[1].getInt()));
    }
    virtual z3::expr encodeZ3Expr(z3::context &ctx, const std::vector<z3::expr> &inp) {
        return inp[0] >= inp[1];
    }
};

class IntGq: public IntCompare {
public:
    IntGq(): IntCompare(">") {}
    virtual Data run(const DataList &inp, const DataList &param_value_list) {
#ifdef DEBUG
        check(inp);
#endif
        return Data(new BoolValue(inp[0].getInt() > inp[1].getInt()));
    }
    virtual z3::expr encodeZ3Expr(z3::context &ctx, const std::vector<z3::expr> &inp) {
        return inp[0] > inp[1];
    }
};

class IntEq: public IntBinary {
public:
    IntEq(): IntBinary(TBOOL, "==") {}
    virtual Data run(const DataList &inp, const DataList &param_value_list) {
#ifdef DEBUG
        check(inp);
#endif
        return Data(new BoolValue(inp[0].getInt() == inp[1].getInt()));
    }
    virtual z3::expr encodeZ3Expr(z3::context &ctx, const std::vector<z3::expr> &inp) {
        return inp[0] == inp[1];
    }
};

class IntIte: public FunctionSemantics {
protected:
public:
    IntIte(): FunctionSemantics({TBOOL, TINT, TINT}, TINT, "ite") {}
    virtual WitnessList witnessFunction(const DataList &oup, const DataList &para_list);
    virtual Data run(const DataList &inp, const DataList &param_value_list) {
#ifdef DEBUG
        check(inp);
#endif
        int result = inp[0].getBool() ? inp[1].getInt() : inp[2].getInt();
        return Data(new IntValue(result));
    }
    virtual z3::expr encodeZ3Expr(z3::context &ctx, const std::vector<z3::expr> &inp) {
        return z3::ite(inp[0], inp[1], inp[2]);
    }

};



#endif //FLASHFILL_INT_RULE_H
