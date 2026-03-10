#ifndef SRC_COMMON_BOARD_TS004X_H_
#define SRC_COMMON_BOARD_TS004X_H_

/**************************** Configure UART ***************************************/
#if UART_PRINTF_MODE
#define DEBUG_INFO_TX_PIN       GPIO_PB1
#define DEBUG_BAUDRATE          115200
#endif /* UART_PRINTF_MODE */

/********************* Configure External Battery GPIO ******************************/
#define VOLTAGE_DETECT_PIN      GPIO_PC5

/***************************** Configure LED  ***************************************/

#define LED_ON                  1
#define LED_OFF                 0
#define LED1                    GPIO_PD7
#define PD7_FUNC                AS_GPIO
#define PD7_OUTPUT_ENABLE       ON
#define PD7_INPUT_ENABLE        OFF
#define PD7_DATA_OUT            LED_OFF

/************************* Configure BUTTON GPIO ***************************************/
#define MAX_BUTTON_NUM      6

#define BUTTON1_GPIO            GPIO_PA0
#define PA0_INPUT_ENABLE        ON
#define PA0_OUTPUT_ENABLE       OFF
#define PA0_FUNC                AS_GPIO
#define PULL_WAKEUP_SRC_PA0     PM_PIN_PULLUP_1M

#define BUTTON2_GPIO            GPIO_PB5
#define PB5_INPUT_ENABLE        ON
#define PB5_OUTPUT_ENABLE       OFF
#define PB5_FUNC                AS_GPIO
#define PULL_WAKEUP_SRC_PB5     PM_PIN_PULLUP_1M

#define BUTTON3_GPIO            GPIO_PC2
#define PC2_INPUT_ENABLE        ON
#define PC2_OUTPUT_ENABLE       OFF
#define PC2_FUNC                AS_GPIO
#define PULL_WAKEUP_SRC_PC2     PM_PIN_PULLUP_1M

#define BUTTON4_GPIO            GPIO_PC3
#define PC3_INPUT_ENABLE        ON
#define PC3_OUTPUT_ENABLE       OFF
#define PC3_FUNC                AS_GPIO
#define PULL_WAKEUP_SRC_PC3     PM_PIN_PULLUP_1M

/* for 6 key's switch */
/* key num 4 */

#define BUTTON4_6_GPIO          GPIO_PC4
#define PC4_INPUT_ENABLE        ON
#define PC4_OUTPUT_ENABLE       OFF
#define PC4_FUNC                AS_GPIO
#define PULL_WAKEUP_SRC_PC4     PM_PIN_PULLUP_1M

/* key num 5 */
#define BUTTON5_GPIO            GPIO_PB4
#define PB4_INPUT_ENABLE        ON
#define PB4_OUTPUT_ENABLE       OFF
#define PB4_FUNC                AS_GPIO
#define PULL_WAKEUP_SRC_PB4     PM_PIN_PULLUP_1M

/* key num 6 */
#define BUTTON6_GPIO            GPIO_PC0
#define PC0_INPUT_ENABLE        ON
#define PC0_OUTPUT_ENABLE       OFF
#define PC0_FUNC                AS_GPIO
#define PULL_WAKEUP_SRC_PC0     PM_PIN_PULLUP_1M


#endif /* SRC_COMMON_BOARD_TS004X_H_ */
