//
// Created by pro on 2019/10/24.
//

#include "config.h"

const std::string KProjectPath = SOURCEPATH;
const std::string KPythonPath = KProjectPath + "/parser/python/";
const std::string KCircuitBenchmarkPath = KProjectPath + "/../benchmarks/circuit/final/";
const std::string KIntBenchmarkPath = KProjectPath + "/../benchmarks/repair/";
const std::string KStringBenchmarkPath = KProjectPath + "/../benchmarks/string/final/";
const std::string KEusolverPath = KProjectPath + "/../recommend/eusolver/";
const std::string KEuphonyPath = KProjectPath + "/../recommend/euphony/";
int KMaxDepth = 8;
int KIntMax = 5;
int KEdgeMax = 3e8;
int KSampleLimit = 1e9;
std::string global_oup_file;
Record global_record;