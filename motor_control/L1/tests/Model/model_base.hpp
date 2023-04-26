/*
Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

Except as contained in this notice, the name of Advanced Micro Devices
shall not be used in advertising or otherwise to promote the sale,
use or other dealings in this Software without prior written authorization
from Advanced Micro Devices, Inc.
*/
#ifndef _MODEL_BASE_HPP_
#define _MODEL_BASE_HPP_
//#define _DEBUG_SCREEN_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <assert.h>
#include <algorithm>
#include <unistd.h>
#include <termios.h>

template <class T>
T clamp(T& v, T vmax) {
    if (v > vmax)
        v = vmax;
    else if (v < -vmax)
        v = -vmax;
    return v;
}

static char getch2(void) {
    char buf = 0;
    struct termios old = {0};
    fflush(stdout);
    if (tcgetattr(0, &old) < 0) perror("tcsetattr()");
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &old) < 0) perror("tcsetattr ICANON");
    if (read(0, &buf, 1) < 0) perror("read()");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &old) < 0) perror("tcsetattr ~ICANON");
    // printf("%c\n", buf);
    return buf;
}
static int strcmp_space(char* str_s, char* str) {
    if (strlen(str_s) < strlen(str)) return 2;
    for (int i = 0; i < strlen(str); i++)
        if (str_s[i] != str[i]) return 1;
    return 0;
}

struct myRect {
    int left_;
    int right_;
    int top_;
    int bottom_;
    char str_info[100];
    myRect(int left, int right, int top, int bottom) { set(left, right, top, bottom); }
    myRect() { set(0, 0, 0, 0); }
    void set(int left, int right, int top, int bottom) {
        left_ = left;
        top_ = top;
        right_ = right;
        bottom_ = bottom;
    }
    void set(myRect& rct) {
        left_ = rct.left_;
        top_ = rct.top_;
        right_ = rct.right_;
        bottom_ = rct.bottom_;
    }
    int width() { return right_ - left_; }
    int height() { return bottom_ - top_; }
    void print() {
        printf("RCTDBG::<%d, %d, %d, %d>\n", left_, right_, top_, bottom_);
        sprintf(str_info, "<%d, %d, %d, %d>", left_, right_, top_, bottom_);
        // return str_info;
    }
    void print(char* nm) {
        assert(nm != NULL);
        printf("RCTDBG::%s=<%d, %d, %d, %d>\n", nm, left_, right_, top_, bottom_);
        sprintf(str_info, "%s=<%d, %d, %d, %d>", nm, left_, right_, top_, bottom_);
        // return str_info;
    }
    void print(char* nm1, char* nm2) {
        assert(nm1 != NULL);
        assert(nm2 != NULL);
        printf("RCTDBG::%s.%s=<%d, %d, %d, %d>", nm1, nm2, left_, right_, top_, bottom_);
        sprintf(str_info, "%s.%s=<%d, %d, %d, %d>", nm1, nm2, left_, right_, top_, bottom_);
        // return str_info;
    }
    char* sprint() {
        // printf("RCTDBG::<%d, %d, %d, %d>\n", left_, right_, top_, bottom_);
        sprintf(str_info, "<%d, %d, %d, %d>", left_, right_, top_, bottom_);
        return str_info;
    }
    char* sprint(char* nm) {
        assert(nm != NULL);
        // printf("RCTDBG::%s=<%d, %d, %d, %d>\n", nm, left_, right_, top_, bottom_);
        sprintf(str_info, "%s=<%d, %d, %d, %d>", nm, left_, right_, top_, bottom_);
        return str_info;
    }
    char* sprint(char* nm1, char* nm2) {
        assert(nm1 != NULL);
        assert(nm2 != NULL);
        // printf("RCTDBG::%s.%s=<%d, %d, %d, %d>", nm1, nm2, left_, right_, top_, bottom_);
        sprintf(str_info, "%s.%s=<%d, %d, %d, %d>", nm1, nm2, left_, right_, top_, bottom_);
        return str_info;
    }
    bool isIn_y(int y) { return (y >= top_) & (y < bottom_); }
    bool isIn_x(int x) { return (x >= left_) & (x < right_); }
    bool isIn(int y, int x) { return isIn_x(x) & isIn_y(y); }
    bool isValid() { return width() > 0 && height() > 0; }
};

class Model_base {
   public:
    enum TYPE_M { MODEL_BASE = 0, MODEL_MOTOR, MODEL_PID, MODEL_FOC, MODEL_QEI, MODEL_SIM };
    TYPE_M id_type; // should be initialized by sub-class
    static const int num_type = MODEL_SIM + 1;
    const char* name_type[num_type] = {"MODEL_BASE", "MODEL_MOTOR", "MODEL_PID", "MODEL_FOC", "MODEL_QEI", "MODEL_SIM"};
    int ID;

    enum TYPE_PARA { T_DOUBLE = 0, T_FLOAT, T_INT, T_STR, T_BOOL, T_UNKNOWN };
    Model_base* list_model[100]; // list to contain the models for base-class-based virtual operations like printing,
                                 // pulling input, pushing output and updating ...
    int num_models;
    void* list_pPara[100];
    TYPE_PARA list_paraType[100];
    int num_para;  // number of parameter about model which will be record, set or displayed
    double dt_sim; // local time interval for simulation
    double t_cur;  //	absolut time
    static const int max_para = 100;
    static const int len_name = 12;
    static const int len_value = 10;
    static const int len_screen = 26;
    char* str_names[max_para];
    char str_values[max_para][len_value];
    char str_screen[max_para][len_screen];
    char name_obj[len_screen]; // should be initialized by sub-class's instances
    char full_name[len_screen];
    myRect rect_all;  // screen can be used for displaying parameters
    myRect rect_name; //
    myRect rect_para; //
    Model_base() {
        id_type = MODEL_BASE;
        strcpy(name_obj, "base");
        num_models = 0;
        num_para = 0;
        t_cur = 0;
        dt_sim = 0.01;
        init_pPara();
        init_ParaType();
    }
    void setObjName(char* nm) {
        assert(nm != NULL);
        strcpy(name_obj, nm);
    };
    virtual void connectSubModels(){};
    virtual void pullInput(){};
    virtual void pushOutput(){};
    virtual void updating(double dt){};
    // virtual void prepareScreen(){}; //preparing content for displaying on screen by ncurses, mainly by transfroming
    // variables to strings
    virtual void printParameters(FILE*, int){}; // printing all parameter on stdio or file
    const char* GetTypeStr() { return name_type[id_type]; }
    const char* GetFullName() {
        sprintf(full_name, "%s.%s", GetTypeStr(), name_obj);
        return full_name;
    }
    virtual void init_ParaType() {
        for (int i = 0; i < num_models; i++) this->list_model[i]->init_ParaType();
        for (int i = 0; i < num_para; i++) list_paraType[i] = T_DOUBLE;
    };
    virtual int setPara(char* str_nm, char* str_vl) { return 0; }
    virtual void screen_assign(myRect& rct) {
        // an obj can take a rect range from the remaining rect area, and the remaining part must be a rect.
        // so the algorithm can take some lines firstly and then take some columns with width defined by 'len_screen'
        // 1) checking space
        // rct.print("befor", name_obj);
        if (rct.width() < len_screen || rct.height() < 1) return;
        if (num_para == 0 && num_models == 0) return;

        myRect r_tmp;
        rect_all.set(rct);
        // now not sure the right_'s value
        r_tmp.set(rct);
        rect_name.set(r_tmp.left_, r_tmp.left_ + len_screen, r_tmp.top_ + 0, r_tmp.top_ + 1);
        r_tmp.top_++;
        if (num_para != 0) {
            rect_para.set(r_tmp.left_, r_tmp.left_ + len_screen, r_tmp.top_ + 1, r_tmp.bottom_);
            r_tmp.left_ += len_screen;
        }
        for (int i = 0; i < num_models; i++) this->list_model[i]->screen_assign(r_tmp);

        rct.left_ = r_tmp.left_;
        rect_all.right_ = rct.left_;

        // r_tmp.print(name_obj, "r_tmp");
        // rct.print(name_obj, "rct");
        // rect_all.print(name_obj, "all");
        // rect_name.print(name_obj, "name");
        // rect_para.print(name_obj, "para");
    }
    void screen_addch(myRect rct, char ch) {
#ifndef _DEBUG_SCREEN_
        mvaddch(rct.top_, rct.left_, ch);
#else
        printf("%d\t %d\t %c", rct.top_, rct.left_, ch);
#endif
    }
    void screen_addch(int lin, int col, char ch) {
#ifndef _DEBUG_SCREEN_
        mvaddch(lin, col, ch);
#else
        printf("%d\t %d\t %c", lin, col, ch);
#endif
    }

    void screen_addstr(myRect rct, char* str, bool isStandout) {
#ifndef _DEBUG_SCREEN_
        if (isStandout) {
            standout();
            mvaddnstr(rct.top_, rct.left_, str, rct.width());
            standend();
        } else
            mvaddnstr(rct.top_, rct.left_, str, rct.width());
#else
        printf("%d\t %d\t %s\n", rct.top_, rct.left_, str);
#endif
    }
    void screen_addstr_stand(myRect rct, char* str) {
#ifndef _DEBUG_SCREEN_
        screen_addstr(rct, str, true);
#else
        printf("%d\t %d\t %s\n", rct.top_, rct.left_, str);
#endif
    }
    void screen_addstr(myRect rct, char* str) {
#ifndef _DEBUG_SCREEN_
        screen_addstr(rct, str, false);
#else
        printf("%d\t %d\t %s\n", rct.top_, rct.left_, str);
#endif
    }
    void screen_addstr(int lin, int col, char* str) {
#ifndef _DEBUG_SCREEN_
        mvaddstr(lin, col, str);
#else
        printf("%d\t %d\t %s\n", lin, col, str);
#endif
    }
    void screen_clear() {
#ifndef _DEBUG_SCREEN_
        clear();
#endif
    }
    void screen_showRect() {
        screen_addstr_stand(rect_name, rect_name.sprint(name_obj));
        if (num_para != 0) screen_addstr(rect_para, rect_para.sprint("num_para"));
        for (int i = 0; i < num_models; i++) this->list_model[i]->screen_showRect();
    }
    virtual void init_pPara() {
        for (int i = 0; i < num_models; i++) this->list_model[i]->init_pPara();
    }
    virtual void prepareScreen() {
        for (int i = 0; i < num_models; i++) this->list_model[i]->prepareScreen();
    }
    virtual void stop() {
        for (int i = 0; i < num_models; i++) this->list_model[i]->stop();
    }
    virtual Model_base* findFirstPara(myRect& rct, char* str_nm, void* ppPara[1], int& y, int& x) {
        if (num_para != 0 && rect_para.isValid()) {
            int idx_para = 0;
            strcpy(str_nm, this->str_names[0]);
            *ppPara = list_pPara[idx_para];
            rct.set(rect_para);
            rct.bottom_ = rct.top_ + 1;
            x = rct.left_;
            y = rct.top_;
            return this;
        }

        Model_base* ret = NULL;
        for (int i = 0; i < num_models; i++) {
            ret = this->list_model[i]->findFirstPara(rct, str_nm, ppPara, y, x);
            if (ret) return ret;
        }
        return NULL;
    }

    virtual Model_base* findParaInScreen(myRect& rct, char* str_nm, char* str_vl, void* ppPara[1], int y, int x) {
        Model_base* ret = NULL;
        for (int i = 0; i < num_models; i++) {
            ret = this->list_model[i]->findParaInScreen(rct, str_nm, str_vl, ppPara, y, x);
            if (ret) return ret;
        }

        if (num_para == 0) return NULL;
        if (!this->rect_para.isIn(y, x)) return NULL;

        int idx_para = y - rect_para.top_;
        if (idx_para >= num_para) {
            idx_para = num_para - 1;
            y = rect_para.top_ + num_para - 1;
        }

        strcpy(str_nm, this->str_names[idx_para]);
        strcpy(str_vl, (this->str_values[idx_para]));
        *ppPara = list_pPara[idx_para];
        rct.set(rect_para);
        rct.top_ += idx_para;
        rct.bottom_ = rct.top_ + 1;
        return this;
    }
    int findParaByName(char* nm_para) {
        int ret = -1;
        for (int i = 0; i < num_para; i++)
            if (0 == strcmp_space(str_names[i], nm_para)) {
                ret = i;
            }
        return ret;
    }
    virtual int setParaByName(char* nm_para, char* str_vl) {
        int idx = findParaByName(nm_para);
        if (idx == -1) return -1;
        if (list_paraType[idx] == T_DOUBLE)
            *(double*)list_pPara[idx] = atof(str_vl);
        else if (list_paraType[idx] == T_FLOAT)
            *(float*)list_pPara[idx] = atof(str_vl);
        else if (list_paraType[idx] == T_INT)
            *(int*)list_pPara[idx] = atoi(str_vl);
        else if (list_paraType[idx] == T_STR)
            strcpy((char*)(list_pPara[idx]), str_vl);
        else if (list_paraType[idx] == T_BOOL) {
            if (strcpy("true", str_vl) == 0)
                *(bool*)list_pPara[idx] = true;
            else
                *(bool*)list_pPara[idx] = false;
        }

        return idx;
    }
    virtual bool findParaByName(void* ppPara[1], char* nm_obj, char* nm_para) {
        if (0 != strcmp(name_obj, nm_obj)) return false;
        for (int i = 0; i < num_para; i++)
            if (0 == strcmp_space(str_names[i], nm_para)) {
                *ppPara = list_pPara[i];
            }
        for (int i = 0; i < num_models; i++)
            if (0 == this->list_model[i]->findParaByName(ppPara, nm_obj, nm_para)) return true;
        return false;
    }
    void screen_updating_auto() {
        if (num_para == 0 && num_models == 0) return;
        screen_addstr_stand(rect_name, name_obj);
        myRect rtmp;

        for (int i = 0; i < num_models; i++) {
            this->list_model[i]->screen_updating_auto();
        }

        if (num_models > 0) {
            rtmp.set(this->rect_name);
            rtmp.left_ = this->rect_name.left_ + strlen(this->name_obj);
            for (int i = 0; i < num_models; i++) {
                if (rtmp.right_ < this->list_model[i]->rect_name.left_ + 1)
                    rtmp.right_ = this->list_model[i]->rect_name.left_ + 1;
            }
            screen_fillRect(rtmp, '-');
            for (int i = 0; i < num_models; i++) {
                int col = this->list_model[i]->rect_name.left_;
                if (col > rtmp.left_) screen_addch(rtmp.top_, col + 2, '|');
            }
        }

        if (num_para == 0) return;
        int col = rect_para.left_;
        int lin = rect_para.top_;
        int screen_max_y = rect_para.bottom_;
        int i = 0;
        if (rect_para.height() == 0 || rect_para.width() == 0) return;
        for (int i = 0; i < std::min(num_para, rect_para.height()); i++) {
            myRect tmpr;
            tmpr.set(col, col + this->len_screen, lin + i, lin + i + 1);
            screen_fillRect(tmpr, ' ');
            screen_addstr(tmpr, str_screen[i], false);
            // screen_addstr(lin + i, col, str_screen[i] );
        }
    }
    void screen_fillRect(myRect rct, char ch) {
        for (int y = rct.top_; y < rct.bottom_; y++)
            for (int x = rct.right_ - 1; x >= rct.left_; x--) screen_addch(y, x, ch);
    }

    char* screen_getLine(myRect rct, char* str_ret) {
        assert(str_ret);
        assert(rct.isValid());
        char ch;
        int cnt = 0;
        do {
            ch = getch();
            switch (ch) {
                case '\n':
                    str_ret[cnt] = '\0';
                    break;
                case 127: // back
                    if (cnt == 0) break;
                    cnt--;
                    mvdelch(rct.top_, rct.left_ + cnt);
                    break;
                case 27: // directions or ESC do nothing
                    break;
                case 91:     // directions
                    getch(); // consuming the comming char
                    break;
                default:
                    screen_addch(rct.top_, rct.left_ + cnt, ch);
                    str_ret[cnt] = ch;
                    if (cnt < rct.width()) cnt++;
                    break;
            }
        } while (ch != '\n');
        return str_ret;
    }
    char* screen_getLine(myRect rct, char* str_ret, char* str_header) {
        assert(str_ret);
        assert(rct.isValid());
        assert(str_header);
        int len_hd = strlen(str_header);

        myRect rct_value;
        rct_value.set(rct);
        rct_value.left_ += len_hd;
        myRect rct_header;
        rct_header.set(rct);
        rct_header.right_ = rct_header.left_ + len_hd;
        assert(rct_value.isValid());
        assert(rct_header.isValid());

        screen_fillRect(rct_value, ' ');
        screen_addstr(rct_header, str_header);
        // mvcur(rct.bottom_-1, rct.right_-1, rct.top_, rct.left_);
        return screen_getLine(rct_value, str_ret);
    }

    void drawBoard(myRect rct) {
        if (!rct.isValid()) return;
        for (int y = rct.top_; y < rct.bottom_; y++) {
            for (int x = rct.left_; x < rct.right_; x++) {
                if (y == rct.top_ || y == rct.bottom_ - 1)
                    screen_addch(y, x, '_');
                else if (x == rct.left_ || x == rct.right_ - 1)
                    screen_addch(y, x, '|');
            }
        }
    }
};

// AP_T design
//-------------------------
template <class T_NUM>
static void Park_Inverse_T_numeral(T_NUM& va_out, T_NUM& vb_out, T_NUM vd_in, T_NUM vq_in, T_NUM Theta_in) {
#pragma HLS INLINE off
    va_out = T_NUM(vd_in) * cos(Theta_in) - T_NUM(vq_in) * sin(Theta_in);
    vb_out = T_NUM(vq_in) * cos(Theta_in) + T_NUM(vd_in) * sin(Theta_in);
}

#endif // model_base.hpp