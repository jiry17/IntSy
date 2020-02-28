//
// Created by pro on 2019/10/15.
//

#ifndef FLASHFILL_CIRCUIT_OPERATOR_H
#define FLASHFILL_CIRCUIT_OPERATOR_H


#include "../lib/rule.h"

#include <memory>

class CircuitBinary: public FunctionSemantics {
public:
    CircuitBinary(std::string name): FunctionSemantics({TBOOL, TBOOL}, TBOOL, name) {}
    virtual WitnessList witnessFunction(const DataList& oup, const DataList& para_list);
    virtual Data run(const DataList &inp, const DataList &param_value_list) = 0;
};

class CircuitAnd: public CircuitBinary {
public:
    CircuitAnd(): CircuitBinary("and") {}

    virtual Data run(const DataList& input_list, const DataList &param_value_list) {
#ifdef DEBUG
        check(input_list);
#endif
        return Data(new BoolValue(input_list[0].getBool() & input_list[1].getBool()));
    }

    virtual z3::expr encodeZ3Expr(z3::context& ctx, const std::vector<z3::expr>& inp) {
        return inp[0] && inp[1];
    }

    virtual WitnessList witnessFunction(const DataList& oup, const DataList& para_list);
};

class CircuitOr: public CircuitBinary {
public:
    CircuitOr(): CircuitBinary("or") {}

    virtual Data run(const DataList& input_list, const DataList &param_value_list) {
#ifdef DEBUG
        check(input_list);
#endif
        return Data(new BoolValue(input_list[0].getBool() | input_list[1].getBool()));
    }

    virtual z3::expr encodeZ3Expr(z3::context& ctx, const std::vector<z3::expr>& inp) {
        return inp[0] || inp[1];
    }
    virtual WitnessList witnessFunction(const DataList& oup, const DataList& para_list);
};

class CircuitXor: public CircuitBinary {
public:
    CircuitXor(): CircuitBinary("xor") {}

    virtual Data run(const DataList& input_list, const DataList &param_value_list) {
#ifdef DEBUG
        check(input_list);
#endif
        return Data(new BoolValue(input_list[0].getBool() ^ input_list[1].getBool()));
    }

    virtual z3::expr encodeZ3Expr(z3::context& ctx, const std::vector<z3::expr>& inp) {
        return inp[0] != inp[1];
    }
};

class CircuitEq: public CircuitBinary {
public:
    CircuitEq(): CircuitBinary("==") {}

    virtual Data run(const DataList& input_list, const DataList &param_value_list) {
#ifdef DEBUG
        check(input_list);
#endif
        return Data(new BoolValue(input_list[0].getBool() == input_list[1].getBool()));
    }

    virtual z3::expr encodeZ3Expr(z3::context& ctx, const std::vector<z3::expr>& inp) {
        return inp[0] == inp[1];
    }
};

class CircuitNot: public FunctionSemantics {
public:
    CircuitNot(): FunctionSemantics({TBOOL}, TBOOL, "not") {}

    virtual Data run(const DataList& input_list, const DataList &param_value_list) {
#ifdef DEBUG
        check(input_list);
#endif
        return Data(new BoolValue(!input_list[0].getBool()));
    }

    virtual WitnessList witnessFunction(const DataList& oup, const DataList& param_list);

    virtual z3::expr encodeZ3Expr(z3::context& ctx, const std::vector<z3::expr>& inp) {
        return !inp[0];
    }
};

#endif //FLASHFILL_CIRCUIT_OPERATOR_H
