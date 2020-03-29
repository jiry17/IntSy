//
// Created by pro on 2019/10/16.
//

#include "solver.h"
#include "../lib/log_util.h"
#include "../parser/smtlib_parser.h"
#include "glog/logging.h"
#include "vsa_encoder.h"

#include <iostream>
#include <algorithm>
#include <set>

namespace {
    z3::context ctx;
    z3::solver solver(ctx);


    void setTimeout(unsigned int remain_time) {
        z3::params p(ctx);
        p.set(":timeout", remain_time);
        solver.set(p);
    }

    void clearTimeout() {
        setTimeout(5000000u);
    }

    clock_t start_time = 0;

    unsigned int calculateTime() {
        return (unsigned int)(1.0 * (clock() - start_time) / CLOCKS_PER_SEC * 1000);
    }

    z3::expr getVar(std::string name, Type type) {
        switch (type) {
            case TBOOL: return ctx.bool_const(name.c_str());
            case TINT: return ctx.int_const(name.c_str());
            default: assert(0);
        }
    }

    z3::expr getIndicator(int id, Type type) {
        return getVar("Indicator@" + std::to_string(id), type);
    }

    int calculateValue(std::vector<Program*>::iterator first, std::vector<Program*>::iterator last, const DataList& inp) {
        int ans = 0;
        for (auto i = first; i < last; ++i) {
            int size = 0;
            for (auto j = first; j <= i; ++j) {
                if ((*i)->run(inp) == (*j)->run(inp)) {
                    ++size;
                }
            }
            ans = std::max(ans, size);
        }
        return ans;
    }

    void addBestConstraint(std::vector<Program*>::iterator first, std::vector<Program*>::iterator last, int size, Type type) {
        std::vector<z3::expr> indicator_list;
        for (auto now = first; now < last; now++) {
            auto indicator = getIndicator(int(now - first), type);
            solver.add(indicator == (*now)->parseProgrm(ctx));
            indicator_list.push_back(indicator);
        }
        int n = int(last - first);
        if (type == TBOOL) {
            auto sum = getVar("Sum", TINT);
            z3::expr_vector expr_vec(ctx);
            for (int i = 0; i < indicator_list.size(); ++i) {
                expr_vec.push_back(z3::ite(indicator_list[i], ctx.int_val(1), ctx.int_val(0)));
            }
            solver.add(sum == z3::sum(expr_vec));
            solver.add(sum <= size && n - sum <= size);
        } else {
            std::vector<z3::expr> size_var_list;
            for (int i = 0; i < indicator_list.size(); ++i) {
                size_var_list.push_back(getVar("Size@" + std::to_string(i), TINT));
            }
            for (auto& var: size_var_list) {
                solver.add(var >= 1 && var <= size);
            }
            for (int i = 0; i < indicator_list.size(); ++i) {
                for (int j = 0; j < i; ++j) {
                    solver.add(z3::implies(indicator_list[i] == indicator_list[j], size_var_list[i] >= size_var_list[j] + 1));
                }
            }
        }
    }

    bool checkAllSame(ExampleSpace* example_space, std::vector<Program*>::iterator first, std::vector<Program*>::iterator last) {
        for (auto i = first; i < last; ++i) {
            if (!example_space->equal(*i, *first)) {
                return false;
            }
        }
        return true;
    }

    void filter(std::vector<Program*>& samples, DataList inp, Data oup) {
        int n = 0;
        for (int i = 0; i < samples.size(); ++i) {
            if (samples[i]->run(inp) == oup) {
                samples[n] = samples[i];
                ++n;
            }
        }
        samples.resize(n);
    }

    void getAllCombine(int pos, std::vector<int>& sub_node_list, Semantics* semantics,
                       std::vector<DataList>& cache, std::set<Data>& result_set, DataList& inp, DataList& param_value) {
        if (pos == sub_node_list.size()) {
            result_set.insert(semantics->run(inp, param_value));
            return;
        }
        for (auto& data: cache[sub_node_list[pos]]) {
            inp.push_back(data);
            getAllCombine(pos + 1, sub_node_list, semantics, cache, result_set, inp, param_value);
            inp.pop_back();
        }
    }

    Program* findProgramWithGivenOuput(VSANode* root, DataList& param_value, Data oup, std::vector<DataList>& cache);

    void mergeToTargetProgram(int pos, std::vector<int>& sub_node_list, Semantics* semantics, std::vector<DataList>& cache,
                              Program*& result, DataList& inp, Data& oup, DataList& param_value, std::vector<VSANode*> sub_list) {
        if (pos == sub_node_list.size()) {
            if (semantics->run(inp, param_value) == oup) {
                std::vector<Program*> sub_program;
                for (int i = 0; i < sub_node_list.size(); ++i) {
                    sub_program.push_back(findProgramWithGivenOuput(sub_list[i], param_value, inp[i], cache));
                }
                result = new Program(semantics, sub_program);
            }
            return;
        }
        for (auto& data: cache[sub_node_list[pos]]) {
            if (result != nullptr) continue;
            inp.push_back(data);
            mergeToTargetProgram(pos + 1, sub_node_list, semantics, cache, result, inp, oup, param_value, sub_list);
            inp.pop_back();
        }
    }

    Program* findProgramWithGivenOuput(VSANode* root, DataList& param_value, Data oup, std::vector<DataList>& cache) {
        Program* result = nullptr;
        for (int i = 0; i < root->children.size(); ++i) {
            Rule& rule = root->spec->rule_list[root->symbol.symbol_id][i];
            for (auto& edge: root->children[i]) {
                std::vector<int> sub_node_list;
                std::vector<VSANode*> sub_list;
                for (VSANode* sub_node: edge) {
                    sub_node_list.push_back(sub_node->temp_id);
                    sub_list.push_back(sub_node);
                }
                DataList inp = {};
                mergeToTargetProgram(0, sub_node_list, rule.semantics, cache, result, inp, oup, param_value, sub_list);
                if (result != nullptr) return result;
            }
        }
        assert(0);
    }

    void getPossibleOutput(VSANode* root, DataList& param_value, std::vector<DataList>& cache) {
        int id = root->temp_id;
        if (!cache[id].empty()) return;
        DataList& possible_result = cache[id];
        std::set<Data> result_set;
        for (int i = 0; i < root->children.size(); ++i) {
            Rule& rule = root->spec->rule_list[root->symbol.symbol_id][i];
            for (auto& edge: root->children[i]) {
                std::vector<int> sub_node_list;
                for (VSANode* sub_node: edge) {
                    getPossibleOutput(sub_node, param_value, cache);
                    sub_node_list.push_back(sub_node->temp_id);
                }
                DataList inp = {};
                getAllCombine(0, sub_node_list, rule.semantics, cache, result_set, inp, param_value);
            }
        }
        for (auto iterator = result_set.begin(); iterator != result_set.end(); ++iterator) {
            possible_result.push_back(*iterator);
        }
    }

    bool checkAllSameWithGivenInput(VSANode* root, DataList param_value, Sampler* sampler, ExampleSpace* example_space) {
        int n = 0;
        std::vector<Program*> samples = sampler->sampleFromVSA(root, example_space, root->spec, 10, 100);
        for (int i = 1; i < samples.size(); ++i) {
            if (samples[0]->run(param_value) == samples[i]->run(param_value)) {
                continue;
            }
            return false;
        }
        root->clearTempId();
        root->assignTempId(n);
        std::vector<DataList> cache(n);
        getPossibleOutput(root, param_value, cache);
        return cache[root->temp_id].size() == 1;
    }

    std::vector<Program*> getAllPossibleProgram(VSANode* root, DataList param_value, Sampler* sampler, ExampleSpace* example_space, int limit) {
        int n = 0;
        std::vector<Program*> samples = sampler->sampleFromVSA(root, example_space, root->spec, 10, 100);
        for (int i = 1; i < samples.size(); ++i) {
            if (samples[0]->run(param_value) == samples[i]->run(param_value)) {
                continue;
            }
            return {samples[0], samples[i]};
        }
        root->clearTempId();
        root->assignTempId(n);
        std::vector<DataList> cache(n);
        getPossibleOutput(root, param_value, cache);
        std::vector<Program*> result;
        int count = 0;
        for (Data& oup: cache[root->temp_id]) {
            if (oup == samples[0]->run(param_value)) continue;
            ++count;
            result.push_back(findProgramWithGivenOuput(root, param_value, oup, cache));
            if (count == limit) continue;
        }
        return result;
    }

    void addGoodExampleConstraint(Program* recommend, std::vector<Program*>::iterator left, std::vector<Program*>::iterator right,
                                  Type type, bool has_indicator = false) {
        if (!has_indicator) {
            for (auto now = left; now < right; ++now) {
                auto indicator = getIndicator(int(now - left), type);
                solver.add(indicator == (*now)->parseProgrm(ctx));
            }
        }
        auto recommend_var = getVar("recommend", type);
        solver.add(recommend_var == recommend->parseProgrm(ctx));
        z3::expr_vector sub_expr(ctx);
        for (auto now = left; now < right; ++now) {
            auto indicator = getIndicator(int(now - left), type);
            sub_expr.push_back(z3::ite(indicator == recommend_var, ctx.int_val(1), ctx.int_val(0)));
        }
        solver.add(z3::sum(sub_expr) <= ctx.int_val(int(right - left) / 2));
    }

    void checkConsistent(Program* program, std::vector<std::pair<DataList, Data>> example_list) {
        for (auto& example: example_list) {
            assert(program->run(example.first) == example.second);
        }
    }

    Program* getNewSampleByZ3(VSANode* root, ExampleSpace* example_space, const std::vector<Program*>& samples) {
        assert(samples.size() <= 1);
        SecondOrderEncoder encoder(root);
        solver.reset();
        clearTimeout();
        Specification *spec = root->spec;
        encoder.addConstraint(solver, ctx, spec->example_list);
        solver.add(example_space->getConstraint(ctx));
        if (samples.size() == 1) {
            Program* sample = samples[0];
            solver.add(sample->parseProgrm(ctx) != encoder.encodeTree(solver, ctx));
        }
        auto status = solver.check();
        if (status == z3::unsat) {
            solver.reset();
            return nullptr;
        }
        auto model = solver.get_model();
        solver.reset();
        DataList counter_example = example_space->getExampleFromModel(model, ctx);
        Program* new_sample = encoder.getProgramFromModel(model, ctx);
        if (samples.size() == 1) {
            assert(!(new_sample->run(counter_example) == samples[0]->run(counter_example)));
        }
        return new_sample;
    }

    int getCost(std::vector<Program*>& sample_list, const DataList& inp) {
        std::map<Data, int> result;
        int now = 0;
        for (auto* program: sample_list) {
            Data output = program->run(inp);
            result[output] += 1;
            if (result[output] > now) now = result[output];
        }
        return now;
    }
}

bool Solver::checkFinished(VSANode* root, DataList& counter_example, bool is_use_special) {
    if (example_space->getSize() <= 100 || dynamic_cast<ListExampleSapce*>(example_space)) {
        //std::cout << "checkFinished " << is_use_special << std::endl;
        std::vector<Program*> sample_list = sampler->sampleFromVSA(root, example_space, spec, 10, 10);
        for (auto* program: sample_list) {
            if (!example_space->equal(program, sample_list[0])) return false;
        }
        if (!is_use_special) return false;
        static std::map<int, int> cache;
        std::vector<DataList> all_inp = example_space->getAllInput();
        std::vector<int> id;
        for (int i = 0; i < all_inp.size(); ++i) id.push_back(i);
        std::random_shuffle(id.begin(), id.end());
        //std::random_shuffle(counter_example.begin(), counter_example.end());
        for (int i = 0; i < all_inp.size(); ++i) {
            if (cache[id[i]]) continue;
            //std::cout << "start " << LogUtil::dataListToString(all_inp[id[i]]) << std::endl;
            if (!checkAllSameWithGivenInput(root, all_inp[id[i]], sampler, example_space)) {
                //std::cout << "Fail" << std::endl;
                counter_example.clear();
                for (const auto& data: all_inp[id[i]]) {
                    counter_example.push_back(data);
                }
                return false;
            } else {
                cache[id[i]] = 1;
            }
        }
        return true;
    }

    for (int i = 0; i < 10; ++i) {
        DataList inp = example_space->randomSelect();
        if (!checkAllSameWithGivenInput(root, inp, sampler, example_space)) {
            counter_example.clear();
            for (Data& value: inp) {
                counter_example.push_back(value);
            }
            return false;
        }
    }

    /*std::vector<Program*> samples = sampler->sampleFromVSA(root, example_space, spec, 10, 500);
    for (int i = 1; i < samples.size(); ++i)
        if (!example_space->equal(samples[i-1], samples[i])) return false;*/

    SecondOrderEncoder encoder(root);
    solver.reset();
    clearTimeout();
    Program* program = sampler->sampleFromVSA(root, example_space, spec, 10, 1)[0];
    solver.add(example_space->getConstraint(ctx));
    encoder.addConstraint(solver, ctx, spec->example_list);
    solver.add(encoder.encodeTree(solver, ctx) != program->parseProgrm(ctx));
    auto status = solver.check();
    if (status == z3::unsat) {
        return true;
    }
    auto model = solver.get_model();
    counter_example = example_space->getExampleFromModel(model, ctx);
    Program* program1 = encoder.getProgramFromModel(model, ctx);
    assert(!(program->run(counter_example) == program1->run(counter_example)));
    solver.reset();
    return false;
}

std::pair<Program*, int> getMostInfo(std::vector<Program*> samples, ExampleSpace* example_space) {
    Program* current = samples[0];
    int count = 0;
    for (auto* program: samples) {
        if (example_space->equal(program, current)) {
            ++count;
        } else if (count == 0) {
            current = program;
        } else --count;
    }
    count = 0;
    for (auto* program: samples) {
        if (example_space->equal(program, current)) ++count;
    }
    return std::make_pair(current, count);
}

void SampleSy::refineSamples(VSANode* root, std::vector<Program *> &samples) {
    auto list_example_space = dynamic_cast<ListExampleSapce*>(example_space);
    std::pair<Program*, int> most_info = getMostInfo(samples, example_space);
    int count = most_info.second;
    Program* current = most_info.first;
    // std::cout << count << " " << samples.size() << std::endl;
    if (count < samples.size()) {
        return;
    }
    const int timeout = 15000ul;
    if (list_example_space) {
        start_time = clock();
        std::vector<DataList> all_inp = list_example_space->getAllInput();
        static std::map<int, int> cache;
        std::vector<int> id;
        for (int i = 0; i < all_inp.size(); ++i) {
            if (cache[i] == 0) {
                id.push_back(i);
            }
        }
        std::random_shuffle(id.begin(), id.end());
        for (auto& inp_id: id) {
            if (calculateTime() + 1000ul > timeout) break;
            std::vector<Program*> new_example = getAllPossibleProgram(root, all_inp[inp_id], sampler, example_space, 15);
            if (new_example.size() > 1) {
                for (auto *program: new_example) {
                    samples.push_back(program);
                }
            } else {
                cache[inp_id] = 1;
            }
        }
        return;
    }
    start_time = clock();

    if (example_space->getSize() <= 200 && spec->start_symbol.type == TBOOL) {
        std::vector<DataList> input_list = example_space->getAllInput();
        std::vector<int> perm;
        for (int i = 0; i < input_list.size(); ++i) perm.push_back(i);
        std::random_shuffle(perm.begin(), perm.end());

        int current_example_id = int(spec->example_list.size());
        std::vector<Program*> extra_examples;
        for (int id: perm) {
            if (calculateTime() + 1000ul > timeout) break;
            DataList& inp = input_list[id];
            Data oup = current->run(inp);
            Data new_oup = Data(new BoolValue(!oup.getBool()));
            spec->addExample(inp, new_oup);
            VSANode* current_vsa = builder.addExample(root, current_example_id, true);
            if (!current_vsa) {
                spec->example_list.pop_back();
                continue;
            }

            if (calculateTime() + 10ul < timeout) {
                unsigned int remain = timeout - calculateTime();
                std::vector<Program *> new_samples = sampler->sampleFromVSA(current_vsa, example_space, spec, remain,
                                                                            10);
                for (auto sample: new_samples) extra_examples.push_back(sample);
            }

            // recover
            spec->example_list.pop_back();
            builder.deleteVSA(current_vsa);
        }
        std::random_shuffle(extra_examples.begin(), extra_examples.end());
        if (extra_examples.size() > samples.size()) {
            extra_examples.resize(samples.size());
        }
        for (auto sample: extra_examples) {
            samples.push_back(sample);
        }
        return;
    }

    solver.reset();
    solver.add(example_space->getConstraint(ctx));
    SecondOrderEncoder encoder(root);
    encoder.addConstraint(solver, ctx, spec->example_list);
    solver.add(encoder.encodeTree(solver, ctx) != current->parseProgrm(ctx));
    std::vector<Program*> extra_samples;
    while (extra_samples.size() < 2 * samples.size()) {
        unsigned int current_time = calculateTime();
        if (current_time + 10ul > timeout) break;
        setTimeout(std::min(timeout - current_time, 10000u));
        auto result = solver.check();
        if (result != z3::sat) break;
        auto model = solver.get_model();
        DataList example = example_space->getExampleFromModel(model, ctx);
        Program* current_program = encoder.getProgramFromModel(model, ctx);
        extra_samples.push_back(current_program);
        assert(!(current_program->run(example) == current->run(example)));
        encoder.addDifferent(model, solver, ctx);
    }

    std::random_shuffle(extra_samples.begin(), extra_samples.end());
    if (extra_samples.size() > samples.size()) {
        extra_samples.resize(samples.size());
    }
    for (auto* sample: extra_samples) samples.push_back(sample);
}

DataList SampleSy::getBestExample(std::vector<Program *> sample_list) {
    auto most_info = getMostInfo(sample_list, example_space);
    Program* current = most_info.first;
    int num = most_info.second;
    int rem = int(sample_list.size() - num);
    int now = 0;
    // std::cout << "init " << sample_list.size() << " " << num << " " << rem << std::endl;
    for (int i = 0; i <sample_list.size(); ++i) {
        if (!example_space->equal(current, sample_list[i]))  {
            sample_list[now++] = sample_list[i];
        } else {
            if (rem == 0) continue; rem--;
            sample_list[now++] = sample_list[i];
        }
    }
    sample_list.resize(now);
    int r = std::min(KSampleLimit, int(sample_list.size()));
    while (true) {
        std::random_shuffle(sample_list.begin(), sample_list.end());
        bool is_finished = false;
        for (int i = 1; i < r; ++i) {
            if (!example_space->equal(sample_list[i], sample_list[0])) {
                is_finished = true;
            }
        }
        if (is_finished) break;
    }
    sample_list.resize(r);
    // std::cout << "remain size " << now << std::endl;
    auto list_example_spec = dynamic_cast<ListExampleSapce*>(example_space);
    if (list_example_spec) {
        DataList best_inp;
        std::vector<DataList> example_list = example_space->getAllInput();
        int best_cost = int(sample_list.size()) + 10;
        for (auto& example: example_list) {
            int current_cost = getCost(sample_list, example);
            if (current_cost < best_cost) {
                best_cost = current_cost;
                best_inp.clear();
                for (auto& data: example) {
                    best_inp.push_back(data);
                }
            }
        }
        return best_inp;
    }

    const unsigned int kTotalTime = 2000u;

    int start_size = spec->start_symbol.type == TINT ? 15 : 100;
    int step = spec->start_symbol.type == TINT ? 5 : 50;

    start_time = clock();
    int current_size = std::min(start_size, int(sample_list.size()));
    DataList best_inp;
    while (true) {
        int l = 1, r = current_size + 1, ans = 0;
        DataList current_inp;
        while (l < r) {
            int mid = l + r >> 1;
            solver.reset();
            unsigned int current_time = calculateTime();
            if (current_time >= kTotalTime) {
                clearTimeout();
                return best_inp;
            }
            setTimeout(kTotalTime - current_time);
            addBestConstraint(sample_list.begin(), sample_list.begin() + current_size, mid, spec->start_symbol.type);
            solver.add(example_space->getConstraint(ctx));
            auto result = solver.check();
            if (result == z3::unknown) {
                clearTimeout();
                return best_inp;
            } else if (result == z3::sat) {
                auto model = solver.get_model();
                current_inp = example_space->getExampleFromModel(model, ctx);
                r = mid;
                ans = mid;
            } else {
                l = mid + 1;
            }
        }
        best_inp.clear();
        for (auto& k: current_inp) best_inp.push_back(k);
        // std::cout << "Current " << current_size << " " << ans << std::endl;
#ifdef DEBUG
        // std::cout << ans << " " << calculateValue(sample_list.begin(), sample_list.begin() + current_size, current_inp) << std::endl;
        assert(calculateValue(sample_list.begin(), sample_list.begin() + current_size, current_inp) == ans);
        for (int t = 0; t <= 1; ++t) {
            DataList new_data = example_space->randomSelect();
            assert(calculateValue(sample_list.begin(), sample_list.begin() + current_size, new_data) >= ans);
        }
#endif
        if (current_size == sample_list.size()) {
            clearTimeout();
            return best_inp;
        }
        current_size = std::min(current_size + step, int(sample_list.size()));
    }
}

void SampleSy::reduceSample(std::vector<Program *> &list) {
    if (sample_limit >= list.size()) return;
    if (checkAllSame(example_space, list.begin(), list.end())) return;
    while (true) {
        std::random_shuffle(list.begin(), list.end());
        if (!checkAllSame(example_space, list.begin(), list.begin() + sample_limit)) break;
    }
    list.resize(sample_limit);
}

int SampleSy::synthesize() {
    VSANode* root = builder.buildInit();
    LogUtil::updateResult(builder.getSize(root));
    std::vector<Program*> sample_list = sampler->sampleFromVSA(root, example_space, spec);

    int sample_num = 0;
    for (auto* sample: sample_list) {
        if (!example_space->checkCorrect(sample)) sample_num += 1;
    }
    DataList inp = getBestExample(sample_list);
    DataList counter_example;

    int example_id = 0;
    do {
        LOG(INFO) << "Offline sampling" << std::endl;
        sample_list = sampler->sampleFromVSA(root, example_space, spec);
        refineSamples(root, sample_list);

#ifdef DEBUG
        for (Program* program: sample_list) {
            for (auto& example: spec->example_list) {
                assert(program->run(example.first) == example.second);
            }
        }
#endif
        Data oup = example_space->getOutput(inp);
        LOG(INFO) << "Previous sample num " << sample_list.size() << std::endl;
        filter(sample_list, inp, oup);
        LOG(INFO) << "Example No." << example_id + 1 << " " << LogUtil::exampleToString(std::make_pair(inp, oup)) << " " <<
                  "Remaining samples: " << sample_list.size() << std::endl;
        spec->addExample(inp, oup);
        root = builder.addExample(root, example_id++);
        LogUtil::updateResult(builder.getSize(root), 0, sample_num);
        reduceSample(sample_list);
        std::vector<Program*> new_samples = sampler->sampleFromVSA(root, example_space, spec, 2);
        for (auto* sample: new_samples) sample_list.push_back(sample);

        if (checkAllSame(example_space, sample_list.begin(), sample_list.end())) {
            LOG(INFO) << "All samples are the same" << std::endl;
            sample_num = 2;
            if (checkFinished(root, inp)) {
                break;
            }
        } else {
            inp = getBestExample(sample_list);
            sample_num = 0;
            for (auto* sample: sample_list) {
                if (!example_space->checkCorrect(sample)) {
                    sample_num += 1;
                }
            }
        }

        bool is_find = false;
        int correct_num = 0;
        for (auto* program: sample_list) {
            if (example_space->checkCorrect(program)) is_find = true, ++correct_num;
        }
        if (is_find) LOG(INFO) << "Already find the correct answer #" << correct_num << std::endl;
    } while (true);

    auto* result = sampler->sampleFromVSA(root, example_space, spec, 10, 1)[0];
    LOG(INFO) << "Result = " << LogUtil::programToString(result) << std::endl;
    assert(example_space->checkCorrect(result));

#ifdef DEBUG
    auto current_list = sampler->sampleFromVSA(root, example_space, spec, 10, 1000);
    for (auto* program: current_list) assert(example_space->equal(program, result));
#endif

    spec->example_list.clear();
    return example_id;
}


int RandomSy::synthesize() {
    VSANode* root = builder.buildInit();
    LogUtil::updateResult(builder.getSize(root));
    // root->print();
    int example_id = 0;
    std::set<std::string> used_inp;
    while (true) {
        DataList inp;
        bool is_finished = true;
        if (checkFinished(root, inp, false)) break;
        do {
            inp = example_space->randomSelect();
            // std::cout << "Check " << LogUtil::dataListToString(inp) << std::endl;
            // std::cout << LogUtil::dataListToString(inp) << std::endl
            std::string id_string = LogUtil::dataListToString(inp);
            if (used_inp.find(id_string) != used_inp.end()) {
                continue;
            }
            used_inp.insert(id_string);
            if (!checkAllSameWithGivenInput(root, inp, sampler, example_space)) {
                is_finished = false;
                break;
            }
        } while (used_inp.size() < example_space->getSize());
        if (is_finished) {
            break;
        }
        Data oup = example_space->getOutput(inp);
        spec->addExample(inp, oup);
        LOG(INFO) << "Try to add " << LogUtil::exampleToString(std::make_pair(inp, oup)) << std::endl;
        root = builder.addExample(root, example_id++);
        LogUtil::updateResult(builder.getSize(root));
        LOG(INFO) << "Added " << example_id << " examples" << std::endl;
        LOG(INFO) << "New example " << LogUtil::exampleToString(std::make_pair(inp, oup)) << std::endl;
    }
    auto* result = sampler->sampleFromVSA(root, example_space, spec, 10, 1)[0];
    LOG(INFO) << "Result = " << LogUtil::programToString(result) << std::endl;
    assert(example_space->checkCorrect(result));
#ifdef DEBUG
    std::vector<Program*> sample_list = sampler->sampleFromVSA(root, example_space, spec);
    for (auto* program: sample_list) assert(example_space->equal(result, program));
#endif
    spec->example_list.clear();
    return example_id;
}

std::pair<DataList, int> EpsSy::getConditionBestExample(std::vector<Program *> sample_list, Program* recommend) {
    const unsigned int kTotalTime = 2000u;

    int start_size = spec->start_symbol.type == TINT ? 15 : 100;
    int step = spec->start_symbol.type == TINT ? 5 : 50;
    int now = 0;
    std::vector<Program*> total_sample_list = sample_list;
    for (int i = 0; i < sample_list.size(); ++i) {
        if (!example_space->equal(recommend, sample_list[i])) {
            sample_list[now] = sample_list[i];
            ++now;
        }
    }
    sample_list.resize(now);
    auto* list_example_space = dynamic_cast<ListExampleSapce*>(example_space);
    if (list_example_space) {
        DataList best_oup;
        bool is_valid = false;
        int best_cost = int(sample_list.size()) + 10;
        for (auto& example: list_example_space->getAllInput()) {
            Data r_oup = recommend->run(example);
            int dis_count = 0;
            for (auto* program: sample_list) {
                if (!(program->run(example) == r_oup)) {
                    dis_count += 1;
                }
            }
            int cost = getCost(total_sample_list, example);
            bool current_valid = (dis_count * 2 > sample_list.size());
            if ((current_valid && !is_valid) || ((current_valid == is_valid) && cost < best_cost)) {
                best_cost = cost;
                is_valid = current_valid;
                best_oup.clear();
                for (auto& data: example) best_oup.push_back(data);
            }
        }
        return std::make_pair(best_oup, is_valid);
    }

    std::random_shuffle(sample_list.begin(), sample_list.end());
    start_time = clock();
    int current_size = std::min(start_size, int(sample_list.size()));
    DataList best_inp;
    int is_good = 0;
    while (true) {
        int l = 1, r = current_size + 1, ans = 0;
        DataList current_inp;
        bool can_be_good = false;
        {
            unsigned int current_time = calculateTime();
            if (current_time + 10ul >= kTotalTime) {
                return std::make_pair(best_inp, is_good);
            }
            solver.reset();
            setTimeout(kTotalTime - current_time);
            addGoodExampleConstraint(recommend, sample_list.begin(), sample_list.begin() + current_size, spec->start_symbol.type);
            solver.add(example_space->getConstraint(ctx));
            auto result = solver.check();
            if (result == z3::unknown) {
                clearTimeout();
                solver.reset();
                return std::make_pair(best_inp, is_good);
            } else if (result == z3::sat) {
                can_be_good = true;
                auto model = solver.get_model();
                current_inp = example_space->getExampleFromModel(model, ctx);
            } else {
                can_be_good = false;
            }
            clearTimeout();
            solver.reset();
        }

        while (l < r) {
            int mid = l + r >> 1;
            solver.reset();
            unsigned int current_time = calculateTime();
            if (current_time >= kTotalTime) {
                clearTimeout();
                return std::make_pair(best_inp, is_good);
            }
            setTimeout(kTotalTime - current_time);
            if (can_be_good) {
                addGoodExampleConstraint(recommend, sample_list.begin(), sample_list.begin() + current_size, spec->start_symbol.type);
            }
            addBestConstraint(sample_list.begin(), sample_list.begin() + current_size, mid, spec->start_symbol.type);
            solver.add(example_space->getConstraint(ctx));
            auto result = solver.check();
            if (result == z3::unknown) {
                clearTimeout();
                return std::make_pair(best_inp, is_good);
            } else if (result == z3::sat) {
                auto model = solver.get_model();
                current_inp = example_space->getExampleFromModel(model, ctx);
                r = mid;
                ans = mid;
            } else {
                l = mid + 1;
            }
        }
        best_inp.clear();
        for (auto& k: current_inp) best_inp.push_back(k);
        is_good = can_be_good;
#ifdef DEBUG
        // std::cout << ans << " " << calculateValue(sample_list.begin(), sample_list.begin() + current_size, current_inp) << std::endl;
        assert(calculateValue(sample_list.begin(), sample_list.begin() + current_size, current_inp) == ans);
        if (is_good) {
            int num = 0;
            for (int i = 0; i < current_size; ++i) {
                num += example_space->equal(sample_list[i], recommend);
            }
            assert(num <= current_size / 2);
        }
#endif
        if (current_size == sample_list.size()) {
            clearTimeout();
            return std::make_pair(best_inp, can_be_good);
        }
        current_size = std::min(current_size + step, int(sample_list.size()));
    }
}

double EpsSy::calculateConfidence(Program *recommend, int id) {
    assert(history.size() > id);
    int num = 0;
    int tot = 0;
    for (auto* sample: history[id]) {
        if (!example_space->equal(sample, recommend)) {
            ++tot;
            if (sample->run(spec->example_list[id].first) == spec->example_list[id].second) {
                ++num;
            }
        }
    }
    double ratio = std::max(1.0 * num / tot, 0.25);
    return ratio > 0.5 ? 0 : 1;
}

double EpsSy::calculateConfidence(Program *recommend) {
    double ans = 0;
    for (int i = 0; i < spec->example_list.size(); ++i) {
        ans += calculateConfidence(recommend, i);
    }
    return ans;
}


Program* EpsSy::getRecommend(VSANode* root) {
    if (spec->benchmark_type == BSTRING) {
        return sampler->getBestSample(root);
    }
    return RecommendParser::getRecommend(benchmark_path, spec->benchmark_type,  spec->example_list, example_space);
}

int EpsSy::synthesize() {
    VSANode* root = builder.buildInit();
    LogUtil::updateResult(builder.getSize(root));
    std::vector<Program*> sample_list = sampler->sampleFromVSA(root, example_space, spec);
    Program* recommend = getRecommend(root);
    //LOG(INFO) << "Oracle = " << spec->oracle->parseProgram(ctx) << std::endl;
    //Program* recommend = sampler->getBestSample(root);
    double confidence = 0;

    history.push_back(sample_list);
    DataList inp = getConditionBestExample(sample_list, recommend).first;

    DataList counter_example;
    Program* result = nullptr;

    int example_id = 0;
    do {
        LOG(INFO) << "Offline sampling" << std::endl;
        sample_list = sampler->sampleFromVSA(root, example_space, spec);
        refineSamples(root, sample_list);

#ifdef DEBUG
        for (Program* program: sample_list) {
            for (auto& example: spec->example_list) {
                assert(program->run(example.first) == example.second);
            }
        }
#endif
        Data oup = example_space->getOutput(inp);
        filter(sample_list, inp, oup);
        LOG(INFO) << "Example No." << example_id + 1 << " " << LogUtil::exampleToString(std::make_pair(inp, oup)) << " " <<
                  "Remaining samples: " << sample_list.size() << std::endl;
        LOG(INFO) << "Recommend " << LogUtil::programToString(recommend) << " Confidence " << confidence << std::endl;
        spec->addExample(inp, oup);
        root = builder.addExample(root, example_id++);
        reduceSample(sample_list);
        bool is_survive = false;
        if (!(recommend->run(inp) == oup)) {
            is_survive = false;
            recommend = getRecommend(root);
            checkConsistent(recommend, spec->example_list);
            confidence = 0; //calculateConfidence(recommend);
        } else {
            confidence += calculateConfidence(recommend, spec->example_list.size() - 1);
            is_survive = true;
        }
        LogUtil::updateResult(builder.getSize(root), is_survive);

        if (confidence >= threshold) {
            result = recommend;
            break;
        }

        std::vector<Program*> new_samples = sampler->sampleFromVSA(root, example_space, spec, 2);
        for (auto* sample: new_samples) sample_list.push_back(sample);
        if (checkAllSame(example_space, sample_list.begin(), sample_list.end())) {
            LOG(INFO) << "All samples are the same" << std::endl;
            if (sample_list.size() >= 100) {
                result = sample_list[0];
                break;
            }
            sample_list = sampler->sampleFromVSA(root, example_space, spec, 10, 100);
            if (checkAllSame(example_space, sample_list.begin(), sample_list.end())) {
                result = sample_list[0];
                break;
            }
        }
        history.push_back(sample_list);
        inp = getConditionBestExample(sample_list, recommend).first;

        bool is_find = false;
        int answer_num = 0;
        for (auto* program: sample_list) {
            if (example_space->checkCorrect(program)) is_find = true, ++answer_num;
        }
        if (is_find) LOG(INFO) << "Already find the correct answer #" << answer_num << std::endl;
    } while (true);


    LOG(INFO) << "Result = " << LogUtil::programToString(result) << std::endl;
    if (example_space->checkCorrect(result)) {
        LOG(INFO) << "Correct" << std::endl;
    } else {
        LOG(INFO) << "Find an incorrect answer" << std::endl;
        example_id = -example_id;
    }

    spec->example_list.clear();
    return example_id;
}