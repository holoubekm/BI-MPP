#include "cpu.h"
#include "led.h"
#include "delay.h"
#include "display.h"
#include "touchpad.h"
#include "log.h"
#include "p24fxxxx.h"

#define TMR1_PERIOD 0x1388

void reset_clock(void)
{
   TMR1 = 0;
   PR1 = TMR1_PERIOD;
   T1CONbits.TCS = 0;
   IPC0bits.T1IP = 4;
   IFS0bits.T1IF = 0;
   IEC0bits.T1IE = 1;
   SRbits.IPL = 3;
   T1CONbits.TON = 1;
}

   static int on = 1;
static int sticks = 0;

void __attribute__((interrupt, auto_psv)) _T1Interrupt(void){ 

   if (sticks++ > 1000)
   {
      sticks = 0;

      if(on) {
         led_off(LED_R);
      }
      else {
         led_on(LED_R);
      }
      on = !on;
   }
   IFS0bits.T1IF = 0; /* clear interrupt flag */
   return;
}

int main() {
   cpu_init();
   led_init();
   disp_init();
   touchpad_init();
   log_init();
   
   // disp_str("Zdarec kamo?");
   // draw_line(0, 10, 127, 10);

   int i;
   int x;
   // _T1Interrupt
   // TMR1 = 0;

   reset_clock();
   while(1);

   while(1) {
   char s[10];
   int v = get_touchpad_status();
   sprintf(s, "%d", v);
   disp_str(s);


   log_int("%ld", v);
}

   for(x = 0; x < 10; x++) {
      led_on(LED_R);
      delay_loop_ms(1);
      led_off(LED_R);
      delay_loop_ms(5);
   }

   // for(i = 0;
   //    pwm(LED_R, 10);
   // }

   for(i = 0; i < 10; i++) {
      led_on(LED_R);
      delay_loop_ms(5000);

      led_on(LED_G);
      delay_loop_ms(2000);
      led_off(LED_R);

      led_on(LED_G);
      delay_loop_ms(5000);
      led_off(LED_G);
   }

   while(1) {
      log_main_loop();
   }
   return 0;
}
