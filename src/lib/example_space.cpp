//
// Created by pro on 2019/10/17.
//

#include "example_space.h"

namespace {
    z3::context ctx;
    z3::solver solver(ctx);

    void collectAllInput(int pos, const std::vector<Type>& type_list, DataList& current, std::vector<DataList>& data_list) {
        if (pos == type_list.size()) {
            data_list.push_back(current);
            return;
        }
        switch (type_list[pos]) {
            case TBOOL:
                current.push_back(Data(new BoolValue(true)));
                collectAllInput(pos + 1, type_list, current, data_list);
                current.pop_back();
                current.push_back(Data(new BoolValue(false)));
                collectAllInput(pos + 1, type_list, current, data_list);
                current.pop_back();
                break;
            case TINT:
                for (int i = -KIntMax; i <= KIntMax; ++i) {
                    current.push_back(Data(new IntValue(i)));
                    collectAllInput(pos + 1, type_list, current, data_list);
                    current.pop_back();
                }
                break;
            case TSTRING:
                LOG(INFO) << "Do not support string type" << std::endl;
                assert(0);
            default:
                assert(0);
        }
    }
}

z3::expr VectorExampleSpace::getConstraint(z3::context& ctx) {
    z3::expr_vector expr_list(ctx);
    for (int i = 0; i < type_list.size(); ++i) {
        if (type_list[i] == TINT) {
            expr_list.push_back(getParamVar(i, ctx) >= ctx.int_val(-max_int) && getParamVar(i, ctx) <= ctx.int_val(max_int));
        }
    }
    return z3::mk_and(expr_list);
}

DataList VectorExampleSpace::getCounterExample(Program *sample1, Program *sample2) {
    for (int i = 1; i <= 20; ++i) {
        DataList current = randomSelect();
        if (!(sample1->run(current) == sample2->run(current))) {
            DataList result;
            for (auto& data: current) {
                result.push_back(data);
            }
            return result;
        }
    }
    solver.reset();
    solver.add(getConstraint(ctx));
    solver.add(sample2->parseProgrm(ctx) != sample1->parseProgrm(ctx));
    std::cout << sample1->parseProgrm(ctx) << " " << sample2->parseProgrm(ctx) << std::endl;
    assert(solver.check() == z3::sat);
    auto model = solver.get_model();
    solver.reset();
    return getExampleFromModel(model, ctx);
}

bool VectorExampleSpace::equal(Program *program1, Program *program2) {
    solver.push();
    solver.add(getConstraint(ctx));
    solver.add(program1->parseProgrm(ctx) != program2->parseProgrm(ctx));
    auto result = solver.check();
    solver.pop();
    return result == z3::unsat;
}

std::vector<Data> VectorExampleSpace::getExampleFromModel(const z3::model &model, z3::context &ctx) {
    std::vector<Data> example_input;
    for (int i = 0; i < type_list.size(); ++i) {
        example_input.push_back(getVariableData(type_list[i], getName(i), model, ctx));
    }
    return example_input;
}

DataList VectorExampleSpace::randomSelect() {
    DataList result;
    for (auto type: type_list) {
        switch (type) {
            case TBOOL: {
                result.push_back(Data(new BoolValue(rand() & 1)));
                break;
            }
            case TINT: {
                result.push_back(Data(new IntValue(rand() % (max_int * 2 + 1) - max_int)));
                break;
            }
            case TSTRING:
                LOG(INFO) << "Do not support string" << std::endl;
                assert(0);
            default:
                assert(0);
        }
    }
    return result;
}

std::vector<DataList> VectorExampleSpace::getAllInput() {
    std::vector<DataList> result;
    DataList current;
    collectAllInput(0, type_list, current, result);
    return result;
}

z3::expr VectorExampleSpace::addDistinct(DataList &dataList, z3::context& ctx) {
    z3::expr_vector condition_list(ctx);
    for (int i = 0; i < type_list.size(); ++i) {
        condition_list.push_back(getParamVar(i, ctx) != dataList[i].getZ3Expr(ctx));
    }
    return z3::mk_or(condition_list);
}

z3::expr ListExampleSapce::getConstraint(z3::context &ctx) {
    return ctx.bool_val(true);
}

bool ListExampleSapce::equal(Program *program1, Program *program2) {
    for (auto& example: example_list) {
        if (!(program1->run(example.first) == program2->run(example.first))) {
            return false;
        }
    }
    return true;
}

DataList ListExampleSapce::randomSelect() {
    DataList result; int now = rand() % int(example_list.size());
    for (auto& data: example_list[now].first) {
        result.push_back(data);
    }
    return result;
}

std::vector<DataList> ListExampleSapce::getAllInput() {
    std::vector<DataList> result;
    for (auto& example: example_list) {
        result.push_back(example.first);
    }
    return result;
}

z3::expr ListExampleSapce::addDistinct(DataList &data, z3::context &ctx) {
    LOG(INFO) << "Do not support string" << std::endl;
    assert(0);
}

Data ListExampleSapce::getOutput(const DataList &inp) {
    for (auto& example: example_list) {
#ifdef DEBUG
        assert(inp.size() == example.first.size());
#endif
        if (inp == example.first) return example.second;
    }
    LOG(INFO) << "Unknown input" << std::endl;
    assert(0);
}

bool ListExampleSapce::checkCorrect(Program *result) {
    for (auto& example: example_list) {
        if (!(result->run(example.first) == example.second)) {
            return false;
        }
    }
    return true;
}

DataList ListExampleSapce::getCounterExample(Program *program1, Program *program2) {
    for (auto& example: example_list) {
        if (!(program1->run(example.first) == program2->run(example.first))) {
            return example.first;
        }
    }
    LOG(INFO) << "Cannot find counter example" << std::endl;
    assert(0);
}