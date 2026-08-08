#include "rc522.h"
#include "main.h"

SPI_HandleTypeDef *rc522_spi;

#define RC522_CS_LOW()                                                         \
  HAL_GPIO_WritePin(SPI3_CS_GPIO_Port, SPI3_CS_Pin, GPIO_PIN_RESET)
#define RC522_CS_HIGH()                                                        \
  HAL_GPIO_WritePin(SPI3_CS_GPIO_Port, SPI3_CS_Pin, GPIO_PIN_SET)
#define RC522_RST_LOW()                                                        \
  HAL_GPIO_WritePin(SPI3_RST_GPIO_Port, SPI3_RST_Pin, GPIO_PIN_RESET)
#define RC522_RST_HIGH()                                                       \
  HAL_GPIO_WritePin(SPI3_RST_GPIO_Port, SPI3_RST_Pin, GPIO_PIN_SET)

#include "usb_host.h"

void RC522_WriteRegister(uint8_t addr, uint8_t val) {
  uint8_t txData[2];
  txData[0] = (addr << 1) & 0x7E;
  txData[1] = val;
  RC522_CS_LOW();
  HAL_SPI_Transmit_DMA(rc522_spi, txData, 2);
  while (HAL_SPI_GetState(rc522_spi) != HAL_SPI_STATE_READY) {
      MX_USB_HOST_Process();
  }
  RC522_CS_HIGH();
}

uint8_t RC522_ReadRegister(uint8_t addr) {
  uint8_t rxData[2];
  uint8_t txData[2];
  txData[0] = ((addr << 1) & 0x7E) | 0x80;
  txData[1] = 0x00;
  RC522_CS_LOW();
  HAL_SPI_TransmitReceive_DMA(rc522_spi, txData, rxData, 2);
  while (HAL_SPI_GetState(rc522_spi) != HAL_SPI_STATE_READY) {
      MX_USB_HOST_Process();
  }
  RC522_CS_HIGH();
  return rxData[1];
}

void RC522_SetBitMask(uint8_t reg, uint8_t mask) {
  uint8_t tmp;
  tmp = RC522_ReadRegister(reg);
  RC522_WriteRegister(reg, tmp | mask);
}

void RC522_ClearBitMask(uint8_t reg, uint8_t mask) {
  uint8_t tmp;
  tmp = RC522_ReadRegister(reg);
  RC522_WriteRegister(reg, tmp & (~mask));
}

void RC522_AntennaOn(void) {
  uint8_t temp;
  temp = RC522_ReadRegister(TxControlReg);
  if (!(temp & 0x03)) {
    RC522_SetBitMask(TxControlReg, 0x03);
  }
}

void RC522_AntennaOff(void) { RC522_ClearBitMask(TxControlReg, 0x03); }

void RC522_Reset(void) {
  RC522_WriteRegister(CommandReg, PCD_RESETPHASE);
  HAL_Delay(10);
}

void RC522_Init(SPI_HandleTypeDef *hspi) {
  rc522_spi = hspi;
  RC522_CS_HIGH();

  RC522_RST_HIGH();
  HAL_Delay(10);
  RC522_Reset();

  RC522_WriteRegister(TModeReg, 0x8D);
  RC522_WriteRegister(TPrescalerReg, 0x3E);
  RC522_WriteRegister(TReloadRegL, 30);
  RC522_WriteRegister(TReloadRegH, 0);

  RC522_WriteRegister(TxAutoReg, 0x40);
  RC522_WriteRegister(ModeReg, 0x3D);
  RC522_WriteRegister(
      RFCfgReg, 0x78); // Set RxGain to maximum (48 dB), keep bits 3:0 at 0x08
  RC522_WriteRegister(
      0x28, 0x3F); // CWGsPReg: Set to max (0x3F) to maximize Tx Power for e-KTP

  RC522_AntennaOn();
}

uint8_t RC522_Request(uint8_t reqMode, uint8_t *TagType) {
  uint8_t status;
  uint16_t backBits;

  RC522_WriteRegister(BitFramingReg, 0x07);

  TagType[0] = reqMode;
  status = RC522_ToCard(PCD_TRANSCEIVE, TagType, 1, TagType, &backBits);

  if (status != MI_OK) {
    status = MI_ERR;
  }
  return status;
}

uint8_t RC522_ToCard(uint8_t command, uint8_t *sendData, uint8_t sendLen,
                     uint8_t *backData, uint16_t *backLen) {
  uint8_t status = MI_ERR;
  uint8_t irqEn = 0x00;
  uint8_t waitIRq = 0x00;
  uint8_t lastBits;
  uint8_t n;
  uint16_t i;

  switch (command) {
  case PCD_AUTHENT:
    irqEn = 0x12;
    waitIRq = 0x10;
    break;
  case PCD_TRANSCEIVE:
    irqEn = 0x77;
    waitIRq = 0x30;
    break;
  default:
    break;
  }

  RC522_WriteRegister(CommIEnReg, irqEn | 0x80);
  RC522_ClearBitMask(CommIrqReg, 0x80);
  RC522_SetBitMask(FIFOLevelReg, 0x80);

  RC522_WriteRegister(CommandReg, PCD_IDLE);

  for (i = 0; i < sendLen; i++) {
    RC522_WriteRegister(FIFODataReg, sendData[i]);
  }

  RC522_WriteRegister(CommandReg, command);
  if (command == PCD_TRANSCEIVE) {
    RC522_SetBitMask(BitFramingReg, 0x80);
  }

  i = 2000;
  do {
    n = RC522_ReadRegister(CommIrqReg);
    i--;
    MX_USB_HOST_Process();
  } while ((i != 0) && !(n & 0x01) && !(n & waitIRq));

  RC522_ClearBitMask(BitFramingReg, 0x80);

  if (i != 0) {
    if (!(RC522_ReadRegister(ErrorReg) & 0x1B)) {
      status = MI_OK;
      if (n & irqEn & 0x01) {
        status = MI_NOTAGERR;
      }

      if (command == PCD_TRANSCEIVE) {
        n = RC522_ReadRegister(FIFOLevelReg);
        lastBits = RC522_ReadRegister(ControlReg) & 0x07;
        if (lastBits) {
          *backLen = (n - 1) * 8 + lastBits;
        } else {
          *backLen = n * 8;
        }

        if (n == 0) {
          n = 1;
        }
        if (n > 16) {
          n = 16;
        }

        for (i = 0; i < n; i++) {
          backData[i] = RC522_ReadRegister(FIFODataReg);
        }
      }
    } else {
      status = MI_ERR;
    }
  }

  return status;
}

uint8_t RC522_Anticoll(uint8_t *serNum) {
  uint8_t status;
  uint8_t i;
  uint8_t serNumCheck = 0;
  uint16_t unLen;

  RC522_WriteRegister(BitFramingReg, 0x00);

  serNum[0] = PICC_ANTICOLL;
  serNum[1] = 0x20;
  status = RC522_ToCard(PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);

  if (status == MI_OK) {
    for (i = 0; i < 4; i++) {
      serNumCheck ^= serNum[i];
    }
    if (serNumCheck != serNum[4]) {
      status = MI_ERR;
    }
  }
  return status;
}

uint8_t RC522_Check(uint8_t *id) {
  uint8_t status;
  status = RC522_Request(PICC_REQIDL, id);
  if (status != MI_OK) {
    status = RC522_Request(PICC_REQALL, id);
  }

  // Jika masih gagal, chip e-KTP mungkin hang karena masuk ke medan RF secara
  // lambat. Kita matikan RF sejenak lalu nyalakan lagi untuk "Cold Boot" (Reset
  // paksa) kartu.
  if (status != MI_OK) {
    RC522_AntennaOff();
    HAL_Delay(5);
    RC522_AntennaOn();
    HAL_Delay(5);
    status = RC522_Request(PICC_REQALL, id);
  }

  if (status == MI_OK) {
    status = RC522_Anticoll(id);
  }
  RC522_Halt();
  return status;
}

uint8_t RC522_Compare(uint8_t *CardID, uint8_t *CompareID) {
  uint8_t i;
  for (i = 0; i < 5; i++) {
    if (CardID[i] != CompareID[i])
      return MI_ERR;
  }
  return MI_OK;
}

void RC522_Halt(void) {
  uint16_t unLen;
  uint8_t buff[4];

  buff[0] = PICC_HALT;
  buff[1] = 0;
  RC522_CalculateCRC(buff, 2, &buff[2]);

  RC522_ToCard(PCD_TRANSCEIVE, buff, 4, buff, &unLen);
}

void RC522_CalculateCRC(uint8_t *pIndata, uint8_t len, uint8_t *pOutData) {
  uint8_t i, n;

  RC522_ClearBitMask(DivIrqReg, 0x04);
  RC522_SetBitMask(FIFOLevelReg, 0x80);

  for (i = 0; i < len; i++) {
    RC522_WriteRegister(FIFODataReg, *(pIndata + i));
  }
  RC522_WriteRegister(CommandReg, PCD_CALCCRC);

  i = 0xFF;
  do {
    n = RC522_ReadRegister(DivIrqReg);
    i--;
  } while ((i != 0) && !(n & 0x04));

  pOutData[0] = RC522_ReadRegister(CRCResultRegL);
  pOutData[1] = RC522_ReadRegister(CRCResultRegM);
}
