/*
 * sms.h
 *
 *  Created on: 1/8/2017
 *      Author: joaquin
 */

#ifndef SMS_H_
#define SMS_H_

#define SECTORNUMTELEF 1
#define SECTORPIN 2
#include "time.h"

uint8_t initSIM800SMS(BaseChannel  *pSD, uint8_t verbose);
void mataSMS(void);
void initThreadSMS(BaseChannel  *pSD);
uint8_t modemOrden(BaseChannel  *pSD, const char *orden, uint8_t *buffer, uint8_t bufferSize, systime_t timeout);
uint8_t modemParametro(BaseChannel  *pSD, const char *orden, uint8_t *buffer, uint16_t bufferSize, systime_t timeout);
uint16_t modemParametros(BaseChannel  *pSD, const char *orden, const char *cadRespuesta, uint8_t *buffer, uint16_t bufferSize, systime_t timeout,
                         uint8_t *numParametros,uint8_t *parametros[]);
void chgetsNoEchoTimeOut(BaseChannel  *pSD,  uint8_t *buffer, uint16_t bufferSize, systime_t timeout, uint8_t *huboTimeout);
uint8_t chgetchTimeOut(BaseChannel  *pSD, systime_t timeout, uint8_t *huboTimeout);
void leoSMS(char *msg, uint16_t buffSize);
int8_t sendSMS(BaseChannel  *pSD, char *msg, char *numTelefono);
uint8_t ponHoraConGprs(BaseChannel  *pSD);
uint8_t horaSMStoTM(uint8_t *cadena, struct tm *fecha);
uint16_t lee2car(uint8_t *buffer, uint16_t posIni,uint16_t minValor, uint16_t maxValor, uint8_t *hayError);
void interpretaSMS(uint8_t *textoSMS);


class sms
{
    //SMS page tlfAdmin logTxt rssiTxt proveedorTxt smsReady
private:
    char telefonoAdmin[50], pin[50];
    char mensajeRecibido[200];
    char msgRespuesta[200];
    uint8_t estadoPuesto;
    char telefonoRecibido[16];
    char telefonoEnvio[16];
    uint8_t bajoConsumo;
    uint8_t durmiendo;
    mutex_t MtxEspSim800SMS;
    uint8_t horaSMStoTM(uint8_t *cadena, struct tm *fecha);
    void procesaOrdenAsignacion(char *orden, char *puntSimbIgual);
    void ponStatusHistorico(void);
    void procesaStatusYAlgo(char *algo);
    void procesaID(char *idNum);
    void procesaStatus(void);
public:
    BaseChannel  *pSD;
    uint8_t callReady, smsReadyVal, estadoCREG, rssiGPRS, pinReady, hayError;
    char proveedor[25];
    time_t tiempoLastConexion;

    sms(const char *numTelefAdminDefault, const char *pinDefault, BaseChannel *puertoSD, uint8_t bajoConsumo);
    ~sms();

    // para implementar "bloque"
    const char *diNombre(void);
    int8_t init(void);
    void calcula(uint8_t hora, uint8_t min, uint8_t seg, uint8_t ds);
    void print(BaseSequentialStream *tty);
    void addTime(uint16_t dsInc, uint8_t hora, uint8_t min, uint8_t seg, uint8_t ds);
    const char *diTipo(void);

    // especifico de SMS
    void startSMS(void);
    void mataSMS(void);
    void leoSMS(char *bufferSendSMS, uint16_t buffSize);
    uint8_t initSIM800SMS(void);
    void initThreadSMS(void);
    const char *diTelefonoAdmin(void);
    const char *diMsgRespuesta(void);
    uint8_t ponHoraConGprs(void);
    void interpretaSMS(uint8_t *textoSMS);
    int8_t sendSMS(char *msg, char *numTelefono);
    int8_t sendSMSAdmin(void);
    int8_t sendSMS(void);
    void sleep(void);
    void despierta(void);
    void trataOrdenNextion(char *vars[], uint16_t numPars);
    void leoSmsCMTI(void);
    void borraMsgRespuesta(void);
    void addMsgRespuesta(const char *texto);
    void ponEstado(void);
    void procesaOrden(char *orden, uint8_t *error);
};

#endif /* SMS_H_ */
