#ifndef __SPEAKER_FUNC_H__
#define __SPEAKER_FUNC_H__

#include <device.h>
#include <errno.h>
#include <sys/util.h>
#include <zephyr.h>
#include <devicetree.h>
#include <drivers/pwm.h>
#include "nrfx_gpiote.h"

#define PWM_SPKR_NODE	DT_ALIAS(pwmspeaker)
#define PWM_CTLR	DT_PWMS_CTLR(PWM_SPKR_NODE)
#define PWM_CHANNEL DT_PWMS_CHANNEL(PWM_SPKR_NODE)

#define SPKR_PWR_SWITCH 29

void speaker_init(void);
void speaker_always_on(int freq, int volume);
void speaker_limit_on(int freq, int volume, int duration);
void speaker_off(void);
void speaker_always_on_alarm(int freq, int volume, int on_time, int idle_time);
void speaker_limit_on_alarm(int freq, int volume, int on_time, int idle_time, int duration);
#endif // !__SPEAKER_FUNC_H__
 