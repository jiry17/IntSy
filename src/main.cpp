#include <iostream>

#include "lib/example_space.h"
#include "lib/config.h"
#include "solver/solver.h"
#include "parser/smtlib_parser.h"
#include "sys/time.h"
#include <unistd.h>

#include <cmath>

Solver* getSolverByName(std::string name, Specification* spec, ExampleSpace* example_space, Sampler* sampler, std::string path) {
    if (name == "SampleSy") {
        return new SampleSy(spec, example_space, sampler);
    } else if (name == "RandomSy") {
        return new RandomSy(spec, example_space, sampler);
    } else if (name == "EpsSy") {
        return new EpsSy(spec, example_space, sampler, 8, path);
    } else if (name == "SampleSyExp") {
        return new SampleSy(spec, example_space, new SizeBasedSampler(new ExpSizeSelecter()));
    } else if (name == "SampleSyUniform") {
        return new SampleSy(spec, example_space, new SizeBasedSampler(new UniformSelecter()));
    } else if (name == "SampleSyMinimal") {
        return new SampleSy(spec, example_space, new MinimalSizeBasedSampler(new SizeWeightFixedSelcter()));
    } else if (name.substr(0, 8) == "SampleSy") {
        KSampleLimit = 0;
        for (int i = 8; i < name.size(); ++i) {
            KSampleLimit = KSampleLimit * 10 + name[i] - '0';
        }
        return new SampleSy(spec, example_space, sampler);
    } else if (name.substr(0, 8) == "SampleDe") {
        return new SampleSy(spec, example_space, new SizeBasedSampler(new SizeWeightFixedSelcter(), 0.5));
    } else if (name == "SampleIn0.1") {
        return new SampleSy(spec, example_space, new SizeBasedSampler(new SizeWeightFixedSelcter(), 0, 0.1));
    }

    else if (name == "EpsSyUniform") {
        return new EpsSy(spec, example_space, new SizeBasedSampler(new UniformSelecter()), 6, path);
    } else if (name == "EpsSyMinimal") {
        return new EpsSy(spec, example_space, new MinimalSizeBasedSampler(new SizeWeightFixedSelcter()), 6, path);
    } else if (name.substr(0, 5) == "EpsDe") {
        return new EpsSy(spec, example_space, new SizeBasedSampler(new SizeWeightFixedSelcter(), 0.5), 6, path);
    } else if (name == "EpsIn0.1") {
        return new EpsSy(spec, example_space, new SizeBasedSampler(new SizeWeightFixedSelcter(), 0, 0.1), 6, path);
    } else if (name.substr(0, 5) == "EpsSy") {
        int num = 0;
        for (int i = 5; i <= name.size(); ++i) {
            num = num * 10 + name[i] - '0';
        }
        return new EpsSy(spec, example_space, sampler, 5, path);
    }
}

BenchmarkType getTypeByName(std::string name) {
    if (name == "repair") {
        return BINT;
    } else if (name == "string") {
        return BSTRING;
    } else assert(0);
}

std::string getRootPathByType(BenchmarkType type) {
    switch (type){
        case BINT: return KIntBenchmarkPath;
        case BCIRCUIT: return KCircuitBenchmarkPath;
        case BSTRING: return KStringBenchmarkPath;
        default: assert(0);
    }
}

int main(int argc, char** argv) {
    assert(argc >= 7);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand(tv.tv_usec);
    //TODO use gflag
    BenchmarkType benchmark_type = getTypeByName(std::string(argv[1]));
    std::string file_name = std::string(argv[2]);
    std::string solver_name = std::string(argv[3]);
    std::string log_file = std::string(argv[4]);
    std::string output_file = std::string(argv[5]);
    KMaxDepth = std::atoi(argv[6]);

    if (log_file != "NONE") {
        google::InitGoogleLogging(argv[0]);
        google::SetLogDestination(google::INFO, ("log/" + log_file).c_str());
    }

    global_oup_file = "temp/" + output_file;

    std::string benchmark_path = getRootPathByType(benchmark_type) + file_name;
    auto parse_result = SMTLibParser::parseFile(benchmark_path, benchmark_type);
    Specification* spec = parse_result.first;
    ExampleSpace* space = parse_result.second;
    Sampler* sampler = new SizeBasedSampler(new SizeWeightFixedSelcter());

    LOG(INFO) << "Intmax " << KIntMax << std::endl;
    Solver* solver = getSolverByName(solver_name, spec, space, sampler, benchmark_path);
    int result = solver->synthesize();
    LogUtil::Finished();
    return 0;
}