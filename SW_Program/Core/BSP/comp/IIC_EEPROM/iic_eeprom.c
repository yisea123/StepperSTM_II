#include <stdlib.h>
#include <string.h>

#include "iic_eeprom.h"


#define     EE24LC08

#ifdef EE24LC08
/* 24LC08 EEPROM */
#define     I2C_EEP_BASE_ADDRESS    0x50
#define     MEMORY_SIZE             1024
#define     PAGE_CALC_SHIFT_VAL     4
#define     BLOCK_CALC_SHIFT_VAL    8
#define     DATA_OFFSET_MASK        0x0F
#define     PAGE_SIZE               16
#define     PAGES_IN_BLOCK          16
#endif

#ifdef EE24LC16
/* 24LC16 EEPROM */
#define     I2C_EEP_BASE_ADDRESS    0x50
#define     MEMORY_SIZE             2048
#define     PAGE_CALC_SHIFT_VAL     4
#define     BLOCK_CALC_SHIFT_VAL    8
#define     DATA_OFFSET_MASK        0x0F
#define     PAGE_SIZE               16
#define     PAGES_IN_BLOCK          16
#endif

enum {IIC_READ, IIC_WRITE} eIIC_PROCESS;  // kaip komunikuojam su iic


/*  Prototipai  */
static uint8_t EEP24XX_Process( uint16_t mem_addr, void *txdata, uint16_t len, uint8_t proc );

/*   */
uint8_t EEP24XX_IsReady(void) {

    if( HAL_I2C_IsDeviceReady(&hi2c1, (I2C_EEP_BASE_ADDRESS<<1), 3, 100 ) != HAL_OK) return RES_ERROR;

    return RES_OK;
}

uint8_t EEP24XX_IsBusy(void) {

    if( HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY ) return RES_BUSY;

    return RES_OK;
}

/* rasymas i EEPROM pool rezime */
uint8_t EEP24XX_Write( uint16_t mem_addr, void *data, size_t size_of_data ) {

    if( mem_addr > MEMORY_SIZE-1 || data == NULL || size_of_data == 0 ) return RES_BAD_PARAMS;    //error

    return EEP24XX_Process( mem_addr, data, size_of_data, IIC_WRITE );
}

/* skaitymas is EEPROM pool rezime */
uint8_t EEP24XX_Read( uint16_t mem_addr, void *rxdata, size_t size_of_data ) {

    if( mem_addr > MEMORY_SIZE-1 || rxdata == NULL || size_of_data == 0 ) return RES_BAD_PARAMS;    //error

    return EEP24XX_Process( mem_addr, rxdata, size_of_data, IIC_READ );
}


uint8_t EEP24XX_Clear(void) {

    uint8_t result = RES_OK;
    uint8_t *data = (uint8_t*)malloc(sizeof(uint8_t)*(PAGE_SIZE));
    uint16_t i = 0;
    uint16_t addr = 0;

    do {
        *(data+i) = 0xFF;
    } while(++i < PAGE_SIZE);

    while(addr < MEMORY_SIZE) {
        if( (result = EEP24XX_Process( addr, data, 1, IIC_WRITE )) != RES_OK) break;
        addr += PAGE_SIZE;
    }

    free(data);

    return result;
}

/* EEPROM processas */
static uint8_t EEP24XX_Process( uint16_t mem_addr, void *data, uint16_t len, uint8_t proc ) {

    uint8_t result = RES_OK;

    uint8_t wr_size = 0;
    uint8_t* pdata = data;

    /* apskaiciuojam pagal adresa eeprom bloka, peidza */
    uint8_t block = mem_addr>>BLOCK_CALC_SHIFT_VAL;                 // kuris blokas
    //uint8_t pages = (len>>PAGE_CALC_SHIFT_VAL)+1;                   // kiek peidzu
    uint8_t page = mem_addr>>PAGE_CALC_SHIFT_VAL;                   // kuris peidzas eeprome
    uint8_t page_in_block = page - (block<<PAGE_CALC_SHIFT_VAL);    // kuris peidzas bloke
    uint8_t offset_in_page = mem_addr & (PAGE_SIZE-1);              // baito offsetas peidze nuo peidzio pradzios

    uint8_t eeaddr = (I2C_EEP_BASE_ADDRESS + block)<<1;

    if( ( result = BSP_IIC_IsReady(I2C_EEP_BASE_ADDRESS) ) != 0 ) return result;

    while(len > 0) {

        wr_size = PAGE_SIZE - offset_in_page;   // galimas duomenu kiekis iki peidzo pabaigos

        if(len < wr_size) wr_size = len;  // jai likusiu duomenu maziau, nei vietos iki paidzo pabaigos, rasom tik likusius duomenys

        // irasom duomenu dali
        switch(proc){
            case IIC_READ:
                BSP_IIC_Read(eeaddr, mem_addr, pdata, (uint16_t)wr_size);
            break;
            case IIC_WRITE:
                BSP_IIC_Write(eeaddr, mem_addr, pdata, (uint16_t)wr_size);
                BSP_Delay(10);
            break;
        }

        /* cia jau esame peidzo gale, reikia pradeti nuo naujo peidzo */
        len -= wr_size;        // mazinam likusiu duomenu kieki

        if(len == 0) break;    // iseinam, jai viska irasem

        page++;
        offset_in_page = 0;
        mem_addr += wr_size;            // didinam likusiu duomenu irasymo adresa
        pdata += wr_size;               // perskaiciuojam pounteri i duomenys buferyje

        if(page_in_block == PAGES_IN_BLOCK-1) { // paskutinis peidzas bloke
            /* perjungiam bloka */
            block++;
            eeaddr = (I2C_EEP_BASE_ADDRESS + block)<<1;
            page_in_block = 0;
        } else {
            page_in_block++;        // pakeliam paidzo numeri bloke
        }
    }

    /* pagalvoti kaip ir ka grazinam!!! */
    return RES_OK;
}
