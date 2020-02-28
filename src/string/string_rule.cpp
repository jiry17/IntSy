//
// Created by jiry on 19-11-3.
//

#include "string_rule.h"
#include "../lib/log_util.h"
#include <cstring>
#include <set>

namespace {
    std::vector<int> getAllOccur(std::string s, std::string t) {
        std::vector<int> pos_list;
        int n = int(s.length()), m = int(t.length());
        for (int i = 0; i + m <= n; ++i) {
            bool is_equal = true;
            for (int j = 0; j < m; ++j) {
                if (s[i + j] != t[j]) {
                    is_equal = false;
                    break;
                }
            }
            if (is_equal) {
                pos_list.push_back(i);
            }
        }
        return pos_list;
    }

    std::string replaceString(std::string all_string, std::string old_string, std::string new_string) {
        std::vector<int> pos_list = getAllOccur(all_string, old_string);
        int r = -1;
        std::string result;
        for (auto pos: pos_list) {
            if (pos > r) {
                for (int i = r + 1; i < pos; ++i) {
                    result += all_string[i];
                }
                result += new_string;
                r = pos + int(old_string.length()) - 1;
            }
        }
        for (int i = r + 1; i < all_string.length(); ++i) {
            result += all_string[i];
        }
        return result;
    }

    void replaceWitness(std::string& old_string, std::string& new_string, std::string& target,
                        const DataList &para_list, WitnessList& result, bool is_ignore_new = false) {
        std::set<std::string> used;
        int m = int(old_string.length());
        for (auto &para_data: para_list) {
            std::string para_value = para_data.getString();
            int n = int(para_value.length());
            for (int l = 0; l < n; ++l) {
                std::string init;
                std::string res, cand;
                for (int r = l; r < n; ++r) {
                    init += para_value[r];
                    cand += para_value[r];
                    if (cand.length() >= m && cand.substr(cand.length() - m, size_t(m)) == old_string) {
                        res += cand.substr(0, cand.length() - m) + new_string;
                        cand = "";
                    }
                    if (res + cand == target) {
                        if (used.find(init) == used.end()) {
                            used.insert(init);
                            if (is_ignore_new) {
                                result.push_back({{Data(new StringValue(init))},
                                                  {Data(new StringValue(old_string))}});
                            } else {
                                result.push_back({{Data(new StringValue(init))},
                                                  {Data(new StringValue(old_string))},
                                                  {Data(new StringValue(new_string))}});
                            }
                        }
                    }
                }
            }
        }
    }

    void dfsCheck(int pos, const WitnessResult& result, DataList& now, Semantics* f, const DataList& oup, const DataList& para) {
        if (pos == result.size()) {
            Data current_result = f->run(now, para);
            for (auto& oup_value: oup) {
                if (current_result == oup_value) return;
            }
            assert(0);
        }
        for (auto& data: result[pos]) {
            now.push_back(data);
            dfsCheck(pos + 1, result, now, f, oup, para);
            now.pop_back();
        }
    }

    void checkValidWitness(const WitnessList& result_list, const DataList& oup, Semantics* f, const DataList& para) {
        for (const auto& result: result_list) {
            DataList now;
            dfsCheck(0, result, now, f, oup, para);
        }
    }
}


Data StringConcat::run(const DataList &inp, const DataList &param_value_list) {
#ifdef DEBUG
    assert(inp.size() == 2);
#endif
    return Data(new StringValue(inp[0].getString() + inp[1].getString()));
}

WitnessList StringConcat::witnessFunction(const DataList &oup_list, const DataList &para_list) {
    if (oup_list.size() == 0) {
        return {{{}, {}}};
    }
    Data oup = oup_list[0];
    WitnessList result;
    std::string oup_value = oup.getString();
    for (int i = 0; i <= oup_value.length(); ++i) {
        result.push_back({{Data(new StringValue(oup_value.substr(0, size_t(i))))},
                          {Data(new StringValue(oup_value.substr(size_t(i), oup_value.length() - i)))}});
    }
#ifdef DEBUG
    checkValidWitness(result, oup_list, this, para_list);
#endif
    return result;
}

Data StringSubstr::run(const DataList &inp, const DataList &param_value_list) {
#ifdef DEBUG
    assert(inp.size() == 3);
#endif
    std::string all_string = inp[0].getString();
    int l = inp[1].getInt(), r = inp[2].getInt(), len = int(all_string.length());
    if (l < 0 || r < 0 || l > r || r > len) {
        LOG(INFO) << "Invalid substr call" << std::endl;
        assert(0);
    }
    return Data(new StringValue(all_string.substr(size_t(l), size_t(r - l))));
}

WitnessList StringSubstr::witnessFunction(const DataList &oup_list, const DataList &para_list) {
    if (oup_list.size() == 0) {
        WitnessList result;
        for (auto& para_data: para_list) {
            auto para_value = para_data.getString();
            int num = int(para_value.length());
            for (int i = 0; i < num; ++i) {
                std::vector<Data> possible_r;
                for (int r = i + 1; r <= num; ++r) {
                    possible_r.push_back(Data(new IntValue(r)));
                }
                WitnessResult current_result = {{para_data}, {Data(new IntValue(i))}, possible_r};
                result.push_back(current_result);
            }
        }
        return result;
    }
    assert(oup_list.size() == 1);
    Data oup = oup_list[0];
    std::string result = oup.getString();
    WitnessList answer;
    int result_len = int(result.length());
    // std::cout << "witness \"" << result << "\"" << std::endl;
    for (auto& para: para_list) {
        std::string para_value = para.getString();
        std::vector<int> pos_list = getAllOccur(para_value, result);
        for (auto pos: pos_list) {
            answer.push_back({{Data(new StringValue(para_value))}, {Data(new IntValue(pos))},
                              {Data(new IntValue(pos + result_len))}});
            // std::cout << para_value << " " << pos << " " << pos + result_len << std::endl;
        }
    }
#ifdef DEBUG
    checkValidWitness(answer, oup_list, this, para_list);
#endif
    return answer;
}

Data StringIndex::run(const DataList &inp, const DataList &param_value_list) {
#ifdef DEBUG
    assert(inp.size() == 3);
#endif
    std::string s = inp[0].getString();
    std::string t = inp[1].getString();
    int c = inp[2].getInt();
    std::vector<int> pos_list = getAllOccur(s, t);
    int k = int(std::lower_bound(pos_list.begin(), pos_list.end(), c) - pos_list.begin());
    if (k == pos_list.size()) {
        LOG(INFO) << "Invalid string index call" << std::endl;
        assert(0);
    }
    return Data(new IntValue(pos_list[k]));
}

WitnessList StringIndex::witnessFunction(const DataList &oup_list, const DataList &para_list) {
    assert(oup_list.size() > 0);
    WitnessList result;
    for (auto& para: para_list) {
        std::string para_value = para.getString();
        int n = int(para_value.length());
        std::string sub_string;
        for (auto& oup_data: oup_list) {
            int target_pos = oup_data.getInt();
            for (int i = target_pos; i < para_value.length(); ++i) {
                sub_string += para_value[i];
                std::vector<int> pos_list = getAllOccur(para_value, sub_string);
                int where = int(std::lower_bound(pos_list.begin(), pos_list.end(), target_pos) - pos_list.begin());
                assert(where < pos_list.size());
                DataList possible_pos;
                int pre = where == 0 ? 0 : pos_list[where - 1] + 1;
                for (int current = pre; current <= target_pos; ++current) {
                    possible_pos.push_back(Data(new IntValue(current)));
                }
                result.push_back({{para}, {Data(new StringValue(sub_string))}, possible_pos});
            }
        }
    }
#ifdef DEBUG
    checkValidWitness(result, oup_list, this, para_list);
#endif
    return result;
}

Data StringIndexMove::run(const DataList &inp, const DataList &param_value_list) {
#ifdef DEBUG
    assert(inp.size() == 2);
#endif
    int l_value = inp[0].getInt();
    int r_value = inp[1].getInt();
    return Data(new IntValue(l_value + r_value));
}

WitnessList StringIndexMove::witnessFunction(const DataList &oup_list, const DataList &para_list) {
    assert(!oup_list.empty());
    WitnessList result;
    for (int i = 0; i <= KIntMax; ++i) {
        for (int j = -KIntMax; j <= KIntMax; ++j) {
            Data sum = Data(new IntValue(i + j));
            if (checkInList(oup_list, sum)) {
                result.push_back({{Data(new IntValue(i))}, {Data(new IntValue(j))}});
            }
        }
    }
#ifdef DEBUG
    checkValidWitness(result, oup_list, this, para_list);
#endif
    return result;
}

Data StringLen::run(const DataList &inp, const DataList &param_value_list) {
    assert(inp.size() == 1);
    std::string inp_value = inp[0].getString();
    return Data(new IntValue(int(inp_value.length())));
}

WitnessList StringLen::witnessFunction(const DataList &oup, const DataList &para_list) {
    assert(oup.size() > 0);
    WitnessList result;
    for (auto& para_data: para_list) {
        Data length = Data(new IntValue(int(para_data.getString().length())));
        if (checkInList(oup, length)) {
            result.push_back({{para_data}});
        }
    }
#ifdef DEBUG
    checkValidWitness(result, oup, this, para_list);
#endif
    return result;
}

Data StringDelete::run(const DataList &inp, const DataList &param_value_list) {
#ifdef DEBUG
    assert(inp.size() == 2);
#endif
    std::string all_string = inp[0].toString();
    std::string delete_string = inp[1].toString();
    std::string result = replaceString(all_string, delete_string, "");
    return Data(new StringValue(result));
}

WitnessList StringDelete::witnessFunction(const DataList &oup, const DataList &para_list) {
    if (oup.empty()) {
        return {{{}, {}}};
    }
    assert(oup.size() == 1);
    std::string oup_value = oup[0].getString();
    WitnessList result;
    std::string empty;
    for (auto& constant: string_constant_list) {
        replaceWitness(constant, empty, oup_value, para_list, result, true);
    }
    for (auto& para: para_list) {
        std::string para_value = para.getString();
        replaceWitness(para_value, empty, oup_value, para_list, result, true);
    }
#ifdef DEBUG
    checkValidWitness(result, oup, this, para_list);
#endif
    return result;
}

Data StringReplace::run(const DataList &inp, const DataList &param_value_list) {
#ifdef DEBUG
    assert(inp.size() == 3);
#endif
    std::string all_string = inp[0].getString();
    std::string old_string = inp[1].getString();
    std::string new_string = inp[2].getString();
    std::string result = replaceString(all_string, old_string, new_string);
    return Data(new StringValue(result));
}

WitnessList StringReplace::witnessFunction(const DataList &oup, const DataList &para_list) {
    if (oup.size() == 0) {
        return {{{}, {}, {}}};
    }
    assert(oup.size() == 1);
    std::string oup_value = oup[0].getString();
    WitnessList result;
    for (auto& old_string: string_constant_list) {
        for (auto &new_string: string_constant_list) {
            if (old_string == new_string) {
                continue;
            }
            replaceWitness(old_string, new_string, oup_value, para_list, result);
        }
        for (auto &new_string: para_list) {
            std::string new_value = new_string.getString();
            replaceWitness(old_string, new_value, oup_value, para_list, result);
        }
    }
   for (auto& old_string: para_list) {
        std::string old_value = old_string.getString();
        for (auto& new_string: string_constant_list) {
            replaceWitness(old_value, new_string, oup_value, para_list, result);
        }
        for (auto& new_string: para_list) {
            std::string new_value = new_string.getString();
            replaceWitness(old_value, new_value, oup_value, para_list, result);
        }
    }
#ifdef DEBUG
    checkValidWitness(result, oup, this, para_list);
#endif
    return result;
}

std::vector<std::string> string_constant_list;