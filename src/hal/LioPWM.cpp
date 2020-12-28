// #include <LioPWM.h>
// #include "../../sdk/sdk14/components/libraries/led_softblink/led_softblink.h"
// #include "../../sdk/sdk14/components/libraries/low_power_pwm/low_power_pwm.h"


// #define LIO_MAX_BRIGHTNESS  200
// #define LIO_PWM_HIGH_ACTIVE true
// #define LIO_SEC             65536 / 2
// LioPWM::LioPWM () {
//     this->lioPins [5] = {0};
// }


// void LioPWM::InitLioPWM ( void ) {
//     FireLightExtPins FireLightPinConfig;
// 	GS->boardconf.getSensorPins(&FireLightPinConfig);

//     if (FireLightPinConfig.lio1 != -1) {
//         this->lioPins[0] = FireLightPinConfig.lio1;
//         this->lioPins[1] = FireLightPinConfig.lio2;
//         this->lioPins[2] = FireLightPinConfig.lio3;
//         this->lioPins[3] = FireLightPinConfig.lio4;
//         this->lioPins[4] = FireLightPinConfig.lio5;

//         uint32_t err_code;
//     }
//     else {
//         logs("No pwm pins found in GS");
//     }
	
// }

// void  LioPWM::fadeLED (u8 lioPos, u16 value, u16 fadeTime) {
//     if (lioPos <= 4) {
//         u8 pin = this->lioPins[ lioPos ];
//         u32 mask = (1 << pin );

//         led_sb_init_params_t led_sb_init_param; 
//         led_sb_init_param.active_high = LIO_PWM_HIGH_ACTIVE; 
//         led_sb_init_param.duty_cycle_max = value;
//         led_sb_init_param.duty_cycle_min = 0;
//         led_sb_init_param.duty_cycle_step = 1;
//         led_sb_init_param.off_time_ticks = LIO_SEC * 1;
//         led_sb_init_param.on_time_ticks =  LIO_SEC * 3;
//         led_sb_init_param.leds_pin_bm = mask;
//         led_sb_init_param.p_leds_port = LED_SB_INIT_PARAMS_LEDS_PORT;
//         logs("set pwm pin %u to %u", pin, value );
//         // port ?
//         u32 err_code;

//         err_code = led_softblink_init(&led_sb_init_param);
//         logs("led_softblink_init %u", err_code);

//         err_code = led_softblink_start(mask);
//         logs("led_softblink_start %u", err_code);
//     }
// }



