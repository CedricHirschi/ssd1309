# SSD1309

Universally usable SSD1309 driver with support for Adafruit GFX fonts.

Methods to include the driver on different platforms are provided in the following sections.

## Usage examples

### ESP-IDF

#### Including the driver

The files needed are located in [`platforms/esp-idf`](platforms/esp-idf). These files include:

- `CMakelists.txt`: The CMake configuration file for the ESP-IDF component.
- `idf_component.yaml`: The ESP-IDF component configuration file.
- `Kconfig.projbuild`: The Kconfig file for the ESP-IDF component. (Empty for now)

To include the driver in your ESP-IDF project, copy the [`esp-idf`](platforms/esp-idf) folder to the `components` folder of your project. You can rename the folder to `ssd1309` if you want to. Also copy the [`fonts`](fonts) folder as well as the implementation files `ssd1309.c` and `ssd1309.h` to the `ssd1309` folder. The folder structure should look like this:

```
esp-idf-project/
├── components/
│   └── ssd1309/
│       ├── fonts/
│       │   ├── Adafruit_GFX.h
│       │   ├── vbzfont.h
│       │   └── <any other fonts...>
│       ├── CMakeLists.txt
│       ├── idf_component.yaml
│       ├── Kconfig.projbuild
│       ├── ssd1309.c
│       └── ssd1309.h
├── main/
├── CMakeLists.txt
└── <any other project files...>
```

The driver should then be available in the project if you reconfigure the project.

#### Using the driver

The implementation in the firmware could look like this:

```c
#define SPI_HOST SPI2_HOST
#define SPI_PIN_SCLK GPIO_NUM_0
#define SPI_PIN_MOSI GPIO_NUM_1

#define OLED_PIN_CS GPIO_NUM_2
#define OLED_PIN_DC GPIO_NUM_3
#define OLED_PIN_RST GPIO_NUM_10

#define OLED_WIDTH 128
#define OLED_HEIGHT 64

spi_device_handle_t spi = NULL;

bool oled_spi_callback(uint8_t *data, size_t len)
{
    spi_transaction_t transfer;
    memset(&transfer, 0, sizeof(transfer));
    transfer.length = len * 8;
    transfer.tx_buffer = data;

    ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &transfer));

    return true;
}

bool oled_pin_callback(ssd1309_pin_t pin, bool state)
{
    switch (pin)
    {
    case SSD1309_PIN_DC:
        ESP_ERROR_CHECK(gpio_set_level(OLED_PIN_DC, state ? 1 : 0));
        break;
    case SSD1309_PIN_CS:
        // gpio_set_level(OLED_PIN_CS, state); // Done by SPI driver
        break;
    case SSD1309_PIN_RST:
        ESP_ERROR_CHECK(gpio_set_level(OLED_PIN_RST, state ? 1 : 0));
        break;
    }

    return true;
}

void oled_delay_callback(uint32_t us)
{
    if (us == 0)
    {
        return;
    }

    uint64_t start = esp_timer_get_time();
    while (esp_timer_get_time() - start < us)
    {
        ;
    }
}

void app_main(void)
{
    spi_bus_config_t bus_config = {
        .mosi_io_num = SPI_PIN_MOSI,
        .sclk_io_num = SPI_PIN_SCLK,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOST, &bus_config, SPI_DMA_CH_AUTO));
    spi_device_interface_config_t dev_config = {
        .clock_speed_hz = 5 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = OLED_PIN_CS,
        .queue_size = 7,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_HOST, &dev_config, &spi));
    ESP_LOGI(__func__, "Initialized SPI");

    gpio_config_t oled_gpio_config = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1 << OLED_PIN_DC) | (1 << OLED_PIN_RST),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    ESP_ERROR_CHECK(gpio_config(&oled_gpio_config));
    ESP_LOGI(__func__, "Initialized GPIO");

    ssd1309_t oled;
    ssd1309_init(&oled, OLED_WIDTH, OLED_HEIGHT, oled_spi_callback, oled_pin_callback, oled_delay_callback);
    ESP_LOGI(__func__, "Initialized OLED");

    while (true)
    {
        ssd1309_clear(&oled);
        ssd1309_printf(&oled, 0, 0, 1, "Hello, world!");
        ssd1309_printf(&oled, 0, 1, 1, "%.2fs elapsed", (float)esp_timer_get_time() / 1000000.0f);
        ssd1309_show(&oled);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

### STM32

#### Including the driver

There are many ways of including drivers in STM32 projects. The following example uses a Makefile project where I just added the files (Implementation files `ssd1309.h`/`ssd1309.c` and `fonts` folder) to a new subdirectory `Lib/ssd1309`. The folder structure looks like this:

```
stm32-project/
├── Drivers/
├── Inc/
├── Lib/
│   └── ssd1309/
│       ├── fonts/
│       │   ├── Adafruit_GFX.h
│       │   ├── vbzfont.h
│       │   └── <any other fonts...>
│       ├── ssd1309.c
│       └── ssd1309.h
├── Src/
├── Makefile
└── <any other project files...>
```

In the makefile, the following line is added to the `C_SOURCES` variable:

```Makefile
C_SOURCES = \
    ...
    Lib/ssd1309/ssd1309.c \
    ...
```

And the following lines are added to the `C_INCLUDES` variable:

```Makefile
C_INCLUDES = \
    ...
    -ILib/ssd1309 \
    -ILib/ssd1309/fonts \
    ...
```

The driver should then be available in the project.

#### Using the driver

The implementation in the firmware could look like this:

```c
#include "main.h"
#include "ssd1309.h"

#define DISP_WIDTH 128
#define DISP_HEIGHT 64

SPI_HandleTypeDef hspi1;
TIM_HandleTypeDef htim1;
ssd1309_t ssd1309;

// Initialization functions provided by STM32CubeMX
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM1_Init(void);

bool oled_spi_callback(uint8_t *data, size_t len)
{
  return HAL_SPI_Transmit(&hspi1, data, len, 1000) == HAL_OK;
}

bool oled_pin_callback(ssd1309_pin_t pin, bool state)
{
  switch(pin)
  {
    case SSD1309_PIN_DC:
      HAL_GPIO_WritePin(DISP_DC_GPIO_Port, DISP_DC_Pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
      break;
    case SSD1309_PIN_CS:
      // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, state ? GPIO_PIN_SET : GPIO_PIN_RESET); // Done by SPI driver
      break;
    case SSD1309_PIN_RST:
      HAL_GPIO_WritePin(DISP_RES_GPIO_Port, DISP_RES_Pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
      break;
  }
  return true;
}

void oled_delay_callback(uint32_t us)
{
  __HAL_TIM_SET_COUNTER(&htim1, 0);
  while (__HAL_TIM_GET_COUNTER(&htim1) < us);
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_TIM1_Init();

  if(!ssd1309_init(&ssd1309, DISP_WIDTH, DISP_HEIGHT, oled_spi_callback, oled_pin_callback, oled_delay_callback))
  {
    Error_Handler();
  }

  while (1)
  {
    ssd1309_clear(&ssd1309);
    ssd1309_printf(&ssd1309, 0, 0, 1, "Hello, World!");
    ssd1309_printf(&ssd1309, 0, 1, 1, "%3lus elapsed", HAL_GetTick() / 1000);
    ssd1309_show(&ssd1309);

    HAL_Delay(1000);
  }
}
```

### RP2040

#### Including the driver

Copy the files `ssd1309.c` and `ssd1309.h` as well as the `fonts` folder to the project directory, for example to a subdirectory `ssd1309`. The folder structure should look like this:

```
rp2040-project/
├── ssd1309/
│   ├── fonts/
│   │   ├── Adafruit_GFX.h
│   │   ├── vbzfont.h
│   │   └── <any other fonts...>
│   ├── ssd1309.c
│   └── ssd1309.h
├── CMakeLists.txt
├── main.c
└── <any other project files...>
```

In the `CMakeLists.txt` file, add the following lines to include the driver in the project:

```cmake
add_executable(rp2040-project
    ...
    ssd1309/ssd1309.c
    ...
)

target_include_directories(rp2040-project PRIVATE
    ...
    ssd1309
    ssd1309/fonts
    ...
)
```

The driver should then be available in the project.

#### Using the driver

The implementation in the firmware could look like this:

```c
#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/spi.h"

#include "ssd1309.h"

#define DISP_WIDTH 128
#define DISP_HEIGHT 64
#define DISP_PIN_SCK 10
#define DISP_PIN_MOSI 11
#define DISP_PIN_MISO 12
#define DISP_PIN_CS 13
#define DISP_PIN_DC 15
#define DISP_PIN_RST 14
#define DISP_SPI_INST spi1

ssd1309_t disp;

bool disp_spi_cb(uint8_t *data, size_t len)
{
    return spi_write_blocking(DISP_SPI_INST, data, len) == len;
}

bool disp_pin_cb(ssd1309_pin_t pin, bool value)
{
    switch (pin)
    {
    case SSD1309_PIN_DC:
        gpio_put(DISP_PIN_DC, value);
        break;
    case SSD1309_PIN_RST:
        gpio_put(DISP_PIN_RST, value);
        break;
    case SSD1309_PIN_CS:
        // gpio_put(DISP_PIN_CS, value); // Handled by SPI
        break;
    }

    return true;
}

void disp_delay_cb(uint32_t us)
{
    sleep_us(us);
}

int main()
{
    stdio_init_all();

    gpio_set_function(DISP_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(DISP_PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(DISP_PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(DISP_PIN_CS, GPIO_FUNC_SPI);
    printf("SPI baudrate: %u\n", spi_init(DISP_SPI_INST, 500000));

    gpio_init(DISP_PIN_RST);
    gpio_set_dir(DISP_PIN_RST, GPIO_OUT);
    gpio_put(DISP_PIN_RST, true);
    gpio_init(DISP_PIN_DC);
    gpio_set_dir(DISP_PIN_DC, GPIO_OUT);
    gpio_put(DISP_PIN_DC, true);

    if (!ssd1309_init(&disp, DISP_WIDTH, DISP_HEIGHT, disp_spi_cb, disp_pin_cb, disp_delay_cb))
    {
        printf("Failed to initialize display\n");
        return 1;
    }

    while (1)
    {
        float time = (float)to_ms_since_boot(get_absolute_time()) / 1000.0f;

        ssd1309_clear(&disp);
        ssd1309_printf(&disp, 0, 1, 1, "Time: %.2fs", (float)to_ms_since_boot(get_absolute_time()) / 1000.0f);
        ssd1309_show(&disp);

        sleep_ms(10);
    }

    return 0;
}
```
