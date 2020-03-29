//
// Created by pro on 2019/10/16.
//

#include "sampler.h"
#include "glog/logging.h"

#include <random>
#include <ctime>
#include <algorithm>
#include <cmath>
#include "../lib/log_util.h"

namespace {
    void vectorMul(std::vector<double>& a, const std::vector<double>& b) {
        int n = a.size() - 1;
        int m = b.size() - 1;
        std::vector<double> c(n + m + 1);
        for (int i = 0; i <= n + m; ++i) {
            c[i] = 0;
        }
        for (int i = 0; i <= n; ++i) {
            for (int j = 0; j <= m; ++j) {
                c[i + j] += a[i] * b[j];
            }
        }
        a = c;
    }

    void vectorAdd(std::vector<double>& a, const std::vector<double>& b) {
        int n = a.size(), m = b.size();
        if (n < m) {
            a.resize(m);
            for (int i = n + 1; i < m; ++i) a[i] = 0;
        }
        for (int i = 0; i < m; ++i) {
            a[i] += b[i];
        }
    }

    void calculateSize(VSANode* node, std::vector<std::vector<double>>& size_info) {
        std::vector<double>& size = size_info[node->temp_id];
        if (size.size() > 0) {
            return;
        }
        for (auto& edge_list: node->children) {
            for (auto& edge: edge_list) {
                std::vector<double> current_size;
                current_size.push_back(0);
                current_size.push_back(1);
                for (VSANode* sub_node: edge) {
                    calculateSize(sub_node, size_info);
                    vectorMul(current_size, size_info[sub_node->temp_id]);
                }
                vectorAdd(size, current_size);
            }
        }
    }

    int proportionalSampleFromVector(const std::vector<double>& A) {
        static std::default_random_engine e(time(0));
        static std::uniform_real_distribution<double> r(0, 1);
        double sum = 0;
        for (auto w: A) sum += w;
        double pos = r(e);
        for (int i = 0; i < A.size(); ++i) {
            if (A[i] >= sum * pos) {
                return i;
            }
            pos -= A[i] / sum;
        }
        return int(A.size()) - 1;
    }

    Program* recursizelySample(VSANode *node, const std::vector<std::vector<double>> &size_info, int size) {
        std::vector<double> choice;
        Specification* spec = node->spec;
        // std::cout << "Sample " << node->temp_id << " " << size << std::endl;
        for (const EdgeList& edge_list: node->children) {
            for (const VSAEdge& edge: edge_list) {
                std::vector<double> current_size = {0, 1};
                for (auto* sub_node: edge) {
                    vectorMul(current_size, size_info[sub_node->temp_id]);;
                }
                if (current_size.size() > size) choice.push_back(current_size[size]); else choice.push_back(0.0);
            }
        }
        // for (int i = 0; i < choice.size(); ++i) std::cout << choice[i] << " "; std::cout << std::endl;
        int pos = proportionalSampleFromVector(choice);
        // std::cout << "pos " << pos << std::endl;
        for (int i = 0; i < spec->rule_list[node->symbol.symbol_id].size(); ++i) {
            const EdgeList& edge_list = node->children[i];
            for (const VSAEdge& edge: edge_list) {
                if (pos == 0) {
                    std::vector<std::vector<double>> prefix_size;
                    prefix_size.push_back({0, 1});
                    for (int j = 0; j < edge.size(); ++j) {
                        VSANode* sub_node = edge[j];
                        prefix_size.push_back(prefix_size[j]);
                        vectorMul(prefix_size[j + 1], size_info[sub_node->temp_id]);
                    }
                    assert(edge.size() <= 3);
                    std::vector<Program*> sub_program(edge.size());
                    int total_size = size;
                    for (int j = edge.size(); j; j--) {
                        choice.resize(total_size);
                        VSANode* sub_node = edge[j - 1];
                        for (int k = 0; k < total_size; ++k) {
                            if (total_size - k >= prefix_size[j-1].size() || k >= size_info[sub_node->temp_id].size()) {
                                choice[k] = 0.0;
                            } else {
                                choice[k] = prefix_size[j-1][total_size - k] * size_info[sub_node->temp_id][k];
                            }
                        }
                        int current_size = proportionalSampleFromVector(choice);
                        sub_program[j - 1] = recursizelySample(sub_node, size_info, current_size);
                        total_size -= current_size;
                    }
                    return new Program(spec->rule_list[node->symbol.symbol_id][i].semantics, sub_program);
                }
                pos--;
            }
        }
        assert(0);
    }
}

void SizeBasedSampler::calculateSizeInfo(VSANode* node) {
    int n = 0;
    node->clearTempId();
    node->assignTempId(n);
    size_info.resize(size_t(n));
    for (int i = 0; i < n; ++i) size_info[i].clear();
    calculateSize(node, size_info);
}

Program* SizeBasedSampler::sampleWithSize(VSANode *node, int size) {
#ifdef DEBUG
    assert(!size_info.empty());
#endif
    return recursizelySample(node, size_info, size);
}

std::vector<Program*> SizeBasedSampler::sampleFromVSA(
        VSANode* node, ExampleSpace* example_space, Specification* spec, int time_limit, int number_limit) {
    clock_t start_time = clock();
    calculateSizeInfo(node);

    double total_size = 0.0;
    for (auto k: size_info[node->temp_id]) total_size += k;
    LOG(INFO) << "Size info " << std::to_string(total_size) << std::endl;

    ListExampleSapce* list_example_space = dynamic_cast<ListExampleSapce*>(example_space);
    std::vector<Program*> program_list;
    do {
        if (rand() % 10000 / 10000.0 < beta) {
            if (list_example_space) {
                program_list.push_back(new Program(new TruthSemantics(list_example_space->example_list), {}));
            } else {
                program_list.push_back(dynamic_cast<VectorExampleSpace*>(example_space)->oracle);
            }
            continue;
        }
        Program* result = sampleWithSize(node, selecter->select(size_info[node->temp_id]));
        if (example_space->checkCorrect(result) && rand() % 100 / 100.0 < alpha) continue;
        program_list.push_back(result);
    } while (1.0 * (clock() - start_time) / CLOCKS_PER_SEC <= time_limit && program_list.size() < number_limit);
#ifdef DEBUG
    for (Program* program: program_list) {
        for (const auto& example: node->spec->example_list) {
            assert(program->run(example.first) == example.second);
        }
    }
#endif
    return program_list;
}

int ExpSizeSelecter::select(std::vector<double> A) {
    for (int i = 0; i < A.size(); ++i) if (A[i] >= 1) A[i] = A[i] * std::exp(-i);
    return proportionalSampleFromVector(A);
}

int MinimalSelecter::select(std::vector<double> A) {
    for (int i = 0; i < A.size(); ++i) {
        if (A[i] >= 1) return i;
    }
}

int SizeWeightFixedSelcter::select(std::vector<double> A) {
    if (!is_initialize) {
        is_initialize = true;
        weight = A;
    }
    for (int i = 0; i < A.size(); ++i) {
        if (A[i] >= 1) {
            A[i] /= weight[i];
        }
    }
    return proportionalSampleFromVector(A);
}

int SizeWeightFixedSelcter::getBestSize(std::vector<double> A) {
    if (!is_initialize) {
        is_initialize = true;
        weight = A;
    }
    int ans = -1;
    for (int i = 0; i < A.size(); ++i) {
        if (A[i] >= 1) {
            if (ans == -1 || weight[ans] >= weight[i]) {
                ans = i;
            }
        }
    }
    return ans;
}

Program* SizeBasedSampler::getBestSample(VSANode *node) {
    calculateSizeInfo(node);
    int target_size = selecter->getBestSize(size_info[node->temp_id]);
    return sampleWithSize(node, target_size);
}

int UniformSelecter::select(std::vector<double> A) {
    return proportionalSampleFromVector(A);
}

int TestSizeWeightFixedSelcter::getBestSize(std::vector<double> A) {
    if (!is_initialize) {
        is_initialize = true;
        weight = A;
        for (int i = 0; i < A.size(); ++i) A[i] *= (i + 1);
    }
    int ans = -1;
    for (int i = 0; i < A.size(); ++i) {
        if (A[i] >= 1) {
            if (ans == -1 || weight[ans] >= weight[i]) {
                ans = i;
            }
        }
    }
    return ans;
}


int TestSizeWeightFixedSelcter::select(std::vector<double> A) {
    if (!is_initialize) {
        is_initialize = true;
        weight = A;
        for (int i = 0; i < A.size(); ++i) A[i] *= (i + 1);
    }
    for (int i = 0; i < A.size(); ++i) {
        if (A[i] >= 1) {
            A[i] /= weight[i];
        }
    }
    return proportionalSampleFromVector(A);
}

std::vector<Program *> MinimalSizeBasedSampler::sampleFromVSA(VSANode *node, ExampleSpace *example_space,
                                                              Specification *spec, int time_limit, int number_limit) {

    clock_t start_time = clock();
    calculateSizeInfo(node);

    double total_size = 0.0;
    for (auto k: size_info[node->temp_id]) total_size += k;
    LOG(INFO) << "Size info " << std::to_string(total_size) << std::endl;

    ListExampleSapce* list_example_space = dynamic_cast<ListExampleSapce*>(example_space);
    std::vector<Program*> program_list;
    int current_size = 0;
    int tempts = 0;
    do {
        if (tempts > 1000 || size_info[node->temp_id][current_size] < 1) {
            current_size += 1;
            tempts = 0;
            continue;
        }
        Program* result = sampleWithSize(node, current_size);
        bool is_new = true;
        for (auto* program: program_list) {
            if (example_space->equal(result, program)) {
                is_new = false;
                break;
            }
        }
        if (is_new) {
            program_list.push_back(result);
            tempts = 0;
        } else {
            tempts += 1;
        }
    } while (1.0 * (clock() - start_time) / CLOCKS_PER_SEC <= time_limit && program_list.size() < number_limit && current_size < size_info[node->temp_id].size());
#ifdef DEBUG
    for (Program* program: program_list) {
        for (const auto& example: node->spec->example_list) {
            assert(program->run(example.first) == example.second);
        }
    }
#endif
    return program_list;
}