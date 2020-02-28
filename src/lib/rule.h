//
// Created by pro on 2019/10/15.
//

#ifndef FLASHFILL_OPERATOR_H
#define FLASHFILL_OPERATOR_H

#include "value.h"
#include "non_terminal.h"
#include "config.h"
#include "z3++.h"
#include "glog/logging.h"

#include <vector>

typedef std::vector<DataList> WitnessResult;
typedef std::vector<WitnessResult> WitnessList;

class Semantics {
protected:
    bool checkInList(const DataList& data_list, Data& value) {
        for (auto& data: data_list) {
            if (data == value) {
                return true;
            }
        }
        return false;
    }
public:
    std::vector<Type> inp;
    Type oup;
    std::string name;
    Semantics(const std::vector<Type>& _inp, Type _oup, std::string _name): inp(_inp), oup(_oup), name(_name) {}
    virtual WitnessList witnessFunction(const DataList& oup, const DataList& para_list) = 0;
    virtual Data run(const DataList &inp, const DataList &param_value_list) = 0;
    virtual z3::expr encodeZ3Expr(z3::context& ctx, const std::vector<z3::expr>& inp) = 0;
};

class Rule {
public:
    NonTerminal return_symbol;
    std::vector<NonTerminal> inp_symbol;
    Semantics* semantics;

    Rule(NonTerminal _return_symbol, const std::vector<NonTerminal>& _inp, Semantics* _semantics):
            return_symbol(_return_symbol), inp_symbol(_inp), semantics(_semantics) {
#ifdef DEBUG
        assert(_return_symbol.type == semantics->oup);
        //std::cout << semantics->name << " " << inp_symbol.size() << " " << semantics->inp.size() << std::endl;
        assert(inp_symbol.size() == semantics->inp.size());

        for (int i = 0; i < inp_symbol.size(); ++i) {
            assert(inp_symbol[i].type == semantics->inp[i]);
        }
#endif
    }

    WitnessList witnessFunction(const DataList& oup, const DataList& para_list) {
        return semantics->witnessFunction(oup, para_list);
    }
    Data run(const DataList &inp, const DataList &param_value_list) {
        return semantics->run(inp, param_value_list);
    }
    z3::expr encodeZ3Expr(z3::context& ctx, const std::vector<z3::expr>& inp) {
        return semantics->encodeZ3Expr(ctx, inp);
    }
};

class FunctionSemantics: public Semantics {
protected:
    void check(const DataList& data_list) {
        assert(inp.size() == data_list.size());
        for (int i = 0; i < inp.size(); ++i) {
            assert(inp[i] == data_list[i].getType());
        }
    }

public:
    FunctionSemantics(const std::vector<Type>& _inp, Type _oup, std::string name): Semantics(_inp, _oup, name) {}
    virtual WitnessList witnessFunction(const DataList& oup, const DataList& para_list) = 0;
    virtual Data run(const DataList &inp, const DataList &param_value_list) = 0;
};

class ValueSemantics: public Semantics {
    Data value;
public:
    ValueSemantics(Data _value): value(_value), Semantics({}, _value.getType(), "value") {}
    virtual WitnessList witnessFunction(const DataList& oup, const DataList& para_list) {
        WitnessResult result;
        if (oup.empty()) return {result};
        for (auto& oup_value: oup) {
            if (oup_value == value) {
                return {result};
            }
        }
        return {};
    }
    virtual Data getData() {return value;}
    virtual Data run(const DataList &inp, const DataList &param_value_list) {
        return value;
    }
    virtual z3::expr encodeZ3Expr(z3::context& ctx, const std::vector<z3::expr>& inp) {
        return value.getZ3Expr(ctx);
    }
};

class DirectSemantics: public FunctionSemantics {
public:
    DirectSemantics(Type type): FunctionSemantics({type}, type, "direct") {}
    virtual z3::expr encodeZ3Expr(z3::context& ctx, const std::vector<z3::expr>& inp) {
        return inp[0];
    }
    virtual Data run(const DataList& inp, const DataList &param_value) {
        return inp[0];
    }
    virtual WitnessList witnessFunction(const DataList& oup, const DataList& param) {
        return {{oup}};
    }
};

class TruthSemantics: public Semantics {
    std::vector<std::pair<DataList, Data> > example_list;
public:
    virtual Data run(const DataList& inp, const DataList &param_value) {
        for (auto& example: example_list) {
            if (example.first == param_value) return example.second;
        }
        assert(false);
    }
    virtual z3::expr encodeZ3Expr(z3::context& ctx, const std::vector<z3::expr>& inp) {
        assert(0);
    }
    virtual WitnessList witnessFunction(const DataList& oup, const DataList& param) {
        assert(0);
    }
    TruthSemantics(const std::vector<std::pair<DataList, Data> >& _example_list):
            Semantics({}, TSTRING, "truth"), example_list(_example_list) {
    }
};

class ParamSemantics: public Semantics {
    int param_id;
    std::string getName() {
        static char name[100];
        sprintf(name, "param@%d", param_id);
        return std::string(name);
    }
public:
    ParamSemantics(int _param_id, Type _type): param_id(_param_id), Semantics({}, _type, "parm") {}
    virtual WitnessList witnessFunction(const DataList& oup, const DataList& para_list) {
        WitnessResult result;
        if (oup.empty()) return {result};
        for (auto& oup_value: oup) {
            if (oup_value == para_list[param_id]) {
                return {result};
            }
        }
        return {};
    }
    int getParamId() {return param_id;}
    virtual Data run(const DataList &inp, const DataList &param_value_list) {
        return param_value_list[param_id];
    }
    virtual z3::expr encodeZ3Expr(z3::context& ctx, const std::vector<z3::expr>& inp) {
        switch (oup){
            case TINT: return ctx.int_const(getName().c_str());
            case TBOOL: return ctx.bool_const(getName().c_str());
            case TSTRING:
                LOG(INFO) << "Do not support string" << std::endl;
                assert(0);
            default:
                LOG(INFO) << "Unknown type" << std::endl;
                assert(0);
        }
    }
    z3::expr encodeZ3ExprWithExample(z3::context& ctx, const DataList& param_value) {
        return param_value[param_id].getZ3Expr(ctx);
    }
};


#endif //FLASHFILL_OPERATOR_H
