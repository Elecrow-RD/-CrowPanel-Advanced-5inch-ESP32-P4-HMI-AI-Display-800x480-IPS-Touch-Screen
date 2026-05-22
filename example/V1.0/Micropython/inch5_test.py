import machine
from machine import Pin
from i2c import I2C
import time
import sys
import lvgl as lv
import lcd_bus

# Import your peripheral driver modules
import gt911
from gt911 import GT911
from rgb_display import RGBDisplay
from stc8h1kxx import STC8H1KXX
from dht20 import DHT20

# --------------------------
# Hardware Parameter Configuration
# --------------------------
LCD_WIDTH = 800
LCD_HEIGHT = 480

I2C_BUS = 0
I2C_SDA_PIN = 45
I2C_SCL_PIN = 46
I2C_FREQ = 400_000

TOUCH_RST_PIN = 36
TOUCH_INT_PIN = 42

# RGB Screen Timing Configuration
RGB_LCD_PIXEL_CLOCK_HZ    = (18_000_000) # Usually write as 18000000 is fine
RGB_LCD_HSYNC             = 4
RGB_LCD_HBP               = 8
RGB_LCD_HFP               = 8
RGB_LCD_VSYNC             = 4
RGB_LCD_VBP               = 16
RGB_LCD_VFP               = 16

# RGB Screen Pin Configuration
RGB_PIN_NUM_HSYNC         = 40
RGB_PIN_NUM_VSYNC         = 41
RGB_PIN_NUM_DE            = 2
RGB_PIN_NUM_PCLK          = 3

# 16-bit RGB565 data pins
RGB_PIN_NUM_DATA0         = 8
RGB_PIN_NUM_DATA1         = 7
RGB_PIN_NUM_DATA2         = 6
RGB_PIN_NUM_DATA3         = 5
RGB_PIN_NUM_DATA4         = 4
RGB_PIN_NUM_DATA5         = 14
RGB_PIN_NUM_DATA6         = 13
RGB_PIN_NUM_DATA7         = 12
RGB_PIN_NUM_DATA8         = 11
RGB_PIN_NUM_DATA9         = 10
RGB_PIN_NUM_DATA10        = 9
RGB_PIN_NUM_DATA11        = 19
RGB_PIN_NUM_DATA12        = 18
RGB_PIN_NUM_DATA13        = 17
RGB_PIN_NUM_DATA14        = 16
RGB_PIN_NUM_DATA15        = 15

LED = Pin(48, Pin.OUT)# Set GPIO pin 48 to output mode

def device_init():
    print("Initializing peripherals...")
    
    # 1. Initialize LVGL
    if not lv.is_initialized():
        lv.init()
    
    # 2. Initialize I2C bus (for STC8 controller, GT911 touch, DHT20 temperature/humidity sensor)
    i2c_bus = I2C.Bus(
        host = I2C_BUS, 
        sda=I2C_SDA_PIN, 
        scl=I2C_SCL_PIN, 
        freq=I2C_FREQ
    )
    print(f"\n📡 Scanning I2C bus devices: {[hex(d) for d in i2c_bus.scan()]}")
    
    i2c_stc8 = I2C.Device(
        bus = i2c_bus, 
        dev_id = STC8H1KXX.I2C_ADDR, 
        reg_bits=8
    )
    # 3. Initialize STC8 and turn on backlight
    stc8 = STC8H1KXX(i2c_stc8)
    print("Enabling LCD backlight power and PWM...")
    stc8.set_gpio_level(STC8H1KXX.GPIO_OUT_LCD_BL_POWER, 1) # Enable LCD power supply
    stc8.set_pwm_duty(STC8H1KXX.PWM_LCD_BL_EN, 50)         # Set backlight brightness to 50%

    i2c_dht20 = I2C.Device(
        bus = i2c_bus, 
        dev_id = DHT20.I2C_ADDR, 
        reg_bits=8
    )

    #Initialize DHT20 sensor
    try :
        dht20 = DHT20(i2c_dht20)
    except Exception as e:
        sys.print_exception(e)
        print('Failed to initialize DHT20 sensor!')

    # 4. Initialize underlying RGB bus (lcd_bus.RGBBus)
    # Modify parameter names according to your lcd_bus.pyi file definition
    print("Configuring RGB bus...")
    rgb_bus = lcd_bus.RGBBus(
        hsync=RGB_PIN_NUM_HSYNC,
        vsync=RGB_PIN_NUM_VSYNC,
        de=RGB_PIN_NUM_DE,
        pclk=RGB_PIN_NUM_PCLK,
        data0=RGB_PIN_NUM_DATA0,
        data1=RGB_PIN_NUM_DATA1,
        data2=RGB_PIN_NUM_DATA2,
        data3=RGB_PIN_NUM_DATA3,
        data4=RGB_PIN_NUM_DATA4,
        data5=RGB_PIN_NUM_DATA5,
        data6=RGB_PIN_NUM_DATA6,
        data7=RGB_PIN_NUM_DATA7,
        data8=RGB_PIN_NUM_DATA8,
        data9=RGB_PIN_NUM_DATA9,
        data10=RGB_PIN_NUM_DATA10,
        data11=RGB_PIN_NUM_DATA11,
        data12=RGB_PIN_NUM_DATA12,
        data13=RGB_PIN_NUM_DATA13,
        data14=RGB_PIN_NUM_DATA14,
        data15=RGB_PIN_NUM_DATA15,
        freq=RGB_LCD_PIXEL_CLOCK_HZ,
        hsync_pulse_width=RGB_LCD_HSYNC,
        hsync_back_porch=RGB_LCD_HBP,
        hsync_front_porch=RGB_LCD_HFP,
        vsync_pulse_width=RGB_LCD_VSYNC,
        vsync_back_porch=RGB_LCD_VBP,
        vsync_front_porch=RGB_LCD_VFP,
        pclk_active_low=False,                # Default value
    )

    # 5. Instantiate RGBDisplay driver and mount to LVGL
    print("Registering display to LVGL...")
    display = RGBDisplay(
        data_bus=rgb_bus,
        display_width=LCD_WIDTH,
        display_height=LCD_HEIGHT,
        color_space=lv.COLOR_FORMAT.RGB565 # 16 pins correspond to RGB565
    )
    display.init()
    
    # 6. Initialize GT911 touch screen
    print("Initializing GT911 touch driver...")
    
    i2c_gt911 = I2C.Device(
        bus = i2c_bus, 
        dev_id = gt911.I2C_ADDR,
        reg_bits=16
    )
    # Assuming gt911 module can be instantiated with i2c bus and auto-register to LVGL indev
    touch = GT911(
        device=i2c_gt911,
        reset_pin=TOUCH_RST_PIN,
        interrupt_pin=TOUCH_INT_PIN,
    )
    
    # Define a touch callback function that meets LVGL requirements
    def touch_read_cb(indev, data):
        # Assuming touch._get_coords() returns format like (press_state, x, y) 
        # You need to adjust according to your actual gt911 library return values
        coords = touch._get_coords() 
        if coords: 
            # Assuming your library returns non-empty for pressed, or parse state, x, y specifically
            # Here is a common format example:
            data.point.x = coords[0] # Your X coordinate
            data.point.y = coords[1] # Your Y coordinate
            data.state = lv.INDEV_STATE.PRESSED
            #print(data.point.x, data.point.y, data.state)
        else:
            data.state = lv.INDEV_STATE.RELEASED

    indev = lv.indev_create()
    indev.set_type(lv.INDEV_TYPE.POINTER)
    indev.set_read_cb(touch_read_cb)
    #indev.set_display(display) # Only needed for multiple displays, single display default is fine

    return display, touch, stc8, dht20

def SetFlag( obj, flag, value):
    if (value):
        obj.add_flag(flag)
    else:
        obj.remove_flag(flag)
    return

# COMPONENTS

def Button1_eventhandler(event_struct):
   event = event_struct.get_code()
   if event == lv.EVENT.CLICKED and True:
      LED.value(1)
   return

def Button2_eventhandler(event_struct):
   event = event_struct.get_code()
   if event == lv.EVENT.CLICKED and True:
      LED.value(0)
   return

def create_lvgl_ui():
    ui_Screen1 = lv.obj()
    SetFlag(ui_Screen1, lv.obj.FLAG.SCROLLABLE, False)
    ui_Screen1.set_style_bg_color(lv.color_hex(0xEFF6DB), lv.PART.MAIN | lv.STATE.DEFAULT )
    ui_Screen1.set_style_bg_opa(255, lv.PART.MAIN| lv.STATE.DEFAULT )
    # ui_Screen1.set_style_bg_img_src( ui_images.TemporaryImage, lv.PART.MAIN | lv.STATE.DEFAULT )

    ui_Button1 = lv.button(ui_Screen1)
    ui_Button1.set_width(85)
    ui_Button1.set_height(85)
    ui_Button1.set_x(208)
    ui_Button1.set_y(-90)
    ui_Button1.set_align( lv.ALIGN.CENTER)
    SetFlag(ui_Button1, lv.obj.FLAG.SCROLLABLE, False)
    SetFlag(ui_Button1, lv.obj.FLAG.SCROLL_ON_FOCUS, True)
        # ========== LVGL 9.4 MicroPython Correct gradient settings（Default state） ==========
    # Set gradient direction (top to bottom)
    ui_Button1.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN | lv.STATE.DEFAULT)
    # Set gradient start color (top)
    ui_Button1.set_style_bg_grad_color(lv.color_hex(0xFFF9C4), lv.PART.MAIN | lv.STATE.DEFAULT)
    # Set gradient end color (bottom)
    ui_Button1.set_style_bg_color(lv.color_hex(0xFFE032), lv.PART.MAIN | lv.STATE.DEFAULT)
    ui_Button1.set_style_bg_opa(255, lv.PART.MAIN| lv.STATE.DEFAULT )
    # ui_Button1.set_style_bg_img_src( ui_images.ui_img_off_png, lv.PART.MAIN | lv.STATE.DEFAULT )
    # ========== LVGL 9.4 MicroPython Correct gradient settings（Pressed state） ==========
    ui_Button1.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN | lv.STATE.PRESSED)
    ui_Button1.set_style_bg_grad_color(lv.color_hex(0xFFE032), lv.PART.MAIN | lv.STATE.PRESSED)
    ui_Button1.set_style_bg_color(lv.color_hex(0xFFE032), lv.PART.MAIN | lv.STATE.PRESSED)
    ui_Button1.set_style_bg_opa(255, lv.PART.MAIN| lv.STATE.PRESSED )
    ui_Button1.add_event_cb(Button1_eventhandler, lv.EVENT.ALL, None)
    ui_Button1.set_style_shadow_width(8, lv.PART.MAIN | lv.STATE.DEFAULT)
    ui_Button1.set_style_shadow_color(lv.color_hex(0xFFEB3B), lv.PART.MAIN | lv.STATE.DEFAULT)  # Glowing shadow
    
    ui_LabelOn = lv.label(ui_Button1)
    ui_LabelOn.set_text("ON")
    ui_LabelOn.set_align( lv.ALIGN.CENTER)
    ui_LabelOn.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN | lv.STATE.DEFAULT )
    ui_LabelOn.set_style_text_opa(255, lv.PART.MAIN| lv.STATE.DEFAULT )
    ui_LabelOn.set_style_text_font( lv.font_montserrat_40, lv.PART.MAIN | lv.STATE.DEFAULT )

    ui_Button2 = lv.button(ui_Screen1)
    ui_Button2.set_width(85)
    ui_Button2.set_height(85)
    ui_Button2.set_x(205)
    ui_Button2.set_y(40)
    ui_Button2.set_align( lv.ALIGN.CENTER)
    SetFlag(ui_Button2, lv.obj.FLAG.SCROLLABLE, False)
    SetFlag(ui_Button2, lv.obj.FLAG.SCROLL_ON_FOCUS, True)
    # ========== LVGL 9.4 MicroPython Correct gradient settings（Default state） ==========
    # Set gradient direction (top to bottom)
    ui_Button2.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN | lv.STATE.DEFAULT)
    # Set gradient start color (top)
    ui_Button2.set_style_bg_grad_color(lv.color_hex(0x9E9E9E), lv.PART.MAIN | lv.STATE.DEFAULT)
    # Set gradient end color (bottom)
    ui_Button2.set_style_bg_color(lv.color_hex(0x616161), lv.PART.MAIN | lv.STATE.DEFAULT)
    ui_Button2.set_style_bg_opa(255, lv.PART.MAIN| lv.STATE.DEFAULT )
    # ui_Button2.set_style_bg_img_src( ui_images.ui_img_off_png, lv.PART.MAIN | lv.STATE.DEFAULT )
    # ========== LVGL 9.4 MicroPython Correct gradient settings（Pressed state） ==========
    ui_Button2.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN | lv.STATE.PRESSED)
    ui_Button2.set_style_bg_grad_color(lv.color_hex(0x616161), lv.PART.MAIN | lv.STATE.PRESSED)
    ui_Button2.set_style_bg_color(lv.color_hex(0x616161), lv.PART.MAIN | lv.STATE.PRESSED)
    ui_Button2.set_style_bg_opa(255, lv.PART.MAIN| lv.STATE.PRESSED )
    ui_Button2.add_event_cb(Button2_eventhandler, lv.EVENT.ALL, None)

    ui_LabelOff = lv.label(ui_Button2)
    ui_LabelOff.set_text("OFF")
    ui_LabelOff.set_align( lv.ALIGN.CENTER)
    ui_LabelOff.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN | lv.STATE.DEFAULT )
    ui_LabelOff.set_style_text_opa(255, lv.PART.MAIN| lv.STATE.DEFAULT )
    ui_LabelOff.set_style_text_font( lv.font_montserrat_40, lv.PART.MAIN | lv.STATE.DEFAULT )

    ui_Image1 = lv.image(ui_Screen1)
    # ui_Image1.set_src(ui_images.ui_img_background1_png)
    ui_Image1.set_width(lv.SIZE_CONTENT)	# 1
    ui_Image1.set_height(lv.SIZE_CONTENT)   # 1
    ui_Image1.set_x(-90)
    ui_Image1.set_y(-14)
    ui_Image1.set_align( lv.ALIGN.CENTER)
    SetFlag(ui_Image1, lv.obj.FLAG.ADV_HITTEST, True)
    SetFlag(ui_Image1, lv.obj.FLAG.SCROLLABLE, False)

    ui_Label1 = lv.label(ui_Screen1)
    ui_Label1.set_text("")
    ui_Label1.set_width(450)	# 1
    ui_Label1.set_height(60)   # 1
    ui_Label1.set_x(-100)
    ui_Label1.set_y(-89)
    ui_Label1.set_style_radius(10, lv.PART.MAIN | lv.STATE.DEFAULT)
    ui_Label1.set_align( lv.ALIGN.CENTER)
    ui_Label1.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN | lv.STATE.DEFAULT)
    ui_Label1.set_style_bg_grad_color(lv.color_hex(0xBBDEFB), lv.PART.MAIN | lv.STATE.DEFAULT)
    ui_Label1.set_style_bg_color(lv.color_hex(0x0000FB), lv.PART.MAIN | lv.STATE.DEFAULT)
    ui_Label1.set_style_bg_opa(255, lv.PART.MAIN| lv.STATE.DEFAULT )
    # ui_Label1.set_style_pad_ver(lv.SIZE_CONTENT, lv.PART.MAIN | lv.STATE.DEFAULT)
    # ui_Label1.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN | lv.STATE.DEFAULT)
    ui_Label1.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN | lv.STATE.DEFAULT )
    ui_Label1.set_style_text_opa(255, lv.PART.MAIN| lv.STATE.DEFAULT )
    ui_Label1.set_style_text_font( lv.font_montserrat_40, lv.PART.MAIN | lv.STATE.DEFAULT )

    ui_Label2 = lv.label(ui_Screen1)
    ui_Label2.set_text("")
    ui_Label2.set_width(450)	# 1
    ui_Label2.set_height(60)   # 1
    ui_Label2.set_x(-100)
    ui_Label2.set_y(40)
    ui_Label2.set_style_radius(10, lv.PART.MAIN | lv.STATE.DEFAULT)
    ui_Label2.set_align( lv.ALIGN.CENTER)
    ui_Label2.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN | lv.STATE.DEFAULT)
    ui_Label2.set_style_bg_grad_color(lv.color_hex(0xBBDEFB), lv.PART.MAIN | lv.STATE.DEFAULT)
    ui_Label2.set_style_bg_color(lv.color_hex(0x0000FB), lv.PART.MAIN | lv.STATE.DEFAULT)
    ui_Label2.set_style_bg_opa(255, lv.PART.MAIN| lv.STATE.DEFAULT )
    # ui_Label2.set_style_pad_ver(lv.SIZE_CONTENT, lv.PART.MAIN | lv.STATE.DEFAULT)
    # ui_Label2.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN | lv.STATE.DEFAULT)
    ui_Label2.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN | lv.STATE.DEFAULT )
    ui_Label2.set_style_text_opa(255, lv.PART.MAIN| lv.STATE.DEFAULT )
    ui_Label2.set_style_text_font( lv.font_montserrat_40, lv.PART.MAIN | lv.STATE.DEFAULT )

    return ui_Screen1, ui_Label1, ui_Label2

def main():
    try:
        # Initialize device peripherals
        display, touch, stc8, dht20 = device_init()
        
        # Build UI
        ui_Screen1, ui_Label1, ui_Label2 = create_lvgl_ui()

        LED.value(1)

        dht20.measure()
        temp = dht20.temperature()
        hum = dht20.humidity()
        ui_Label1.set_text(f"temperature: {round(temp, 1)} C")  
        ui_Label2.set_text(f"humidity: {round(hum)} %")

        lv.screen_load(ui_Screen1)
        
        print("Initialization complete, entering main loop...")
        
        # Record last sensor read time
        last_dht_time = time.ticks_ms()
        
        # Run LVGL polling event loop
        while True:
            current_time = time.ticks_ms()
            
            # 1. Decouple: Read sensor every 1000ms (1 second), non-blocking UI
            if time.ticks_diff(current_time, last_dht_time) >= 1000:
                dht20.measure()
                temp = dht20.temperature()
                hum = dht20.humidity()
                ui_Label1.set_text(f"temperature: {round(temp, 1)} C")  
                ui_Label2.set_text(f"humidity: {round(hum)} %")
                #print(f"temperature: {round(temp, 1)} C")  
                #print(f"humidity: {round(hum)} %")        
                last_dht_time = current_time

            # 2. LVGL heartbeat and event handling (very important)
            lv.tick_inc(10)          # Tell LVGL that 10ms has passed
            lv.timer_handler()      # Handle redraw and touch events
            
            # 3. Very short sleep to ensure UI frame rate and touch response speed
            time.sleep_ms(10)
            
    except Exception as e:
        sys.print_exception(e)

if __name__ == "__main__":
    main()