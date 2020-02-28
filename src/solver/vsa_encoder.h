//
// Created by jiry on 19-10-29.
//

#ifndef FLASHFILL_VSAENCODEUTIL_H
#define FLASHFILL_VSAENCODEUTIL_H

#include "../lib/specification.h"
#include "../lib/vsa.h"
#include <unordered_map>

class VSAEncoder {
    VSANode* root;
    EdgeList node_list;
    std::unordered_map<int, int> t;
    void initialize();
    Program* recursivelyParseProgram(VSANode* node, int ti, const z3::model& model, z3::context& ctx, const DataList& example);
public:
    VSAEncoder(VSANode* _root): root(_root) {
        initialize();
    }

    z3::expr getZ3Expr(z3::context& ctx, z3::solver& solver);

    Program* getProgramFromModel(const z3::model& model, z3::context& ctx, const DataList& example);
};

extern int encoder_ast_id;

class ASTNode {
    std::map<Type, std::vector<ASTNode*> > sub_tree;
public:
    int node_id;
    ASTNode(): node_id(encoder_ast_id++) {}
    ~ASTNode() {
        for (auto pair: sub_tree) {
            for (ASTNode* sub_node: pair.second) {
                delete sub_node;
            }
        }
    }
    ASTNode* getSubNode(Type type, int pos) {
        if (sub_tree[type].size() <= pos) {
            auto* sub_node = new ASTNode();
            sub_tree[type].push_back(sub_node);
            return sub_node;
        }
        return sub_tree[type][pos];
    }
};

class TreeEncoder {
    VSANode* root;
    ASTNode* ast_root;
public:
    TreeEncoder(VSANode* _root): root(_root), ast_root(new ASTNode()) {}

    z3::expr getZ3Expr(z3::context& ctx, z3::solver& solver);

    Program* getProgramFromModel(const z3::model& model, z3::context& ctx, const DataList& example);

    ~TreeEncoder() {delete ast_root;}
};

class TreeNode {
public:
    NonTerminal symbol;
    Specification* spec;
    int id;
    std::map<int, std::vector<TreeNode*> > subtree;
    std::vector<bool> is_valid;

    TreeNode(VSANode* node): id(encoder_ast_id++), spec(node->spec), symbol(node->symbol) {
        int rule_num = int(spec->rule_list[symbol.symbol_id].size());
        for (int i = 0; i < rule_num; ++i) {
            is_valid.push_back(false);
        }
    }
    ~TreeNode() {
        for (auto pair: subtree) {
            for (TreeNode* sub_node: pair.second) {
                delete sub_node;
            }
        }
    }

    TreeNode* getTreeNodeWithVSA(int id, VSANode* node) {
        int symbol_id = node->symbol.symbol_id;
#ifdef DEBUG
        assert(subtree[symbol_id].size() >= id);
#endif
        if (subtree[symbol_id].size() == id) {
            TreeNode* new_node = new TreeNode(node);
            subtree[symbol_id].push_back(new_node);
            return new_node;
        } else {
            return subtree[symbol_id][id];
        }
    }

    std::vector<TreeNode*> getSubnodeByRule(const Rule& rule) {
        std::vector<TreeNode*> result;
        std::map<int, int> symbol_count;
        for (auto& sub_symbol: rule.inp_symbol) {
            int sub_id = sub_symbol.symbol_id;
            int current = symbol_count[sub_id]++;
            assert(subtree[sub_id].size() > current);
            result.push_back(subtree[sub_id][current]);
        }
        return result;
    }
};

class SecondOrderEncoder {
    TreeNode* ast_root;
    void buildInitTree(TreeNode* ast_node, VSANode* vsa_list, std::set<std::pair<int, int>>& cache);
public:
    SecondOrderEncoder(VSANode* root): ast_root(new TreeNode(root)) {
        std::set<std::pair<int, int> > cache;
        buildInitTree(ast_root, root, cache);
    }

    void addConstraint(z3::solver& solver, z3::context& ctx,
                       const std::vector<std::pair<DataList, Data>>& example_list);
    z3::expr encodeTree(z3::solver& solver, z3::context& ctx);
    Program* getProgramFromModel(const z3::model& model, z3::context& ctx);
    void addDifferent(const z3::model& model, z3::solver& solver, z3::context& ctx);
    ~SecondOrderEncoder() {delete ast_root;}
};
#endif //FLASHFILL_VSAENCODEUTIL_H
