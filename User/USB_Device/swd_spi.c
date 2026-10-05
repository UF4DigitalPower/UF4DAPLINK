#include "swd_spi.h"
#include "ch32v30x_gpio.h"
#include "ch32v30x_rcc.h"
#include "ch32v30x_spi.h"

#define SWD_SWCLK_PORT GPIOA
#define SWD_SWCLK_PIN  GPIO_Pin_5
#define SWD_SWDIO_PORT GPIOA
#define SWD_SWDIO_OUT  GPIO_Pin_7
#define SWD_SWDIO_IN   GPIO_Pin_6
#define SWD_OE_PORT    GPIOA
#define SWD_OE_PIN     GPIO_Pin_4
#define G4_NRST_PORT   GPIOA
#define G4_NRST_PIN    GPIO_Pin_0
#define WIFI_BOOT_PORT GPIOC
#define WIFI_BOOT_PIN  GPIO_Pin_8
#define WIFI_NRST_PORT GPIOC
#define WIFI_NRST_PIN  GPIO_Pin_7

static uint32_t swd_clock_hz = 1000000u;
static uint32_t swd_delay_loops = 20u;
static uint8_t control_state;

static void swd_delay(void)
{
    volatile uint32_t n = swd_delay_loops;
    while (n--) __asm volatile("nop");
}

static void swd_clk(uint8_t high)
{
    if (high) GPIO_SetBits(SWD_SWCLK_PORT, SWD_SWCLK_PIN);
    else GPIO_ResetBits(SWD_SWCLK_PORT, SWD_SWCLK_PIN);
    swd_delay();
}

static void swd_output(uint8_t enable)
{
    GPIO_InitTypeDef g = {0};
    g.GPIO_Pin = SWD_SWDIO_OUT;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    g.GPIO_Mode = enable ? GPIO_Mode_Out_PP : GPIO_Mode_IN_FLOATING;
    GPIO_Init(SWD_SWDIO_PORT, &g);
    GPIO_WriteBit(SWD_OE_PORT, SWD_OE_PIN, enable ? Bit_SET : Bit_RESET);
}

static void swd_write_bit(uint8_t bit)
{
    GPIO_WriteBit(SWD_SWDIO_PORT, SWD_SWDIO_OUT, bit ? Bit_SET : Bit_RESET);
    swd_clk(1); swd_clk(0);
}

static uint8_t swd_read_bit(void)
{
    uint8_t bit;
    swd_clk(1);
    bit = GPIO_ReadInputDataBit(SWD_SWDIO_PORT, SWD_SWDIO_IN) ? 1u : 0u;
    swd_clk(0);
    return bit;
}

static void swd_write_bits(uint32_t value, uint8_t count)
{
    while (count--) { swd_write_bit((uint8_t)(value & 1u)); value >>= 1; }
}

static uint32_t swd_read_bits(uint8_t count)
{
    uint32_t value = 0;
    for (uint8_t i = 0; i < count; ++i) if (swd_read_bit()) value |= (1u << i);
    return value;
}

static uint8_t parity32(uint32_t v)
{
    v ^= v >> 16; v ^= v >> 8; v ^= v >> 4; v ^= v >> 2; v ^= v >> 1;
    return (uint8_t)(v & 1u);
}

void SWD_SPI_Init(void)
{
    GPIO_InitTypeDef g = {0};
    SPI_InitTypeDef s = {0};
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC |
                           RCC_APB2Periph_AFIO | RCC_APB2Periph_SPI1, ENABLE);
    g.GPIO_Speed = GPIO_Speed_50MHz;
    g.GPIO_Mode = GPIO_Mode_Out_PP;
    g.GPIO_Pin = SWD_SWCLK_PIN | SWD_SWDIO_OUT | SWD_OE_PIN | G4_NRST_PIN;
    GPIO_Init(GPIOA, &g);
    g.GPIO_Pin = SWD_SWDIO_IN; g.GPIO_Mode = GPIO_Mode_IN_FLOATING; GPIO_Init(GPIOA, &g);
    g.GPIO_Pin = WIFI_BOOT_PIN | WIFI_NRST_PIN; g.GPIO_Mode = GPIO_Mode_Out_PP; GPIO_Init(GPIOC, &g);
    GPIO_ResetBits(GPIOA, SWD_SWCLK_PIN | SWD_SWDIO_OUT | SWD_OE_PIN | G4_NRST_PIN);
    GPIO_ResetBits(GPIOC, WIFI_BOOT_PIN | WIFI_NRST_PIN);
    swd_output(0);

    SPI_StructInit(&s);
    s.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    s.SPI_Mode = SPI_Mode_Master;
    s.SPI_DataSize = SPI_DataSize_8b;
    s.SPI_CPOL = SPI_CPOL_Low;
    s.SPI_CPHA = SPI_CPHA_1Edge;
    s.SPI_NSS = SPI_NSS_Soft;
    s.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128;
    s.SPI_FirstBit = SPI_FirstBit_LSB;
    SPI_Init(SPI1, &s);
    SPI_Cmd(SPI1, ENABLE);
    SWD_SPI_SetClock(1000000u);
}

void SWD_SPI_SetClock(uint32_t hz)
{
    if (hz < 10000u) hz = 10000u;
    if (hz > 4000000u) hz = 4000000u;
    swd_clock_hz = hz;
    swd_delay_loops = (SystemCoreClock / (hz * 8u));
    if (swd_delay_loops < 2u) swd_delay_loops = 2u;
}

uint32_t SWD_SPI_GetClock(void) { return swd_clock_hz; }

void SWD_SPI_ResetSequence(void)
{
    swd_output(1); GPIO_SetBits(SWD_SWDIO_PORT, SWD_SWDIO_OUT);
    for (uint8_t i = 0; i < 64; ++i) { swd_clk(1); swd_clk(0); }
    swd_output(0);
}

void SWD_SPI_SwjSequence(const uint8_t *data, uint16_t bit_count)
{
    swd_output(1);
    for (uint16_t i = 0; i < bit_count; ++i) swd_write_bit((uint8_t)((data[i >> 3] >> (i & 7)) & 1u));
    swd_output(0);
}

uint8_t SWD_SPI_Connect(void)
{
    SWD_SPI_ResetSequence();
    { static const uint8_t seq[2] = {0x9E,0xE7}; SWD_SPI_SwjSequence(seq, 16); }
    SWD_SPI_ResetSequence();
    return 1;
}

uint8_t SWD_SPI_Transfer(uint8_t request, uint32_t *data)
{
    uint8_t ack, read = (request >> 1) & 1u;
    uint32_t value = 0;
    uint8_t hbits = (uint8_t)(request & 0x0Fu);
    uint8_t header = (uint8_t)(0x81u | (hbits << 1) | (parity32(hbits) << 5));
    swd_output(1);
    /* SWD wire header: start, APnDP, RnW, A2, A3, parity, stop, park. */
    swd_write_bits(header, 8);
    swd_output(0);
    (void)swd_read_bit(); /* host-to-target turnaround */
    ack = (uint8_t)swd_read_bits(3);
    if (ack != 1u) return ack;
    if (read) {
        value = swd_read_bits(32);
        if (swd_read_bits(1) != parity32(value)) return 7u;
        swd_output(1);
        swd_write_bit(1); /* target-to-host turnaround */
        if (data) *data = value;
    } else {
        swd_output(1);
        value = data ? *data : 0;
        swd_write_bit(1); /* target-to-host turnaround */
        swd_write_bits(value, 32); swd_write_bit(parity32(value));
        swd_output(0);
    }
    return 1u;
}

void SWD_SetTargetReset(uint8_t asserted) { if (asserted) GPIO_SetBits(G4_NRST_PORT,G4_NRST_PIN); else GPIO_ResetBits(G4_NRST_PORT,G4_NRST_PIN); if (asserted) control_state |= UF4_CTRL_G4_NRST; else control_state &= (uint8_t)~UF4_CTRL_G4_NRST; }
void SWD_SetWifiBoot(uint8_t asserted) { if (asserted) GPIO_SetBits(WIFI_BOOT_PORT,WIFI_BOOT_PIN); else GPIO_ResetBits(WIFI_BOOT_PORT,WIFI_BOOT_PIN); if (asserted) control_state |= UF4_CTRL_WIFI_BOOT; else control_state &= (uint8_t)~UF4_CTRL_WIFI_BOOT; }
void SWD_SetWifiReset(uint8_t asserted) { if (asserted) GPIO_SetBits(WIFI_NRST_PORT,WIFI_NRST_PIN); else GPIO_ResetBits(WIFI_NRST_PORT,WIFI_NRST_PIN); if (asserted) control_state |= UF4_CTRL_WIFI_NRST; else control_state &= (uint8_t)~UF4_CTRL_WIFI_NRST; }
uint8_t SWD_GetControlState(void) { return control_state; }
