/*
 * encoders.c
 *
 *  Created on: May 24, 2026
 *      Author: anbui
 */

/* encoder_count.c
 *
 * Basic STM32 HAL encoder-count test file for Micromouse.
 *
 * Purpose:
 *   - Start encoder timers
 *   - Poll raw encoder counts
 *   - Track signed absolute counts, including negative movement
 *   - Reset counts
 *   - Estimate wheel distance and mouse rotation angle
 *
 * How to use:
 *   1. Add this file to Core/Src in STM32CubeIDE.
 *   2. In CubeMX, configure two timers in Encoder Mode:
 *        - one timer for left encoder
 *        - one timer for right encoder
 *   3. Update the USER CONFIG section below so htim names match your project.
 *   4. Call Encoders_Init() once before while(1).
 *   5. Call Encoders_Update() repeatedly in while(1), SysTick, or a timer interrupt.
 */

#include "main.h"
#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* USER CONFIG: edit these values to match your CubeMX + hardware setup         */
/* -------------------------------------------------------------------------- */


// Example:
//    If left encoder uses TIM3 and right encoder uses TIM4, CubeMX creates:
      extern TIM_HandleTypeDef htim3;
       extern TIM_HandleTypeDef htim4;

// * Change these to match your actual encoder timers.

extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

#define ENC_LEFT_TIMER        htim3
#define ENC_RIGHT_TIMER       htim4

/*
 * Counts per wheel revolution depends on:
 *   encoder pulses per motor rev,
 *   quadrature mode multiplier,
 *   gear ratio.
 *
 * Start with a placeholder, then calibrate.
 */
#define ENC_COUNTS_PER_REV    600.0f

/*
 * Wheel diameter in millimeters.
 * Change this to your measured wheel diameter.
 */
#define WHEEL_DIAMETER_MM     32.0f

/*
 * Distance between left and right wheel contact points in millimeters.
 * Change this to your measured mouse track width.
 */
#define WHEEL_BASE_MM         80.0f

/*
 * If one encoder increases when you expect it to decrease, flip that side.
 */
#define ENC_LEFT_INVERT       1
#define ENC_RIGHT_INVERT      1

/* -------------------------------------------------------------------------- */
/* Public debug variables                                                      */
/* -------------------------------------------------------------------------- */

/*
 * Add these to Live Expressions during Debug mode:
 *   enc_left_raw
 *   enc_right_raw
 *   enc_left_total
 *   enc_right_total
 *   enc_left_mm
 *   enc_right_mm
 *   enc_center_mm
 *   enc_angle_deg
 */

volatile int16_t enc_left_raw = 0;
volatile int16_t enc_right_raw = 0;

volatile int32_t enc_left_total = 0;
volatile int32_t enc_right_total = 0;

volatile float enc_left_mm = 0.0f;
volatile float enc_right_mm = 0.0f;
volatile float enc_center_mm = 0.0f;
volatile float enc_angle_deg = 0.0f;

/* -------------------------------------------------------------------------- */
/* Internal state                                                              */
/* -------------------------------------------------------------------------- */

static int16_t enc_left_prev = 0;
static int16_t enc_right_prev = 0;

/* -------------------------------------------------------------------------- */
/* Public function prototypes                                                  */
/* -------------------------------------------------------------------------- */

void Encoders_Init(void);
void Encoders_Update(void);
void Encoders_Reset(void);

int32_t Encoders_GetLeftCount(void);
int32_t Encoders_GetRightCount(void);

float Encoders_GetLeftDistanceMM(void);
float Encoders_GetRightDistanceMM(void);
float Encoders_GetCenterDistanceMM(void);
float Encoders_GetAngleDeg(void);

void Encoders_TestPoll(void);

/* -------------------------------------------------------------------------- */
/* Internal helpers                                                            */
/* -------------------------------------------------------------------------- */

static int16_t timer_count_16(TIM_HandleTypeDef *htim)
{
  return (int16_t)__HAL_TIM_GET_COUNTER(htim);
}

static float counts_to_mm(int32_t counts)
{
  const float wheel_circumference_mm = 3.1415926f * WHEEL_DIAMETER_MM;
  return ((float)counts / ENC_COUNTS_PER_REV) * wheel_circumference_mm;
}

static void update_distance_values(void)
{
  enc_left_mm = counts_to_mm(enc_left_total);
  enc_right_mm = counts_to_mm(enc_right_total);

  enc_center_mm = (enc_left_mm + enc_right_mm) * 0.5f;

  /*
   * Differential-drive angle estimate:
   * angle_rad = (right_distance - left_distance) / wheel_base
   * angle_deg = angle_rad * 180/pi
   */
  enc_angle_deg = ((enc_right_mm - enc_left_mm) / WHEEL_BASE_MM) * 57.29578f;
}

/* -------------------------------------------------------------------------- */
/* Public encoder functions                                                    */
/* -------------------------------------------------------------------------- */

void Encoders_Init(void)
{
  HAL_TIM_Encoder_Start(&ENC_LEFT_TIMER, TIM_CHANNEL_ALL);
  HAL_TIM_Encoder_Start(&ENC_RIGHT_TIMER, TIM_CHANNEL_ALL);

  Encoders_Reset();
}

void Encoders_Reset(void)
{
  __HAL_TIM_SET_COUNTER(&ENC_LEFT_TIMER, 0);
  __HAL_TIM_SET_COUNTER(&ENC_RIGHT_TIMER, 0);

  enc_left_prev = 0;
  enc_right_prev = 0;

  enc_left_raw = 0;
  enc_right_raw = 0;

  enc_left_total = 0;
  enc_right_total = 0;

  update_distance_values();
}

void Encoders_Update(void)
{
  int16_t left_now = timer_count_16(&ENC_LEFT_TIMER);
  int16_t right_now = timer_count_16(&ENC_RIGHT_TIMER);

  /*
   * Signed difference handles wraparound correctly for 16-bit timer counts,
   * as long as Encoders_Update() is called often enough that the count does
   * not change by more than 32767 ticks between updates.
   */
  int16_t left_delta = (int16_t)(left_now - enc_left_prev);
  int16_t right_delta = (int16_t)(right_now - enc_right_prev);

  enc_left_prev = left_now;
  enc_right_prev = right_now;

#if ENC_LEFT_INVERT
  left_delta = -left_delta;
#endif

#if ENC_RIGHT_INVERT
  right_delta = -right_delta;
#endif

  enc_left_raw = left_now;
  enc_right_raw = right_now;

  enc_left_total += left_delta;
  enc_right_total += right_delta;

  update_distance_values();
}

int32_t Encoders_GetLeftCount(void)
{
  return enc_left_total;
}

int32_t Encoders_GetRightCount(void)
{
  return enc_right_total;
}

float Encoders_GetLeftDistanceMM(void)
{
  return enc_left_mm;
}

float Encoders_GetRightDistanceMM(void)
{
  return enc_right_mm;
}

float Encoders_GetCenterDistanceMM(void)
{
  return enc_center_mm;
}

float Encoders_GetAngleDeg(void)
{
  return enc_angle_deg;
}

/* -------------------------------------------------------------------------- */
/* Simple test helper                                                          */
/* -------------------------------------------------------------------------- */

void Encoders_TestPoll(void)
{
  Encoders_Update();
  HAL_Delay(20);
}
