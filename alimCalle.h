/*
 * alimCalle.h
 *
 *  Created on: 20 sept. 2022
 *      Author: joaquin
 */

#ifndef ALIMCALLE_H_
#define ALIMCALLE_H_

void initServo(void);
void mueveServoAncho(uint16_t ancho);
void mueveServoPos(uint8_t porcPosicion);
uint8_t sleep_for_x_sec(uint32_t nb_sec);
uint8_t stop(uint16_t nb_sec);
uint8_t standby0(uint16_t nb_sec);
void ports_set_lowpower(void);
void ports_set_normal(void);
void initW25q16(void);
void leeHora(struct tm *tmAhora);
void estadoDeseadoPuerta(uint8_t *estDes, uint16_t *sec2change);

#endif /* ALIMCALLE_H_ */
