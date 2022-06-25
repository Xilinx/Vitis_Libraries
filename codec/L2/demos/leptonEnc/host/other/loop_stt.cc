/*
 * Copyright 2021 Xilinx, Inc.
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

#include "loop_stt.h"

/*
LoopNode::LoopNode(){
                name[0]      = 0;
                this->id     = 0;
                AccessTimes  = 0;
                InitCnt();
                //TotalCount   = 0;
                //AverageCount = 0;
                //MaxCount     = 0;
                //MinCoun      = 0;
                num_son      = 0;
                level        = 0;
                this->p_father  = NULL;
                this->hasFather = false;
};
LoopNode::LoopNode(char* nm){

                strcpy(this->name, nm);
                this->id     = 0;
                AccessTimes  = 0;
                InitCnt();
                //TotalCount   = 0;
                //AverageCount = 0;
                //MaxCount     = 0;
                //MinCoun      = 0;
                num_son      = 0;
                level        = 0;
                this->p_father  = NULL;
                this->hasFather = false;
};
*/
LoopNode::LoopNode(char* nm, LoopNode* pf, int id_in) {
    strcpy(this->name, nm);
    this->id = id_in;
    this->type = ANY;
    AccessTimes = 0;
    InitCnt();
    TotalCount = 0;
    AverageCount = 0;
    MaxCount = 0;
    MinCount = 0x7fffffff;
    num_son = 0;
    level = 0;
    this->p_father = NULL;
    this->hasFather = false;
    this->AddFather(pf);
};

LoopNode::LoopNode(char* nm, LoopNode* pf, int id_in, NType tp) {
    strcpy(this->name, nm);
    this->id = id_in;
    this->type = tp;
    AccessTimes = 0;
    InitCnt();
    TotalCount = 0;
    AverageCount = 0;
    MaxCount = 0;
    MinCount = 0x7fffffff;
    num_son = 0;
    level = 0;
    this->p_father = NULL;
    this->hasFather = false;
    this->AddFather(pf);
};

void LoopNode::AddSon(LoopNode* pson) {
    this->p_sons[num_son] = pson;
    this->num_son++;
};

void LoopNode::AddFather(LoopNode* pf) {
    this->p_father = pf;
    this->hasFather = true;
    if (pf != 0) {
        this->level = pf->level + 1;
        this->id_son = pf->num_son;
        sprintf(fname, "%s-%s%d", pf->fname, Prefix_str[this->type], this->id_son);
    } else {
        this->level = 0;
        this->id_son = 0;
        sprintf(fname, "%s%d", Prefix_str[this->type], this->id_son);
    }
};

LoopNodeFactory g_loops;
