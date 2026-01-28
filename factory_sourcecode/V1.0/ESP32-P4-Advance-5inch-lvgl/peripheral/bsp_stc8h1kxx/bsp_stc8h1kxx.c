/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_stc8h1kxx.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#ifdef CONFIG_BSP_STC8H1KXX_ENABLED

static i2c_master_dev_handle_t stc8_handle = NULL;
#endif
/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/
#ifdef CONFIG_BSP_STC8H1KXX_ENABLED

esp_err_t stc8_i2c_init()
{
    stc8_handle = i2c_dev_register(STC8_I2C_SLAVE_DEV_ADDR);
    if (stc8_handle == NULL)
    {
        STC8H1KXX_ERROR("stc8 i2c register fail");
        return ESP_FAIL;
    }
    return ESP_OK;
}

#ifdef CONFIG_BSP_STC8H1KXX_BATTERY_ENABLED

esp_err_t stc8_battery_info_get(Battery_info_t *bat_info)
{
    esp_err_t err = ESP_FAIL;
    // err = i2c_write_read(stc8_handle, STC8_REG_ADDR_BATTERY, (uint8_t*)bat_info, sizeof(Battery_info_t), 1000);
    // if (ESP_OK != err)
    // {
    //     STC8H1KXX_ERROR("stc8 read battery info fail");
    //     return err;
    // }
    for (int i = 0; i < sizeof(Battery_info_t); i++)
    {
        err = i2c_read_reg(stc8_handle, STC8_REG_ADDR_BATTERY+i, (uint8_t*)bat_info+i, 1);
        if (ESP_OK != err)
        {
            STC8H1KXX_ERROR("stc8 read battery info fail");
            return err;
        }
    }
    return err;
}

#endif

#ifdef CONFIG_BSP_STC8H1KXX_GPIO_ENABLED

esp_err_t stc8_gpio_get_level(int gpio_num, uint8_t* level)
{
    esp_err_t err;
    if (STC8_GPIO_IN_MAX <= gpio_num) {
        STC8H1KXX_ERROR("stc8 can't get gpio=%d", gpio_num);
        return ESP_FAIL;
    }    
    err = i2c_read_reg(stc8_handle, STC8_REG_ADDR_GET_GPIO + gpio_num, level, 1);
    if (ESP_OK != err)
    {
        STC8H1KXX_ERROR("stc8 get gpio=%d fail", gpio_num);
        return err;
    }
    return err;
}

esp_err_t stc8_gpio_set_level(int gpio_num, uint8_t level)
{
    esp_err_t err;
    if (STC8_GPIO_OUT_MAX <= gpio_num) {
        STC8H1KXX_ERROR("stc8 can't set gpio=%d", gpio_num);
        return ESP_FAIL;
    }
    err = i2c_write_reg(stc8_handle, STC8_REG_ADDR_SET_GPIO + gpio_num, level);
    if (ESP_OK != err)
    {
        STC8H1KXX_ERROR("stc8 set gpio=%d fail", gpio_num);
        return err;
    }
    return err;
}

#endif

#ifdef CONFIG_BSP_STC8H1KXX_PWM_ENABLED

esp_err_t stc8_set_pwm_duty(int pwm_num, uint8_t duty)
{
    esp_err_t err;
    if (STC8_PWM_MAX <= pwm_num) {
        STC8H1KXX_ERROR("stc8 don't have pwm=%d", pwm_num);
        return false;
    }
    err = i2c_write_reg(stc8_handle, STC8_REG_ADDR_SET_PWM + pwm_num, duty);
    if (ESP_OK != err)
    {
        STC8H1KXX_ERROR("stc8 set pwm=%d fail", pwm_num);
        return err;
    }
    return err;
}

#endif

#endif
/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/