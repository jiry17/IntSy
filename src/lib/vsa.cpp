//
// Created by pro on 2019/10/15.
//

#include "vsa.h"
#include "log_util.h"

#include "glog/logging.h"

namespace {
    void printVSA(VSANode* root, std::map<int, int>& cash) {
        if (cash[root->node_id]) {
            return;
        }
        cash[root->node_id] = 1;
        std::cout << "Name " << root->symbol.name << " Id " << root->node_id << " Depth " << root->depth << std::endl;
        //for (const Data& data: root->value_list) std::cout << data.toString() << " "; std::cout << std::endl;
        int id = -1;
        for (EdgeList& edge_list: root->children) {
            ++id;
            std::cout << "Edge type " << root->spec->rule_list[root->symbol.symbol_id][id].semantics->name << std::endl;
            for (VSAEdge& edge: edge_list) {
                if (edge.size()) {
                    for (VSANode *sub_node: edge) std::cout << sub_node->node_id << " ";
                    std::cout << std::endl;
                } else std::cout << "finish" << std::endl;
            }
        }
        for (EdgeList& edge_list: root->children) {
            for (VSAEdge& edge: edge_list) {
                for (VSANode* sub_node: edge) printVSA(sub_node, cash);
            }
        }

    }

    void printVSA(VSANode* root) {
        std::map<int, int> cash;
        printVSA(root, cash);
    }

    bool checkRedundancy(VSANode* node, int* tag, std::vector<VSANode*>& trash_bin) {
        assert(tag[node->temp_id] != 2);
        if (tag[node->temp_id] != -1) {
            return tag[node->temp_id] != 0;
        }
        tag[node->temp_id] = 2;
        bool is_redundancy = true;
        for (EdgeList& edge_list: node->children) {
            int id = 0;
            for (VSAEdge& edge: edge_list) {
                int flag = false;
                for (VSANode* sub_node: edge) {
                    flag |= checkRedundancy(sub_node, tag, trash_bin);
                }
                if (!flag) {
                    edge_list[id++] = edge;
                    is_redundancy = false;
                }
            }
            edge_list.resize(id);
        }
        tag[node->temp_id] = is_redundancy;
        if (is_redundancy) {
            trash_bin.push_back(node);
        }
        return is_redundancy;
    }

    void collectAllNode(VSANode* node, std::vector<VSANode*>& node_list) {
        if (node->temp_id != -1) {
            return;
        }
        node->temp_id = 0;
        node_list.push_back(node);
        for (const auto& edge_list: node->children) {
            for (const auto& edge: edge_list) {
                for (VSANode* sub_node: edge) {
                    collectAllNode(sub_node, node_list);
                }
            }
        }
    }

    int getVSASize(VSANode* root) {
        int n = 0;
        root->clearTempId();
        root->assignTempId(n);
        return n;
    }

    void reachFromRoot(VSANode* root) {
        if (root->alive) return;
        root->alive = true;
        for (auto& edge_list: root->children) {
            for (auto& edge: edge_list) {
                for (VSANode* node: edge) {
                    reachFromRoot(node);
                }
            }
        }
    }

    int getTotalSize(VSANode* node) {
        if (node->temp_id != -1) return 0;
        node->temp_id = 0;
        int ans = 0;
        for (auto& edge_list: node->children) {
            for (auto& edge: edge_list) {
                for (VSANode* sub_node: edge) {
                    ans += getTotalSize(sub_node) + 1;
                }
            }
        }
        return ans;
    }

    int calculateTotalSize(VSANode* node) {
        node->clearTempId();
        return getTotalSize(node);
    }

    VSANode* recursiveCopyVSA(VSANode* now, std::vector<VSANode*>& cache) {
        int id = now->temp_id;
        if (cache[id]) return cache[id];
        VSANode* new_node = new VSANode(now);
        for (auto& edge_list: now->children) {
            EdgeList current_edge_list;
            for (auto& edge: edge_list) {
                VSAEdge current_edge;
                for (VSANode* sub_node: edge) {
                    current_edge.push_back(recursiveCopyVSA(sub_node, cache));
                }
                current_edge_list.push_back(current_edge);
            }
            new_node->children.push_back(current_edge_list);
        }
        cache[id] = new_node;
        return new_node;
    }

    VSANode* copyVSA(VSANode* base_root) {
        base_root->clearTempId();
        int n = 0;
        base_root->assignTempId(n);
        std::vector<VSANode*> cache;
        for (int i = 0; i < n; ++i) cache.push_back(nullptr);
        return recursiveCopyVSA(base_root, cache);
    }

    bool checkPure(Specification* spec) {
        return spec->benchmark_type != BSTRING;
    }

    double calculateSize(VSANode *node, std::vector<double>& cache) {
        if (cache[node->temp_id] >= 0) {
            return cache[node->temp_id];
        }
        double ans = 0;
        for (auto& edge_list: node->children) {
            for (auto& edge: edge_list) {
                double now = 1;
                for (VSANode* sub_node: edge) {
                    now *= calculateSize(sub_node, cache);
                }
                ans += now;
            }
        }
        cache[node->temp_id] = ans;
        return ans;
    }
}

void VSABuilder::deleteVSA(VSANode *node) {
    node->clearTempId();
    std::vector<VSANode*> node_list;
    collectAllNode(node, node_list);
    for (VSANode* sub_node: node_list) {
        delete sub_node;
    }
}

int vsanode_id = 0;

int VSABuilder::reduceVSA(VSANode *node) {
    int n = 0;
    node->clearTempId();
    node->assignTempId(n);
    auto* tag = new int[n];
    for (int i = 0; i < n; ++i) tag[i] = -1;
    std::vector<VSANode*> trash_bin;
    assert(!checkRedundancy(node, tag, trash_bin));
    for (auto* sub_node: trash_bin) {
        delete sub_node;
    }
    delete[] tag;
    return n - int(trash_bin.size());
}

VSANode* VSABuilder::buildInit() {
    if (base_vsa) return copyVSA(base_vsa);
    std::vector<std::vector<VSANode *>> node_map(spec->depth_limit);
    std::vector<std::vector<bool>> used(spec->depth_limit);
    for (int i = 0; i < spec->depth_limit; ++i) {
        auto &node_list = node_map[i];
        for (auto &nt: spec->symbols) {
            node_list.push_back(new VSANode(nt, spec, i));
            used[i].push_back(false);
        }
    }
    for (int depth = spec->depth_limit - 1; depth >= 0; --depth) {
        for (auto *node: node_map[depth]) {
            for (auto &rule: spec->rule_list[node->symbol.symbol_id]) {
                EdgeList edge_list;
                auto *function_semantics = dynamic_cast<FunctionSemantics *>(rule.semantics);
                if (function_semantics) {
                    if (depth + 1 != spec->depth_limit) {
                        VSAEdge edge;
                        for (const auto &nt: rule.inp_symbol) {
                            edge.push_back(node_map[depth + 1][nt.symbol_id]);
                        }
                        edge_list.push_back(edge);
                    }
                } else {
                    edge_list.push_back({});
                }
                node->children.push_back(edge_list);
            }
        }
    }
    used[0][spec->start_symbol.symbol_id] = true;
    LOG(INFO) << "Depth limit " << spec->depth_limit << std::endl;
    for (int i = 0; i < spec->depth_limit; ++i) {
        for (int j = 0; j < spec->symbols.size(); ++j) {
            if (used[i][j]) {
                VSANode *node = node_map[i][j];
                for (auto &edge_list: node->children) {
                    for (auto &edge: edge_list) {
                        for (VSANode *child: edge) {
                            used[child->depth][child->symbol.symbol_id] = true;
                        }
                    }
                }
            } else {
                delete node_map[i][j];
            }
        }
    }
    reduceVSA(node_map[0][spec->start_symbol.symbol_id]);
    base_vsa = copyVSA(node_map[0][spec->start_symbol.symbol_id]);
    deleteVSA(node_map[0][spec->start_symbol.symbol_id]);
    if (checkPure(spec)) {
        return copyVSA(base_vsa);
    }
    ListExampleSapce* listed_example_space = dynamic_cast<ListExampleSapce*>(example_space);
    assert(listed_example_space);
    assert(spec->example_list.size() == 0);
    VSANode* vsa_node = nullptr;
    for (auto& example: listed_example_space->example_list) {
        clearMemory();
        // std::cout << "Start " << LogUtil::exampleToString(example) << std::endl;
        spec->addExample(example.first, example.second);
        VSANode* new_node = build(base_vsa, {}, 0);
        spec->example_list.pop_back();
        assert(new_node->alive);
        for (int i = 1; i < hash_list.size(); ++i) {
            hash_list[i].node->alive = false;
        }
        reachFromRoot(new_node);
        for (int i = 1; i < hash_list.size(); ++i) {
            if (!hash_list[i].node->alive) {
                delete hash_list[i].node;
            }
        }
        clearMemory();
        if (vsa_node) {
            VSANode* result = build(vsa_node, new_node);
            deleteVSA(vsa_node);
            deleteVSA(new_node);
            vsa_node = result;
            clearMemory();
        } else {
            vsa_node = new_node;
        }
    }
    //std::cout << "Finished " << calculateTotalSize(vsa_node) << " " << getSize(vsa_node) << std::endl;
    deleteVSA(base_vsa);
    base_vsa = vsa_node;
    return copyVSA(base_vsa);
}

VSANode* VSABuilder::buildInit(int example_id) {
    VSANode* base_root = buildInit();
    clearMemory();
    VSANode* new_node = build(base_root, {spec->example_list[example_id].second}, example_id);
    assert(new_node->alive);
    if (new_node->alive) {
        for (int i = 1; i < hash_list.size(); ++i) {
            hash_list[i].node->alive = false;
        }
        reachFromRoot(new_node);
    }
    for (int i = 1; i < hash_list.size(); ++i) {
        if (!hash_list[i].node->alive) {
            delete hash_list[i].node;
        }
    }
    deleteVSA(base_root);
    clearMemory();
    return new_node;
}

VSANode::VSANode(NonTerminal _symbol, Specification *_spec, int _depth):
        symbol(_symbol), spec(_spec), depth(_depth), node_id(++vsanode_id), temp_id(-1){
}

VSANode::VSANode(VSANode* base_node, Specification* _spec, DataList _return_value,
                 int _example_id, VSABuilder* builder):
        symbol(base_node->symbol), spec(_spec), depth(base_node->depth), node_id(++vsanode_id), temp_id(-1) {
#ifdef DEBUG
    assert(base_node->spec == _spec);
#endif
    /*value_list = base_node->value_list;
    example_list = base_node->example_list;
    value_list.push_back(_return_value);
    example_list.push_back(_example_id);*/
    DataList return_value(_return_value);
    int example_id(_example_id);
    for (int i = 0; i < spec->rule_list[symbol.symbol_id].size(); ++i) {
        Rule& rule = spec->rule_list[symbol.symbol_id][i];
        WitnessList result = rule.witnessFunction(return_value, spec->example_list[example_id].first);
        EdgeList& base_edge_list = base_node->children[i];
        EdgeList edge_list;

        auto* function_semantics = dynamic_cast<FunctionSemantics*>(rule.semantics);

        for (WitnessResult& witness_result: result) {
#ifdef DEBUG
            if (function_semantics) {
                // std::cout << function_semantics->name << " " << witness_result.size() << " " << function_semantics->inp.size() << std::endl;
                assert(witness_result.size() == function_semantics->inp.size());
                for (int j = 0; j < witness_result.size(); ++j) {
                    for (Data &possible_value: witness_result[j]) {
                        assert(possible_value.getType() == function_semantics->inp[j]);
                    }
                }
            }
#endif
            for (VSAEdge& base_edge: base_edge_list) {
                if (function_semantics) {
                    VSAEdge edge;
                    bool is_edge_alive = true;
                    for (int j = 0; j < witness_result.size(); ++j) {
                        VSANode* new_node = builder->build(base_edge[j], witness_result[j], example_id);
                        if (!new_node->alive) {
                            is_edge_alive = false;
                            break;
                        }
                        edge.push_back(new_node);
                    }
                    if (is_edge_alive) {
                        alive = true;
                        edge_list.push_back(edge);
                    }
                } else {
                    edge_list.push_back({});
                    alive = true;
                }
            }
        }
        children.push_back(edge_list);
    }
}

VSANode::VSANode(VSANode *node1, VSANode *node2, VSABuilder *builder):
        symbol(node1->symbol), spec(node1->spec), depth(node1->depth), node_id(++vsanode_id), temp_id(-1){
    RuleList& rule_list = spec->rule_list[node1->symbol.symbol_id];
    alive = false;
    for (int rule_id = 0; rule_id < rule_list.size(); ++rule_id) {
        Rule& rule = rule_list[rule_id];
        EdgeList edge_list1 = node1->children[rule_id];
        EdgeList edge_list2 = node2->children[rule_id];
        EdgeList edge_list;
        for (VSAEdge& edge1: edge_list1) {
            for (VSAEdge& edge2: edge_list2) {
                assert(edge1.size() == edge2.size());
                VSAEdge edge;
                bool is_edge_alive = true;
                for (int node_id = 0; node_id < edge1.size(); ++node_id) {
                    VSANode* new_node = builder->build(edge1[node_id], edge2[node_id]);
                    assert(new_node->depth == depth + 1);
                    if (!new_node->alive) {
                        is_edge_alive = false;
                        break;
                    } else {
                        edge.push_back(new_node);
                    }
                }
                if (is_edge_alive) {
                    alive = true;
                    edge_list.push_back(edge);
                }
            }
        }
        children.push_back(edge_list);
    }
}

VSANode* VSABuilder::addExample(VSANode *base_root, int example_id, bool is_temp) {
    VSANode* example_node = buildInit(example_id);
    clearMemory();
    VSANode* new_node = build(base_root, example_node);
    //VSANode* new_node = build(base_root, {spec->example_list[example_id].second}, example_id);
    bool is_empty = !new_node->alive;
    if (new_node->alive) {
        for (int i = 1; i < hash_list.size(); ++i) {
            hash_list[i].node->alive = false;
        }
        reachFromRoot(new_node);
    }
    for (int i = 1; i < hash_list.size(); ++i) {
        if (!hash_list[i].node->alive) {
            delete hash_list[i].node;
        }
    }
    clearMemory();
    deleteVSA(example_node);
    if (!is_temp) {
        deleteVSA(base_root);
        assert(!is_empty);
        LOG(INFO) << "Add finished! Result size = " << getVSASize(new_node) << std::endl;
    } else if (is_empty) return nullptr;

    int total_size = calculateTotalSize(new_node);
    LOG(INFO) << "Total size " << total_size << std::endl;
    if (total_size > KEdgeMax) {
        LOG(INFO) << "Too many edges" << std::endl;
        exit(1);
    }
    return new_node;
}

void VSANode::clearTempId() {
    if (temp_id == -1) {
        return;
    }
    temp_id = -1;
    for (auto& edge_list: children) {
        for (auto& edge: edge_list) {
            for (VSANode* sub_node: edge) {
#ifdef DEBUG
                assert(sub_node->depth == depth + 1);
#endif
                sub_node->clearTempId();
            }
        }
    }
}

void VSANode::assignTempId(int& id) {
    if (temp_id != -1) {
        return;
    }
    temp_id = id++;
    for (auto& edge_list: children) {
        for (auto& edge: edge_list) {
            for (VSANode* sub_node: edge) {
                sub_node->assignTempId(id);
            }
        }
    }
}

void VSANode::print() {
    int n = 0;
    std::map<int, int> cache;
    clearTempId();
    assignTempId(n);
    printVSA(this, cache);
}

double VSABuilder::getSize(VSANode *root) {
    int n = 0;
    root->clearTempId();
    root->assignTempId(n);
    std::vector<double> cache;
    for (int i = 0; i < n; ++i) cache.push_back(-1.0);
    calculateSize(root, cache);
    return cache[root->temp_id];
}