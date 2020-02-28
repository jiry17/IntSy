//
// Created by pro on 2019/10/17.
//

#ifndef FLASHFILL_EXAMPLE_H
#define FLASHFILL_EXAMPLE_H

#include "z3++.h"
#include "value.h"
#include "specification.h"
#include "glog/logging.h"
#include "config.h"
#include <set>

#include <vector>

class ExampleSpace {
protected:
    z3::expr getVar(Type type, std::string name, z3::context& ctx) {
        switch (type) {
            case TINT: return ctx.int_const(name.c_str());
            case TBOOL: return ctx.bool_const(name.c_str());
            default:
                throw "Unknown type";
        }
    }
    Data getVariableData(Type type, std::string name, const z3::model& model, z3::context& ctx) {
        z3::expr var = getVar(type, name, ctx);
        auto value = model.eval(var);
        switch (type) {
            case TINT: {
                if (value.is_numeral()) {
                    return Data(new IntValue(value.get_numeral_int()));
                } else {
                    assert(var.to_string() == value.to_string());
                    return Data(new IntValue(rand() % (2 * KIntMax + 1) - KIntMax));
                }
            }
            case TBOOL: {
                if (value.is_bool()) {
                    return Data(new BoolValue(value.is_true()));
                } else {
                    assert(var.to_string() == value.to_string());
                    return Data(new BoolValue(rand() & 1));
                }
            }
            default:
                assert(0);
        }
    }
public:
    std::vector<Type> type_list;
    ExampleSpace(const std::vector<Type>& _type_list): type_list(_type_list) {}
    virtual Data getOutput(const DataList& inp) = 0;
    virtual bool checkCorrect(Program* result) = 0;
    virtual z3::expr getConstraint(z3::context& ctx) = 0;
    virtual std::vector<Data> getExampleFromModel(const z3::model& model, z3::context& ctx) = 0;
    virtual bool equal(Program* program1, Program* program2) = 0;
    virtual DataList randomSelect() = 0;
    virtual double getSize() {return 1e18;}
    virtual std::vector<DataList> getAllInput() {return {};}
    virtual z3::expr addDistinct(DataList& dataList, z3::context& ctx) = 0;
    virtual DataList getCounterExample(Program* sample1, Program* sample2) = 0;
};

class ListExampleSapce: public ExampleSpace {
    Type oup_type;

public:
    std::vector<std::pair<DataList, Data>> example_list;
    explicit ListExampleSapce(std::vector<Type>& _inp_type_list, Type _oup_type,
                              const std::vector<std::pair<DataList, Data>>& _example_list):
            ExampleSpace(_inp_type_list), oup_type(_oup_type) {
        std::set<DataList> S;
        for (auto& example: _example_list) {
            if (S.find(example.first) == S.end()) {
                S.insert(example.first);
                example_list.push_back(example);
            }
        }
    }

    Data getOutput(const DataList &inp);
    virtual z3::expr getConstraint(z3::context& ctx);
    virtual bool equal(Program* program1, Program* program2);
    virtual DataList randomSelect();
    virtual DataList getExampleFromModel(const z3::model& model, z3::context& ctx) {
        LOG(INFO) << "Do not support z3" << std::endl;
        assert(0);
    }
    virtual double getSize() {return example_list.size();}
    virtual std::vector<DataList> getAllInput();
    virtual z3::expr addDistinct(DataList& data, z3::context& ctx);
    virtual bool checkCorrect(Program* result);
    virtual DataList getCounterExample(Program* program1, Program* program2);
};

class VectorExampleSpace: public ExampleSpace {

    std::string getName(int k) {
        static char name[100];
        sprintf(name, "param@%d", k);
        return std::string(name);
    }

    z3::expr getParamVar(int k, z3::context& ctx) {
        std::string name = getName(k);
        return getVar(type_list[k], name, ctx);
    }

    int max_int;
public:
    Program* oracle;
    explicit VectorExampleSpace(const std::vector<Type>& _type_list, Program* _oracle, int _max_int = 10):
            ExampleSpace(_type_list), oracle(_oracle), max_int(_max_int) {}
    virtual z3::expr getConstraint(z3::context& ctx);
    Data getOutput(const DataList &inp) {return oracle->run(inp);}
    virtual bool equal(Program* progran1, Program* program2);
    virtual std::vector<Data> getExampleFromModel(const z3::model& model, z3::context& ctx);
    virtual DataList randomSelect();
    virtual double getSize() {
        double size = 1;
        for (auto type: type_list) {
            switch (type) {
                case TINT:
                    size *= (KIntMax * 2 + 1);
                    break;
                case TBOOL:
                    size *= 2;
                    break;
                case TSTRING:
                    LOG(INFO) << "Do not support string" << std::endl;
                    assert(0);
                default:
                    assert(0);
            }
        }
        return size;
    }
    virtual std::vector<DataList> getAllInput();

    virtual z3::expr addDistinct(DataList& dataList, z3::context& ctx);
    virtual bool checkCorrect(Program* result) {return equal(oracle, result);}
    virtual DataList getCounterExample(Program* sample1, Program* sample2);

};
#endif //FLASHFILL_EXAMPLE_H
