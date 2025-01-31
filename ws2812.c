/*
 * Por: Wilton Lacerda Silva
 * 
 * Este código é uma adaptação do código original do SDK Pico
 * para a utilização da matriz de LEDs WS2812 do BitDogLab.
 * 
 * A seleção de LEDs acesos é feita por meio de um buffer de LEDs, onde
 * cada posição do buffer representa um LED da matriz 5x5.
 * 
 * Original em:
 * https://github.com/raspberrypi/pico-examples/tree/master/pio/ws2812
 */

#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"
#include "hardware/timer.h"

#define IS_RGBW false
#define NUM_PIXELS 25
#define WS2812_PIN 7
#define tempo 100
#define RED 13
#define BUTTON_A 5
#define BUTTON_B 6
#define FRAME_NUMBER 10 //De 0 a 9

// Variável global para armazenar a cor (Entre 0 e 255 para intensidade)
uint8_t led_r = 0; // Intensidade do vermelho
uint8_t led_g = 0; // Intensidade do verde
uint8_t led_b = 20; // Intensidade do azul

static volatile uint32_t last_time = 0;//Usado para o debounce
unsigned short int escolhido=0;//Determina o numero que sera exibido na matriz de leds

// Buffer para armazenar quais LEDs estão ligados matriz 5x5
 bool led_buffer[FRAME_NUMBER][NUM_PIXELS] ={
    {0,1,1,1,0,
     0,1,0,1,0,
     0,1,0,1,0,
     0,1,0,1,0,
     0,1,1,1,0
    },
    {0,0,1,0,0,
     0,1,1,0,0,
     0,0,1,0,0,
     0,0,1,0,0,
     0,0,1,0,0
    },
    {0,1,1,1,0,
     0,0,0,1,0,
     0,1,1,1,0,
     0,1,0,0,0,
     0,1,1,1,0
    },
    {0,1,1,1,0,
     0,0,0,1,0,
     0,1,1,1,0,
     0,0,0,1,0,
     0,1,1,1,0
    },
    {0,1,0,1,0,
     0,1,0,1,0,
     0,1,1,1,0,
     0,0,0,1,0,
     0,0,0,1,0
    },
    {0,1,1,1,0,
     0,1,0,0,0,
     0,1,1,1,0,
     0,0,0,1,0,
     0,1,1,1,0
    },
    {0,1,1,1,0,
     0,1,0,0,0,
     0,1,1,1,0,
     0,1,0,1,0,
     0,1,1,1,0
    },
    {0,1,1,1,0,
     0,0,0,1,0,
     0,0,0,1,0,
     0,0,0,1,0,
     0,0,0,1,0
    },
    {0,1,1,1,0,
     0,1,0,1,0,
     0,1,1,1,0,
     0,1,0,1,0,
     0,1,1,1,0
    },
    {0,1,1,1,0,
     0,1,0,1,0,
     0,1,1,1,0,
     0,0,0,1,0,
     0,1,1,1,0
    }
};

//Corrige o índice para que o LED certo seja acendido
uint correcao_index(int index){
     //Caso esteja numa linha ímpar
     if((index>=5 && index<10) || (index>=15 && index<20))
     return index<10 ? index+10:index-10;
     else
     return NUM_PIXELS-index-1;
    }

//Aparentemente essa função só funciona com vetor de 1 dimensão
static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

//Liga os LEDs na matriz que estiverem com '1'
void set_one_led(uint8_t r, uint8_t g, uint8_t b)
{
    // Define a cor com base nos parâmetros fornecidos
    uint32_t color = urgb_u32(r, g, b);

    // Define todos os LEDs com a cor especificada
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        if (led_buffer[escolhido][correcao_index(i)])
        {
            put_pixel(color); // Liga o LED com um no buffer
        }
        else
        {
            put_pixel(0);  // Desliga os LEDs com zero no buffer
        }
    }
}

//Rotina de interrupção, o botão A decrementa e o botão B incrementa
void interrupt(uint gpio, uint32_t events)
{
    // Obtem o tempo atual em microssegundos
    uint32_t current_time = to_us_since_boot(get_absolute_time());
    // Verifica se passou tempo suficiente desde o último evento
    if (current_time - last_time > 250000) // 250 ms de debouncing
    {
      last_time=current_time;
      if(gpio==BUTTON_B){//Verifica se foi o botao B (incremento) ou o botao A (decremento)
        escolhido=(escolhido+1)%FRAME_NUMBER;
      }else{
        escolhido--;
        if(escolhido>FRAME_NUMBER) escolhido=FRAME_NUMBER-1;
      }
     printf("Valor escolhido = %hu\n",escolhido);
     set_one_led(led_r, led_g, led_b);
    }
}

int main()
{
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);

    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    stdio_init_all();

    gpio_init(RED);              // Inicializa o pino do LED
    gpio_set_dir(RED, GPIO_OUT); // Configura o pino como saida
    
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN); // Configura o pino como entrada
    gpio_pull_up(BUTTON_A);          // Habilita o pull-up interno

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN); // Configura o pino como entrada
    gpio_pull_up(BUTTON_B);

    set_one_led(led_r, led_g, led_b);

    //Só é possivel ter uma unica função de interrupção
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &interrupt);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &interrupt);

    //Faz o LED vermelho piscar 5 vezes por segundo
    while (1)
    {   
     gpio_put(RED,0);
     sleep_ms(tempo);
     gpio_put(RED,1);
     sleep_ms(tempo);
    }

    return 0; //Teoricamente, o programa nunca chegará aqui
}
