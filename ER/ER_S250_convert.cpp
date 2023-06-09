#include "mbed.h"
#include "math.h"

/*ER試験のときRとLは逆に考えて実施*/

#define ELE_U 0.001365
#define ELE_N 0.001703
#define ELE_D 0.002012
#define RUD_R 0.001965 //RとLが逆
#define RUD_N 0.001730
#define RUD_L 0.001358//RとLが逆

/*#define RUD_R 0.00105
#define RUD_N 0.0015
#define RUD_L 0.001838
#define ELE_U 0.002439 //変更点
#define ELE_N 0.002030 //変更点
#define ELE_D 0.00109  //変更点 */

#define ELE_TRIM2 0.0000135
#define RUD_TRIM2 0.0000135

#define ELE_TRIM 0.0000005*9
#define RUD_TRIM 0.0000005*9

#define MIN_ANGLE 0.00095
#define MAX_ANGLE 0.00215 

#define er_max 0.99
#define er_n 0.49
#define er_min 0.001
#define rr_max 0.99
#define rr_n 0.5
#define rr_min 0.003

DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);

InterruptIn eu_trim(p26);
InterruptIn ed_trim(p25);
InterruptIn rr_trim(p24);
InterruptIn rl_trim(p23);

PwmOut pwm_ele(p21);
PwmOut pwm_rud(p22);

AnalogIn joy_ele(p19);
AnalogIn joy_rud(p20);

Serial pc(USBTX,USBRX);
Serial to_main(p9,p10);
Serial to_meter(p13,p14);

Ticker communication,t,t_pc;

/*==============ERのpwm値==============*/

double pw_ele;
double pw_rud;

/*==============ERジョイスティックanalog値==============*/

double er;
double rr;

/*==============ERのpwm値傾き係数==============*/

double er_slope_u = 1/(er_n - er_min);
double er_slope_d = 1/(er_max - er_n);
double rr_slope_l = 1/(rr_n - rr_min);
double rr_slope_r = 1/(rr_max - rr_n);


/*==============ERジョイスティックのニュートラル遊び==============*/

double er_n_range1 = er_n-0.01;
double er_n_range2 = er_n+0.01;
double rr_n_range1 = rr_n-0.01;
double rr_n_range2 = rr_n+0.01;

/*==============ERに関するパラメータ==============*/

double ele_u = ELE_U;
double ele_n = ELE_N;
double ele_d = ELE_D;
double rud_r = RUD_R;
double rud_n = RUD_N;
double rud_l = RUD_L;
double t_now_ele = ELE_N;
double t_now_rud = RUD_N;
double t_pre_ele = ELE_N;
double t_pre_rud = RUD_N;
double t_ele_trim = 0;
double x = 0;
double x_up = 0;
double x_down = 0;
double y = 0;
double y_right = 0;
double y_left = 0;

double pre_ele_pw = ELE_N;
double pre_rud_pw = RUD_N;
double now_ele_pwm;



double n[52] = {
    0.000002,
    0.000222,
    0.002704,
    0.012511,
    0.03448,
    0.070046,
    0.11738,
    0.17305,
    0.233434,
    0.29545,
    0.356785,
    0.415856,
    0.47168,
    0.523714,
    0.571736,
    0.61573,
    0.655818,
    0.692201,
    0.725124,
    0.754852,
    0.781651,
    0.80578,
    0.827485,
    0.846995,
    0.864521,
    0.880256,
    0.894376,
    0.907039,
    0.918389,
    0.928554,
    0.937649,
    0.94578,
    0.953039,
    0.959511,
    0.965271,
    0.970386,
    0.974917,
    0.97892,
    0.982442,
    0.98553,
    0.988222,
    0.990555,
    0.992562,
    0.994271,
    0.995711,
    0.996904,
    0.997873,
    0.998638,
    0.999216,
    0.999625,
    0.999879,
    0.999992,

};
/*============================*/

bool w;

char ele_data = '4';
char rud_data = '4';
signed char trim_ele = 0;
signed char trim_rud = 0;
char x_data = 0x00;
char y_data = 0x00;
char k;

void control();
void enter_pc_control();
void pc_control();
void control_speed(double next_ele_pwm,double pre_ele_pwm,double next_rud_pwm,double pre_rud_pwm);
void ele_control_speed(double next_ele_pwm, double pre_ele_pwm);
void rud_control_speed(double next_rud_pwm, double pre_rud_pwm);
void to_main_communication();
void to_meter_communication();
void trim_ele_up();
void trim_ele_down();
void trim_rud_right();
void trim_rud_left();
void trim_ele_up2();
void trim_ele_down2();
void trim_rud_right2();
void trim_rud_left2();
int main()
{
    wait(0.1);
    to_main.baud(115200);
    pc.attach(pc_control,Serial::RxIrq);
    communication.attach(&to_main_communication,0.5);
    //t_pc.attach(&to_meter_communication,0.5);
    t.attach(&control,0.25);
    eu_trim.rise(&trim_ele_up2);
    ed_trim.rise(&trim_ele_down2);
    rr_trim.rise(&trim_rud_right2);
    rl_trim.rise(&trim_rud_left2);

    while(1) {
    }
}
void control()
{
    er = joy_ele.read();
    rr = joy_rud.read();
    if(er<er_n_range1) {
        led1 = 1;
        pw_ele = er_slope_u*(ele_n-ele_u)*(er - er_min) + ele_u + x;
    } else if(er>er_n_range2) {
        led1 = 0;
        pw_ele = er_slope_d*(ele_d-ele_n)*(er - er_n) + ele_n + x;
    } else {
        pw_ele = ele_n;
    }
    if(rr<rr_n_range1) {
        led2 = 1;
        pw_rud = rr_slope_l*(rud_n-rud_l)*(rr - rr_min) + rud_l + y;
    } else if(rr>rr_n_range2) {
        led2 = 0;
        pw_rud = rr_slope_r*(rud_r-rud_n)*(rr - rr_n) + rud_n + y;
    } else {
        pw_rud = rud_n;
    }

    if(pw_ele<MIN_ANGLE) {
        pw_ele = MIN_ANGLE;
    }
    if(pw_ele>MAX_ANGLE) {
        pw_ele = MAX_ANGLE;
    }
    if(pw_rud<MIN_ANGLE) {
        pw_rud = MIN_ANGLE;
    }
    if(pw_rud>MAX_ANGLE) {
        pw_rud = MAX_ANGLE;
    }

    control_speed(pw_ele,pre_ele_pw,pw_rud,pre_rud_pw);

    //pwm_ele.pulsewidth(pw_ele);
    //pwm_rud.pulsewidth(pw_rud);

    pre_ele_pw = pw_ele;
    pre_rud_pw = pw_rud;

    pc.printf("ele_pwidth: %lf\r\n",pre_ele_pw);
    pc.printf("rud_pwidth: %lf\r\n",pre_rud_pw);


    if(0<=er&&er<0.125) {
        ele_data = '8';
    } else if(0.125<=er&&er<0.25) {
        ele_data = '7';
    } else if(0.25<=er&&er<0.375) {
        ele_data = '6';
    } else if(0.375<=er&&er<er_n_range1) {
        ele_data = '5';
    } else if(er_n_range1<=er&&er<=er_n_range2) {
        ele_data = '4';
    } else if(er_n_range2<er&&er<=0.675) {
        ele_data = '3';
    } else if(0.675<er&&er<=0.75) {
        ele_data = '2';
    } else if(0.75<er&&er<=0.875) {
        ele_data = '1';
    } else if(0.875<er&&er<=1) {
        ele_data = '0';
    }
    if(0<=rr&&rr<0.125) {
        rud_data = '8';
    } else if(0.125<=rr&&rr<0.25) {
        rud_data = '7';
    } else if(0.25<=rr&&rr<0.375) {
        rud_data = '6';
    } else if(0.375<=rr&&rr<rr_n_range1) {
        rud_data = '5';
    } else if(rr_n_range1<=rr&&rr<=rr_n_range2) {
        rud_data = '4';
    } else if(rr_n_range2<rr&&rr<=0.675) {
        rud_data = '3';
    } else if(0.675<rr&&rr<=0.75) {
        rud_data = '2';
    } else if(0.75<rr&&rr<=0.875) {
        rud_data = '1';
    } else if(0.875<rr&&rr<=1) {
        rud_data = '0';
    }
    to_meter_communication();
    
}
void enter_pc_control()
{
    pc.printf("pc");
}
void pc_control()
{
    k = pc.getc();
    if(k == '\r') {
        t.detach();
        k = '\0';
        int i = 1;
        // pc.attach(enter_pc_control,Serial::RxIrq);
        pc.printf("PCモードに入りました。\r\n");
        do {
            char c;
            led4 = 1;
            wait(0.1);
            led4 = 0;
            pc.printf("==============================================================================\r\n\n");
            pc.printf("操作する項目を選んでキーを押してください。\r\n");
            pc.printf("\r\n");
            pc.printf("試験データ確認 : S\r\n");
            pc.printf("エレベーター操作 : E\r\n");
            pc.printf("ラダー操作 : R\r\n");
            pc.printf("PCモード終了 : C\r\n");
            pc.putc('\n');

            c = pc.getc();
            if(c == 'C') {
                i = 0;
            } else if(c == 'S') {
                pc.printf("==================================\r\n\n");
                pc.printf("t_now_ele = %f\r\n",t_now_ele + x);
                pc.printf("t_now_rud = %f\r\n",t_now_rud + y);
                pc.putc('\n');

            } else if(c == 'E') {
                pc.printf("==================================\r\n\n");
                pc.printf("UP : U\r\n");
                pc.printf("N : N\r\n");
                pc.printf("DOWN : D\r\n");
                pc.printf("トリムUP : k\r\n");
                pc.printf("トリムDOWN : m\r\n\n");
                c = pc.getc();
                if(c == 'U') {
                    x = 0;
                    t_now_ele = ele_u;
                } else if(c == 'D') {
                    x = 0;
                    t_now_ele = ele_d;
                } else if(c == 'N') {
                    x = 0;
                    t_now_ele = ele_n;
                } else if(c == 'k') {
                    int count = 0;
                    pc.printf("UPする回数+を押してください\r\n");
                    pc.printf("入力が終了したらEを押してください\r\n");
                    do {
                        w = 1;
                        c = pc.getc();
                        if(c == '+') {
                            count++;
                            pc.printf("%d\r\n",count);
                        } else if(c == 'E') {
                            pc.putc('\n');
                            pc.printf("%d段階UPします\r\n\n",count);
                            w = 0;
                        } else {}

                    } while(w);
                    for(int i =0; i<count; i++) {

                        trim_ele_up();
                    }

                } else if(c == 'm') {
                    int count = 0;
                    pc.printf("DOWNする回数+を押してください\r\n");
                    pc.printf("入力が終了したらEを押してください\r\n");
                    do {
                        w = 1;
                        c = pc.getc();
                        if(c == '+') {
                            count++;
                            pc.printf("%d\r\n",count);
                        } else if(c == 'E') {
                            pc.putc('\n');
                            pc.printf("%d段階DOWNします\r\n\n",count);
                            w = 0;
                        } else {}

                    } while(w);
                    for(int i =0; i<count; i++) {
                        trim_ele_down();
                    }

                }
                if((t_now_ele + x)>= MAX_ANGLE) {
                    x = MAX_ANGLE - t_now_ele;
                } else if((t_now_ele + x)<= MIN_ANGLE) {
                    x = MIN_ANGLE - t_now_ele;
                }
                ele_control_speed(t_now_ele + x,t_pre_ele);
                t_pre_ele = t_now_ele + x;
            
            }  else if(c == 'R') {
                pc.printf("==================================\r\n\n");
                pc.printf("RIGHT : R\r\n");
                pc.printf("N : N\r\n");
                pc.printf("LEFT : L\r\n");
                pc.printf("トリムRIGHT : s\r\n");
                pc.printf("トリムLEFT : z\r\n\n");
                c = pc.getc();
                if(c == 'R') {
                    y = 0;
                    t_now_rud = rud_r;
                } else if(c == 'L') {
                    y = 0;
                    t_now_rud = rud_l;
                } else if(c == 'N') {
                    y = 0;
                    t_now_rud = rud_n;
                } else if(c == 's') {
                    int count = 0;
                    pc.printf("動かす回数+を押してください\r\n");
                    pc.printf("入力が終了したらEを押してください\r\n");
                    do {
                        w = 1;
                        c = pc.getc();
                        if(c == '+') {
                            count++;
                            pc.printf("%d\r\n",count);
                        } else if(c == 'E') {
                            pc.putc('\n');
                            pc.printf("%d段階動かします\r\n\n",count);
                            w = 0;
                        } else {}

                    } while(w);
                    for(int i =0; i<count; i++) {
                        trim_rud_right();
                    }

                } else if(c == 'z') {
                    int count = 0;
                    pc.printf("動かす回数+を押してください\r\n");
                    pc.printf("入力が終了したらEを押してください\r\n");
                    do {
                        w = 1;
                        c = pc.getc();
                        if(c == '+') {
                            count++;
                            pc.printf("%d\r\n",count);
                        } else if(c == 'E') {
                            pc.putc('\n');
                            pc.printf("%d段階動かします\r\n\n",count);
                            w = 0;
                        } else {}

                    } while(w);
                    for(int i =0; i<count; i++) {
                        trim_rud_left();
                    }
                }
                rud_control_speed(t_now_rud + y,t_pre_rud);
                t_pre_rud = t_now_rud + y;
            } else pc.printf("その操作は出来ません。\r\n\n");
        } while(i);
        x = 0;
        y = 0;
        pc.printf("PCモードを終了しました。\r\n");
        pc.attach(pc_control,Serial::RxIrq);
        t.attach(&control,0.25);
    }
}

void to_main_communication()
{
    to_main.putc(ele_data);
    to_main.putc(rud_data);
    //to_main.putc(trim_ele);
    //to_main.putc(trim_rud);
     //pc.printf("er = %f\r\npw = %f\r\n\n",er,pw_ele);
     //pc.printf("rr = %f\r\npw = %f\r\n\n",rr,pw_rud);

}

void ele_control_speed(double next_ele_pwm,double pre_ele_pwm)
{
    double now_ele_pwm;
    for(int i =0; i<52; i++) {
        now_ele_pwm = (next_ele_pwm - pre_ele_pwm)*n[i]+ pre_ele_pwm;        
        pwm_ele.pulsewidth(now_ele_pwm);
        wait(0.02);
    }
    pwm_ele.pulsewidth(next_ele_pwm);
    
}
void rud_control_speed(double next_rud_pwm,double pre_rud_pwm)
{
    double now_rud_pwm;
    for(int i = 0; i < 52; i++) {
        now_rud_pwm = (next_rud_pwm - pre_rud_pwm)*n[i] + pre_rud_pwm;
        pwm_rud.pulsewidth(now_rud_pwm);
        wait(0.02);
    }
    pwm_rud.pulsewidth(next_rud_pwm);
    
}
void control_speed(double next_ele_pwm,double pre_ele_pwm,double next_rud_pwm,double pre_rud_pwm)
{
    double now_ele_pwm;
    double now_rud_pwm;
    for(int i =0; i<52; i++) {
        now_ele_pwm = (next_ele_pwm - pre_ele_pwm)*n[i]+ pre_ele_pwm;
        now_rud_pwm = (next_rud_pwm - pre_rud_pwm)*n[i] + pre_rud_pwm;
        pwm_ele.pulsewidth(now_ele_pwm);
        pwm_rud.pulsewidth(now_rud_pwm);
        wait(0.004);
    }
    pwm_ele.pulsewidth(next_ele_pwm);
    pwm_rud.pulsewidth(next_rud_pwm);
    //return now_ele_pwm;
}
void to_meter_communication()
{
    to_meter.putc(ele_data);
    to_meter.putc(rud_data);
    led4 = !led4;
}

void trim_ele_up()
{
    x = x - ELE_TRIM;
    // x_up = x_up - 0.0000005*9;
}
void trim_ele_down()
{
    x = x + ELE_TRIM;
    // x_down = x_down + 0.0000005*9;
}
void trim_rud_right()
{
    y = y + RUD_TRIM;
    // y_right = y_right +0.0000005*9;
}
void trim_rud_left()
{
    y = y - RUD_TRIM;
    // y_left = y_left - 0.0000005*9;
}
void trim_ele_up2()
{
    x = x - ELE_TRIM2;
    //trim_ele++;
    // x_up = x_up - 0.0000005*9;
}
void trim_ele_down2()
{
    x = x + ELE_TRIM2;
    //trim_ele--;
    led3 = !led3;
    // x_down = x_down + 0.0000005*9;
}
void trim_rud_right2()
{
    y = y + RUD_TRIM2;
    // y_right = y_right +0.0000005*9;
}
void trim_rud_left2()
{
    y = y - RUD_TRIM2;
    // y_left = y_left - 0.0000005*9;
}
