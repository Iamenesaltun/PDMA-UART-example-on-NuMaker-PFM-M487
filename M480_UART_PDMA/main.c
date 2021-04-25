
#include <stdio.h>
#include "NuMicro.h"
#include "string.h"


#define PLL_CLOCK   192000000
#define PDMA_TIME 0x5555


/*---------------------------------------------------------------------------------------------------------*/
/* Global Degiskenler                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/

char str[10];
uint8_t FLAG_ISLEM_BITTI = 0;
uint8_t deneme_counter=0 ; 
uint8_t FLAG_DMA_BASLAT=0;

/*---------------------------------------------------------------------------------------------------------*/
/* Fonksiyon Prototipleri                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA_IRQHandler(void);
void UART_PDMATest(void);
void UART_reloadPDMATest(void);

void SYS_Init(void)
{
/*---------------------------------------------------------------------------------------------------------*/
/* Sistem clock tanimlamalari                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
/* Korumali registerlarin kilidini ac */
SYS_UnlockReg();
/* Ic osilator aktiflestir */
CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
/* Ic osilator hazir oluncaya kadar bekle */
CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);
/*  XT1_OUT(PF.2) ve XT1_IN(PF.3) giris olarak belirle */
PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);
/* Harici clock aktiflestir(XTAL 12MHz) */
CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
/* Harici osilator hazir oluncaya kadar bekle */
CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);
/* Cekirdek clock'u PLL'den beslenecek */
CLK_SetCoreClock(192000000);
CLK->PCLKDIV = (CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2); // PCLK divider set 2
/* IP clock'lari aktiflestir*/
CLK->APBCLK0 |= CLK_APBCLK0_UART0CKEN_Msk; // UART0 Clock aktiflestir.
CLK->APBCLK0 |= CLK_APBCLK0_UART1CKEN_Msk; // UART1 Clock aktiflestir.
CLK->AHBCLK  |= CLK_AHBCLK_PDMACKEN_Msk; // PDMA Clock aktiflestir.

/* UART0 clock harici osilatör'den beslenecek */
CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_UART0SEL_Msk) | (0x0 << CLK_CLKSEL1_UART0SEL_Pos);
/* UART1 clock harici osilatör'den beslenecek */
CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_UART1SEL_Msk) | (0x0 << CLK_CLKSEL1_UART1SEL_Pos);

/* PLL Clock'u guncellenir*/
SystemCoreClockUpdate();
/* PB portlarini UART0 RXD and TXD icin ayarla*/
SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk);
SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);
/* PA portlarini UART1 TXD, RXD, CTS and RTS icin ayarla*/
SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA0MFP_Msk | SYS_GPA_MFPL_PA1MFP_Msk |
							 SYS_GPA_MFPL_PA2MFP_Msk | SYS_GPA_MFPL_PA3MFP_Msk);
SYS->GPA_MFPL |= (0x8 << SYS_GPA_MFPL_PA0MFP_Pos) | (0x8 << SYS_GPA_MFPL_PA1MFP_Pos) |
						 (0x8 << SYS_GPA_MFPL_PA2MFP_Pos) | (0x8 << SYS_GPA_MFPL_PA3MFP_Pos);
/* Korumali registerlari kilitle */
SYS_LockReg();
}

void UART0_Init()
{
UART_Open(UART0, 115200);  //UART0 115200 baudrate olacak sekilde ac.
}

void UART1_Init()
{
UART_Open(UART1, 115200);  //UART1 115200 baudrate olacak sekilde ac.
}


void PDMA_Init(void)
{
PDMA_Open(PDMA,1 << 0); // DMA Kanal 0 için UART1 TX ayarla
PDMA_SetTransferMode(PDMA,0, PDMA_UART1_TX, 0, 0); //DMA Kanal 0 için "Basic Mod" ayarla 
PDMA_SetTransferCnt(PDMA,0, PDMA_WIDTH_8, strlen(str)); //DMA Kanal 0 için str uzunlugu kadar 8bit(PDMA_WIDTH_8) data gönder
PDMA_SetTransferAddr(PDMA,0, ((uint32_t) (&str[0])), PDMA_SAR_INC, UART1_BASE, PDMA_DAR_FIX);//DMA Kanal 0 için kaynak-hedef adreslerini ve niteliklerini belirle
PDMA_SetBurstType(PDMA,0, PDMA_REQ_SINGLE, 0);//DMA Kanal 0 için transfer tipini single olarak belirle.
PDMA_EnableInt(PDMA,0, 0);//DMA Kanal 0 için kesme aktiflestir. 
NVIC_EnableIRQ(PDMA_IRQn);//NVIC kesmesini DMA için aktiflestir.
FLAG_ISLEM_BITTI = 0;//Islem bayragini sifirla.
}


void ReloadPDMA()
{
PDMA_SetTransferCnt(PDMA,0, PDMA_WIDTH_8, strlen(str));
PDMA_SetTransferMode(PDMA,0, PDMA_UART1_TX, 0, 0);
FLAG_ISLEM_BITTI = 0;
}


int main(void)
{
SYS_Init();
UART0_Init();
UART1_Init();
GPIO_SetMode(PH, BIT0, GPIO_MODE_OUTPUT);
GPIO_SetMode(PH, BIT1, GPIO_MODE_OUTPUT);
printf("\n\nCPU @ %dHz\n", SystemCoreClock);



sprintf(str, "Deger:%u\n", deneme_counter);
printf("uzunluk : %d \n",strlen(str)); 
PDMA_Init();
NVIC_EnableIRQ(UART1_IRQn);
UART1->INTEN |= UART_INTEN_TXPDMAEN_Msk;


while(1){
UART_reloadPDMATest();

PH0 = 0;
PH1 = 1;
CLK_SysTickDelay(1000000);
CLK_SysTickDelay(1000000);
CLK_SysTickDelay(1000000);		
PH0 = 1;
PH1 = 0;
CLK_SysTickDelay(1000000);
CLK_SysTickDelay(1000000);
CLK_SysTickDelay(1000000);
}
}


void PDMA_IRQHandler(void)
{
uint32_t status = PDMA_GET_INT_STATUS(PDMA);

printf("Interrupt Geldi!!Durum:%u \n",status);

if (status & 0x1)   /* basarisiz */
{
printf("Basarisiz kesme !!\n");
if (PDMA_GET_ABORT_STS(PDMA) & 0x4)
PDMA_CLR_ABORT_FLAG(PDMA,PDMA_GET_ABORT_STS(PDMA));
}
else if (status & 0x2)     /* basarili */
{
if ( (PDMA_GET_TD_STS(PDMA) & (1 << 0)) )
{
FLAG_ISLEM_BITTI = 1;
PDMA_CLR_TD_FLAG(PDMA,PDMA_GET_TD_STS(PDMA));
}
}
else if (status & 0x300)     /* timeout */
{
printf("Timeout kesme !!\n");
PDMA_SetTimeOut(PDMA,0, 0, 0);
PDMA_CLR_TMOUT_FLAG(PDMA,0);
PDMA_SetTimeOut(PDMA,0, 1, PDMA_TIME);
}
else
printf("Bilinmeyen kesme !!\n");
}



void UART_reloadPDMATest()
{

if(FLAG_DMA_BASLAT==1){	
deneme_counter++;
if(deneme_counter>250){
deneme_counter=0;
};
sprintf(str, "Deger:%u\n", deneme_counter);	
printf("uzunluk : %d \n",strlen(str)); 
ReloadPDMA();
NVIC_EnableIRQ(UART1_IRQn);
UART1->INTEN |= UART_INTEN_TXPDMAEN_Msk;
FLAG_DMA_BASLAT=0;
};
if(FLAG_ISLEM_BITTI == 1){
UART1->INTEN &= ~UART_INTEN_TXPDMAEN_Msk;
FLAG_DMA_BASLAT=1;
};
}


void UART1_IRQHandler(void)
{
uint32_t u32DAT;
uint32_t u32IntSts = UART1->INTSTS;
if(u32IntSts & UART_INTSTS_HWRLSIF_Msk)
{
if(UART1->FIFOSTS & UART_FIFOSTS_BIF_Msk)
printf("\n BIF \n");
if(UART1->FIFOSTS & UART_FIFOSTS_FEF_Msk)
printf("\n FEF \n");
if(UART1->FIFOSTS & UART_FIFOSTS_PEF_Msk)
printf("\n PEF \n");
u32DAT = UART1->DAT;
printf("\n Error Data is '0x%x' \n", u32DAT);
UART1->FIFOSTS = (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk);
}
}





