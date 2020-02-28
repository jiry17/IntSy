//
// Created by jiry on 19-10-26.
//

#include "int_rule.h"
#include "../lib/config.h"

WitnessList IntBinary::witnessFunction(const DataList &oup_value, const DataList &para_list) {
#ifdef DEBUG
    for (auto& current_oup: oup_value) {
        assert(current_oup.getType() == oup);
    }
#endif
    if (oup_value.empty()) {
        return {{{}, {}}};
    }
    WitnessList result;
    int temp_max = KIntMax * 2;
    for (int value_left = -temp_max; value_left <= temp_max; ++value_left) {
        for (int value_right = -temp_max; value_right <= temp_max; ++value_right) {
            Data inp1 = Data(new IntValue(value_left));
            Data inp2 = Data(new IntValue(value_right));
            DataList current_inp = {inp1, inp2};
            Data current_result = run(current_inp, para_list);
            if (checkInList(oup_value, current_result)) {
                result.push_back({{inp1}, {inp2}});
            }
        }
    }
    return result;
}

WitnessList IntIte::witnessFunction(const DataList &oup_value, const DataList &para_list) {
    if (oup_value.empty()) {
        return {{{}, {}, {}}};
    }
    WitnessList result;
    result.push_back({{Data(new BoolValue(true))}, oup_value, {}});
    result.push_back({{Data(new BoolValue(false))}, {}, oup_value});
    return result;
}

WitnessList IntCompare::witnessFunction(const DataList &inp, const DataList &para_list) {
    DataList all_int = Data::getAllValue(TINT);
    if (inp.empty()) {
        return {{{}, {}}};
    }
    /*if (inp.size() == 2) {
        return {{all_int, all_int}};
    }*/
    assert(inp.size() == 1);
    WitnessList result;
    int temp_max = KIntMax * 2;
    Data int_min = Data(new IntValue(-temp_max));
    Data int_max = Data(new IntValue(temp_max));
    if (run({int_min, int_max}, para_list) == inp[0]) {
        for (auto& l_value: all_int) {
            DataList possible_value;
            for (auto& r_value: all_int) {
                if (run({l_value, r_value}, para_list) == inp[0]) {
                    possible_value.push_back(r_value);
                }
            }
            if (!possible_value.empty()) {
                result.push_back({{l_value}, possible_value});
            }
        }
    } else {
        for (auto& r_value: all_int) {
            DataList possible_value;
            for (auto& l_value: all_int) {
                if (run({l_value, r_value}, para_list) == inp[0]) {
                    possible_value.push_back(l_value);
                }
            }
            if (!possible_value.empty()) {
                result.push_back({possible_value, {r_value}});
            }
        }
    }
    return result;
}