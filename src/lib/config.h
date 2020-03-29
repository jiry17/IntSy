//
// Created by pro on 2019/10/24.
//

#ifndef FLASHFILL_CONFIG_H
#define FLASHFILL_CONFIG_H

#include <string>
#include <vector>

struct Record {
    std::vector<double> remain_size;
    std::vector<int> is_survive;
    std::vector<int> sample_num;
};

extern const std::string KProjectPath;
extern const std::string KPythonPath;
extern const std::string KCircuitBenchmarkPath;
extern const std::string KIntBenchmarkPath;
extern const std::string KStringBenchmarkPath;
extern const std::string KEusolverPath;
extern const std::string KEuphonyPath;
extern int KMaxDepth;
extern int KIntMax;
extern int KEdgeMax;
extern std::string global_oup_file;
extern Record global_record;
extern int KSampleLimit;

#endif //FLASHFILL_CONFIG_H
