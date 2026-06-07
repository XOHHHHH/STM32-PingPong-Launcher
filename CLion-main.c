/* USER CODE BEGIN Header */
/**
  * 乒乓球发球机终极纯净版 (含舵机平滑阻尼算法)
  * ----------------------------------------------------
  * 【接线说明】务必严格对齐！
  * 舵机 Pitch (仰角) -> PA7  (TIM3_CH2)
  * 舵机 Yaw   (偏航) -> PA6  (TIM3_CH1)
  * 电机 左轮PWM     -> PD12 (TIM4_CH1)
  * 电机 右轮PWM     -> PD13 (TIM4_CH2)
  * 电机 转向IN1~IN4  -> PD0, PD1, PD2, PD3
  * 控制 按键PE0~PE4  -> PE0~PE4 (另一端必须接GND)
  * ----------------------------------------------------
  */
/* USER CODE END Header */

#include "main.h"
#include <stdint.h>
#include <stdlib.h>

/* --- 全局变量声明 --- */
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;

/* --- 函数原型声明 --- */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM4_Init(void);
void L298N_Set_Speed(int16_t speed_left, int16_t speed_right);
void Servo_SetPitch(uint8_t angle);
void Servo_SetYaw(uint8_t angle);

/* --- 底层驱动函数 --- */
void L298N_Set_Speed(int16_t speed_left, int16_t speed_right) {
    int16_t abs_left = abs(speed_left);
    int16_t abs_right = abs(speed_right);
    uint8_t dir_left = speed_left >= 0 ? 1 : 0;
    uint8_t dir_right = speed_right >= 0 ? 1 : 0;

    if (abs_left > 1000) abs_left = 1000;
    if (abs_right > 1000) abs_right = 1000;

    if (abs_left == 0 && abs_right == 0) {
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, dir_left ? GPIO_PIN_RESET : GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_1, dir_left ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, dir_right ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, dir_right ? GPIO_PIN_RESET : GPIO_PIN_SET);
    }
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, abs_left);
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, abs_right);
}

void Servo_SetPitch(uint8_t angle) {
    if(angle > 180) angle = 180;
    uint32_t pulse = 500 + (angle * 2000) / 180;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pulse);
}

void Servo_SetYaw(uint8_t angle) {
    if(angle > 180) angle = 180;
    uint32_t pulse = 500 + (angle * 2000) / 180;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse);
}

/* --- 主函数 --- */
int main(void)
{
    /* 1. 系统初始化 */
    HAL_Init();
    SystemClock_Config(); // 注入 168MHz 灵魂时钟

    /* 2. 外设初始化 */
    MX_GPIO_Init();
    MX_TIM3_Init();
    MX_TIM4_Init();

    /* 3. 启动 PWM 输出 */
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);

    /* 4. 初始状态设定 */
    const uint8_t PITCH_ONE_BOUNCE = 115; // 一跳球（仰角）
    const uint8_t PITCH_TWO_BOUNCE = 70;  // 两跳球（俯角）
    const uint8_t PITCH_CENTER = 90;      // 回正（水平）

    const uint8_t YAW_CENTER = 90;
    const uint8_t YAW_LEFT   = 120;
    const uint8_t YAW_RIGHT  = 60;

    // 初始复位到一跳球、居中
    Servo_SetPitch(PITCH_ONE_BOUNCE);
    Servo_SetYaw(YAW_CENTER);
    L298N_Set_Speed(0, 0);

    /* 5. 控制变量 */
    uint8_t current_gear = 0;
    int16_t target_speed = 0;
    int16_t actual_speed = 0;
    uint8_t spin_mode = 0;
    uint8_t pitch_step = 0;
    uint8_t yaw_step = 0;
    const int16_t GEAR_SPEEDS[6] = {0, 300, 350, 400, 450, 500};

    // --- 【新增】舵机平滑阻尼控制变量 ---
    uint8_t target_pitch = PITCH_ONE_BOUNCE;
    float actual_pitch = PITCH_ONE_BOUNCE;
    uint8_t target_yaw = YAW_CENTER;
    float actual_yaw = YAW_CENTER;
    const float SERVO_SPEED = 0.5f; // 舵机旋转步长(数值越小越慢，0.5f表示极具机械阻尼感)

    /* 6. 主循环 */
    while (1)
    {
        // --- 档位与旋球按键逻辑 ---
        if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_0) == GPIO_PIN_RESET) {
            HAL_Delay(20);
            if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_0) == GPIO_PIN_RESET) {
                if (current_gear < 5) current_gear++;
                while(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_0) == GPIO_PIN_RESET) HAL_Delay(10);
            }
        }
        if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_1) == GPIO_PIN_RESET) {
            HAL_Delay(20);
            if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_1) == GPIO_PIN_RESET) {
                if (current_gear > 0) current_gear--;
                while(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_1) == GPIO_PIN_RESET) HAL_Delay(10);
            }
        }
        if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_2) == GPIO_PIN_RESET) {
            HAL_Delay(20);
            if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_2) == GPIO_PIN_RESET) {
                spin_mode = (spin_mode + 1) % 3;
                while(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_2) == GPIO_PIN_RESET) HAL_Delay(10);
            }
        }

        // --- PE3 俯仰角控制 (修改为设定目标角度) ---
        if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_3) == GPIO_PIN_RESET) {
            HAL_Delay(20); 
            if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_3) == GPIO_PIN_RESET) {
                pitch_step = (pitch_step + 1) % 4;

                if (pitch_step == 0) {
                    target_pitch = PITCH_ONE_BOUNCE; 
                } else if (pitch_step == 1) {
                    target_pitch = PITCH_CENTER;     
                } else if (pitch_step == 2) {
                    target_pitch = PITCH_TWO_BOUNCE; 
                } else if (pitch_step == 3) {
                    target_pitch = PITCH_CENTER;     
                }

                while(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_3) == GPIO_PIN_RESET) HAL_Delay(10);
            }
        }

        // --- PE4 偏航角控制 (修改为设定目标角度) ---
        if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_4) == GPIO_PIN_RESET) {
            HAL_Delay(20);
            if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_4) == GPIO_PIN_RESET) {
                yaw_step = (yaw_step + 1) % 4;
                
                if (yaw_step == 0) target_yaw = YAW_CENTER;
                else if (yaw_step == 1) target_yaw = YAW_LEFT;
                else if (yaw_step == 2) target_yaw = YAW_CENTER;
                else if (yaw_step == 3) target_yaw = YAW_RIGHT;
                
                while(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_4) == GPIO_PIN_RESET) HAL_Delay(10);
            }
        }

        // --- 【新增】舵机平滑过渡算法执行层 ---
        if (actual_pitch < target_pitch) {
            actual_pitch += SERVO_SPEED;
            if (actual_pitch > target_pitch) actual_pitch = target_pitch;
            Servo_SetPitch((uint8_t)actual_pitch);
        } else if (actual_pitch > target_pitch) {
            actual_pitch -= SERVO_SPEED;
            if (actual_pitch < target_pitch) actual_pitch = target_pitch;
            Servo_SetPitch((uint8_t)actual_pitch);
        }

        if (actual_yaw < target_yaw) {
            actual_yaw += SERVO_SPEED;
            if (actual_yaw > target_yaw) actual_yaw = target_yaw;
            Servo_SetYaw((uint8_t)actual_yaw);
        } else if (actual_yaw > target_yaw) {
            actual_yaw -= SERVO_SPEED;
            if (actual_yaw < target_yaw) actual_yaw = target_yaw;
            Servo_SetYaw((uint8_t)actual_yaw);
        }

        // --- 电机平滑控制与侧旋 ---
        target_speed = GEAR_SPEEDS[current_gear];
        uint8_t is_kick_starting = 0;

        if (actual_speed < target_speed) {
            if (actual_speed == 0) { is_kick_starting = 1; actual_speed = 300; }
            else actual_speed += 2;
            if (actual_speed > target_speed) actual_speed = target_speed;
        } else if (actual_speed > target_speed) {
            actual_speed -= 4;
            if (actual_speed < target_speed) actual_speed = target_speed;
        }

        int16_t out_left = actual_speed;
        int16_t out_right = actual_speed;

        if (actual_speed > 0) {
            if (spin_mode == 1) { out_right = actual_speed; out_left = -(actual_speed / 2); }
            else if (spin_mode == 2) { out_left = actual_speed; out_right = -(actual_speed / 2); }
        }

        // 起步冲刺克服静摩擦
        if (is_kick_starting) {
            int16_t kick_left = (out_left > 0) ? 800 : ((out_left < 0) ? -800 : 0);
            int16_t kick_right = (out_right > 0) ? 800 : ((out_right < 0) ? -800 : 0);
            L298N_Set_Speed(kick_left, kick_right);
            HAL_Delay(50);
        }

        L298N_Set_Speed(out_left, out_right);
        HAL_Delay(5); // 控制整个系统的刷新频率，与平滑步长共同决定视觉速度
    }
}

/* --- 核心时钟配置 (修复单片机假死的关键) --- */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    // 配置内部 HSI 时钟 (16MHz) 并通过 PLL 倍频到 168MHz
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 168;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        __disable_irq(); while (1);
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4; // 84MHz 定时器时钟
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        __disable_irq(); while (1);
    }
}

/* --- TIM3 初始化: 舵机专用 (50Hz) --- */
static void MX_TIM3_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 83;      // 84MHz / 84 = 1MHz
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 19999;      // 20ms = 50Hz
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim3);

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 1500;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1); // PA6 (Yaw)
    HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2); // PA7 (Pitch)

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/* --- TIM4 初始化: 电机专用 (1kHz) --- */
static void MX_TIM4_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};
    __HAL_RCC_TIM4_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 83;     // 84MHz / 84 = 1MHz
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 999;       // 1ms = 1kHz
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim4);

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_1); // PD12 (Left)
    HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_2); // PD13 (Right)

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

/* --- GPIO 初始化: 按键与电机方向 --- */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    // 1. 电机方向控制脚 (PD0 ~ PD3)
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // 2. 按键输入脚 (PE0 ~ PE4)
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
}

/* 异常处理函数 */
void Error_Handler(void) {
    __disable_irq();
    while (1) {}
}
