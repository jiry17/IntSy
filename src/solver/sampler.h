//
// Created by pro on 2019/10/16.
//

#ifndef FLASHFILL_RANKER_H
#define FLASHFILL_RANKER_H

#include "../lib/specification.h"
#include "../lib/example_space.h"
#include "../lib/vsa.h"

class Sampler {
public:
    virtual std::vector<Program*> sampleFromVSA(VSANode* node, ExampleSpace* example_space, Specification* spec,
                                                int time_limit = 10, int number_limit = 5000) = 0;
    virtual Program* getBestSample(VSANode* node) {
        LOG(INFO) << "Do not support EpsSy" << std::endl;
        assert(0);
    }
};

class SizeSelecter {
public:
    virtual int select(std::vector<double> A) = 0;
    virtual int getBestSize(std::vector<double> A) {
        LOG(INFO) << "Do not support EpsSy" << std::endl;
        assert(0);
    }
};

class MinimalSelecter: public SizeSelecter {
public:
    virtual int select(std::vector<double> A);
};

/*
class AdaptiveSizeSelecter: public SizeSelecter {
public:
    double b, step;
    int limit;
    virtual int select(std::vector<double> A);
    AdaptiveSizeSelecter(double _b, int _limit, double _step = 0.9):
            b(_b), limit(_limit), step(_step) {}
    virtual int modify(std::vector<Program*> samples, ExampleSpace* example_sace, Specification* spec);
};*/

class ExpSizeSelecter: public SizeSelecter {
public:
    virtual int select(std::vector<double> A);
};

class UniformSelecter: public SizeSelecter {
public:
    virtual int select(std::vector<double> A);
    virtual int getBestSize(std::vector<double> A) {
        for (int i=0;i<A.size();++i) if (A[i]>0.5) return i;
    }
};

class SizeWeightFixedSelcter: public SizeSelecter {
public:
    bool is_initialize = false;
    std::vector<double> weight;
    virtual int select(std::vector<double> A);
    virtual int getBestSize(std::vector<double> A);
};

class TestSizeWeightFixedSelcter: public SizeSelecter {
public:
    bool is_initialize = false;
    std::vector<double> weight;
    virtual int select(std::vector<double> A);
    virtual int getBestSize(std::vector<double> A);
};

class SizeBasedSampler: public Sampler{
protected:
    Program* sampleWithSize(VSANode* node, int size);
    void calculateSizeInfo(VSANode* node);
    std::vector<std::vector<double> > size_info;
    SizeSelecter* selecter;
    std::vector<std::vector<Program*> > sample_history;
    double alpha, beta;
public:
    virtual std::vector<Program*> sampleFromVSA(VSANode* node, ExampleSpace* example_space, Specification* spec,
                                                int time_limit = 10, int number_limit = 500);
    virtual Program* getBestSample(VSANode* node);
    SizeBasedSampler(SizeSelecter* _selecter, double _alpha = 0, double _beta = 0): selecter(_selecter), alpha(_alpha), beta(_beta) {}
};

class MinimalSizeBasedSampler: public SizeBasedSampler {
public:
    MinimalSizeBasedSampler(SizeSelecter* _selector): SizeBasedSampler(_selector) {}
    virtual std::vector<Program*> sampleFromVSA(VSANode* node, ExampleSpace* example_space, Specification* spec,
            int time_limit = 10, int number_limit = 500);
};


#endif //FLASHFILL_RANKER_H
