/*
 * micromouse.h
 * Shared declarations for main.c, floodfill.c, and motor_control.c
 */

#ifndef MICROMOUSE_H_
#define MICROMOUSE_H_

#include "main.h"
#include <stdint.h>

/* Keep this enum in ONE place so every .c file uses the same values. */
typedef enum {
  DIST_FL,
  DIST_FR,
  DIST_L,
  DIST_R
} dist_t;

uint16_t measure_dist(dist_t dist);

void micromouse_init(void);
void micromouse_step(void);

void Motors_Init(void);
void Motors_SetSpeed(int16_t left_speed, int16_t right_speed);
void Motors_Forward(uint16_t speed);
void Motors_Backward(uint16_t speed);
void Motors_Stop(void);
void Motors_Brake(void);
void Motors_TestSequence(void);

void MM_TurnRight90(void);
void MM_TurnLeft90(void);
void MM_MoveForwardOneCell(void);

void Encoders_Init(void);
void Encoders_Update(void);
void Encoders_Reset(void);

int32_t Encoders_GetLeftCount(void);
int32_t Encoders_GetRightCount(void);

float Encoders_GetLeftDistanceMM(void);
float Encoders_GetRightDistanceMM(void);
float Encoders_GetCenterDistanceMM(void);
float Encoders_GetAngleDeg(void);

#endif /* MICROMOUSE_H_ */
