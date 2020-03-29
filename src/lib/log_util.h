//
// Created by jiry on 19-10-20.
//

#ifndef FLASHFILL_LOG_UTIL_H
#define FLASHFILL_LOG_UTIL_H

#include "glog/logging.h"
#include "specification.h"
#include "value.h"
#include <cmath>

class LogUtil {

    static void printResult(bool is_finished) {
        FILE* oup = fopen(global_oup_file.c_str(), "w");
        if (is_finished) {
            fprintf(oup, "Finished\n");
        } else {
            fprintf(oup, "Unfinished\n");
        }
        int num = int(global_record.remain_size.size());
        fprintf(oup, "%d\n", num - 1);
        for (int i = 0; i < num; ++i) {
            if (i) fprintf(oup, " ");
            fprintf(oup, "%.3lf", global_record.remain_size[i]);
        }
        fprintf(oup, "\n");
        for (int i = 1; i < num; ++i) {
            if (i > 1) fprintf(oup, " ");
            fprintf(oup, "%d", global_record.is_survive[i]);
        }
        fprintf(oup, "\n");
        for (int i = 0; i < num; ++i) {
            if (i) fprintf(oup, " ");
            fprintf(oup, "%d", global_record.sample_num[i]);
        }
        fclose(oup);
    }

public:
    static std::string programToString(Program* program) {
        Semantics* semantics = program->semantics;
        ParamSemantics* param_semantics = dynamic_cast<ParamSemantics*>(semantics);
        ValueSemantics* value_semantics = dynamic_cast<ValueSemantics*>(semantics);
        FunctionSemantics* function_semantics = dynamic_cast<FunctionSemantics*>(semantics);
        TruthSemantics* truth_semantics = dynamic_cast<TruthSemantics*>(semantics);
        if (param_semantics) {
            return "#P" + std::to_string(param_semantics->getParamId());
        } else if (value_semantics) {
            return value_semantics->getData().toString();
        } else if (truth_semantics) {
            return "Truth";
        } else {
            std::vector<std::string> name;
            for (Program *sub_program: program->children) {
                name.push_back(programToString(sub_program));
            }
            std::string result = "{" + function_semantics->name;
            for (auto sub_str: name) {
                result += " " + sub_str;
            }
            return result + "}";
        }

    }

    static std::string dataListToString(DataList dataList) {
        std::string ans = "{";
        for (const Data& inp: dataList) {
            ans += "" + inp.toString() + ",";
        }
        ans[ans.length() - 1] = '}';
        return ans;
    }

    static std::string exampleToString(std::pair<DataList, Data> example) {
        std::string ans = dataListToString(example.first);
        ans += " => " + example.second.toString();
        return ans;
    }

    static std::string sizeInfoToString(std::vector<double> A) {
        std::string ans = "{";
        for (int i = 0; i < A.size(); ++i) {
            if (A[i] > 1) {
                ans += std::to_string(i) + ":" + std::to_string(A[i]) + ",";
            }
        }
        ans[ans.length() - 1] = '}';
        return ans;
    }

    static void updateResult(double size, int is_survive = 0, int sample_num = 0) {
        global_record.remain_size.push_back(std::log(size));
        global_record.is_survive.push_back(is_survive);
        global_record.sample_num.push_back(sample_num);
        printResult(false);
    }

    static void Finished() {
        printResult(true);
    }
};

#endif //FLASHFILL_LOG_UTIL_H
