//
// Created by pro on 2019/10/16.
//

#ifndef FLASHFILL_SPECIFICATION_H
#define FLASHFILL_SPECIFICATION_H

#include "rule.h"
#include <map>

enum BenchmarkType {
    BCIRCUIT, BINT, BSTRING
};

class Program {
public:
    Semantics* semantics;
    std::vector<Program*> children;

    Data run(const DataList& input) const{
        DataList children_value;
        for (const auto* child: children) {
            children_value.push_back(child->run(input));
        }
        return semantics->run(children_value, input);
    }

    Program(Semantics* _semantics, std::vector<Program*> _children): semantics(_semantics), children(_children) {}

    z3::expr parseProgrm(z3::context& ctx) const{
        std::vector<z3::expr> sub_program;
        for (const auto* child: children) {
            sub_program.push_back(child->parseProgrm(ctx));
        }
        return semantics->encodeZ3Expr(ctx, sub_program);
    }
};

typedef std::vector<Rule> RuleList;

class Specification {
    void checkValid() {
        if (symbols.size() != rule_list.size()) {
            LOG(INFO) << "Number of symbols should be equal to the number of rule lists" << std::endl;
            assert(0);
        }
        for (int i = 0; i < symbols.size(); ++i) {
            if (symbols[i].symbol_id != i) {
                LOG(INFO) << "Invalid symbol id" << std::endl;
                assert(0);
            }
            for (auto rule: rule_list[i]) {
                if (rule.return_symbol.symbol_id != i) {
                    LOG(INFO) << "Invalid rule" << std::endl;
                    assert(0);
                }
            }
        }
    }
public:
    BenchmarkType benchmark_type;
    std::vector<NonTerminal> symbols;
    std::vector<RuleList> rule_list;
    NonTerminal start_symbol;
    int depth_limit;
    std::vector<std::pair<DataList, Data>> example_list;

    void addExample(const DataList& inp, Data oup) {
        example_list.push_back(std::make_pair(inp, oup));
    }

    Specification(const std::vector<NonTerminal>& _symbols, const std::vector<RuleList>& _rule_list,
                  NonTerminal _start_symbol, int _depth_limit, BenchmarkType _benchmark_type):
            symbols(_symbols), rule_list(_rule_list), start_symbol(_start_symbol), depth_limit(_depth_limit),
            benchmark_type(_benchmark_type) {
#ifdef DEBUG
        checkValid();
#endif
    }
};


#endif //FLASHFILL_SPECIFICATION_H
