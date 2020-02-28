//
// Created by pro on 2019/10/22.
//

#include "smtlib_parser.h"
#include "../lib/config.h"
#include "../int/int_rule.h"
#include "../circuit/circuit_rule.h"
#include "../string/string_rule.h"
#include "../lib/log_util.h"

#include <unistd.h>
#include <map>
#include <queue>

namespace {
    std::map<std::string, FunctionSemantics*> function_semantics_map = {
            {"and", new CircuitAnd()}, {"or", new CircuitOr()}, {"xor", new CircuitXor()},
            {"not", new CircuitNot()}, {"+", new IntAdd()}, {"-", new IntMinus()},
            {"<", new IntLq()}, {">", new IntGq()}, {"<=", new IntLe()}, {">=", new IntGe()},
            {"=", new IntEq()}, {"ite", new IntIte()}, {"=b", new CircuitEq()},
            {"cons", new StringConcat()}, {"move", new StringIndexMove()}, {"substr", new StringSubstr()},
            {"index", new StringIndex()}, {"len", new StringLen()}, {"replace", new StringReplace()},
            {"delete", new StringDelete()}
    };

    std::string getTypeName(BenchmarkType benchmark_type) {
        switch (benchmark_type) {
            case BCIRCUIT: return "circuit";
            case BINT: return "int";
            case BSTRING: return "string";
            default: assert(0);
        }
    }

    void checkStatus(std::string output_name) {
        FILE* result_file = fopen((output_name + ".log").c_str(), "r");
        int result;
        fscanf(result_file, "%d", &result);
        assert(result == 0);
        fclose(result_file);
    }

    void callPythonParser(std::string file_name, BenchmarkType benchmark_type, std::string output_name) {
        system(("python3 " + KPythonPath + "main.py " + file_name + " " + getTypeName(benchmark_type) + " " + output_name).c_str());
        checkStatus(output_name);
    }

    void callRecommendSolver(std::string file_name, BenchmarkType benchmarkType, std::string output_name) {
        std::string recommend_solver_path;
        if (benchmarkType == BINT) {
            recommend_solver_path = KEusolverPath;
        } else {
            recommend_solver_path = KEuphonyPath;
        }
        std::string command = "python3 " + KPythonPath + "recommend.py " + file_name + " " + getTypeName(benchmarkType)
                + " " + output_name + " " + recommend_solver_path;
        system(command.c_str());
        checkStatus(output_name);
    }

    class DataTree {
    public:
        std::string data;
        std::vector<DataTree*> subtrees;
        ~DataTree() {
            for (auto* subtree: subtrees) delete subtree;
        }
        DataTree(FILE* file) {
            static char ch[1000];
            int size;
            fscanf(file, "%d\n", &size);
            if (size == 0) {
                fgets(ch, 900, file);
                int n = int(strlen(ch));
                assert(ch[n - 1] == '\n');
                ch[n - 1] = '\0';
                data = ch;
            } else {
                for (int i = 0; i < size; ++i) {
                    subtrees.push_back(new DataTree(file));
                }
            }
        }
    };

    std::string printTree(DataTree* root) {
        if (root->subtrees.empty()) {
            return root->data;
        } else {
            std::string name = " ";
            for (auto* sub_node: root->subtrees) {
                name += " " + printTree(sub_node);
            }
            name += ")";
            name[0] = '(';
            return name;
        }
    }

    DataTree* getResultRoot(std::string output_name) {
        FILE* parse_result = fopen((output_name + ".res").c_str(), "r");
        return new DataTree(parse_result);
    }

    Type getTypeFromString(const std::string& type_name) {
        if (type_name == "Bool") return TBOOL;
        else if (type_name == "Int") return TINT;
        else if (type_name == "String") return TSTRING;
        else assert(0);
    }

    std::vector<Type> parseTypeList(DataTree* data_root) {
        std::vector<Type> type_list;
        for (auto *subtree: data_root->subtrees) {
            type_list.push_back(getTypeFromString(subtree->data));
        }
        return type_list;
    }

    Data getDataFromInfo(std::string info) {
        assert(info[0] == '#');
        char type = info[1];
        std::string data = info.substr(2, info.size() - 2);
        switch (type) {
            case 'I': {
                int value = std::stoi(data);
                return Data(new IntValue(value));
            }
            case 'B': {
                bool value;
                if (data == "true") {
                    value = true;
                } else if (data == "false") {
                    value = false;
                } else assert(0);
                return Data(new BoolValue(value));
            }
            case 'S': {
                KIntMax = std::max(KIntMax, int(data.length()));
                return Data(new StringValue(data));
            }
            default:
                LOG(INFO) << "Unknown data type" << std::endl;
                assert(0);
        }
    }

    Semantics* getSemanticsFromName(std::string info, const std::vector<Type>& param_type_list){
        if (info[0] == '#') {
            char type = info[1];
            std::string data = info.substr(2, info.size() - 2);
            switch (type) {
                case 'I':
                case 'B':
                case 'S': {
                    Data semantic_value = getDataFromInfo(info);
                    if (semantic_value.getType() == TINT) {
                        KIntMax = std::max(KIntMax, std::abs(semantic_value.getInt()) + 1);
                    }
                    // std::cout << "Constant " << semantic_value.toString() << std::endl;
                    return new ValueSemantics(semantic_value);
                }
                case 'P': {
                    int id = std::stoi(data);
                    return new ParamSemantics(id, param_type_list[id]);
                }
                default: assert(0);
            }
        } else {
            assert(function_semantics_map.count(info));
            return function_semantics_map[info];
        }
    }

    Program* parseProgram(DataTree* data_root, const std::vector<Type>& param_type_list) {
        if (data_root->subtrees.empty()) {
            Semantics* result = getSemanticsFromName(data_root->data, param_type_list);
#ifdef DEBUG
            assert(!dynamic_cast<FunctionSemantics*>(result));
#endif
            return new Program(result, {});
        } else {
#ifdef DEBUG
            assert(data_root->subtrees[0]->subtrees.size() == 0);
#endif
            Semantics* semantics = getSemanticsFromName(data_root->subtrees[0]->data, param_type_list);
#ifdef DEBUG
            assert(dynamic_cast<FunctionSemantics*>(semantics));
#endif
            std::vector<Program*> sub_program_list;
            for (int i = 1; i < data_root->subtrees.size(); ++i) {
                sub_program_list.push_back(parseProgram(data_root->subtrees[i], param_type_list));
            }
            return new Program(semantics, sub_program_list);
        }
    }

    NonTerminal getNonTerminal(const std::vector<NonTerminal>& symbol_list, std::string name) {
        for (int i = 0; i < symbol_list.size(); ++i) {
            if (symbol_list[i].name == name) {
                return symbol_list[i];
            }
        }
        assert(0);
    }

    std::vector<NonTerminal> parseSymbolList(DataTree* root) {
        std::vector<NonTerminal> result;
        for (auto* subtree: root->subtrees) {
#ifdef DEBUG
            assert(subtree->subtrees.size() > 0 && subtree->subtrees[0]->subtrees.size() == 0 && subtree->subtrees[1]->subtrees.size() == 0);
#endif
            std::string symbol_name = subtree->subtrees[0]->data;
            std::string type_name = subtree->subtrees[1]->data;
            int current_id = int(result.size());
            result.push_back(NonTerminal(symbol_name, getTypeFromString(type_name), current_id));
        }
        return result;
    }

    std::vector<RuleList> parseRuleList(DataTree* root, const std::vector<NonTerminal>& symbol_list, const std::vector<Type>& type_list) {
        std::vector<RuleList> result;
        for (auto* symbol_node: root->subtrees) {
            RuleList rule_list;
            NonTerminal oup_symbol = getNonTerminal(symbol_list, symbol_node->subtrees[0]->data);
            assert(symbol_node->subtrees.size() == 3);
            auto* rule_list_node = symbol_node->subtrees[2];
            for (auto* rule_node: rule_list_node->subtrees) {
                if (rule_node->subtrees.size() > 0) {
#ifdef DEBUG
                    for (auto *element_node: rule_node->subtrees) {
                        assert(element_node->subtrees.empty());
                    }
#endif
                    std::string semantics_name = rule_node->subtrees[0]->data;
                    std::vector<NonTerminal> inp_symbol;
                    for (int i = 1; i < rule_node->subtrees.size(); ++i) {
                        inp_symbol.push_back(getNonTerminal(symbol_list, rule_node->subtrees[i]->data));
                    }
                    Semantics *semantics = getSemanticsFromName(semantics_name, type_list);
                    rule_list.emplace_back(Rule(oup_symbol, inp_symbol, semantics));
                } else {
                    std::string info = rule_node->data;
                    bool is_direct_rule = false;
                    for (auto& symbol: symbol_list) {
                        if (symbol.name == info) {
                            is_direct_rule = true;
                            //std::cout << "Add direct rule " << oup_symbol.name << " " << info << std::endl;
                            rule_list.emplace_back(Rule(oup_symbol, {symbol}, new DirectSemantics(oup_symbol.type)));
                            break;
                        }
                    }
                    if (is_direct_rule) continue;
                    std::string semantics_name = info;
                    rule_list.emplace_back(Rule(oup_symbol, {}, getSemanticsFromName(semantics_name, type_list)));
                }
            }
            result.emplace_back(rule_list);
        }
        return result;
    }

    int getDepthLimit(const std::vector<NonTerminal>& symbol_list, const std::vector<RuleList>& rule_list, int start_id) {
        int n = int(symbol_list.size());
        int* d = new int[n];
        for (int i = 0; i < n; ++i) d[i] = 0;
        for (const RuleList& rules: rule_list) {
            for (const Rule& rule: rules) {
                for (auto non_terminal: rule.inp_symbol) {
                    d[non_terminal.symbol_id] += 1;
                }
            }
        }
        std::vector<int> x;
        std::queue<int> Q;
        if (d[start_id] == 0) Q.push(start_id);
        for (int i = 1; i <= n; ++i) {
            if (Q.empty()) {
                delete[] d;
                return KMaxDepth;
            }
            int k = Q.front(); Q.pop(); x.push_back(k);
            for (const Rule& rule: rule_list[k]) {
                for (auto non_terminal: rule.inp_symbol) {
                    --d[non_terminal.symbol_id];
                    if (d[non_terminal.symbol_id] == 0) {
                        Q.push(non_terminal.symbol_id);
                    }
                }
            }
        }
        for (int i = 0; i < n; ++i) d[i] = 0;
        for (int i = int(x.size()) - 1; i >= 0; --i) {
            for (const Rule& rule: rule_list[x[i]]) {
                for (auto non_terminal: rule.inp_symbol) {
                    d[x[i]] = std::max(d[x[i]], d[non_terminal.symbol_id]);
                }
            }
            ++d[x[i]];
        }
        int ans = d[start_id];
        delete[] d;
        return ans;
    }

    typedef std::pair<DataList, Data> Example;
    typedef std::vector<Example> ExampleList;

    Example parseExample(DataTree* data_root) {
        assert(data_root->subtrees.size() == 2);
        DataTree* input_node = data_root->subtrees[0];
        DataTree* output_node = data_root->subtrees[1];
        DataList input;
        for (auto* child: input_node->subtrees) {
            assert(child->subtrees.empty());
            input.push_back(getDataFromInfo(child->data));
        }
        assert(output_node->subtrees.empty());
        Data output = getDataFromInfo(output_node->data);
        return std::make_pair(input, output);
    }

    ExampleList parseExampleList(DataTree* data_root) {
        ExampleList result;
        for (auto* child: data_root->subtrees) {
            result.push_back(parseExample(child));
        }
        return result;
    }

    Specification* buildSpec(DataTree* spec_root, std::vector<Type>& inp_type_list, BenchmarkType benchmark_type) {
        std::vector<NonTerminal> symbol_list = parseSymbolList(spec_root);
        std::vector<RuleList> rule_list = parseRuleList(spec_root, symbol_list, inp_type_list);
        NonTerminal start_symbol = getNonTerminal(symbol_list, "Start");
        int depth_limit = getDepthLimit(symbol_list, rule_list, start_symbol.symbol_id);
        return new Specification(symbol_list, rule_list, start_symbol, depth_limit, benchmark_type);
    }

    std::pair<Specification*, ExampleSpace*> parseOracle(DataTree* data_root, BenchmarkType benchmark_type) {
        assert(data_root->subtrees.size() == 3);
        std::vector<Type> inp_type_list = parseTypeList(data_root->subtrees[0]);
        Program* oracle = parseProgram(data_root->subtrees[1], inp_type_list);
        Specification* spec = buildSpec(data_root->subtrees[2], inp_type_list, benchmark_type);
        return std::make_pair(spec, new VectorExampleSpace(inp_type_list, oracle, KIntMax));
    }

    DataList parseDataList(DataTree* data_root) {
        DataList result;
        assert(!data_root->subtrees.empty());
        for (DataTree* sub_node: data_root->subtrees) {
            assert(sub_node->subtrees.empty());
            std::string data = sub_node->data;
            assert(data[0] == '#' && data[1] == 'S');
            std::string current_str = data.substr(2, data.length() - 2);
            // std::cout << current_str << std::endl;
            result.push_back(Data(new StringValue(current_str)));
        }
        return result;
    }

    std::pair<Specification*, ExampleSpace*> parseExampleListBenchmark(DataTree* data_root, BenchmarkType benchmark_type) {
        assert(data_root->subtrees.size() == 5);
        std::vector<Type> inp_type_list = parseTypeList(data_root->subtrees[0]);

        DataTree *oup_type_node = data_root->subtrees[1];
        assert(oup_type_node->subtrees.empty());
        Type output_type = getTypeFromString(oup_type_node->data);

        ExampleList example_list = parseExampleList(data_root->subtrees[2]);
#ifdef DEBUG
        for (auto &example: example_list) {
            assert(example.first.size() == inp_type_list.size());
            assert(example.second.getType() == output_type);
            for (int i = 0; i < example.first.size(); ++i) {
                Data info = example.first[i];
                assert(info.getType() == inp_type_list[i]);
            }
        }
#endif

        Specification *spec = buildSpec(data_root->subtrees[3], inp_type_list, benchmark_type);
        ListExampleSapce *example_space = new ListExampleSapce(inp_type_list, output_type, example_list);
        DataList constant_list = parseDataList(data_root->subtrees[4]);
        string_constant_list.clear();
        for (auto& constant: constant_list) {
            string_constant_list.push_back(constant.getString());
        }
        return std::make_pair(spec, example_space);
    }
}

std::pair<Specification*, ExampleSpace*> SMTLibParser::parseFile(std::string file_name, BenchmarkType benchmark_type) {
    std::string output_name = std::to_string(random());
    callPythonParser(file_name, benchmark_type, output_name);
    DataTree* data_root = getResultRoot(output_name);
    std::pair<Specification*, ExampleSpace*> result;
    switch (benchmark_type) {
        case BINT:
        case BCIRCUIT:
            result = parseOracle(data_root, benchmark_type);
            break;
        case BSTRING:
            result = parseExampleListBenchmark(data_root, benchmark_type);
            break;
        default: assert(0);
    }
    delete data_root;
    std::remove((output_name + ".res").c_str());
    std::remove((output_name + ".log").c_str());
    return result;
}

Program* RecommendParser::getRecommend(std::string file_name, BenchmarkType benchmarkType,
                                       std::vector<std::pair<DataList, Data> > examples, ExampleSpace* example_space) {
    std::string output_name = std::to_string(rand());
    FILE* oup_file = fopen((output_name + ".in").c_str(), "w");
    for (auto& example: examples) {
        fprintf(oup_file, "%s\n", LogUtil::exampleToString(example).c_str());
    }
    fclose(oup_file);
    callRecommendSolver(file_name, benchmarkType, output_name);
    DataTree* result_root = getResultRoot(output_name);
    Program* result = parseProgram(result_root, example_space->type_list);
    z3::context con;
    LOG(INFO) << "Finish get recommend " << result->parseProgrm(con) << std::endl;
    delete result_root;
    std::remove((output_name + ".in").c_str());
    std::remove((output_name + ".res").c_str());
    std::remove((output_name + ".log").c_str());
    return result;
}