//
// Created by pro on 2019/10/16.
//

#ifndef FLASHFILL_SOLVER_H
#define FLASHFILL_SOLVER_H

#include "../lib/specification.h"
#include "../lib/example_space.h"
#include "sampler.h"

class Solver {
protected:
    Specification* spec;
    ExampleSpace* example_space;
    Sampler* sampler;
    VSABuilder builder;
    bool checkFinished(VSANode* root, DataList& counter_example, bool use_special = true);
public:
    Solver(Specification* _spec, ExampleSpace* _example_space, Sampler* _sampler):
            spec(_spec), example_space(_example_space), sampler(_sampler), builder(_spec, example_space) {}
    virtual int synthesize() = 0;
};

class RandomSy: public Solver {
public:
    RandomSy(Specification* _spec, ExampleSpace* _example_space, Sampler* _sampler):
            Solver(_spec, _example_space, _sampler) {}
    virtual int synthesize();
};

class SampleSy: public Solver {
protected:
    int sample_limit;
    void reduceSample(std::vector<Program*>& list);
    void refineSamples(VSANode* root, std::vector<Program*>& samples);
public:
    DataList getBestExample(std::vector<Program*> sample_list);
    SampleSy(Specification* _spec, ExampleSpace* _example_space, Sampler* _sampler, int _sample_limit = 500):
            Solver(_spec, _example_space, _sampler), sample_limit(_sample_limit) {}
    virtual int synthesize();
};

class EpsSy: public SampleSy {
    int threshold;
    std::string benchmark_path;
    std::vector<std::vector<Program*> > history;
    std::pair<DataList, int> getConditionBestExample(std::vector<Program*> sample_list, Program* recommend);
    double calculateConfidence(Program* recommend);
    double calculateConfidence(Program* recommend, int id);
    Program* getRecommend(VSANode* root);
public:
    EpsSy(Specification* _spec, ExampleSpace* _example_space, Sampler* _sampler, int _threshold, std::string path):
            SampleSy(_spec, _example_space, _sampler), threshold(_threshold), benchmark_path(path) {}
    virtual int synthesize();
};

#endif //FLASHFILL_SOLVER_H
