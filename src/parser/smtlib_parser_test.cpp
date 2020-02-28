//
// Created by pro on 2019/10/24.
//

#include "smtlib_parser.h"

#include "glog/logging.h"

int main() {
    auto result = SMTLibParser::parseFile("../parser/python/test.sl", BSTRING);
    LOG(INFO) << "Parse finished" << std::endl;
    return 0;
}