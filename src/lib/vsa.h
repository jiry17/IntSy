//
// Created by pro on 2019/10/15.
//

#ifndef FLASHFILL_VSA_H
#define FLASHFILL_VSA_H

#include "specification.h"
#include "example_space.h"
#include "config.h"
#include "log_util.h"
#include <set>
#include <map>

class VSANode;
class VSABuilder;

typedef std::vector<VSANode*> VSAEdge;
typedef std::vector<VSAEdge> EdgeList;

extern int vsanode_id;

class VSANode {
    VSANode(NonTerminal _symbol, Specification* _spec, int _depth);
public:
    NonTerminal symbol;
    Specification* spec;
    //std::vector<Data> value_list;
    //std::vector<int> example_list;
    int depth, node_id, temp_id = -1;
    std::vector<EdgeList> children;
    bool alive = false;
    friend class VSABuilder;

    VSANode(VSANode* base_node, Specification* _spec, DataList _return_value, int _example_id, VSABuilder* builder);
    VSANode(VSANode* node1, VSANode* node2, VSABuilder* builder);
    VSANode(VSANode* base): symbol(base->symbol), spec(base->spec), depth(base->depth), node_id(++vsanode_id) {}
    void clearTempId();
    void assignTempId(int& id);

    void print();
};

class VSABuilder {
    VSANode* base_vsa = nullptr;
    Specification* spec;
    unsigned long long key;
    std::map<std::pair<int, unsigned long long>, int> memory;
    unsigned long long char_map[256];
    ExampleSpace* example_space;

    class HashItem {
    public:
        int next;
        VSANode* node;
        DataList value;
    };

    std::vector<HashItem> hash_list;


    unsigned long long getHash(const Data& data) {
        switch(data.getType()) {
            case TBOOL: return data.getBool() ? 2 : 1;
            case TINT: return (unsigned long long)(data.getInt() + KIntMax + 1);
            case TSTRING: {
                unsigned long long ans = 0;
                std::string value = data.getString();
                for (int i = 0; i < value.length(); ++i) {
                    ans = ans * key + char_map[value[i]];
                }
                return ans;
            }
            default:
                assert(0);
        }
    }

    unsigned long long getHash(const DataList& data_list) {
        unsigned long long ans = 0;
        for (auto& data: data_list) {
            ans = ans * key + getHash(data);
        }
        return ans;
    }

    void clearMemory() {
        memory.clear();
        hash_list.clear();
        hash_list.push_back((HashItem){0, nullptr, {}});
    }

    VSANode* build(VSANode* node1, VSANode* node2) {
        auto info = std::make_pair(node1->node_id, node2->node_id);
        if (memory[info]) return hash_list[memory[info]].node;
        int current_id = hash_list.size();
        hash_list.push_back({0, nullptr, {}});
        memory[info] = current_id;
        return hash_list[current_id].node = new VSANode(node1, node2, this);
    }

    VSANode* build(VSANode* base_node, DataList return_value, int example_id) {
        auto info = std::make_pair(base_node->node_id, getHash(return_value));
        for (int now = memory[info]; now; now = hash_list[now].next) {
            assert(hash_list[now].node->depth == base_node->depth);
            if (hash_list[now].value == return_value) {
#ifdef DEBUG
                assert(hash_list[now].value.size() == return_value.size());
                for (int i = 0; i < return_value.size(); ++i) {
                    assert(hash_list[now].value[i] == return_value[i]);
                }
#endif
                return hash_list[now].node;
            }
        }
        int current_id = hash_list.size();
        hash_list.push_back({memory[info], nullptr, return_value});
        memory[info] = current_id;
        hash_list[current_id].node = new VSANode(base_node, spec, return_value, example_id, this);
        return hash_list[current_id].node;
    }

public:

    int reduceVSA(VSANode* node);

    void deleteVSA(VSANode* node);
    friend class VSANode;

    VSABuilder(Specification* _spec, ExampleSpace* _example_space): spec(_spec), example_space(_example_space) {
        while (1) {
            key = (unsigned long long) (rand());
            if (key % 2 == 0 || key < KIntMax * 10) continue;
            int is_prime = true;
            for (int i = 2; 1ll * i * i <= key; ++i) {
                if (key % i == 0) is_prime = false;
            }
            if (is_prime) break;
        }
        std::set<unsigned long long> used_key;
        used_key.insert(0ul);
        for (int i = 0; i < 256; ++i) {
            unsigned long long now = (unsigned long long)(rand());
            while (used_key.find(now) != used_key.end()) {
                now = (unsigned long long)(rand());
            }
            used_key.insert(now);
        }
    }

    VSANode* buildInit();

    VSANode* buildInit(int example_id);

    VSANode* addExample(VSANode* base_root, int example_id, bool is_temp=false);

    double getSize(VSANode* root);
};

#endif //FLASHFILL_VSA_H
