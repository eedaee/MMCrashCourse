#include "main.h"
#include "micromouse.h"
#include <stdint.h>

extern TIM_HandleTypeDef htim2;

#define MOTOR_LEFT_PWM_TIMER       htim2
#define MOTOR_RIGHT_PWM_TIMER      htim2

#define MOTOR_LEFT_PWM_CHANNEL     TIM_CHANNEL_3
#define MOTOR_RIGHT_PWM_CHANNEL    TIM_CHANNEL_4

#define MOTOR_L_IN1_PORT           ML_FWD_GPIO_Port
#define MOTOR_L_IN1_PIN            ML_FWD_Pin

#define MOTOR_L_IN2_PORT           ML_BWD_GPIO_Port
#define MOTOR_L_IN2_PIN            ML_BWD_Pin

#define MOTOR_R_IN1_PORT           MR_FWD_GPIO_Port
#define MOTOR_R_IN1_PIN            MR_FWD_Pin

#define MOTOR_R_IN2_PORT           MR_BWD_GPIO_Port
#define MOTOR_R_IN2_PIN            MR_BWD_Pin

/* Uncomment if driver has a standby/sleep pin labeled MOTOR_STBY. */
// #define MOTOR_HAS_STBY
#ifdef MOTOR_HAS_STBY
#define MOTOR_STBY_PORT            MOTOR_STBY_GPIO_Port
#define MOTOR_STBY_PIN             MOTOR_STBY_Pin
#endif

/* Change to 1 if a motor spins opposite of expected direction. */
#define MOTOR_LEFT_INVERT          0
#define MOTOR_RIGHT_INVERT         0

/* Speed command range: 0 = stop, 1000 = full PWM duty. */
#define MOTOR_MAX_CMD              1000

void Motors_Init(void);
void Motors_SetSpeed(int16_t left_speed, int16_t right_speed);
void Motors_Forward(uint16_t speed);
void Motors_Backward(uint16_t speed);
void Motors_Stop(void);
void Motors_Brake(void);
void Motors_TestSequence(void);

static int16_t clamp_speed(int16_t speed)
{
  if (speed > MOTOR_MAX_CMD) {
    return MOTOR_MAX_CMD;
  }
  if (speed < -MOTOR_MAX_CMD) {
    return -MOTOR_MAX_CMD;
  }
  return speed;
}

static uint32_t pwm_period(TIM_HandleTypeDef *htim)
{
  return __HAL_TIM_GET_AUTORELOAD(htim);
}

static void set_pwm(TIM_HandleTypeDef *htim, uint32_t channel, uint16_t command)
{
  if (command > MOTOR_MAX_CMD) {
    command = MOTOR_MAX_CMD;
  }

  uint32_t period = pwm_period(htim);
  uint32_t compare = ((uint32_t)command * period) / MOTOR_MAX_CMD;

  __HAL_TIM_SET_COMPARE(htim, channel, compare);
}

static void set_motor_direction(GPIO_TypeDef *in1_port,
                                uint16_t in1_pin,
                                GPIO_TypeDef *in2_port,
                                uint16_t in2_pin,
                                int forward,
                                int inverted)
{
  if (inverted) {
    forward = !forward;
  }

  if (forward) {
    HAL_GPIO_WritePin(in1_port, in1_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(in2_port, in2_pin, GPIO_PIN_RESET);
  } else {
    HAL_GPIO_WritePin(in1_port, in1_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(in2_port, in2_pin, GPIO_PIN_SET);
  }
}

static void set_left_motor(int16_t speed)
{
  speed = clamp_speed(speed);

  if (speed == 0) {
    set_pwm(&MOTOR_LEFT_PWM_TIMER, MOTOR_LEFT_PWM_CHANNEL, 0);
    HAL_GPIO_WritePin(MOTOR_L_IN1_PORT, MOTOR_L_IN1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_L_IN2_PORT, MOTOR_L_IN2_PIN, GPIO_PIN_RESET);
    return;
  }

  int forward = (speed > 0);
  uint16_t duty = (uint16_t)(speed > 0 ? speed : -speed);

  set_motor_direction(MOTOR_L_IN1_PORT, MOTOR_L_IN1_PIN,
                      MOTOR_L_IN2_PORT, MOTOR_L_IN2_PIN,
                      forward,
                      MOTOR_LEFT_INVERT);

  set_pwm(&MOTOR_LEFT_PWM_TIMER, MOTOR_LEFT_PWM_CHANNEL, duty);
}

static void set_right_motor(int16_t speed)
{
  speed = clamp_speed(speed);

  if (speed == 0) {
    set_pwm(&MOTOR_RIGHT_PWM_TIMER, MOTOR_RIGHT_PWM_CHANNEL, 0);
    HAL_GPIO_WritePin(MOTOR_R_IN1_PORT, MOTOR_R_IN1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_R_IN2_PORT, MOTOR_R_IN2_PIN, GPIO_PIN_RESET);
    return;
  }

  int forward = (speed > 0);
  uint16_t duty = (uint16_t)(speed > 0 ? speed : -speed);

  set_motor_direction(MOTOR_R_IN1_PORT, MOTOR_R_IN1_PIN,
                      MOTOR_R_IN2_PORT, MOTOR_R_IN2_PIN,
                      forward,
                      MOTOR_RIGHT_INVERT);

  set_pwm(&MOTOR_RIGHT_PWM_TIMER, MOTOR_RIGHT_PWM_CHANNEL, duty);
}

void Motors_Init(void)
{
#ifdef MOTOR_HAS_STBY
  HAL_GPIO_WritePin(MOTOR_STBY_PORT, MOTOR_STBY_PIN, GPIO_PIN_SET);
#endif

  HAL_TIM_PWM_Start(&MOTOR_LEFT_PWM_TIMER, MOTOR_LEFT_PWM_CHANNEL);
  HAL_TIM_PWM_Start(&MOTOR_RIGHT_PWM_TIMER, MOTOR_RIGHT_PWM_CHANNEL);

  Motors_Stop();
}

void Motors_SetSpeed(int16_t left_speed, int16_t right_speed)
{
  set_left_motor(left_speed);
  set_right_motor(right_speed);
}

void Motors_Forward(uint16_t speed)
{
  if (speed > MOTOR_MAX_CMD) {
    speed = MOTOR_MAX_CMD;
  }

  Motors_SetSpeed((int16_t)speed, (int16_t)speed);
}

void Motors_Backward(uint16_t speed)
{
  if (speed > MOTOR_MAX_CMD) {
    speed = MOTOR_MAX_CMD;
  }

  Motors_SetSpeed(-(int16_t)speed, -(int16_t)speed);
}

void Motors_Stop(void)
{
  set_left_motor(0);
  set_right_motor(0);
}

void Motors_Brake(void)
{
  /*
   * IN1 = HIGH, IN2 = HIGH, PWM = HIGH.
   */
  HAL_GPIO_WritePin(MOTOR_L_IN1_PORT, MOTOR_L_IN1_PIN, GPIO_PIN_SET);
  HAL_GPIO_WritePin(MOTOR_L_IN2_PORT, MOTOR_L_IN2_PIN, GPIO_PIN_SET);
  HAL_GPIO_WritePin(MOTOR_R_IN1_PORT, MOTOR_R_IN1_PIN, GPIO_PIN_SET);
  HAL_GPIO_WritePin(MOTOR_R_IN2_PORT, MOTOR_R_IN2_PIN, GPIO_PIN_SET);

  set_pwm(&MOTOR_LEFT_PWM_TIMER, MOTOR_LEFT_PWM_CHANNEL, MOTOR_MAX_CMD);
  set_pwm(&MOTOR_RIGHT_PWM_TIMER, MOTOR_RIGHT_PWM_CHANNEL, MOTOR_MAX_CMD);
}

void Motors_TestSequence(void)
{
  Motors_Forward(250);
  HAL_Delay(1000);

  Motors_Stop();
  HAL_Delay(500);

  Motors_Backward(250);
  HAL_Delay(1000);

  Motors_Stop();
  HAL_Delay(500);

  Motors_Forward(350);
  HAL_Delay(700);

  Motors_Brake();
  HAL_Delay(500);

  Motors_Stop();
}

