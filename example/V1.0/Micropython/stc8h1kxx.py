import time
import struct

class STC8H1KXX:
    # I2C 设备地址
    I2C_ADDR = 0x2F

    # 寄存器地址
    REG_BATTERY  = 0x00
    REG_GET_GPIO = 0x10
    REG_SET_GPIO = 0x18
    REG_SET_PWM  = 0x20

    # 输入 GPIO 枚举 (对应 EM_STC8_GPIO_IN)
    GPIO_IN_SW_SPI_UART = 0

    # 输出 GPIO 枚举 (对应 EM_STC8_GPIO_OUT)
    GPIO_OUT_TP_RST        = 0
    GPIO_OUT_CSI_RST       = 1
    GPIO_OUT_AUDIO_SD      = 2
    GPIO_OUT_LCD_BL_POWER  = 3

    # PWM 枚举 (对应 EM_STC8_PWM)
    PWM_LCD_BL_EN = 0

    # 电池状态字典映射
    BAT_STATE_MAP = {
        0: "IDLE (空闲)",
        1: "CHARGING (充电中)",
        2: "FULLY_CHARGED (已充满)",
        3: "NO_CHARGE (未充电)",
        4: "ERROR (错误)"
    }

    # LED 状态字典映射
    LED_STATE_MAP = {
        0: "IDLE (空闲)",
        1: "CHARGING (红灯)",
        2: "FULLY_CHARGED (绿灯)",
        3: "NO_CHARGE (不充电)",
        4: "LOW_POWER (低压 0.5Hz闪烁红灯)"
    }

    def __init__(self, i2c, addr=I2C_ADDR):
        """
        初始化 STC8 驱动
        :param i2c: machine.I2C 实例
        :param addr: STC8 的 I2C 从机地址
        """
        self.i2c = i2c
        self.addr = addr

    def get_battery_info(self):
        """
        获取电池信息
        :return: 包含电池数据的字典
        """
        # C 代码中使用了循环逐字节读取，且 sizeof(Battery_info_t) 通常为 12 字节（带 padding）
        # 我们严格模拟 C 代码的读取方式，读取 12 个字节
        raw_data = bytearray(12)
        for i in range(12):
            # 逐个寄存器读取 1 byte
            val = self.i2c.read_mem(self.REG_BATTERY + i, 1)
            raw_data[i] = val[0]
            
        # 前 11 个字节是实际数据：u32, u32, u8, u8, u8
        # 使用小端模式 (<) 解析：I(4字节无符号整数), B(1字节无符号整数)
        adc_voltage, bat_voltage, bat_level, bat_state, led_state = struct.unpack('<IIBBB', raw_data[:11])
        
        return {
            "adc_voltage_mv": adc_voltage,
            "bat_voltage_mv": bat_voltage,
            "bat_level_pct": bat_level,
            "bat_state": bat_state,
            "bat_state_desc": self.BAT_STATE_MAP.get(bat_state, "UNKNOWN"),
            "led_state": led_state,
            "led_state_desc": self.LED_STATE_MAP.get(led_state, "UNKNOWN")
        }

    def get_gpio_level(self, gpio_num):
        """
        获取 STC8 的输入引脚电平
        :param gpio_num: GPIO 编号 (例如 STC8H1KXX.GPIO_IN_SW_SPI_UART)
        :return: 0 或 1
        """
        if gpio_num > 0: # 依据枚举，当前最大只有 0
            raise ValueError(f"Invalid GPIO IN num: {gpio_num}")
            
        val = self.i2c.read_mem(self.REG_GET_GPIO + gpio_num, 1)
        return val[0]

    def set_gpio_level(self, gpio_num, level):
        """
        设置 STC8 的输出引脚电平
        :param gpio_num: GPIO 编号
        :param level: 0(低电平) 或 1(高电平)
        """
        if gpio_num > 3: # 依据枚举，当前最大只有 3
            raise ValueError(f"Invalid GPIO OUT num: {gpio_num}")
            
        # 限制 level 只能是 0 或 1
        level = 1 if level > 0 else 0
        self.i2c.write_mem(self.REG_SET_GPIO + gpio_num, bytes([level]))

    def set_pwm_duty(self, pwm_num, duty):
        """
        设置 PWM 占空比
        :param pwm_num: PWM 通道编号 (例如 STC8H1KXX.PWM_LCD_BL_EN)
        :param duty: 0-100 (%)
        """
        if pwm_num > 0: # 依据枚举，当前最大只有 0
            raise ValueError(f"Invalid PWM num: {pwm_num}")
        
        if not 0 <= duty <= 100:
            raise ValueError("Duty cycle must be between 0 and 100")
            
        self.i2c.write_mem(self.REG_SET_PWM + pwm_num, bytes([duty]))