/*---------------------------------------------------------------
 * Teaching module overview: Board support
 * This file groups the bsp_camera responsibilities so learners can
 * follow the subsystem boundary before reading individual routines.
 *--------------------------------------------------------------*/

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_camera.h"
// Adapt to LVGL9 + esp_lvgl_port used in current project
#include <esp_lvgl_port.h>
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
// Add the missing macro definitions.
#ifndef BITS_PER_PIXEL
#define BITS_PER_PIXEL 16
#endif

static i2c_master_bus_handle_t sccb_bus_handle = NULL;
static esp_sccb_io_handle_t sccb_io_handle = NULL;
static esp_cam_sensor_device_t *cam = NULL;
static isp_proc_handle_t isp_proc = NULL;
static isp_ae_ctlr_t ae_ctlr = NULL;
static isp_awb_ctlr_t awb_ctlr = NULL;
esp_cam_ctlr_trans_t my_trans;
esp_cam_ctlr_handle_t cam_handle = NULL;
void *camera_buffer = NULL;

static lv_obj_t *camera_obj;
lv_img_dsc_t img_camera;

/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/

/**
 * @brief Notify the waiting task that automatic white-balance data is ready.
 * @param awb_ctlr AWB controller that produced the event.
 * @param edata Event statistics supplied by the ISP driver.
 * @param user_data Task handle waiting for the notification.
 * @return true when a higher-priority task was awakened.
 * @note Called by the ISP driver from its statistics callback context.
 */
bool example_isp_awb_on_statistics_done_cb(isp_awb_ctlr_t awb_ctlr, const esp_isp_awb_evt_data_t *edata, void *user_data)
{
    return true;
}

/**
 * @brief Supply the CSI controller with the reusable PSRAM frame buffer.
 * @param handle Active camera-controller handle.
 * @param trans Transaction descriptor to populate.
 * @param user_data Unused callback context.
 * @return false because no task switch is requested.
 * @note Called by the camera driver whenever it needs a destination buffer.
 */
IRAM_ATTR bool camera_get_new_vb(esp_cam_ctlr_handle_t handle, esp_cam_ctlr_trans_t *trans, void *user_data)
{
    esp_cam_ctlr_trans_t new_trans = *(esp_cam_ctlr_trans_t *)user_data;
    trans->buffer = new_trans.buffer;
    trans->buflen = new_trans.buflen;
    return false;
}

/**
 * @brief Publish a completed CSI frame to the consumer queue.
 * @param handle Active camera-controller handle.
 * @param trans Completed transaction descriptor.
 * @param user_data Unused callback context.
 * @return true when queue delivery wakes a higher-priority task.
 * @note Called by the camera driver when a full frame has been captured.
 */
IRAM_ATTR bool camera_get_finished_trans(esp_cam_ctlr_handle_t handle, esp_cam_ctlr_trans_t *trans, void *user_data)
{
    return false;
}

/**
 * @brief Discover and configure the MIPI CSI image sensor.
 * @param None.
 * @return ESP_OK on success, otherwise the sensor-driver error.
 * @note Called by camera_init() before creating the CSI and ISP stages.
 */
static esp_err_t camera_sensor_init()
{
    esp_err_t err = ESP_OK;
    int enable_flag = 1;
    uint32_t exposure_us = 4000;
    uint32_t exposure_val = 700;
    esp_cam_sensor_format_t *cam_cur_fmt = NULL;
    i2c_master_bus_config_t sccb_conf = {
        .i2c_port = SCCB_MASTER_PORT,
        .sda_io_num = SCCB_GPIO_SDA,
        .scl_io_num = SCCB_GPIO_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .flags.enable_internal_pullup = true,
    };
    err = i2c_new_master_bus(&sccb_conf, &sccb_bus_handle);
    if (err != ESP_OK)
        return err;
    esp_cam_sensor_config_t cam_config = {
        .sccb_handle = sccb_io_handle,
        .reset_pin = -1,
        .pwdn_pin = -1,
        .xclk_pin = -1,
        .sensor_port = ESP_CAM_SENSOR_MIPI_CSI,
    };
    for (esp_cam_sensor_detect_fn_t *p = &__esp_cam_sensor_detect_fn_array_start; p < &__esp_cam_sensor_detect_fn_array_end; ++p)
    {
        sccb_i2c_config_t i2c_config = {
            .scl_speed_hz = 100000,
            .device_address = p->sccb_addr,
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        };
        ESP_ERROR_CHECK(sccb_new_i2c_io(sccb_bus_handle, &i2c_config, &cam_config.sccb_handle));
        cam = (*(p->detect))(&cam_config);
        if (cam)
        {
            if (p->port != ESP_CAM_SENSOR_MIPI_CSI)
            {
                err = ESP_FAIL;
                CAMERA_ERROR("detect a camera sensor with mismatched interface");
                return err;
            }
            break;
        }
        err = esp_sccb_del_i2c_io(cam_config.sccb_handle);
        if (err != ESP_OK)
            return err;
    }
    if (cam == NULL)
    {
        err = ESP_FAIL;
        CAMERA_ERROR("failed to detect camera sensor");
        return err;
    }
    esp_cam_sensor_format_array_t cam_fmt_array = {0};
    err = esp_cam_sensor_query_format(cam, &cam_fmt_array);
    if (err != ESP_OK) {
        return err;
    }
    const esp_cam_sensor_format_t *parray = cam_fmt_array.format_array;

    CAMERA_INFO("Supported sensor formats (count=%lu):", (unsigned long)cam_fmt_array.count);
    for (int i = 0; i < cam_fmt_array.count; i++) {
        CAMERA_INFO("  [%d] name=%s",
                    i,
                    parray[i].name ? parray[i].name : "NULL");
    }

    // Prefer the RAW8 1024x600 mode; fall back to the first format if not found
    for (int i = 0; i < cam_fmt_array.count; i++) {
        if (parray[i].name &&
            !strcmp(parray[i].name, "MIPI_2lane_24Minput_RAW8_1024x600_30fps")) {
            cam_cur_fmt = &parray[i];
            break;
        }
    }
    if (cam_cur_fmt == NULL && cam_fmt_array.count > 0) {
        CAMERA_INFO("Preferred RAW8 1024x600 format not found, use first format as fallback");
        cam_cur_fmt = &parray[0];
    }
    if (cam_cur_fmt == NULL) {
        CAMERA_ERROR("No valid camera format found");
        return ESP_FAIL;
    }

    CAMERA_INFO("Using sensor format: %s",
                cam_cur_fmt->name ? cam_cur_fmt->name : "UNKNOWN");

    err = esp_cam_sensor_set_format(cam, (const esp_cam_sensor_format_t *)cam_cur_fmt);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("Format set fail");
        return err;
    }

    // Basic mirror setting
    err = esp_cam_sensor_set_para_value(cam, ESP_CAM_SENSOR_HMIRROR, &enable_flag, 1);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("camera mirror fail");
        return err;
    }

    // Use more conservative initial exposure to avoid overexposure (white image)
    exposure_us = 1000;   // was 4000
    exposure_val = 300;   // was 700

    err = esp_cam_sensor_set_para_value(cam, ESP_CAM_SENSOR_EXPOSURE_US, &exposure_us, 1);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("camera set exposure time fail");
        return err;
    }

    err = esp_cam_sensor_set_para_value(cam, ESP_CAM_SENSOR_EXPOSURE_VAL, &exposure_val, 1);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("camera set exposure val fail");
        return err;
    }
    err = esp_cam_sensor_ioctl(cam, ESP_CAM_SENSOR_IOC_S_STREAM, &enable_flag); // Set sensor output stream
    if (err != ESP_OK)
    {
        CAMERA_ERROR("Start stream fail");
        return err;
    }
    return err;
}

/**
 * @brief Configure the CSI controller for 1024 x 600 RAW8 input.
 * @param None.
 * @return ESP_OK on success, otherwise the camera-controller error.
 * @note Called by camera_init() after the sensor output format is selected.
 */
static esp_err_t camera_csi_init()
{
    esp_err_t err = ESP_OK;
    esp_cam_ctlr_csi_config_t csi_config = {
        .ctlr_id = 1,
        .h_res = 1024,
        .v_res = 600,
        .lane_bit_rate_mbps = 200,
        .input_data_color_type = CAM_CTLR_COLOR_RAW8,
        .output_data_color_type = CAM_CTLR_COLOR_RGB565,
        .data_lane_num = 2,
        .byte_swap_en = false,
        .queue_items = 5,
    };
    err = esp_cam_new_csi_ctlr(&csi_config, &cam_handle);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("csi init fail[%d]", err);
        return err;
    }
    esp_cam_ctlr_evt_cbs_t cbs = {
        .on_get_new_trans = camera_get_new_vb,
        .on_trans_finished = camera_get_finished_trans,
    };
    err = esp_cam_ctlr_register_event_callbacks(cam_handle, &cbs, &my_trans);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("ops register fail");
        return err;
    }
    err = esp_cam_ctlr_enable(cam_handle);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("enable cam controller fail");
        return err;
    }
    return err;
}

/**
 * @brief Configure the ISP to convert RAW8 sensor data into RGB565 frames.
 * @param None.
 * @return ESP_OK on success, otherwise the ISP configuration error.
 * @note Called by camera_init() after the CSI controller is created.
 */
static esp_err_t isp_init()
{
    esp_err_t err = ESP_OK;
    esp_isp_processor_cfg_t isp_config = {
        .clk_hz = 80 * 1000 * 1000,
        .input_data_source = ISP_INPUT_DATA_SOURCE_CSI,
        .input_data_color_type = ISP_COLOR_RAW8,
        .output_data_color_type = ISP_COLOR_RGB565,
        .bayer_order = COLOR_RAW_ELEMENT_ORDER_BGGR,
        .has_line_start_packet = false,
        .has_line_end_packet = false,
        .h_res = 1024,
        .v_res = 600,
    };
    err = esp_isp_new_processor(&isp_config, &isp_proc);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("isp register fail");
        return err;
    }
    err = esp_isp_enable(isp_proc);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("enable isp fail");
        return err;
    }
    esp_isp_color_config_t color_config = {
        .color_contrast = {
            .integer = 0,
            .decimal = 88,
        },
        .color_saturation = {
            .integer = 1,
            .decimal = 0,
        },
        .color_hue = 0,
        .color_brightness = 40,
    };
    err = esp_isp_color_configure(isp_proc, &color_config);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("isp color configure fail");
        return err;
    }
    err = esp_isp_color_enable(isp_proc);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("enable isp color fail");
        return err;
    }
    esp_isp_awb_config_t awb_config = {
        .sample_point = ISP_AWB_SAMPLE_POINT_AFTER_CCM,
        .white_patch.luminance.min = 0,
        .white_patch.luminance.max = 255,
        .white_patch.red_green_ratio.min = 0.7,
        .white_patch.red_green_ratio.max = 1.0,
        .white_patch.blue_green_ratio.min = 0.7,
        .white_patch.blue_green_ratio.max = 1.0,
    };
    err = esp_isp_new_awb_controller(isp_proc, &awb_config, &awb_ctlr);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("enable isp awb controller fail");
        return err;
    }
    esp_isp_awb_cbs_t awb_cb = {
        .on_statistics_done = example_isp_awb_on_statistics_done_cb,
    };
    err = esp_isp_awb_register_event_callbacks(awb_ctlr, &awb_cb, NULL);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("register isp awb callback fail");
        return err;
    }
    err = esp_isp_awb_controller_enable(awb_ctlr);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("enable isp awb fail");
        return err;
    }
    err = esp_isp_awb_controller_start_continuous_statistics(awb_ctlr);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("start isp awb fail");
        return err;
    }
    esp_isp_ae_config_t ae_config = {
        .sample_point = ISP_AE_SAMPLE_POINT_AFTER_GAMMA,
    };
    err = esp_isp_new_ae_controller(isp_proc, &ae_config, &ae_ctlr);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("isp ae configure fail");
        return err;
    }
    err = esp_isp_ae_controller_enable(ae_ctlr);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("isp ae enable fail");
        return err;
    }
    err = esp_isp_ae_controller_start_continuous_statistics(ae_ctlr);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("isp ae start fail");
        return err;
    }
    esp_isp_ccm_config_t ccm_cfg = {
        .matrix = {
            {1.0, 0.0, 0.0},
            {0.0, 0.5, 0.0},
        {0.0, 0.0, 1.0},
        },
        .saturation = false,
    };
    err = esp_isp_ccm_configure(isp_proc, &ccm_cfg);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("isp ccm configure fail");
        return err;
    }
    err = esp_isp_ccm_enable(isp_proc);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("isp ccm enable fail");
        return err;
    }
    return err;
}

/**
 * @brief Allocate buffers and start the complete sensor-to-PSRAM pipeline.
 * @param None.
 * @return ESP_OK when streaming starts, otherwise the first startup error.
 * @note Called once by P4BspCamera construction after camera power is enabled.
 */
esp_err_t camera_init()
{
    esp_err_t err = ESP_OK;
    uint32_t cache_line_size = cache_hal_get_cache_line_size(CACHE_LL_LEVEL_EXT_MEM, CACHE_TYPE_DATA);
    size_t camera_buffer_size = 0;
    camera_buffer_size = 1024 * 600 * ((BITS_PER_PIXEL + 7) / 8);
    camera_buffer = heap_caps_aligned_calloc(cache_line_size, 1, camera_buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (camera_buffer == NULL)
        return ESP_FAIL;
    my_trans.buffer = camera_buffer;
    my_trans.buflen = camera_buffer_size;
    err = camera_sensor_init();
    if (err != ESP_OK)
        return err;
    err = camera_csi_init();
    if (err != ESP_OK)
        return err;
    err = isp_init();
    if (err != ESP_OK)
        return err;
    memset(my_trans.buffer, 0xFF, my_trans.buflen);
    esp_cache_msync((void *)my_trans.buffer, my_trans.buflen, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    err = esp_cam_ctlr_start(cam_handle);
    if (err != ESP_OK)
    {
        CAMERA_ERROR(" Camera Driver start fail");
        return err;
    }
    return err;
}

/**
 * @brief Wait for and expose the next completed camera frame.
 * @param None.
 * @return ESP_OK when a frame is available, otherwise a queue or driver error.
 * @note Called by preview and image-explanation paths when fresh pixels are needed.
 */
esp_err_t camera_refresh()
{
    esp_err_t err = ESP_OK;
    err = esp_cam_ctlr_receive(cam_handle, &my_trans, ESP_CAM_CTLR_MAX_DELAY);
    if (err != ESP_OK)
    {
        CAMERA_INFO("Receive failed: %s", esp_err_to_name(err));
        return err;
    }

    /* Invalidate cache to ensure CPU sees the latest data written by DMA/CSI/ISP */
    esp_cache_msync((void *)my_trans.buffer, my_trans.buflen, ESP_CACHE_MSYNC_FLAG_INVALIDATE);

    /* Sample the first small segment to check whether it is still all 0xFF (typical pattern of blank/unwritten buffer) */
    const uint8_t *buf = (const uint8_t *)my_trans.buffer;
    size_t total = my_trans.buflen;
    size_t sample = total < 4096 ? total : 4096;
    size_t non_ff = 0;
    size_t non_00 = 0;

    for (size_t i = 0; i < sample; ++i) {
        if (buf[i] != 0xFF) {
            non_ff++;
        }
        if (buf[i] != 0x00) {
            non_00++;
        }
    }

    if (non_ff == 0) {
        CAMERA_ERROR("Buffer sample is all 0xFF (sample=%lu). CSI/ISP may not be writing data!",
                     (unsigned long)sample);
    } else {
        CAMERA_INFO("Buffer check: sample=%lu, nonFF=%lu, non00=%lu, first bytes=%02X %02X %02X %02X",
                    (unsigned long)sample,
                    (unsigned long)non_ff,
                    (unsigned long)non_00,
                    sample > 0 ? buf[0] : 0,
                    sample > 1 ? buf[1] : 0,
                    sample > 2 ? buf[2] : 0,
                    sample > 3 ? buf[3] : 0);
    }

    return err;
}

/**
 * @brief Mark the current camera image descriptor for display refresh.
 * @param None.
 * @return Nothing.
 * @note Called by the camera display task after a new frame is available.
 */
void camera_display_refresh()
{
    lv_obj_invalidate(camera_obj);
}

/**
 * @brief Run one camera-frame display update.
 * @param None.
 * @return Nothing.
 * @note Called repeatedly by P4BspCamera::CameraDisplayTask().
 */
void camera_display()
{
    // Use lock provided by esp_lvgl_port to protect LVGL calls
    if (!lvgl_port_lock(0)) {
        return;
    }

    // Create image object
    camera_obj = lv_img_create(lv_scr_act());
    lv_obj_align(camera_obj, LV_ALIGN_CENTER, 0, 0);

    // Initialize global img_camera descriptor, adapt to LVGL9 structure
    memset(&img_camera, 0, sizeof(img_camera));
    img_camera.header.magic  = LV_IMAGE_HEADER_MAGIC;
    img_camera.header.cf     = LV_COLOR_FORMAT_RGB565;
    img_camera.header.flags  = 0;
    img_camera.header.w      = 1024;
    img_camera.header.h      = 600;
    img_camera.header.stride = 1024 * 2;
    img_camera.data          = my_trans.buffer;
    img_camera.data_size     = 1024 * 600 * 2;

    lv_img_set_src(camera_obj, &img_camera);

    lvgl_port_unlock();
}
