//
// Created by pro on 2019/10/22.
//

#ifndef FLASHFILL_SMTLIB_PARSER_H
#define FLASHFILL_SMTLIB_PARSER_H

#include <vector>
#include <cstring>

#include "../lib/specification.h"
#include "../lib/example_space.h"

class SMTLibParser {
public:
    static std::pair<Specification*, ExampleSpace*> parseFile(std::string file_name, BenchmarkType benchmark_type);
};

class RecommendParser {
public:
    static Program* getRecommend(std::string file_name, BenchmarkType benchmarkType,
                                 std::vector<std::pair<DataList, Data> > examples, ExampleSpace* example_space);
};


#endif //FLASHFILL_SMTLIB_PARSER_H
