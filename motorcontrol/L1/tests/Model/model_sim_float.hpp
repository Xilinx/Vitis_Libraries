/*
 * Copyright 2022 Xilinx, Inc.
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

#ifndef _MODEL_SIM_FLOAT_HPP_
#define _MODEL_SIM_FLOAT_HPP_
#include <ncurses.h>
#include "model_motor.hpp"
#include "model_foc.hpp"
#include <thread>
#include <unistd.h>
#include "model_qei.hpp"
#include "model_screen.hpp"
#include "model_cmd.hpp"
#include "model_smo.hpp"

//#define _DEBUG_SCREEN_

class SimulatorCtrl_float;
static void thread_gui_float(SimulatorCtrl_float* simctrl);

class SimulatorCtrl_float : public SimulatorCtrl {
   public:
    // specify models
    FOC_Simple_1<double> foc_float;
    SMO<double, int> smo;

    void connectSubModels() {
        num_models = 3;
        // COUSTION! the list order determine the sequence of updating
        list_model[0] = (Model_base*)&motor;
        list_model[1] = (Model_base*)&smo;
        list_model[2] = (Model_base*)&foc_float;

        motor.input_va_pull = &foc_float.out_va;
        motor.input_vb_pull = &foc_float.out_vb;
        motor.input_vc_pull = &foc_float.out_vc;
        foc_float.input_Ia_pull = &motor.out_va;
        foc_float.input_Ib_pull = &motor.out_vb;
        foc_float.input_Ic_pull = &motor.out_vc;
        foc_float.input_theta_e_pull = &motor.theta_e;
        foc_float.input_w_pull = &motor.w;

        smo.input_Ia_pull = &motor.out_va;
        smo.input_Ib_pull = &motor.out_vb;
        smo.input_Ic_pull = &motor.out_vc;
        smo.input_Va_pull = &foc_float.out_va;
        smo.input_Vb_pull = &foc_float.out_vb;
        smo.input_Vc_pull = &foc_float.out_vc;
    }
    SimulatorCtrl_float() {
        id_type = MODEL_SIM;
        this->setObjName("sim");
        foc_float.setObjName("foc_float");
        motor.setObjName("motor");
        smo.setObjName("smo");
        connectSubModels();
        // setting default values
        state_sim = SIM_STOP;
        state_gui = GUI_INIT;
        dt_sim = 0.000001;     // 10us
        motor.dt_sim = dt_sim; // 10us
        smo.dt_sim = dt_sim;
        nStep_sim = 10000;
        runSec_sim = dt_sim * nStep_sim;
        mode_disp = DISP_CMD;
        isPrint = true;
        isPrintf = false;
        isGUI = false;
        isCmd = false;
        strcpy(name_log, "./sim_float.log");
        inteval_print = 10;  // printing when nStep_sim%inteval_print==0
        inteval_screen = 10; // screen updating when %inteval_screen==0

        str_names[SIM_DT] = "SIM_DT     ";
        str_names[SIM_RUNSEC] = "SIM_RUNSEC ";
        str_names[SIM_NSTEP] = "SIM_NSTEP  ";
        str_names[SIM_ISPRT] = "SIM_ISPRT  ";
        str_names[SIM_ISPRTF] = "SIM_ISPRTF ";
        str_names[SIM_ISGUI] = "SIM_ISGUI  ";
        str_names[SIM_ISCMD] = "SIM_ISCMD  ";
        str_names[SIM_LOGNAME] = "SIM_LOGNAME";
        str_names[SIM_IIPRINT] = "SIM_IIPRINT";
        str_names[SIM_IISCRN] = "SIM_IISCRN ";
        num_para = SIM_IISCRN + 1;
        init_pPara();
        init_ParaType();
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

    void thread_sim() {
        std::thread th_run;
        std::thread th_gui;
        std::thread th_ctrl;
        this->isPrint = false;
        this->isGUI = true;
        th_run = std::thread(thread_run, this);
        th_gui = std::thread(thread_gui_float, this);
        // th_ctrl = std::thread(thread_ctrl, this);
        th_run.join();
        th_gui.join();
        // th_ctrl.join();
        printf("gui done\n");
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
        int rt = this->motor.rect_all.right_ + this->foc_float.pid_id.len_screen * 3;
        if (rt > rct.right_) rt = rct.right_;
        int tp = this->foc_float.pid_id.rect_para.top_ + this->foc_float.pid_id.num_para; // rct.bottom_ - 30;
        int btm = rct.bottom_;
        rect_cmd.set(lft, rt, tp, btm);
        // rect_cmd.set(sim.motor.rect_all.right_, rct.right_, rct.bottom_-30, rct.bottom_);//
        assert(rect_cmd.isValid());
        scrn_cmd.init(rect_cmd);
    }
};

static void thread_gui_float(SimulatorCtrl_float* simctrl) {
    simctrl->gui();
}
#endif