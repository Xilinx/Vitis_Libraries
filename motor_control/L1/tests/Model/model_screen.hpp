/*
Copyright (C) 2022-2022, Xilinx, Inc.
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
#ifndef _MODEL_SCREEN_HPP_
#define _MODEL_SCREEN_HPP_
//#define _DEBUG_SCREEN_
#include <ncurses.h>
#include "model_motor.hpp"
#include "model_foc.hpp"
#include <thread>
#include <unistd.h>
// Basic ideal:
// Each object only needs to care about the display of the space rect allocated to it, so a object needs to
// 1) Assign display space to itself and sub objects
// 2) Prepare its own content and let sub objects prepare the content
// 3) Refresh its own content on screen and Call objects to their allocated space
// The simulator TB will has a screen obj;

class SimulatorCtrl;

class winCmdline : public Model_base {
   public:
    myRect rect_stdin;
    myRect rect_stdout;
    int m_w;
    int m_h;
    int p_latest; // position of latest input from stdin
    char* str_lines[128];
    void init(myRect rct) {
        rect_all.set(rct);
        rect_stdout.set(rct);
        rect_stdout.top_++;
        rect_stdout.bottom_--;
        rect_stdout.bottom_--;
        rect_stdout.left_++;
        rect_stdout.right_--;
        rect_stdin.set(rect_stdout);
        rect_stdin.top_ = rect_stdout.bottom_;
        rect_stdin.bottom_ = rect_stdout.bottom_ + 1;

        m_w = rect_stdout.width();
        m_h = rect_stdout.height();
        p_latest = 0;
        for (int i = 0; i < m_h; i++) {
            str_lines[i] = (char*)malloc(m_w + 1);
            str_lines[i][0] = 0;
        }
    }
    winCmdline() {
        m_w = 0;
        m_h = 0;
        p_latest = 0;
    }
    winCmdline(myRect rct) { init(rct); }
    void addLine(char* str) {
        int len = strlen(str);
        if (len > m_w) len = m_w;
        for (int i = 0; i < len; i++) str_lines[p_latest][i] = str[i];
        str_lines[p_latest][len] = '\0';
        p_latest = (p_latest + 1) % m_h;
    }
    void display() {
        drawBoard(rect_all);
        myRect rct;
        rct.set(rect_stdout);
        rct.bottom_ = rct.bottom_ + 1;
        screen_fillRect(rct, ' '); // to clean content in window for stdout
        int idx = p_latest;
        idx = idx % m_h;
        for (int i = 0; i < m_h; i++) { // from oldest to latest
            screen_addstr(rct, str_lines[idx]);
            rct.bottom_++;
            rct.top_++;
            idx = (idx + 1) % m_h;
        }
    }
    char* inputLine(char* str_ret) {
        assert(str_ret);
        screen_getLine(rect_stdin, str_ret, "SIM\/:");
        addLine(str_ret);
        display();
        return str_ret;
    }
    void run(int argv, char** argc) {
        do {
            char tmp[128];
            screen_getLine(rect_stdin, tmp, "SIM\/:");
            addLine(tmp);
            display();
        } while (1);
    }
    ~winCmdline() {
        for (int i = 0; i < m_h; i++) {
            free(str_lines[i]);
        }
    }
};
#endif