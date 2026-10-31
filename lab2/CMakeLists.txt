cmake_minimum_required(VERSION 3.20)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/arm-gcc.cmake")
project(Lab2_20261 LANGUAGES C ASM)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(LAB_STAGE 9 CACHE STRING "6: RTC; 7: OLED clock; 8: RFID; 9: access control")
if(NOT LAB_STAGE MATCHES "^[6-9]$")
  message(FATAL_ERROR "LAB_STAGE must be 6, 7, 8 or 9")
endif()

# Reuse the checked-in CMSIS/HAL base from Lab 1. Extra I2C/SPI files are
# vendored here at the same HAL version (v1.8.3); no download during build.
set(BASE "${CMAKE_CURRENT_SOURCE_DIR}/../lab1/Drivers")
set(HAL "${BASE}/STM32F4xx_HAL_Driver")
set(EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver")
add_executable(${PROJECT_NAME}
  Core/Startup/startup_stm32f429zitx.s
  Core/Src/system_stm32f4xx.c Core/Src/sysmem.c Core/Src/syscalls.c
  Core/Src/main.c Core/Src/stm32f4xx_hal_msp.c Core/Src/stm32f4xx_it.c
  App/lab_app.c App/access.c App/console.c App/diagnostics.c
  Libraries/rtc/ds1307.c Libraries/rfid/rc522.c
  Libraries/oled/SH1106.c Libraries/oled/fonts.c
  ${EXTRA}/Src/stm32f4xx_hal_i2c.c
  ${EXTRA}/Src/stm32f4xx_hal_i2c_ex.c
  ${EXTRA}/Src/stm32f4xx_hal_spi.c)
foreach(module hal hal_cortex hal_dma hal_dma_ex hal_flash hal_flash_ex
    hal_gpio hal_pwr hal_pwr_ex hal_rcc hal_rcc_ex hal_uart)
  target_sources(${PROJECT_NAME} PRIVATE ${HAL}/Src/stm32f4xx_${module}.c)
endforeach()
target_include_directories(${PROJECT_NAME} PRIVATE
  Core/Inc App Libraries/rtc Libraries/rfid Libraries/oled
  ${EXTRA}/Inc ${HAL}/Inc ${HAL}/Inc/Legacy
  ${BASE}/CMSIS/Include ${BASE}/CMSIS/Device/ST/STM32F4xx/Include)
target_compile_definitions(${PROJECT_NAME} PRIVATE USE_HAL_DRIVER STM32F429xx LAB_STAGE=${LAB_STAGE})
set(CPU -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard)
target_compile_options(${PROJECT_NAME} PRIVATE ${CPU} -ffunction-sections -fdata-sections
  $<$<COMPILE_LANGUAGE:C>:-std=c11;-Wall;-Wextra>
  $<$<CONFIG:Debug>:-Og;-g3> $<$<CONFIG:Release>:-Os>)
target_link_options(${PROJECT_NAME} PRIVATE ${CPU} --specs=nano.specs --specs=nosys.specs
  -T${CMAKE_CURRENT_SOURCE_DIR}/STM32F429ZITX_FLASH.ld
  -Wl,--gc-sections,-Map=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map
  -Wl,--print-memory-usage)
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX .elf)
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY LINK_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/STM32F429ZITX_FLASH.ld)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND arm-none-eabi-objcopy -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
  COMMAND arm-none-eabi-objcopy -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
  COMMAND arm-none-eabi-size $<TARGET_FILE:${PROJECT_NAME}>)
