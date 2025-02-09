#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "pico/time.h"
#include "inc/ssd1306.h"
#include "inc/fonte.h"
#include "inc/ws2812.pio.h"  // Biblioteca para WS2812 que fornece ws2812_init() e ws2812_put_pixel()

#define I2C_PORT    i2c1
#define I2C_SDA     14
#define I2C_SCL     15
#define OLED_ADDR   0x3C

#define LED_RED     13
#define LED_GREEN   11
#define LED_BLUE    12

#define BTN_A       5
#define BTN_B       6

#define WS2812_PIN  7  // Matriz de LEDs

// Declaração do display
ssd1306_t ssd;

// Estados dos LEDs RGB
volatile bool led_green_state = false;
volatile bool led_blue_state  = false;

// Variáveis de debounce (volatile)
volatile uint32_t last_press_A = 0;
volatile uint32_t last_press_B = 0;

volatile uint8_t digit_received = 0; // para armazenar o dígito recebido

// Prototipação das funções
void setup();
void handle_button_a();
void handle_button_b();
void debounce_button(uint gpio, volatile uint32_t *last_press, void (*callback)());
static void btn_a_callback(uint gpio, uint32_t events);
static void btn_b_callback(uint gpio, uint32_t events);

void display_char(char c);
void display_number(uint8_t num);
void exibir_numero_na_matriz(int numero);  
void desenhar_matriz(const uint8_t matriz[5][5]); 

// Matrizes 5x5 para os dígitos 0 a 9
const uint8_t numeros[10][5][5] = {
    { // Número 0
        {1, 1, 1, 1, 1},
        {1, 0, 0, 0, 1},
        {1, 0, 0, 0, 1},
        {1, 0, 0, 0, 1},
        {1, 1, 1, 1, 1}
    },
    { // Número 1
        {0, 0, 1, 0, 0},
        {0, 1, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 1, 1, 1, 0}
    },
    { // Número 2
        {1, 1, 1, 1, 1},
        {0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0},
        {1, 1, 1, 1, 1}
    },
    { // Número 3
        {1, 1, 1, 1, 1},
        {0, 0, 0, 0, 1},
        {0, 1, 1, 1, 1},
        {0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1}
    },
    { // Número 4
        {0, 1, 0, 0, 1},
        {1, 0, 0, 1, 0},
        {1, 1, 1, 1, 1},
        {0, 0, 0, 1, 0},
        {0, 1, 0, 0, 0}
    },
    { // Número 5
        {1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0},
        {1, 1, 1, 1, 1},
        {0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1}
    },
    { // Número 6
        {1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0},
        {1, 1, 1, 1, 1},
        {1, 0, 0, 0, 1},
        {1, 1, 1, 1, 1}
    },
    { // Número 7
        {1, 1, 1, 1, 1},
        {0, 0, 0, 0, 1},
        {0, 1, 0, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 0, 1, 0}
    },
    { // Número 8
        {1, 1, 1, 1, 1},
        {1, 0, 0, 0, 1},
        {1, 1, 1, 1, 1},
        {1, 0, 0, 0, 1},
        {1, 1, 1, 1, 1}
    },
    { // Número 9
        {1, 1, 1, 1, 1},
        {1, 0, 0, 0, 1},
        {1, 1, 1, 1, 1},
        {0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1}
    }
};

int main() {
    setup();
    char input;
    while (true) {
        if (stdio_usb_connected()) {  // Garante que o USB está conectado
            int ch = getchar_timeout_us(0); 
            if (ch != PICO_ERROR_TIMEOUT) {
                input = (char)ch;
                if (input >= '0' && input <= '9') {
                    display_number(input - '0');
                    exibir_numero_na_matriz(input - '0');  
                } else {
                    display_char(input);
                }
            }
        }
        sleep_ms(10);
    }
    return 0;
}

static void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == BTN_A) {
        btn_a_callback(gpio, events);
    } else if (gpio == BTN_B) {
        btn_b_callback(gpio, events);
    }
}

void setup() {
    stdio_init_all();  // Inicializa a comunicação USB CDC (para Serial Monitor)

    // Inicializa I2C e o display SSD1306
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, OLED_ADDR, I2C_PORT);
    ssd1306_config(&ssd);  
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // Configuração dos LEDs RGB
    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);
    gpio_put(LED_RED, 0);

    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_put(LED_GREEN, 0);

    gpio_init(LED_BLUE);
    gpio_set_dir(LED_BLUE, GPIO_OUT);
    gpio_put(LED_BLUE, 0);

    // Inicializa a matriz WS2812 
    ws2812_init(WS2812_PIN);

    // Configuração dos botões com pull-up e interrupções
    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A); 


    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);
    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true, gpio_callback);
    gpio_set_irq_enabled_with_callback(BTN_B, GPIO_IRQ_EDGE_FALL, true, gpio_callback);
}



// Funções de callback dos botões 
static void btn_a_callback(uint gpio, uint32_t events) {
    printf("BTN_A IRQ acionado\n");
    debounce_button(BTN_A, &last_press_A, handle_button_a);
}

static void btn_b_callback(uint gpio, uint32_t events) {
    printf("BTN_B IRQ acionado\n");
    debounce_button(BTN_B, &last_press_B, handle_button_b);
}


void debounce_button(uint gpio, volatile uint32_t *last_press, void (*callback)()) {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - *last_press > 200) {  // 200 ms de debounce
        *last_press = now;
        if (gpio_get(gpio) == 0) {
            callback();
        }
    }
}

void handle_button_a() {
    led_green_state = !led_green_state;
    gpio_put(LED_GREEN, led_green_state);
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, led_green_state ? "LED Verde ON" : "LED Verde OFF", 8, 10);
    ssd1306_send_data(&ssd);
    printf("%s\n", led_green_state ? "Botao B: LED Verde Ligado" : "Botao B: LED Verde Desligado");
}

void handle_button_b() {
    led_blue_state = !led_blue_state;
    gpio_put(LED_BLUE, led_blue_state);
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, led_blue_state ? "LED Azul ON" : "LED Azul OFF", 8, 10);
    ssd1306_send_data(&ssd);
    printf("%s\n", led_blue_state ? "Botao B: LED Azul Ligado" : "Botao B: LED Azul Desligado");
}

void display_char(char c) {
    ssd1306_fill(&ssd, false);
    ssd1306_draw_char(&ssd, c, 32, 20);
    ssd1306_send_data(&ssd);
}

void display_number(uint8_t num) {
    ssd1306_fill(&ssd, false);
    char buffer[2] = {num + '0', '\0'};
    ssd1306_draw_string(&ssd, buffer, 40, 25);
    ssd1306_send_data(&ssd);
}

// --- Funções para exibir número na matriz WS2812 ---

// Recebe um número (0-9) e exibe o padrão correspondente na matriz
void exibir_numero_na_matriz(int numero) {
    if (numero < 0 || numero > 9) return;
    desenhar_matriz(numeros[numero]);
}

// Percorre a matriz 5x5 e envia os pixels para a WS2812
void desenhar_matriz(const uint8_t matriz[5][5]) {
    for (int y = 4; y >= 0; y--) {
        for (int x = 0; x < 5; x++) {
            if (matriz[y][x])
                ws2812_put_pixel(0xFFFFFF); // Pixel ligado (branco)
            else
                ws2812_put_pixel(0x000000); // Pixel apagado
        }
    }
}
