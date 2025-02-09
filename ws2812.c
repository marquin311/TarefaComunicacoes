#include "hardware/pio.h"
#include "inc/ws2812.pio.h"  // Esse arquivo deve ser gerado pelo pioasm a partir do seu código PIO

// Declaramos variáveis globais para armazenar a instância do PIO e a máquina de estado
PIO ws2812_pio = pio0;
uint ws2812_sm = 0;

// Função de inicialização do WS2812 usando PIO
// 'pin' é o pino de saída onde os LEDs WS2812 estão conectados
// 'freq' é a frequência de dados, geralmente 800000 (800 kHz) para WS2812B
// 'rgb_order' indica se a ordem das cores é RGB ou GRB (no exemplo usamos false para GRB padrão)
void ws2812_init(uint pin) {
    // Adiciona o programa WS2812 ao PIO
    uint offset = pio_add_program(ws2812_pio, &ws2812_program);
    // Inicializa o programa na máquina de estado disponível
    ws2812_program_init(ws2812_pio, ws2812_sm, offset, pin, 800000, false);
}

// Função que envia um pixel (cor) para a fita WS2812.
// 'color' é um valor de 32 bits que contém a cor no formato GRB (por exemplo, 0x00FF0000 para vermelho se o WS2812 usar GRB).
void ws2812_put_pixel(uint32_t color) {
    pio_sm_put_blocking(ws2812_pio, ws2812_sm, color);
}
