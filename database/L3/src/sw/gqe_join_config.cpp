/*
 * Copyright 2020 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "xf_database/gqe_join_config.hpp"
#include "xf_database/join_command.hpp"

namespace xf {
namespace database {
namespace gqe {

//-----------------------------------------------------------------------------------------------
// detailed implementations

void JoinConfig::CHECK_0(std::vector<std::string> str, size_t len, std::string sinfo) {
    if (str.size() > len) {
        std::cout << "Most Support " << len << " columns in " << sinfo << std::endl;
        exit(1);
    }
}

void JoinConfig::extractKeys(std::string join_str) {
    std::vector<std::string> values_0;
    std::vector<std::string> values_1;
    if (join_str == "") return;
    std::vector<std::string> maps;
    std::istringstream f(join_str);
    std::string s;
    while (getline(f, s, ',')) {
        maps.push_back(s);
    }
    // int i = 0;
    for (auto eval_0 : maps) {
        int notation_eq = eval_0.find_first_of("=");
        std::string key_0 = eval_0.substr(0, notation_eq);
        std::string key_1 = eval_0.substr(notation_eq + 1);
        values_0.push_back(key_0);
        values_1.push_back(key_1);
    }
    join_keys.push_back(values_0);
    join_keys.push_back(values_1);
    if (join_keys[0].size() != join_keys[1].size()) {
        std::cout << "Join Input Error!" << std::endl;
        exit(1);
    }
#ifdef USER_DEBUG
    std::cout << "2. ExtractKeys " << std::endl;
    for (size_t i = 0; i < join_keys.size(); i++) {
        std::cout << "Join keys in table " << i << std::endl;
        for (size_t j = 0; j < join_keys[i].size(); j++) {
            std::cout << join_keys[i][j] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
#endif
}
//

void JoinConfig::extractWcols(std::string outputs) {
    std::vector<std::string> maps;
    std::istringstream f(outputs);
    std::string s;
    while (getline(f, s, ',')) {
        maps.push_back(s);
        // write_cols.push_back(s);
    }
    for (auto ss : maps) {
        int notation_eq = ss.find_first_of("=");
        std::string key_0_ = ss.substr(0, notation_eq);
        std::string key_1_ = ss.substr(notation_eq + 1);
        // if include L join keys rep with O join keys
        for (size_t j = 0; j < join_keys[0].size(); j++) {
            size_t found = key_1_.find(join_keys[1][j]);
            if (found != std::string::npos) {
                ReplaceAll(key_1_, join_keys[1][j], join_keys[0][j]);
            }
        }
        write_cols_out.push_back(key_0_);
        write_cols.push_back(key_1_);
    }
#ifdef USER_DEBUG
    std::cout << "3. extractWcols" << std::endl;
    for (size_t i = 0; i < write_cols.size(); i++) {
        std::cout << write_cols_out[i] << "<--" << write_cols[i] << " ";
    }
    std::cout << std::endl;
#endif
}

void JoinConfig::extractEvals(std::string eval_str, std::vector<std::string> col_names) {
    std::vector<std::string> eval_cols_1;
    if (eval_str == "") return;
    std::map<int, int> kk;
    for (size_t i = 0; i < col_names.size(); i++) {
        size_t found = eval_str.find(col_names[i]);
        if (found != std::string::npos) {
            kk.insert(std::make_pair(found, i));
        }
    }
    for (auto it = kk.begin(); it != kk.end(); ++it) {
        // int pos = it->first;
        int ind = it->second;
        eval_cols_1.push_back(col_names[ind]);
    }
    eval_cols.push_back(eval_cols_1);
#ifdef USER_DEBUG
    std::cout << "extractEvals:" << eval_str << std::endl;
    for (size_t i = 0; i < eval_cols.size(); i++) {
        std::cout << "Eval keys in eval " << i << std::endl;
        for (size_t j = 0; j < eval_cols[i].size(); j++) {
            std::cout << eval_cols[i][j] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
#endif
}
// private function

std::vector<std::string> JoinConfig::evalStrRep(std::initializer_list<std::string> evals) {
#ifdef USER_DEBUG
    std::cout << "4. evalStrRep" << std::endl;
#endif
    std::vector<std::string> eval_strs;
    for (size_t i = 0; i < evals.size(); i++) {
        std::string ss = *(evals.begin() + i);
        for (size_t j = 0; j < join_keys[0].size(); j++) {
            size_t found = ss.find(join_keys[1][j]);
            if (found != std::string::npos) {
                ReplaceAll(ss, join_keys[1][j], join_keys[0][j]);
                // int len = ss.length();
                // ss.replace(found, len, join_keys[0][i]);
            }
        }
#ifdef USER_DEBUG
        std::cout << i << ":" << *(evals.begin() + i) << " -> " << ss << std::endl;
#endif
        eval_strs.push_back(ss);
    }
    return eval_strs;
}
// for filter

void JoinConfig::ReplaceAll(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
}
// 1. set sw shuffle list
// sw_shuffle is used for part scan when part_flag is ture, and join scan when par_flag is false and L3 choose right
// column (sw _scan)
// 2. return scan_list, only be used when part_flag is true, used for join scan ahdn L3 choose right column from
// partition out (sw scan)

ErrCode JoinConfig::shuffle(const int tab_id,
                            std::string& filter_str,
                            std::vector<std::string>& col_names,
                            bool part_flag,
                            std::vector<std::string> strs,
                            bool replace) {
    std::vector<int> shuffle_last;
    std::map<int, int> kk_key_filter; // both join key and filter key
    std::map<int, int> kk_filter;     // only join key
    std::map<int, int> kk_key;        // only filter key
    for (size_t i = 0; i < col_names.size(); i++) {
        int found_key = findJoinInd(join_keys[tab_id], col_names[i]);
        size_t found_filter = filter_str.find(col_names[i]);
        if (found_key != -1 && filter_str != "" && found_filter != std::string::npos) {
            kk_key_filter.insert(std::make_pair(found_key, (int)i));
        } else if (found_key != -1) {
            kk_key.insert(std::make_pair(found_key, (int)i));
        } else if (filter_str != "" && found_filter != std::string::npos) {
            kk_filter.insert(std::make_pair((int)found_filter, (int)i));
        } else {
            shuffle_last.push_back(i);
        }
    }
    // when part_flag is ture, use
    std::vector<int8_t> scan_list; // for sol2 join sw shuffle
    std::vector<int8_t> filter_keys_tab;
    for (auto it = kk_key_filter.begin(); it != kk_key_filter.end(); ++it) {
        filter_keys_tab.push_back(it->second);
    }
    if (part_flag) {
        // add join key first, due to kernel requirements
        for (auto it = kk_key.begin(); it != kk_key.end(); ++it) {
            filter_keys_tab.push_back(it->second);
        }
        for (auto it = kk_filter.begin(); it != kk_filter.end(); ++it) {
            filter_keys_tab.push_back(it->second);
        }
        if (filter_keys_tab.size() > 4) {
            std::cout << "This case use part kernel to filter, Most support filter column number is: "
                      << (4 - kk_key.size()) << std::endl;
            exit(1);
        }
        for (size_t i = 0; i < shuffle_last.size(); i++) {
            if (std::find(write_cols.begin(), write_cols.end(), col_names[shuffle_last[i]]) != write_cols.end()) {
                filter_keys_tab.push_back(shuffle_last[i]);
            }
        }
#ifdef USER_DEBUG
///////////////////////////////////////////////////////
#endif
    } else {
        if (kk_key_filter.size() + kk_filter.size() > 4) {
            std::cout << "This case use join kernel to filter, Most support filter column number is: " << (4)
                      << std::endl;
            exit(1);
        }
        // in sol0|sol1, in input seq, filter move headers, supporting more filters
        for (auto it = kk_filter.begin(); it != kk_filter.end(); ++it) {
            filter_keys_tab.push_back(it->second);
        }
        for (auto it = kk_key.begin(); it != kk_key.end(); ++it) {
            filter_keys_tab.push_back(it->second);
        }
        for (size_t i = 0; i < shuffle_last.size(); i++) {
            if (std::find(write_cols.begin(), write_cols.end(), col_names[shuffle_last[i]]) != write_cols.end())
                filter_keys_tab.push_back(shuffle_last[i]);
        }
    }
    // sw shuffle(sol0-sol1 or sol2 part). then real input sequence
    scan_sw_shuf1[tab_id] = filter_keys_tab;
    // get the real shuffled column names
    std::vector<std::string> col_names_helper;
    std::copy(col_names.begin(), col_names.end(), std::back_inserter(col_names_helper));
    col_names.resize(filter_keys_tab.size());
    for (size_t i = 0; i < filter_keys_tab.size(); i++) {
        col_names[i] = col_names_helper[filter_keys_tab[i]];
    }
#ifdef USER_DEBUG
    std::cout << "After (SW)Scan shuffle, colum names: ";
    for (size_t i = 0; i < col_names.size(); i++) {
        std::cout << col_names[i] << " ";
    }
    std::cout << std::endl;
#endif
    // Replace filter str,  based on the firset SW shuffled column sequence
    for (size_t i = 0; i < kk_key_filter.size(); i++) {
        int ind = i;
        if (replace) {
            ReplaceAll(filter_str, col_names[ind], strs[ind]);
        }
    }
    for (size_t i = 0; i < kk_filter.size(); i++) {
        int ind = kk_key_filter.size();
        if (part_flag) ind += kk_key.size();
        if (replace) {
            ReplaceAll(filter_str, col_names[ind], strs[ind]);
        }
    }
    // maybe join key is also filter key
    // In sol2, get scan list specially for join
    if (part_flag) {
        for (size_t i = 0; i < col_names.size(); i++) {
            int found_key = findJoinInd(join_keys[tab_id], col_names[i]);
            int found_write = findJoinInd(write_cols, col_names[i]);
            if (found_key != -1 || found_write != -1) {
                scan_list.push_back(i);
            }
        }
        scan_sw_shuf2[tab_id] = scan_list;
        col_names_helper.clear();
        std::copy(col_names.begin(), col_names.end(), std::back_inserter(col_names_helper));
        col_names.resize(scan_list.size());
        for (size_t i = 0; i < scan_list.size(); i++) {
            col_names[i] = col_names_helper[scan_list[i]];
        }
#ifdef USER_DEBUG
        std::cout << "After (SW)Scan shuffle(specially for join), colum names: ";
        for (size_t i = 0; i < col_names.size(); i++) {
            std::cout << col_names[i] << " ";
        }
        std::cout << std::endl;
#endif
    }

#ifdef USER_DEBUG
/*
for (int i = 0; i < scan_list.size(); i++) {
    std::cout << (int)scan_list[i] << ", ";
}
std::cout << std::endl;
std::cout << filter_str << std::endl;
*/
#endif

    return SUCCESS;
}

int JoinConfig::findJoinInd(std::vector<std::string> join_str, std::string ss) {
    auto pos_iter = std::find(join_str.begin(), join_str.end(), ss);
    if (pos_iter != join_str.end()) return (pos_iter - join_str.begin());
    return -1;
}
// for join

std::vector<int8_t> JoinConfig::shuffle(std::vector<std::string> join_str, std::vector<std::string>& col_names) {
    std::vector<int8_t> shuffle0_last;
    std::map<int, int> kk;
    for (size_t i = 0; i < col_names.size(); i++) {
        if (join_str.size() == 0) {
            shuffle0_last.push_back(i);
        } else {
            int found = findJoinInd(join_str, col_names[i]);
            if (found != -1) {
                kk.insert(std::make_pair(found, i));
            } else {
                shuffle0_last.push_back(i);
            }
        }
    }
    std::vector<int8_t> shuffle0;
    std::vector<std::string> col_names_helper;
    std::copy(col_names.begin(), col_names.end(), std::back_inserter(col_names_helper));
    for (auto it = kk.begin(); it != kk.end(); ++it) {
        // int pos = it->first;
        int ind = it->second;
        shuffle0.push_back(ind);
    }
    shuffle0.insert(shuffle0.end(), shuffle0_last.begin(), shuffle0_last.end());
    col_names.resize(shuffle0.size());
    for (size_t i = 0; i < shuffle0.size(); i++) {
        col_names[i] = col_names_helper[shuffle0[i]];
    }
    return shuffle0;
}

void JoinConfig::compressCol(std::vector<int8_t>& shuffle,
                             std::vector<std::string>& col_names,
                             std::vector<std::string> ref_cols) {
    for (size_t i = 0; i < shuffle.size(); i++) {
        auto pos_iter = std::find(ref_cols.begin(), ref_cols.end(), col_names[i]);
        if (pos_iter == ref_cols.end()) {
            col_names.erase(col_names.begin() + i);
            shuffle.erase(shuffle.begin() + i);
            i--;
        }
    }
}
// for eval

std::vector<int8_t> JoinConfig::shuffle(size_t eval_id,
                                        std::string& eval_str,
                                        std::vector<std::string>& col_names,
                                        bool replace,
                                        std::vector<std::string> strs) {
    // kk.key - the index by findin
    std::vector<int8_t> shuffle0_last;
    std::map<int, int> kk;
    for (size_t i = 0; i < col_names.size(); i++) {
        if (col_names[i] == "undefined") continue;
        if (eval_str == "") {
            shuffle0_last.push_back(i);
        } else {
            size_t found = eval_str.find(col_names[i]);
            if (found != std::string::npos) {
                kk.insert(std::make_pair(found, i));
            } else {
                shuffle0_last.push_back(i);
            }
        }
    }
    std::vector<int8_t> shuffle0;
    std::vector<std::string> col_names_helper;
    std::copy(col_names.begin(), col_names.end(), std::back_inserter(col_names_helper));
    int i = 0;
    for (auto it = kk.begin(); it != kk.end(); ++it) {
        // int pos = it->first;
        int ind = it->second;
        shuffle0.push_back(ind);
        if (replace) {
            // int len = col_names[ind].length();
            // eval_str.replace(pos, len, strs[i]);
            ReplaceAll(eval_str, col_names[ind], strs[i]);
            i++;
        }
    }
    shuffle0.insert(shuffle0.end(), shuffle0_last.begin(), shuffle0_last.end());
    col_names.resize(shuffle0.size());
    for (size_t i = 0; i < shuffle0.size(); i++) {
        col_names[i] = col_names_helper[shuffle0[i]];
    }
    // if (shuffle0.size() > 8) {
    std::vector<std::string> cols_ref;
    std::copy(write_cols.begin(), write_cols.end(), std::back_inserter(cols_ref));
    if (eval_id < eval_cols.size()) {
        cols_ref.insert(cols_ref.end(), eval_cols[eval_id].begin(), eval_cols[eval_id].end());
    }
    if (eval_id == 0 && eval_cols.size() > 1) {
        cols_ref.insert(cols_ref.end(), eval_cols[1].begin(), eval_cols[1].end());
    }
    compressCol(shuffle0, col_names, cols_ref);
    //}
    CHECK_0(col_names, 8, "After eval" + eval_id);
    return shuffle0;
}

JoinConfig::JoinConfig(Table a,
                       std::string filter_a,
                       Table b,
                       std::string filter_b,
                       std::string join_str, // comma separated
                       std::initializer_list<std::string> evals,
                       std::initializer_list<std::initializer_list<int> > evals_const,
                       Table c,
                       // std::initializer_list<std::string> output_map_list,
                       std::string output_str,
                       int join_type,
                       bool part_flag) {
    std::vector<std::string> a_col_names = a.getColNames();
    std::vector<std::string> b_col_names = b.getColNames();
    int table_b_valid_col_num = b_col_names.size();
    CHECK_0(a_col_names, 8, "scanA");
    CHECK_0(b_col_names, 8, "scanB");
    xf::database::internals::filter_config::trim(join_str);
    xf::database::internals::filter_config::trim(output_str);
    if_filter_l = true;
    if (filter_a == "") if_filter_l = false;

#ifdef USER_DEBUG
    std::cout << "1. Get cols form table" << std::endl;
    for (size_t i = 0; i < a_col_names.size(); i++) {
        std::cout << a_col_names[i] << " ";
    }
    std::cout << std::endl;
    for (size_t i = 0; i < b_col_names.size(); i++) {
        std::cout << b_col_names[i] << " ";
    }
    std::cout << std::endl;
#endif
    extractKeys(join_str);
    extractWcols(output_str);
    std::vector<std::string> eval_strs = evalStrRep(evals);
    scan_sw_shuf1.resize(2);
    scan_sw_shuf2.resize(2);
#ifdef USER_DEBUG
    std::cout << "----------------Cfg init done----------------" << std::endl;
    std::cout << "join_type:" << join_type << std::endl;
#endif
    // Scan
    using jcmdclass = xf::database::gqe::JoinCommand;
    jcmdclass jcmd = jcmdclass();
    jcmdclass pcmd = jcmdclass();
    if (join_keys[0].size() == 2) {
        jcmd.setDualKeyOn();
        pcmd.setDualKeyOn();
    }
    jcmd.setJoinType(join_type);
    pcmd.setJoinType(join_type);

    // Filter
    // jcmd.setFilter(0, "19940101<=b && b<19950101");
    ErrCode err = shuffle(0, filter_a, a_col_names, part_flag);
    if (err != SUCCESS) {
        std::cout << "ERROR:filter columns must in first 4 columns" << std::endl;
        exit(1);
    }
    if (part_flag) {
        pcmd.Scan(0, scan_sw_shuf1[0]);
        jcmd.Scan(0, scan_sw_shuf2[0]);
        if (filter_a != "") pcmd.setFilter(0, filter_a);
    } else {
        jcmd.Scan(0, scan_sw_shuf1[0]);
        if (filter_a != "") jcmd.setFilter(0, filter_a);
    }
    err = shuffle(1, filter_b, b_col_names, part_flag);
    if (err != SUCCESS) {
        std::cout << "ERROR:filter columns must in first 4 columns" << std::endl;
        exit(1);
    }
    if (part_flag) {
        pcmd.Scan(1, scan_sw_shuf1[1]);
        jcmd.Scan(1, scan_sw_shuf2[1]);
        if (filter_b != "") pcmd.setFilter(1, filter_b);
    } else {
        jcmd.Scan(1, scan_sw_shuf1[1]);
        if (filter_b != "") jcmd.setFilter(1, filter_b);
    }
#ifdef USER_DEBUG
    std::cout << "1. Scan(Shuffle0/sw_shuffle) a: ";
    for (size_t i = 0; i < scan_sw_shuf1[0].size(); i++) {
        std::cout << (int)scan_sw_shuf1[0][i] << " ";
    }
    std::cout << std::endl;
    if (part_flag) {
        std::cout << "  1.1 Scan a for sol2 join: ";
        for (size_t i = 0; i < scan_sw_shuf2[0].size(); i++) {
            std::cout << (int)scan_sw_shuf2[0][i] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "2. Scan(Shuffle0/sw_shuffle) b: ";
    for (size_t i = 0; i < scan_sw_shuf1[1].size(); i++) {
        std::cout << (int)scan_sw_shuf1[1][i] << " ";
    }
    std::cout << std::endl;
    if (part_flag) {
        std::cout << "  1.2 Scan b for sol2 join: ";
        for (size_t i = 0; i < scan_sw_shuf2[1].size(); i++) {
            std::cout << (int)scan_sw_shuf2[1][i] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "filter_a str:" << filter_a << std::endl;
    std::cout << "filter_b str:" << filter_b << std::endl;
    std::cout << "After Scan and Shuffle0_a, colum names: ";
    for (size_t i = 0; i < a_col_names.size(); i++) {
        std::cout << a_col_names[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "After Scan and Shuffle0_b, colum names: ";
    for (size_t i = 0; i < b_col_names.size(); i++) {
        std::cout << b_col_names[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "----------------Scan and Shuffle0 done----------------" << std::endl;
#endif
    // Join Keys
    // jcmd.setShuffle1(0, {0, 1});    // setJoinKeys
    // jcmd.setShuffle1(1, {0, 1, 2}); // setJoinKeys
    int join_keys_num = 1;
    std::vector<std::string> col_names;
    if (join_str != "") {
        join_keys_num = join_keys[0].size();
        std::vector<int8_t> shuffle1_a_ = shuffle(join_keys[0], a_col_names);
        std::vector<int8_t> shuffle1_b_ = shuffle(join_keys[1], b_col_names);
#ifdef USER_DEBUG
        std::cout << "1. Shuffle1_a: ";
        for (size_t i = 0; i < shuffle1_a_.size(); i++) {
            std::cout << (int)shuffle1_a_[i] << " ";
        }
        std::cout << std::endl;
        std::cout << "2. Shuffle1_b: ";
        for (size_t i = 0; i < shuffle1_b_.size(); i++) {
            std::cout << (int)shuffle1_b_[i] << " ";
        }
        std::cout << std::endl;
        std::cout << "after Shuffle1_a colum names: ";
        for (size_t i = 0; i < a_col_names.size(); i++) {
            std::cout << a_col_names[i] << " ";
        }
        std::cout << std::endl;
        std::cout << "after Shuffle1_b colum names: ";
        for (size_t i = 0; i < b_col_names.size(); i++) {
            std::cout << b_col_names[i] << " ";
        }
        std::cout << std::endl;
#endif
        jcmd.setShuffle1(0, shuffle1_a_); // setJoinKeys
        jcmd.setShuffle1(1, shuffle1_b_); // setJoinKeys
        std::copy(b_col_names.begin() + join_keys_num, b_col_names.end(), std::back_inserter(col_names));
        for (size_t i = col_names.size(); i < 6; i++) {
            col_names.push_back("undefined");
        }
        col_names.insert(col_names.end(), a_col_names.begin() + join_keys_num, a_col_names.end());
        for (size_t i = col_names.size(); i < 12; i++) {
            col_names.push_back("undefined");
        }
        col_names.insert(col_names.end(), a_col_names.begin(), a_col_names.begin() + join_keys_num);
        // CHECK_0(col_names, 14, "Join");

    } else {
        if (table_b_valid_col_num != 0) {
            std::cout << "WARNING:passby and  ignore table b." << std::endl;
        }
        std::copy(a_col_names.begin(), a_col_names.end(), std::back_inserter(col_names));
    }
#ifdef USER_DEBUG
    std::cout << "After shuffle1 Merge, column names: ";
    for (size_t i = 0; i < col_names.size(); i++) {
        std::cout << col_names[i] << " ";
    }
    std::cout << std::endl;
#endif
#ifdef USER_DEBUG
    std::cout << "----------------Shuffle1 done----------------" << std::endl;
#endif

    // shuffle4_ for shuffle2, no shuffle4 now
    std::vector<int8_t> shuffle4_;
    for (size_t i = 0; i < write_cols.size(); i++) {
        std::string ss = write_cols[i];
        auto pos_iter = std::find(col_names.begin(), col_names.end(), ss);
        if (pos_iter != col_names.end()) {
            int8_t index = pos_iter - col_names.begin();
            shuffle4_.push_back(index);
        }
    }
    jcmd.setShuffle2(shuffle4_);
    // writeout
    std::vector<int8_t> wr;
    for (size_t i = 0; i < shuffle4_.size(); i++) wr.push_back(i);
    jcmd.setWriteCol(wr);
#ifdef USER_DEBUG
    std::cout << "Shuffle2: ";
    for (size_t i = 0; i < shuffle4_.size(); i++) {
        std::cout << (int)shuffle4_[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "----------------Shuffle2 done----------------" << std::endl;
    std::cout << "WriteOut: ";
    for (size_t i = 0; i < wr.size(); i++) {
        std::cout << (int)wr[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "----------------WriteOut done----------------" << std::endl;
#endif

    ap_uint<512>* config_bits = jcmd.getConfigBits();
    table_join_cfg = mm.aligned_alloc<ap_uint<512> >(9);
    memcpy(table_join_cfg, config_bits, sizeof(ap_uint<512>) * 9);

    ap_uint<512>* config_bits_part = pcmd.getConfigBits();
    table_part_cfg = mm.aligned_alloc<ap_uint<512> >(9);
    memcpy(table_part_cfg, config_bits_part, sizeof(ap_uint<512>) * 9);
}

ap_uint<512>* JoinConfig::getJoinConfigBits() const {
    return table_join_cfg;
}

ap_uint<512>* JoinConfig::getPartConfigBits() const {
    return table_part_cfg;
}

std::vector<std::vector<int8_t> > JoinConfig::getScanSwShuf1() const {
    return scan_sw_shuf1;
}

std::vector<std::vector<int8_t> > JoinConfig::getScanSwShuf2() const {
    return scan_sw_shuf2;
}
bool JoinConfig::getIfFilterL() const {
    return if_filter_l;
}

} // database
} // gqe
} // xf
