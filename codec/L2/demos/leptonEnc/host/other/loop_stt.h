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

// loop_stt.h
#ifndef _LOOP_STT_H_
#define _LOOP_STT_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ACCESS (65536)
#define ENSTT
#define STTSTART(a) g_loops.START(a, ANY)
#define STTFOR(a) g_loops.START(a, FOR)
#define STTWHILE(a) g_loops.START(a, WHILE)
#define STTPROC(a) g_loops.START(a, PROC)
#define STTBRCH(a) g_loops.START(a, BRCH)
#define STTCNT g_loops.CNT()
#define STTEND g_loops.END()

//#define STTCNT  g_loops.CntCur();
//#define STTEND  g_loops.UpLevel();
enum NType { ANY = 0, FOR, WHILE, PROC, BRCH };
static char Prefix[5] = {'A', 'L', 'l', 'P', 'B'};
static char* Prefix_str[5] = {"A", "L", "l", "P", "B"};
class LoopNode {
   public:
    char name[256];
    char fname[256];
    int id;
    int id_son;
    NType type;
    //
    int AccessTimes; // Can be too bit to be stored by static array OnceCount
    int OnceCount[MAX_ACCESS];
    int TotalCount;
    int CurrentCount;
    int AverageCount;
    int MaxCount;
    int MinCount;
    //
    int level;
    bool hasFather;
    LoopNode* p_father;
    int num_son;
    LoopNode* p_sons[512];

   public:
    // LoopNode();
    // LoopNode( char* name);
    LoopNode(char* nm, LoopNode* pf, int id_in);
    LoopNode(char* nm, LoopNode* pf, int id_in, NType tp);
    void InitCnt() {
        for (int i = 0; i < MAX_ACCESS; i++) OnceCount[i] = 0;
    };
    void AddSon(LoopNode* pson);
    void AddFather(LoopNode* pf);
    int CntNode() {
        // TotalCount++;
        return CurrentCount++;
        // return (this->OnceCount[this->AccessTimes-1]++);
    }
    LoopNode* FindSon(char* nm) {
        for (int i = 0; i < this->num_son; i++) {
            if (strcmp(this->p_sons[i]->name, nm) == 0) return this->p_sons[i];
        }
        return 0;
    } // FindSon
    int GetTotalCount() { return TotalCount; }
    int GetAverageCount() { return GetTotalCount() / AccessTimes; }
    int GetMaxCount() { return MaxCount; }
    int GetMinCount() { return MinCount; }
    int UpdateRecord() {
        TotalCount += CurrentCount;
        if (this->MaxCount < CurrentCount) MaxCount = CurrentCount;
        if (MinCount > CurrentCount) MinCount = CurrentCount;
        return TotalCount;
    }
    void PrintSelf3(bool isMore) {
        char head[128];
        for (int i = 0; i < level; i++) head[i] = ' ';
        head[level] = 0;
        printf("%s NAME=%s; ", head, this->name);
        printf("%s L=%d, T=%d, Cnt=%d, Ave=%d, Max=%d, Min=%d \n", head, level, this->AccessTimes,
               this->GetTotalCount(), this->GetAverageCount(), this->GetMaxCount(), this->GetMinCount());
        if (isMore) {
            for (int i = 0; i < this->AccessTimes; i++) printf("[%d, %d]", AccessTimes, OnceCount[AccessTimes]);
            printf("\n");
        }
    } // PrintSelf
    void PrintSelf(bool isMore, int depth) {
        char tail[128] = "";
        for (int i = 0; i < (depth - level); i++) strcat(tail, "   ");
        printf("%s%s", this->fname, tail);
        printf(" , %s, Lvl=%d, Accss=%9d, AllCnt=%9d, Ave=%4d, Max=%4d, Min=%4d,", Prefix_str[this->type], level,
               this->AccessTimes, this->GetTotalCount(), this->GetAverageCount(), this->GetMaxCount(),
               this->GetMinCount());
        printf(" NAME=%s \n", this->name);
        if (isMore) {
            for (int i = 0; i < this->AccessTimes; i++) printf("[%d, %d]", AccessTimes, OnceCount[AccessTimes]);
            printf("\n");
        }
    } // PrintSelf2
};

class LoopNodeFactory {
   public:
    int num_node;
    int depth;
    int depth_max;
    LoopNode* p_top;
    LoopNode* p_cur;
    LoopNodeFactory() {
        num_node = 0;
        depth = 0;
        depth_max = 0;
        p_top = 0;
        p_cur = 0;
    }
    LoopNode* DownLevel(char* nm, NType tp) {
        if (num_node == 0) {
            p_top = new LoopNode(nm, 0, num_node, tp);
            p_cur = p_top;
            num_node++;
        } else {
            LoopNode* aSon = p_cur->FindSon(nm);
            if (aSon == 0) {
                LoopNode* p_new = new LoopNode(nm, p_cur, num_node, tp);
                p_cur->AddSon(p_new);
                p_cur = p_new;
                num_node++;
            } else {
                p_cur = aSon;
            }
        }
        p_cur->AccessTimes++;
        p_cur->CurrentCount = 0;
        this->depth++;
        if (depth_max < depth) depth_max = depth;
        return p_cur;
    }
    LoopNode* START(char* nm, NType tp) { return DownLevel(nm, tp); }
    LoopNode* UpLevel() {
        p_cur->UpdateRecord();
        if (depth == 0)
            return p_top;
        else
            p_cur = p_cur->p_father;
        this->depth--;
        return p_cur;
    }
    LoopNode* END() { return UpLevel(); }
    int CntCur() { return p_cur->CntNode(); }
    int CNT() { return CntCur(); }
    void PrintTree(LoopNode* p_f, bool mode) {
        p_f->PrintSelf(mode, this->depth_max);
        if (p_f->num_son == 0)
            return;
        else {
            for (int i = 0; i < p_f->num_son; i++) PrintTree(p_f->p_sons[i], mode);
        }
    }
    void PrintTree() { PrintTree(p_top, false); };
};

#endif
