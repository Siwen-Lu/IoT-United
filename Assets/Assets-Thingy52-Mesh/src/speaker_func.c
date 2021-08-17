#include "speaker_func.h"
static const struct device *pwm_dev;
enum
{
	ALARM_OFF       = 0,
	ALARM_ALWAYS_ON,
	ALARM_ON_LIMIT
};

enum
{
	ALARM_IDLE_PHASE = 0,
	ALARM_ON_PHASE
};

uint8_t alarm_mode = ALARM_OFF;
uint8_t alarm_phase = ALARM_IDLE_PHASE;
void speaker_init()
{
	nrf_gpio_cfg_output(SPKR_PWR_SWITCH);
	nrf_gpio_pin_clear(SPKR_PWR_SWITCH);
	
	pwm_dev = DEVICE_DT_GET(PWM_CTLR);
	if (!pwm_dev) {
		printk("Cannot find %s!\n", "Speaker");
		return;
	}
	
	uint64_t cycles_per_sec;

	if (pwm_get_cycles_per_sec(pwm_dev, PWM_CHANNEL, &cycles_per_sec) != 0) {
		return;
	}
}

// beeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee.....
void speaker_always_on(int freq, int volume)
{
	//configure the PWM
	uint32_t period = 1000000 / freq;
	uint32_t pulse = (period * volume) / 10000;
	int err = pwm_pin_set_usec(pwm_dev, PWM_CHANNEL, period, pulse, 0);
	if (err) {
		printk("pwm pin set fails %d\n",err);
		return;
	}
	
	//power on speaker
	nrf_gpio_pin_set(SPKR_PWR_SWITCH);
	
}
static void speaker_timer_stop(struct k_timer *dummy)
{
	speaker_off();
}

K_TIMER_DEFINE(speaker_timer, speaker_timer_stop, NULL);
// beeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeep
void speaker_limit_on(int freq, int volume, int duration)
{
	//configure the PWM
	uint32_t period = 1000000 / freq;
	uint32_t pulse = (period * volume) / 10000;
	int err = pwm_pin_set_usec(pwm_dev, PWM_CHANNEL, period, pulse, 0);
	if (err) {
		printk("pwm pin set fails %d\n", err);
		return;
	}
	
	//power on speaker
	nrf_gpio_pin_set(SPKR_PWR_SWITCH);
	
	k_timer_start(&speaker_timer, K_MSEC(duration), K_NO_WAIT);
}

static void alarm_phase_timer_handler(struct k_timer *dummy)
{
	if (alarm_mode == ALARM_ALWAYS_ON || alarm_mode == ALARM_ON_LIMIT)
	{
		int *time_info = k_timer_user_data_get(dummy);
		if (alarm_phase)
		{
			alarm_phase = ALARM_IDLE_PHASE;
			nrf_gpio_pin_clear(SPKR_PWR_SWITCH);
			k_timer_start(dummy, K_MSEC(time_info[1]), K_NO_WAIT);
		}
		else
		{
			alarm_phase = ALARM_ON_PHASE;
			nrf_gpio_pin_set(SPKR_PWR_SWITCH);
			k_timer_start(dummy, K_MSEC(time_info[0]), K_NO_WAIT);
		}
	}
	else
	{
		speaker_off();
	}
}
K_TIMER_DEFINE(alarm_phase_timer, alarm_phase_timer_handler, NULL);

//beep beep beep beep beep .....................................................
void speaker_always_on_alarm(int freq, int volume, int on_time, int idle_time)
{
	
	alarm_mode = ALARM_ALWAYS_ON;
	
	//configure the PWM
	uint32_t period = 1000000 / freq;
	uint32_t pulse = (period * volume) / 10000;
	int err = pwm_pin_set_usec(pwm_dev, PWM_CHANNEL, period, pulse, 0);
	if (err) {
		printk("pwm pin set fails %d\n", err);
		return;
	}
	
	int *time_info = k_malloc(2*sizeof(int));
	time_info[0] = on_time;
	time_info[1] = idle_time;
	
	k_timer_user_data_set(&alarm_phase_timer, time_info);

	k_timer_start(&alarm_phase_timer, K_MSEC(on_time), K_NO_WAIT);
	alarm_phase = ALARM_ON_PHASE;
	nrf_gpio_pin_set(SPKR_PWR_SWITCH);
}

// beep beep beep beep beep beep
void speaker_limit_on_alarm(int freq, int volume, int on_time, int idle_time, int duration)
{
	
	alarm_mode = ALARM_ON_LIMIT;
	
	//configure the PWM
	uint32_t period = 1000000 / freq;
	uint32_t pulse = (period * volume) / 10000;
	int err = pwm_pin_set_usec(pwm_dev, PWM_CHANNEL, period, pulse, 0);
	if (err) {
		printk("pwm pin set fails %d\n", err);
		return;
	}
	
	int *time_info = k_malloc(2*sizeof(int));
	time_info[0] = on_time;
	time_info[1] = idle_time;
	
	k_timer_user_data_set(&alarm_phase_timer, time_info);

	k_timer_start(&alarm_phase_timer, K_MSEC(on_time), K_NO_WAIT);
	alarm_phase = ALARM_ON_PHASE;
	nrf_gpio_pin_set(SPKR_PWR_SWITCH);
	
	k_timer_start(&speaker_timer, K_MSEC(duration), K_NO_WAIT);
}

// ...p
void speaker_off(void)
{
	alarm_mode = ALARM_OFF;
	alarm_phase = ALARM_IDLE_PHASE;
	//power off speaker
	nrf_gpio_pin_clear(SPKR_PWR_SWITCH);
	
	//configure the PWM
	int err = pwm_pin_set_usec(pwm_dev, PWM_CHANNEL, 1000, 0, 0);
	if (err)
	{
		printk("pwm pin stop fails %d\n", err);
		return;
	}
}