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

#ifndef _MODEL_SIM_HPP_
#define _MODEL_SIM_HPP_
#include <ncurses.h>
#include "model_motor.hpp"
#include "model_foc.hpp"
#include <thread>
#include <unistd.h>
#include "model_qei.hpp"
#include "model_screen.hpp"
#include "model_cmd.hpp"

#include "model_foc_int.hpp"
#include "model_adc.hpp"
#include "model_dac.hpp"
//#define _DEBUG_SCREEN_

class asynScreenIO {
   public:
    char ch;
    long i_w; //++ after input thread updated ch; when metting '\r', taking strline as output;
    long i_r; //++ after screen thread read the ch; when metting '\r', cleanning strline display;
    char strline[1000];
    char str_info[1000];
    int p_str;
    asynScreenIO() {
        strline[0] = 0;
        i_w = i_r = p_str = 0;
    }
    void setCh(char c, char* str_out) {
        while (i_w != i_r) usleep(1000);
        ch = c;
        if ('\r' == ch) {
            strcpy(str_out, strline);
            i_w = 0;
        } else {
            i_w++;
        }
    }
    void checkCh() {
        while (i_w == i_r) {
            // usleep(100);
            return;
        };
        if ('\r' == ch) {
            strline[0] = 0;
            i_r = p_str = 0;
        } else {
            strline[p_str + 1] = 0;
            strline[p_str++] = ch;
            i_r = i_w;
        }
    }
    char* getInfo() {
        sprintf(str_info, "%d, %d, %d, %c %s\n", i_w, i_r, p_str, ch, strline);
        return str_info;
    }
};
class SimulatorCtrl;
static void thread_run(SimulatorCtrl* simctrl);
static void thread_ctrl(SimulatorCtrl* simctrl);
static void thread_gui(SimulatorCtrl* simctrl);

class SimulatorCtrl : public Model_base {
   public:
    // specify models
    FOC_Simple_2<int> foc1;
    Model_motor<double, int> motor;
    ADC<double, int> adc;
    DAC<double, int> dac;

    myRect rect_title0;
    myRect rect_cmdout;
    myRect rect_cmdin;
    myRect rect_cmd;
    char str_title0[256];
    char str_cmdout[256];
    char str_cmdin[256];
    winCmdline scrn_cmd;
    // InversQEI(A,B, I, w, theta);
    // QEI()
    // time control
    double dt_sim;
    double runSec_sim;
    double runSec_sim2;
    long nStep_sim;
    long nStep_sim2;
    long allStep_sim;
    double Umax;
    enum MODE_DISP { DISP_CMD = 0, DISP_GUI } mode_disp;
    enum STATE_SIM { SIM_STOP = 0, SIM_RUN, SIM_PAUSE } state_sim;
    enum STATE_GUI { GUI_STOP = 0, GUI_INIT, GUI_READY, GUI_SIM } state_gui;
    // display control
    bool isPrint;
    bool isPrintf;
    bool isGUI;
    bool isCmd;
    char name_log[128];
    int inteval_print;  // printing when %inteval_print==0
    int inteval_screen; // screen updating when %inteval_screen==0
    int screen_max_y;
    int screen_max_x;
    enum TYPE_PARA {
        SIM_DT = 0,
        SIM_RUNSEC,
        SIM_NSTEP,
        SIM_ALLSTEP,
        SIM_ISPRT,
        SIM_ISPRTF,
        SIM_ISGUI,
        SIM_ISCMD,
        SIM_LOGNAME,
        SIM_IIPRINT,
        SIM_IISCRN,
        SIM_UMAX,
        SIM_TOTAL_NUM
    };

    int setPara(char* str_nm, char* str_vl) { return 0; }
    void connectSubModels() {
        num_models = 4;
        // COUSTION! the list order determine the sequence of updating
        list_model[0] = (Model_base*)&motor;
        list_model[1] = (Model_base*)&adc;
        list_model[2] = (Model_base*)&foc1;
        list_model[3] = (Model_base*)&dac;
        dac.input_Va_pull = &foc1.out_va;
        dac.input_Vb_pull = &foc1.out_vb;
        dac.input_Vc_pull = &foc1.out_vc;
        motor.input_va_pull = &dac.out_Va;
        motor.input_vb_pull = &dac.out_Vb;
        motor.input_vc_pull = &dac.out_Vc;

        adc.input_Ia_pull = &motor.out_va;
        adc.input_Ib_pull = &motor.out_vb;
        adc.input_Ic_pull = &motor.out_vc;
        foc1.input_Ia_pull = &adc.out_Ia;
        foc1.input_Ib_pull = &adc.out_Ib;
        foc1.input_Ic_pull = &adc.out_Ic;

        adc.input_theta_e_pull = &motor.theta_e;
        foc1.input_theta_e_pull = &adc.out_theta_e;

        adc.input_w_pull = &motor.w;
        foc1.input_w_pull = &adc.out_w;
        // connection models' ports mannylly
        // motor.input_va_pull = &foc1.out_va;
        // motor.input_vb_pull = &foc1.out_vb;
        // motor.input_vc_pull = &foc1.out_vc;
        // foc1.input_Ia_pull = &motor.out_va;
        // foc1.input_Ib_pull = &motor.out_vb;
        // foc1.input_Ic_pull = &motor.out_vc;
        // foc1.input_theta_e_pull = &motor.theta_e;
        // foc1.input_w_pull = &motor.w;
    }
    SimulatorCtrl() {
        id_type = MODEL_SIM;
        this->setObjName("sim");
        foc1.setObjName("foc1");
        motor.setObjName("motor");
        connectSubModels();
        // setting default values
        state_sim = SIM_STOP;
        state_gui = GUI_INIT;
        dt_sim = 0.0001;        // 10us
        motor.dt_sim = 0.00001; // 10us
        nStep_sim = 100000;
        nStep_sim2 = nStep_sim;
        allStep_sim = 0;
        runSec_sim = dt_sim * nStep_sim;
        runSec_sim2 = runSec_sim;
        mode_disp = DISP_CMD;
        isPrint = true;
        isPrintf = false;
        isGUI = false;
        isCmd = false;
        strcpy(name_log, "./sim.log");
        inteval_print = 100;  // printing when nStep_sim%inteval_print==0
        inteval_screen = 100; // screen updating when %inteval_screen==0

        Umax = 24;
        sync_Umax();

        str_names[SIM_DT] = "SIM_DT     ";
        str_names[SIM_RUNSEC] = "SIM_RUNSEC ";
        str_names[SIM_NSTEP] = "SIM_NSTEP  ";
        str_names[SIM_ALLSTEP] = "SIM_ALLSTEP";
        str_names[SIM_ISPRT] = "SIM_ISPRT  ";
        str_names[SIM_ISPRTF] = "SIM_ISPRTF ";
        str_names[SIM_ISGUI] = "SIM_ISGUI  ";
        str_names[SIM_ISCMD] = "SIM_ISCMD  ";
        str_names[SIM_LOGNAME] = "SIM_LOGNAME";
        str_names[SIM_IIPRINT] = "SIM_IIPRINT";
        str_names[SIM_IISCRN] = "SIM_IISCRN ";
        str_names[SIM_UMAX] = "SIM_UMAX   ";
        num_para = SIM_TOTAL_NUM;
        init_pPara();
        init_ParaType();
    }
    void prepareScreen() {
        Model_base::prepareScreen();
        sprintf(str_screen[SIM_DT], "%s : %0.6f", str_names[SIM_DT], dt_sim);
        sprintf(str_screen[SIM_RUNSEC], "%s : %3.4f", str_names[SIM_RUNSEC], runSec_sim);
        sprintf(str_screen[SIM_NSTEP], "%s : %7d", str_names[SIM_NSTEP], nStep_sim);
        sprintf(str_screen[SIM_ALLSTEP], "%s : %7d", str_names[SIM_ALLSTEP], allStep_sim);
        sprintf(str_screen[SIM_ISPRT], "%s : %d", str_names[SIM_ISPRT], isPrint);
        sprintf(str_screen[SIM_ISPRTF], "%s : %d", str_names[SIM_ISPRTF], isPrintf);
        sprintf(str_screen[SIM_ISGUI], "%s : %d", str_names[SIM_ISGUI], isGUI);
        sprintf(str_screen[SIM_ISCMD], "%s : %d", str_names[SIM_ISCMD], isCmd);
        sprintf(str_screen[SIM_LOGNAME], "%s : %s", str_names[SIM_LOGNAME], name_log);
        sprintf(str_screen[SIM_IIPRINT], "%s : %d", str_names[SIM_IIPRINT], inteval_print);
        sprintf(str_screen[SIM_IISCRN], "%s : %d", str_names[SIM_IISCRN], inteval_screen);
        sprintf(str_screen[SIM_UMAX], "%s : %3.4f", str_names[SIM_UMAX], Umax);
        for (int i = 0; i < num_para; i++) {
            strcpy(str_values[i], str_screen[i] + strlen(str_names[i]) + strlen(" : "));
            str_screen[i][this->len_screen - 1] = '\0';
        }
    }
    void init_pPara() {
        Model_base::init_pPara();
        list_pPara[SIM_DT] = &dt_sim;
        list_pPara[SIM_RUNSEC] = &runSec_sim;
        list_pPara[SIM_NSTEP] = &nStep_sim;
        list_pPara[SIM_ALLSTEP] = &allStep_sim;
        list_pPara[SIM_ISPRT] = &isPrint;
        list_pPara[SIM_ISPRTF] = &isPrintf;
        list_pPara[SIM_ISGUI] = &isGUI;
        list_pPara[SIM_ISCMD] = &isCmd;
        list_pPara[SIM_LOGNAME] = &name_log;
        list_pPara[SIM_IIPRINT] = &inteval_print;
        list_pPara[SIM_IISCRN] = &inteval_screen;
        list_pPara[SIM_UMAX] = &Umax;
    }
    void init_ParaType() {
        Model_base::init_ParaType();
        list_paraType[SIM_DT] = T_DOUBLE;
        list_paraType[SIM_RUNSEC] = T_DOUBLE;
        list_paraType[SIM_NSTEP] = T_INT;
        list_paraType[SIM_ALLSTEP] = T_INT;
        list_paraType[SIM_ISPRT] = T_INT;
        list_paraType[SIM_ISPRTF] = T_INT;
        list_paraType[SIM_ISGUI] = T_INT;
        list_paraType[SIM_ISCMD] = T_INT;
        list_paraType[SIM_LOGNAME] = T_STR;
        list_paraType[SIM_IIPRINT] = T_INT;
        list_paraType[SIM_IISCRN] = T_INT;
        list_paraType[SIM_UMAX] = T_DOUBLE;
    }
    void printParameters(FILE* fp, int idx) {
        if (idx == 0) {
            fprintf(fp, "MM2_B_Time is \t%04.6f(s)\t", t_cur);
            for (int m = 0; m < num_models; m++) list_model[m]->printParameters(fp, -1);
            fprintf(fp, "\tMM2_E\n");
        }
        {
            fprintf(fp, "MM2_B_Time is \t%04.6f(s)\t", t_cur);
            for (int m = 0; m < num_models; m++) list_model[m]->printParameters(fp, idx);
            fprintf(fp, "\tMM2_E\n");
        }
    }
    void Ctrl_IO(FILE* fp, int i) {
        // screen content generating
        bool isTitle = i == 0 ? true : false;
        if (isGUI) {
            if (i % inteval_screen == 0) {
                prepareScreen();
            }
        } else if (isPrint) {
            if (i % inteval_print == 0) printParameters(stdout, i);
        }
        // file content generating
        if (isPrintf) {
            if (i % inteval_print == 0) printParameters(fp, i);
        }
    }
    void updating(double dt) {
        FILE* fp;
        if (isPrintf) fp = fopen(name_log, "w");
        assert(fp != 0);
        for (int i = 0; i < nStep_sim; i++) {
            // simulating
            for (int m = 0; m < num_models; m++) {
                list_model[m]->pullInput();
                list_model[m]->updating(dt_sim);
                list_model[m]->pushOutput();
            }
            Ctrl_IO(fp, i);
            t_cur += dt_sim;
            allStep_sim++;
        }
        if (isPrintf) fclose(fp);
    }
    void sync_step_time() {
        // who changed, who is target
        if (runSec_sim != runSec_sim2) {
            runSec_sim2 = runSec_sim;
            nStep_sim = runSec_sim / dt_sim;
            nStep_sim2 = nStep_sim;
        } else {
            nStep_sim2 = nStep_sim;
            runSec_sim = nStep_sim * dt_sim;
            runSec_sim2 = runSec_sim;
        }
    }
    void sync_Umax() {
        this->motor.Umax = Umax;
        this->adc.Umax = Umax;
        this->dac.Umax = Umax;
    }
    void run() {
        if (!isGUI) {
            state_sim = SIM_RUN;
            assert(nStep_sim > 0);
            sync_step_time(); // runSec_sim = dt_sim * nStep_sim;
            sync_Umax();
            updating(runSec_sim);
        } else {
            while (state_gui != GUI_STOP) {
                usleep(10000);
                if (state_gui == GUI_SIM) {
                    state_sim = SIM_RUN;
                    assert(nStep_sim > 0);
                    sync_step_time(); // runSec_sim = dt_sim * nStep_sim;
                    sync_Umax();
                    updating(runSec_sim);
                    state_sim = SIM_STOP;
                    while (state_gui == GUI_SIM) usleep(100);
                }
            }
        }
        state_sim = SIM_STOP;
    }
    void ctrl() {}
    void gui_ready() { // return next state;
        cmd2Argv cmd_arg;
        char str_cmd[128];
        prepareScreen();
        screen_updating_model();
        scrn_cmd.display();

        do {
            scrn_cmd.inputLine(str_cmd);
            cmd_arg.cmd_getArgs(str_cmd);
            int id_cmd = cmd_arg.cmd_getCmdID();

            switch (id_cmd) {
                case CMD_MAN:
                    for (int i = 0; i < NUM_CMD; i++) scrn_cmd.addLine((char*)cmdlist_intro[i]);
                    scrn_cmd.display();
                    break;
                case CMD_RUN:
                    state_gui = GUI_SIM;
                    break;
                case CMD_SET:
                    gui_setPara();
                    break;
                case CMD_STOP:
                    this->stop();
                    this->prepareScreen();
                    screen_updating_model();
                    scrn_cmd.display();
                    break;
                case CMD_PL:
                    break;
                case CMD_QUIT:
                    state_gui = GUI_STOP;
                    break;
                default:
                    break;
            }
        } while (state_gui == GUI_READY);
    }
    void gui_sim() {
        screen_clear();
        do {
            usleep(10000);
            screen_updating_model();
            refresh();
        } while (SIM_RUN == state_sim);
        state_gui = GUI_READY;
        // screen_updating_model();
        // refresh();
    }
    void gui() {
        if (isGUI) state_gui = GUI_INIT;
        do {
            switch (state_gui) {
                case GUI_INIT:
                    screen_init();
                    state_gui = GUI_READY;
                    screen_addstr_stand(rect_cmdin, "NOTE: Please using command line");
                    break;
                case GUI_READY:
                    gui_ready();
                    break;
                case GUI_SIM:
                    gui_sim();
                    break;
                default:
                    state_gui = GUI_STOP;
            }
        } while (state_gui != GUI_STOP);
#ifndef _DEBUG_SCREEN_
        screen_end();
#endif
    }
    void gui_setPara() {
        ///////////////////////////////////////////////////////////////////////////

        char ch, ch0, ch00;
        int x = 0;
        int y = 0;
        myRect cur;
        char str_nm[100];
        char str_vl[100];
        void* pPara;
        void* ppPara[1]; //=&(pPara);
        int* pN = (int*)ppPara[0];
        curs_set(true);
        Model_base* pSel = this->findFirstPara(cur, str_nm, ppPara, y, x);
        standout();
        screen_addstr(cur, str_nm);
        standend();
        screen_addstr_stand(rect_cmdin, "NOTE: Press ENTER to set value");

        do {
            // printf("KEYINPUT\n");
            ch00 = ch0;
            ch0 = ch;
            ch = getch();
            // printf("KEY<%c> <%d>\n", ch, ch);
            myRect rct;
            rct.set(rect_all);
            // double* p_v;
            if (ch == '\n') {
                myRect r_set(cur);
                r_set.right_ = this->rect_all.right_;
                char str_v[64];
                screen_fillRect(r_set, ' ');
                standout();
                sprintf(str_cmdout, "%s : ", str_nm);
                screen_fillRect(r_set, ' ');
                screen_getLine(r_set, str_v, str_cmdout);
                standend();
                // double v = atof(str_v);
                //*p_v = v;
                pSel->setParaByName(str_nm, str_v);
                this->prepareScreen();

                sprintf(str_cmdout, "%s : %s", str_nm, str_v);
                screen_fillRect(r_set, ' ');
                screen_addstr(r_set, str_cmdout);

                screen_updating_model();
                screen_addstr_stand(cur, str_nm);
                //
            }
            if (ch0 == 91) {
                // printf("KEYINPUT\n");
                // char ch2 = getch();
                // printf("KEY<%c> <%d>\n", ch, ch);
                int x2;
                int y2;
                int x_step = cur.width();
                if (ch == 65) // up
                    y2 = y > rct.top_ ? y - 1 : y;
                if (ch == 66) // down
                    y2 = y < rct.bottom_ - 1 ? y + 1 : x;
                if (ch == 68) { // left
                    if (x - x_step >= rct.left_)
                        x2 = x - x_step;
                    else
                        x2 = x;
                }
                if (ch == 67) { // right
                    if (x + x_step < rct.right_)
                        x2 = x + x_step;
                    else
                        x2 = x;
                }
                myRect cur_new;
                char str_nm_new[100];
                char str_vl_new[100];
                Model_base* pm = this->findParaInScreen(cur_new, str_nm_new, str_vl_new, ppPara, y2, x2);
                if (pm != NULL) {
                    x = x2;
                    y = y2;
                    screen_addstr(cur, str_nm);
                    cur.set(cur_new);
                    strcpy(str_nm, str_nm_new);
                    strcpy(str_vl, str_vl_new);
                    screen_addstr_stand(cur, str_nm);
                    pSel = pm;
                }
            }
            sprintf(str_cmdin, "NOTE: Press 'q' to back to command-line  \t ch1=%d, ch0=%d ch00=%d cur_x=%d, cur_y=%d",
                    ch, ch0, ch00, x, y);
            screen_addstr_stand(rect_cmdin, str_cmdin);

#ifndef _DEBUG_SCREEN_
            refresh();
#endif

        } while (ch != 'q'); // double ESC
        standend();
        screen_addstr_stand(cur, str_nm);
        standend();
        /*
                #ifndef _DEBUG_SCREEN_
                screen_end();
                #endif
                state_gui = GUI_STOP;       */
    }
    void runBycycle(long cycle) {
        nStep_sim = cycle;
        run();
    }
    void runBytime(double t_len) {
        nStep_sim = t_len / dt_sim;
        run();
    }

    void thread_sim() {
        std::thread th_run;
        std::thread th_gui;
        std::thread th_ctrl;
        this->isPrint = false;
        this->isGUI = true;
        th_run = std::thread(thread_run, this);
        th_gui = std::thread(thread_gui, this);
        // th_ctrl = std::thread(thread_ctrl, this);
        th_run.join();
        th_gui.join();
        // th_ctrl.join();
        printf("gui done\n");
    }
    void screen_updating_col(int lin, int col, Model_base* pMd) {
        if (!isGUI) return;

// int lin=0;
#ifndef _DEBUG_SCREEN_
        mvaddstr(lin, col, pMd->GetTypeStr());
        mvaddstr(lin, col + strlen(pMd->GetTypeStr()), pMd->name_obj);
#endif
        lin++;
        for (int i = 0; i < pMd->num_para % (screen_max_y - lin); i++) {
#ifndef _DEBUG_SCREEN_
            mvaddstr(lin + i, col, pMd->str_screen[i]);
#else
            printf("%d\t %d\t %s\n", i, col, pMd->str_screen[i]);
#endif
        }
    }

    void screen_init() {
        myRect rct;
#ifdef _DEBUG_SCREEN_
        rct.set(0, 198, 0, 50);
        rect_title0.set(0, rct.right_, rct.top_, rct.top_ + 1);
        rct.top_++;
        rect_cmdin.set(0, rct.right_, rct.top_, rct.top_ + 1);
        rct.top_++;
        rect_cmdout.set(0, rct.right_, rct.top_, rct.top_ + 1);
        rct.top_++;
        this->screen_assign(rct);
        return;
#endif
        if (!isGUI) return;
        initscr();
        noecho();
        curs_set(false);
        getmaxyx(stdscr, screen_max_y, screen_max_x);
        rct.set(0, screen_max_x, 0, screen_max_y);
        rect_title0.set(0, rct.right_, rct.top_, rct.top_ + 1);
        rct.top_++;
        rect_cmdin.set(0, rct.right_, rct.top_, rct.top_ + 1);
        rct.top_++;
        rect_cmdout.set(0, rct.right_, rct.top_, rct.top_ + 1);
        rct.top_++;
        this->screen_assign(rct);

        // find a room for commandline window after assining
        int lft = this->motor.rect_all.right_;
        int rt = this->motor.rect_all.right_ + this->foc1.pid_id.len_screen * 3;
        if (rt > rct.right_) rt = rct.right_;
        int tp = this->foc1.pid_id.rect_para.top_ + this->foc1.pid_id.num_para; // rct.bottom_ - 30;
        int btm = rct.bottom_;
        rect_cmd.set(lft, rt, tp, btm);
        // rect_cmd.set(sim.motor.rect_all.right_, rct.right_, rct.bottom_-30, rct.bottom_);//
        assert(rect_cmd.isValid());
        scrn_cmd.init(rect_cmd);
    }

    void screen_updating_model() {
        sprintf(str_title0, "Simulating TB has %d sub-models. \t Current time=%3.6f(s) \tcomplete:%2.2f%%",
                this->num_models, this->t_cur, t_cur / runSec_sim * 100.0);
        screen_addstr(rect_title0, str_title0);
        screen_updating_auto();
    }
    void screen_showRect() {
        screen_addstr(rect_title0, rect_title0.sprint("rect_title0"));
        screen_addstr(rect_cmdin, rect_cmdin.sprint("rect_cmdin"));
        screen_addstr(rect_cmdout, rect_cmdout.sprint("rect_cmdout"));
        Model_base::screen_showRect();
    }
    void screen_end() {
#ifndef _DEBUG_SCREEN_
        // clear();
        // this->screen_showRect();
        // refresh();

        // test begin
        /*
        getch();
        char str_tmp[100];
        screen_clear();
         screen_addstr(rect_cmdin, rect_cmdin.sprint("rect_cmdin"));
         getch();
           screen_clear();
        screen_getLine(rect_cmdin, str_tmp);

          screen_clear();*/
        // screen_addstr(rect_all, rect_all.sprintf("rect_all"));
        // screen_addstr(rect_cmd, rect_cmd.sprintf("rect_cmd"));
        // screen_addstr(rect_stdin, rect_stdin.sprintf("rect_stdin"));
        // getch();
        //    winCmdline scrcmd(rect_all);
        //    scrcmd.();
        // test end
        // getch();
        endwin();
#endif
        // printf("WINDOW END %s   %s\n", rect_all.sprint("WINDOW INFO: "), str_tmp);
        printf("WINDOW END %s \n", rect_all.sprint("WINDOW INFO: "));
    }
};

static void thread_run(SimulatorCtrl* simctrl) {
    simctrl->run();
}
static void thread_ctrl(SimulatorCtrl* simctrl) {
    simctrl->ctrl();
}
static void thread_gui(SimulatorCtrl* simctrl) {
    simctrl->gui();
}
#endif // _MY_MOTOR_HPP_