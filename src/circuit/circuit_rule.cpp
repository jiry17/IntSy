//
// Created by pro on 2019/10/15.
//

#include "circuit_rule.h"

WitnessList CircuitBinary::witnessFunction(const DataList& oup, const DataList& param_list) {
    if (oup.size() == 0) {
        return {{{}, {}}};
    }
    assert(oup.size() == 1);
    WitnessList result;
    for (int a = 0; a < 2; ++a) {
        for (int b = 0; b < 2; ++b) {
            Data inp1 = Data(new BoolValue(a == 1));
            Data inp2 = Data(new BoolValue(b == 1));
            DataList input({inp1, inp2});
            Data run_result = run(input, param_list);
            if (checkInList(oup, run_result)) {
                result.push_back({{inp1}, {inp2}});
            }
        }
    }
    return result;
}

WitnessList CircuitNot::witnessFunction(const DataList &oup, const DataList& param_list) {
    if (oup.size() == 0) {
        return {{{}}};
    } else {
        assert(oup.size() == 1);
        bool w = !oup[0].getBool();
        return {{{Data(new BoolValue(w))}}};
    }
}

WitnessList CircuitAnd::witnessFunction(const DataList &oup, const DataList &para_list) {
    if (oup.size() == 0) {
        return {{{}, {}}};
    }
    assert(oup.size() == 1);
    bool value = oup[0].getBool();
    if (value) {
        return {{{Data(new BoolValue(true))}, {Data(new BoolValue(true))}}};
    } else {
        WitnessList result;
        result.push_back({{Data(new BoolValue(false))}, {}});
        result.push_back({{Data(new BoolValue(true))}, {Data(new BoolValue(false))}});
        return result;
    }
}

WitnessList CircuitOr::witnessFunction(const DataList &oup, const DataList &para_list) {
    if (oup.size() == 0) {
        return {{{}, {}}};
    }
    assert(oup.size() == 1);
    bool value = oup[0].getBool();
    if (value) {
        WitnessList result;
        result.push_back({{Data(new BoolValue(true))}, {}});
        result.push_back({{Data(new BoolValue(false))}, {Data(new BoolValue(true))}});
        return result;
    } else {
        return {{{Data(new BoolValue(false))}, {Data(new BoolValue(false))}}};
    }
}

