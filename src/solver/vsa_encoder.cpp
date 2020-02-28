//
// Created by jiry on 19-10-29.
//

#include "vsa_encoder.h"
#include <set>

typedef std::unordered_map<int, int> Tinfo;

namespace {
    void collectAllNode(VSANode* root, EdgeList& node_list, std::vector<int>& tag) {
        if (tag[root->temp_id] != -1) {
            return;
        }
        tag[root->temp_id] = 0;
        if (root->depth >= node_list.size()) {
            assert(root->depth == node_list.size());
            node_list.push_back({});
        }
        node_list[root->depth].push_back(root);
        for (auto& edge_list: root->children) {
            for (auto& edge: edge_list) {
                for (VSANode* sub_node: edge) {
                    collectAllNode(sub_node, node_list, tag);
                }
            }
        }
    }

    void mergeInfo(Tinfo* result, std::vector<Tinfo*> info_list) {
        static Tinfo current;
        current.clear();
        for (auto* info: info_list) {
            for (auto data_pair: *info) {
                current[data_pair.first] += data_pair.second;
            }
        }
        for (auto data_pair: current) {
            (*result)[data_pair.first] = std::max((*result)[data_pair.first], data_pair.second);
        }
    }

    z3::expr getVar(std::string name, Type type, z3::context& ctx) {
        switch (type) {
            case TINT: return ctx.int_const(name.c_str());
            case TBOOL: return ctx.bool_const(name.c_str());
            default: assert(0);
        }
    }

    Data parseDataFromModel(z3::expr var, Type type, const z3::model& model) {
        // std::cout << var << " " << model << std::endl;
        switch (type) {
            case TINT: return Data(new IntValue(model.eval(var).get_numeral_int()));
            case TBOOL: return Data(new BoolValue(model.eval(var).is_true()));
            default:
                assert(0);
        }
    }

    z3::expr getVarForNode(VSANode* node, int pos, z3::context& ctx) {
        static char ch[1000];
        sprintf(ch, "result@%d,%d", node->node_id, pos);
        return getVar(std::string(ch), node->symbol.type, ctx);
    }

    z3::expr getTempVar(VSANode* node, int ti, int rule_id, int edge_id, VSANode* sub_node, z3::context& ctx) {
        static char ch[1000];
        sprintf(ch, "temp@%d,%d,%d,%d,%d", node->node_id, ti, rule_id, edge_id, sub_node->node_id);
        return getVar(std::string(ch), sub_node->symbol.type, ctx);
    }

    z3::expr_vector getAllOup(VSANode* node, int num, z3::context& ctx) {
        z3::expr_vector oup_list(ctx);
        for (int i = 0; i < num; ++i) oup_list.push_back(getVarForNode(node, i, ctx));
        return oup_list;
    }

    bool checkContain(const z3::model& model, z3::expr& var, Type type) {
        switch (type) {
            case TINT: return model.eval(var).is_numeral();
            case TBOOL: return model.eval(var).is_bool();
            default: assert(0);
        }
    }

    z3::expr getASTVar(VSANode* vsa_node, ASTNode* ast_node, z3::context& ctx) {
        static char ch[1000];
        sprintf(ch, "ASTValue@%d,%d", vsa_node->node_id, ast_node->node_id);
        return getVar(std::string(ch), vsa_node->symbol.type, ctx);
    }

    void matchVSAandAST(VSANode* vsa_node, ASTNode* ast_node, std::set<std::pair<int,int> >& memory,
                        z3::context& ctx, z3::solver& solver) {
        auto state = std::make_pair(vsa_node->node_id, ast_node->node_id);
        if (memory.find(state) != memory.end()) return;
        memory.insert(state);
        auto result_var = getASTVar(vsa_node, ast_node, ctx);
        z3::expr_vector possible_list(ctx);
        int rule_id = -1;
        RuleList& rule_list = vsa_node->spec->rule_list[vsa_node->symbol.symbol_id];
        for (auto& edge_list: vsa_node->children) {
            ++rule_id;
            Rule& rule = rule_list[rule_id];
            for (auto& edge: edge_list) {
                std::map<Type, int> count;
                std::vector<z3::expr> inp_list;
                for (VSANode* sub_vsa_node: edge) {
                    int current_id = count[sub_vsa_node->symbol.type];
                    count[sub_vsa_node->symbol.type] += 1;
                    ASTNode* sub_ast_node = ast_node->getSubNode(sub_vsa_node->symbol.type, current_id);
                    matchVSAandAST(sub_vsa_node, sub_ast_node, memory, ctx, solver);
                    inp_list.push_back(getASTVar(sub_vsa_node, sub_ast_node, ctx));
                }
                possible_list.push_back(result_var == rule.semantics->encodeZ3Expr(ctx, inp_list));
            }
        }
        assert(possible_list.size() > 0);
        solver.add(z3::mk_or(possible_list));
    }

    Program* recursivelyBuildProgram(VSANode* vsa_node, ASTNode* ast_node, const z3::model& model, z3::context& ctx,
                                     const DataList& example) {
        auto result_var = getASTVar(vsa_node, ast_node, ctx);
        auto result_type = vsa_node->symbol.type;
        assert(checkContain(model, result_var, vsa_node->symbol.type));
        Data result_value = parseDataFromModel(result_var, result_type, model);
        RuleList& rule_list = vsa_node->spec->rule_list[vsa_node->symbol.symbol_id];
        int rule_id = -1;
        for (auto& edge_list: vsa_node->children) {
            ++rule_id;
            Rule& rule = rule_list[rule_id];
            for (auto& edge: edge_list) {
                std::vector<Data> inp_list;
                std::map<Type, int> count;
                bool is_valid = true;
                for (VSANode* sub_vsa_node: edge) {
                    auto sub_type = sub_vsa_node->symbol.type;
                    ASTNode* sub_ast_node = ast_node->getSubNode(sub_type, count[sub_type]);
                    count[sub_type] += 1;
                    auto sub_var = getASTVar(sub_vsa_node, sub_ast_node, ctx);
                    if (!checkContain(model, sub_var, sub_type)) {
                        is_valid = false;
                        break;
                    }
                    inp_list.push_back(parseDataFromModel(sub_var, sub_type, model));
                }
                if (is_valid && rule.run(inp_list, example) == result_value) {
                    std::vector<Program*> program_list;
                    count.clear();
                    for (VSANode* sub_vsa_node: edge) {
                        auto sub_type = sub_vsa_node->symbol.type;
                        ASTNode* sub_ast_node = ast_node->getSubNode(sub_type, count[sub_type]);
                        count[sub_type] += 1;
                        program_list.push_back(recursivelyBuildProgram(sub_vsa_node, sub_ast_node, model, ctx, example));
                    }
                    return new Program(rule.semantics, program_list);
                }
            }
        }
        assert(0);
    }

    std::string getNameWithPrefix(std::string prefix, std::vector<int> ids) {
        prefix += "@";
        for (int i = 0; i < ids.size(); ++i) {
            if (i) prefix += ",";
            prefix += std::to_string(ids[i]);
        }
        return prefix;
    }

    z3::expr getStructureVar(TreeNode* node, int rule_id, z3::context& ctx) {
        std::string name = getNameWithPrefix("Structure", {node->id, rule_id});
        return ctx.bool_const(name.c_str());
    }

    z3::expr getVarByNameAndType(std::string name, Type type, z3::context& ctx) {
        switch (type) {
            case TINT: return ctx.int_const(name.c_str());
            case TBOOL: return ctx.bool_const(name.c_str());
            case TSTRING:
                LOG(INFO) << "String type can not be used in z3" << std::endl;
                assert(0);
            default:
                LOG(INFO) << "Unknown type" << std::endl;
                assert(0);
        }
    }

    z3::expr getTempVar(Type type, z3::context& ctx) {
        static int temp_id = 0;
        ++temp_id;
        std::string name = getNameWithPrefix("Temp", {temp_id});
        return getVarByNameAndType(name, type, ctx);
    }

    z3::expr encodeOnlyOne(std::vector<z3::expr>::iterator l, std::vector<z3::expr>::iterator r,
                           z3::context& ctx, z3::solver& solver) {
        if (r == l + 1) {
            return *l;
        }
        int size = int(r - l) / 2;
        std::vector<z3::expr>::iterator mid = l + size;
        z3::expr l_result = encodeOnlyOne(l, mid, ctx, solver);
        z3::expr r_result = encodeOnlyOne(mid, r, ctx, solver);
        z3::expr temp = getTempVar(TBOOL, ctx);
        solver.add(!(l_result && r_result));
        solver.add(temp == (l_result || r_result));
        return temp;
    }

    void addStructureConstraint(TreeNode* node, z3::solver &solver, z3::context &ctx) {
        Specification* spec = node->spec;
        RuleList& rule_list = spec->rule_list[node->symbol.symbol_id];
        std::vector<z3::expr> structure_var_list;
        for (int rule_id = 0; rule_id < rule_list.size(); ++rule_id) {
            if (!node->is_valid[rule_id]) continue;
            structure_var_list.push_back(getStructureVar(node, rule_id, ctx));
        }
        solver.add(encodeOnlyOne(structure_var_list.begin(), structure_var_list.end(), ctx, solver));
        for (auto& pair: node->subtree) {
            for (TreeNode* sub_node: pair.second) {
                addStructureConstraint(sub_node, solver, ctx);
            }
        }
    }

    z3::expr getExampleOup(TreeNode* node, int example_id, z3::context &ctx) {
        std::string name = getNameWithPrefix("ExampleOutput", {node->id, example_id});
        return getVarByNameAndType(name, node->symbol.type, ctx);
    }

    void encodeExample(TreeNode* node, z3::solver &solver, z3::context &ctx, int example_id, const DataList& param) {
        z3::expr oup_var = getExampleOup(node, example_id, ctx);
        Specification* spec = node->spec;
        RuleList& rule_list = spec->rule_list[node->symbol.symbol_id];
        for (int rule_id = 0; rule_id < rule_list.size(); ++rule_id) {
            if (!node->is_valid[rule_id]) continue;
            Rule& rule = rule_list[rule_id];
            z3::expr structure_var = getStructureVar(node, rule_id, ctx);
            ParamSemantics* param_semantics = dynamic_cast<ParamSemantics*>(rule.semantics);
            if (param_semantics) {
                solver.add(z3::implies(structure_var, oup_var == param_semantics->encodeZ3ExprWithExample(ctx, param)));
            } else {
                std::vector<TreeNode *> sub_expr_node = node->getSubnodeByRule(rule);
                std::vector<z3::expr> sub_expr_list;
                for (TreeNode *sub_node: sub_expr_node) {
                    sub_expr_list.push_back(getExampleOup(sub_node, example_id, ctx));
                }
                solver.add(z3::implies(structure_var, oup_var == rule.encodeZ3Expr(ctx, sub_expr_list)));
            }
        }
        for (auto& pair: node->subtree) {
            for (TreeNode* tree_node: pair.second) {
                encodeExample(tree_node, solver, ctx, example_id, param);
            }
        }
    }

    void recursivelyEncodeTree(TreeNode* node, z3::solver& solver, z3::context& ctx) {
        z3::expr oup_var = getExampleOup(node, -1, ctx);
        Specification* spec = node->spec;
        RuleList& rule_list = spec->rule_list[node->symbol.symbol_id];
        for (int rule_id = 0; rule_id < rule_list.size(); ++rule_id) {
            if (!node->is_valid[rule_id]) continue;
            Rule& rule = rule_list[rule_id];
            z3::expr structure_var = getStructureVar(node, rule_id, ctx);
            std::vector<TreeNode*> sub_expr_node = node->getSubnodeByRule(rule);
            std::vector<z3::expr> sub_expr_list;
            for (TreeNode* sub_node: sub_expr_node) {
                sub_expr_list.push_back(getExampleOup(sub_node, -1, ctx));
            }
            solver.add(z3::implies(structure_var, oup_var == rule.encodeZ3Expr(ctx, sub_expr_list)));
        }
        for (auto& pair: node->subtree) {
            for (TreeNode* tree_node: pair.second) {
                recursivelyEncodeTree(tree_node, solver, ctx);
            }
        }
    }

    Program* secondOrderBuilder(TreeNode* node, const z3::model& model, z3::context& ctx) {
        Specification* spec = node->spec;
        RuleList& rule_list = spec->rule_list[node->symbol.symbol_id];
        int where = -1;
        for (int i = 0; i < rule_list.size(); ++i) {
            if (!node->is_valid[i]) continue;
            z3::expr structure_var = getStructureVar(node, i, ctx);
            bool is_selected = parseDataFromModel(structure_var, TBOOL, model).getBool();
            if (is_selected) {
                assert(where == -1);
                where = i;
            }
        }
        assert(where != -1);
        Rule& rule = rule_list[where];
        std::vector<TreeNode*> sub_node_list = node->getSubnodeByRule(rule);
        std::vector<Program*> sub_program;
        for (TreeNode* sub_node: sub_node_list) {
            sub_program.push_back(secondOrderBuilder(sub_node, model, ctx));
        }
        return new Program(rule.semantics, sub_program);
    }

    void collectDifferentConstraint(TreeNode* node, const z3::model& model, z3::context& ctx, z3::expr_vector& result) {
        int chosen_rule = -1;
        for (int i = 0; i < node->is_valid.size(); ++i) {
            if (!node->is_valid[i]) continue;
            z3::expr structure_var = getStructureVar(node, i, ctx);
            if (parseDataFromModel(structure_var, TBOOL, model).getBool()) {
                assert(chosen_rule == -1);
                chosen_rule = i;
                result.push_back(!structure_var);
            }
        }
        assert(chosen_rule != -1);
        Rule& rule = node->spec->rule_list[node->symbol.symbol_id][chosen_rule];
        std::vector<TreeNode*> sub_node_list = node->getSubnodeByRule(rule);
        for (auto* sub_node: sub_node_list) {
            collectDifferentConstraint(sub_node, model, ctx, result);
        }
    }
}

void VSAEncoder::initialize() {
    int n = 0;
    root->clearTempId();
    root->assignTempId(n);
    auto **info_pool = new Tinfo *[n];
    for (int i = 0; i < n; ++i) info_pool[i] = new Tinfo();
    std::vector<int> tag(n);
    for (int i = 0; i < n; ++i) tag[i] = -1;
    collectAllNode(root, node_list, tag);
    assert(!node_list.empty() && node_list[0].size() == 1);
    int max_depth = int(node_list.size());
    for (int current_depth = max_depth - 1; current_depth >= 0; --current_depth) {
        for (VSANode* node: node_list[current_depth]) {
            for (auto& edge_list: node->children) {
                for (auto& edge: edge_list) {
                    std::vector<Tinfo*> info_list;
                    for (VSANode* sub_node: edge) {
                        info_list.push_back(info_pool[sub_node->temp_id]);
                    }
                    mergeInfo(info_pool[node->temp_id], info_list);
                }
            }
            (*info_pool[node->temp_id])[node->node_id] = 1;
        }

        if (current_depth + 1 < max_depth) {
            for (VSANode* node: node_list[current_depth + 1]) {
                delete info_pool[node->temp_id];
            }
        }
    }
    int num = 0, sum = 0, max_t = 0;
    for (auto data_pair: *info_pool[0]) {
        t[data_pair.first] = data_pair.second;
        ++num; sum += data_pair.second; max_t = std::max(max_t, data_pair.second);
    }
    std::cout << "average span " << 1.0 * sum / num << " max: " << max_t << std::endl;
    // release memory
    delete info_pool[0];
    delete[] info_pool;
}

z3::expr VSAEncoder::getZ3Expr(z3::context &ctx, z3::solver &solver) {
    int max_depth = int(node_list.size());
    for (int depth = max_depth - 1; depth >= 0; --depth) {
        for (VSANode* node: node_list[depth]) {
            int total = t[node->node_id];
            // std::cout << depth << " " << node->node_id << " " << total << std::endl;
            for (int id = 0; id < total; ++id) {
                z3::expr_vector possible_list(ctx);
                int list_id = -1;
                z3::expr result = getVarForNode(node, id, ctx);
                for (auto &edge_list: node->children) {
                    ++list_id;
                    int edge_id = -1;
                    for (auto &edge: edge_list) {
                        std::vector<z3::expr> inp_list;
                        ++edge_id;
                        int sub_node_id = -1;
                        for (VSANode *sub_node: edge) {
                            ++sub_node_id;
                            z3::expr_vector oup_list = getAllOup(sub_node, t[sub_node->node_id], ctx);
                            z3::expr temp = getTempVar(node, id, list_id, edge_id, sub_node, ctx);
                            // std::cout << oup_list << " "<< temp << std::endl;
                            z3::expr_vector new_list(ctx);
                            for (int i = 0; i < oup_list.size(); ++i) {
                                new_list.push_back(oup_list[i] == temp);
                            }
                            // std::cout << "Add " << z3::mk_or(new_list) << std::endl;
                            solver.add(z3::mk_or(new_list));
                            inp_list.push_back(temp);
                        }
                        possible_list.push_back(result == node->spec->rule_list[
                                node->symbol.symbol_id][list_id].semantics->encodeZ3Expr(ctx, inp_list));
                    }
                }
                solver.add(z3::mk_or(possible_list));
            }
        }
    }
    return getVarForNode(root, 0, ctx);
}

Program* VSAEncoder::recursivelyParseProgram(
        VSANode* node, int ti, const z3::model& model, z3::context& ctx, const DataList& example) {
    auto result_var = getVarForNode(node, ti, ctx);
    assert(checkContain(model, result_var, node->symbol.type));
    Data result_value = parseDataFromModel(result_var, node->symbol.type, model);
    Specification* spec = node->spec;
    for (int rule_id = 0; rule_id < node->children.size(); ++rule_id) {
        auto& edge_list = node->children[rule_id];
        Rule& rule = spec->rule_list[node->symbol.symbol_id][rule_id];
        for (int edge_id = 0; edge_id < edge_list.size(); ++edge_id) {
            auto& edge = edge_list[edge_id];
            DataList temp_inp;
            bool is_valid = true;
            for (int node_id = 0; node_id < edge.size(); ++node_id) {
                VSANode* sub_node = edge[node_id];
                auto temp_var = getTempVar(node, ti, rule_id, edge_id, sub_node, ctx);
                if (!checkContain(model, temp_var, sub_node->symbol.type)) {
                    is_valid = false;
                    break;
                }
                Data temp_value = parseDataFromModel(temp_var, sub_node->symbol.type, model);
                temp_inp.push_back(temp_value);
            }
            if (!is_valid) continue;
            if (rule.run(temp_inp, example) == result_value) {
                std::vector<Program*> sub_program;
                for (int node_id = 0; node_id < edge.size(); ++node_id) {
                    Data temp_value = temp_inp[node_id];
                    VSANode* sub_node = edge[node_id];
                    int occur_time = t[sub_node->node_id];
                    int is_satisfy = false;
                    for (int i = 0; i < occur_time; ++i) {
                        auto sub_var = getVarForNode(sub_node, i, ctx);
                        if (!checkContain(model, sub_var, sub_node->symbol.type)) continue;
                        if (parseDataFromModel(sub_var, sub_node->symbol.type, model) == temp_value) {
                            sub_program.push_back(recursivelyParseProgram(sub_node, i, model, ctx, example));
                            is_satisfy = true;
                            break;
                        }
                    }
                    assert(is_satisfy);
                }
                return new Program(rule.semantics, sub_program);
            }
        }
    }
    assert(0);
}

Program* VSAEncoder::getProgramFromModel(const z3::model &model, z3::context &ctx, const DataList& example) {
    return recursivelyParseProgram(root, 0, model, ctx, example);
}

int encoder_ast_id = 0;

z3::expr TreeEncoder::getZ3Expr(z3::context &ctx, z3::solver &solver) {
    std::set<std::pair<int, int> > memory;
    matchVSAandAST(root, ast_root, memory, ctx, solver);
    return getASTVar(root, ast_root, ctx);
}

Program* TreeEncoder::getProgramFromModel(const z3::model &model, z3::context &ctx, const DataList &example) {
    return recursivelyBuildProgram(root, ast_root, model, ctx, example);
}

void SecondOrderEncoder::buildInitTree(TreeNode *ast_node, VSANode* vsa_node, std::set<std::pair<int,int> >& cache) {
    if (cache.find(std::make_pair(ast_node->id, vsa_node->node_id)) != cache.end()) {
        return;
    }
    assert(ast_node->symbol.symbol_id == vsa_node->symbol.symbol_id);
    cache.insert(std::make_pair(ast_node->id, vsa_node->node_id));
    int rule_id = -1;
    for (auto& edge_list: vsa_node->children) {
        ++rule_id;
        if (edge_list.size() > 0) {
            ast_node->is_valid[rule_id] = true;
        }
        for (auto& edge: edge_list) {
            std::map<int, int> count;
            for (VSANode* sub_node: edge) {
                int symbol_id = sub_node->symbol.symbol_id;
                int current_id = count[symbol_id]++;
                TreeNode* tree_sub_node = ast_node->getTreeNodeWithVSA(current_id, sub_node);
                buildInitTree(tree_sub_node, sub_node, cache);
            }
        }
    }
}

void SecondOrderEncoder::addConstraint(z3::solver &solver, z3::context& ctx,
                                       const std::vector<std::pair<DataList, Data>> &example_list) {
    addStructureConstraint(ast_root, solver, ctx);
    int example_id = -1;
    for (auto& example: example_list) {
        ++example_id;
        encodeExample(ast_root, solver, ctx, example_id, example.first);
        solver.add(getExampleOup(ast_root, example_id, ctx) == example.second.getZ3Expr(ctx));
    }
}

z3::expr SecondOrderEncoder::encodeTree(z3::solver &solver, z3::context &ctx) {
    recursivelyEncodeTree(ast_root, solver, ctx);
    return getExampleOup(ast_root, -1, ctx);
}

Program* SecondOrderEncoder::getProgramFromModel(const z3::model &model, z3::context &ctx) {
    return secondOrderBuilder(ast_root, model, ctx);
}

void SecondOrderEncoder::addDifferent(const z3::model &model, z3::solver &solver, z3::context &ctx) {
    z3::expr_vector result(ctx);
    collectDifferentConstraint(ast_root, model, ctx, result);
    solver.add(z3::mk_or(result));
}