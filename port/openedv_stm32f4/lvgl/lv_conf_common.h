/**
 * @file    lv_conf_common.h
 * @brief   Shared LVGL configuration for all LVGL apps.
 *
 * Generated from lv_conf_template.h (LVGL v8.3.11): every `#define LV_*` is
 * wrapped in `#ifndef`, so an app can override settings before including this
 * header (mirrors FreeRTOSConfig_common.h / FreeRTOSConfig.h).
 *
 *     #define LV_MEM_ADR 0xC0100000U
 *     #include "lv_conf_common.h"
 */
/**
 * @file lv_conf.h
 * Configuration file for v8.3.11
 */

/*
 * Copy this file as `lv_conf.h`
 * 1. simply next to the `lvgl` folder
 * 2. or any other places and
 *    - define `LV_CONF_INCLUDE_SIMPLE`
 *    - add the path as include path
 */

/* clang-format off */


#ifndef LV_CONF_COMMON_H
#define LV_CONF_COMMON_H

#include <stdint.h>

/* Port tick source (overridable by the app before including this header). */
#ifndef LV_TICK_CUSTOM
#define LV_TICK_CUSTOM 1
#endif
#ifndef LV_TICK_CUSTOM_INCLUDE
#define LV_TICK_CUSTOM_INCLUDE "lv_port.h"
#endif
#ifndef LV_TICK_CUSTOM_SYS_TIME_EXPR
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (lv_port_tick_get())
#endif



/*====================
   COLOR SETTINGS
 *====================*/

/*Color depth: 1 (1 byte per pixel), 8 (RGB332), 16 (RGB565), 32 (ARGB8888)*/
#ifndef LV_COLOR_DEPTH
#define LV_COLOR_DEPTH 16
#endif

/*Swap the 2 bytes of RGB565 color. Useful if the display has an 8-bit interface (e.g. SPI)*/
#ifndef LV_COLOR_16_SWAP
#define LV_COLOR_16_SWAP 0
#endif

/*Enable features to draw on transparent background.
 *It's required if opa, and transform_* style properties are used.
 *Can be also used if the UI is above another layer, e.g. an OSD menu or video player.*/
#ifndef LV_COLOR_SCREEN_TRANSP
#define LV_COLOR_SCREEN_TRANSP 0
#endif

/* Adjust color mix functions rounding. GPUs might calculate color mix (blending) differently.
 * 0: round down, 64: round up from x.75, 128: round up from half, 192: round up from x.25, 254: round up */
#ifndef LV_COLOR_MIX_ROUND_OFS
#define LV_COLOR_MIX_ROUND_OFS 0
#endif

/*Images pixels with this color will not be drawn if they are chroma keyed)*/
#ifndef LV_COLOR_CHROMA_KEY
#define LV_COLOR_CHROMA_KEY lv_color_hex(0x00ff00)         /*pure green*/
#endif

/*=========================
   MEMORY SETTINGS
 *=========================*/

/*1: use custom malloc/free, 0: use the built-in `lv_mem_alloc()` and `lv_mem_free()`*/
#ifndef LV_MEM_CUSTOM
#define LV_MEM_CUSTOM 0
#endif
#if LV_MEM_CUSTOM == 0
    /*Size of the memory available for `lv_mem_alloc()` in bytes (>= 2kB)*/
#ifndef LV_MEM_SIZE
    #define LV_MEM_SIZE (48U * 1024U)          /*[bytes]*/
#endif

    /*Set an address for the memory pool instead of allocating it as a normal array. Can be in external SRAM too.*/
#ifndef LV_MEM_ADR
    #define LV_MEM_ADR 0     /*0: unused*/
#endif
    /*Instead of an address give a memory allocator that will be called to get a memory pool for LVGL. E.g. my_malloc*/
    #if LV_MEM_ADR == 0
        #undef LV_MEM_POOL_INCLUDE
        #undef LV_MEM_POOL_ALLOC
    #endif

#else       /*LV_MEM_CUSTOM*/
#ifndef LV_MEM_CUSTOM_INCLUDE
    #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>   /*Header for the dynamic memory function*/
#endif
#ifndef LV_MEM_CUSTOM_ALLOC
    #define LV_MEM_CUSTOM_ALLOC   malloc
#endif
#ifndef LV_MEM_CUSTOM_FREE
    #define LV_MEM_CUSTOM_FREE    free
#endif
#ifndef LV_MEM_CUSTOM_REALLOC
    #define LV_MEM_CUSTOM_REALLOC realloc
#endif
#endif     /*LV_MEM_CUSTOM*/

/*Number of the intermediate memory buffer used during rendering and other internal processing mechanisms.
 *You will see an error log message if there wasn't enough buffers. */
#ifndef LV_MEM_BUF_MAX_NUM
#define LV_MEM_BUF_MAX_NUM 16
#endif

/*Use the standard `memcpy` and `memset` instead of LVGL's own functions. (Might or might not be faster).*/
#ifndef LV_MEMCPY_MEMSET_STD
#define LV_MEMCPY_MEMSET_STD 0
#endif

/*====================
   HAL SETTINGS
 *====================*/

/*Default display refresh period. LVG will redraw changed areas with this period time*/
#ifndef LV_DISP_DEF_REFR_PERIOD
#define LV_DISP_DEF_REFR_PERIOD 30      /*[ms]*/
#endif

/*Input device read period in milliseconds*/
#ifndef LV_INDEV_DEF_READ_PERIOD
#define LV_INDEV_DEF_READ_PERIOD 30     /*[ms]*/
#endif

/*Use a custom tick source that tells the elapsed time in milliseconds.
 *It removes the need to manually update the tick with `lv_tick_inc()`)*/
#ifndef LV_TICK_CUSTOM
#define LV_TICK_CUSTOM 0
#endif
#if LV_TICK_CUSTOM
#ifndef LV_TICK_CUSTOM_INCLUDE
    #define LV_TICK_CUSTOM_INCLUDE "Arduino.h"         /*Header for the system time function*/
#endif
#ifndef LV_TICK_CUSTOM_SYS_TIME_EXPR
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())    /*Expression evaluating to current system time in ms*/
#endif
    /*If using lvgl as ESP32 component*/
    // #define LV_TICK_CUSTOM_INCLUDE "esp_timer.h"
    // #define LV_TICK_CUSTOM_SYS_TIME_EXPR ((esp_timer_get_time() / 1000LL))
#endif   /*LV_TICK_CUSTOM*/

/*Default Dot Per Inch. Used to initialize default sizes such as widgets sized, style paddings.
 *(Not so important, you can adjust it to modify default sizes and spaces)*/
#ifndef LV_DPI_DEF
#define LV_DPI_DEF 130     /*[px/inch]*/
#endif

/*=======================
 * FEATURE CONFIGURATION
 *=======================*/

/*-------------
 * Drawing
 *-----------*/

/*Enable complex draw engine.
 *Required to draw shadow, gradient, rounded corners, circles, arc, skew lines, image transformations or any masks*/
#ifndef LV_DRAW_COMPLEX
#define LV_DRAW_COMPLEX 1
#endif
#if LV_DRAW_COMPLEX != 0

    /*Allow buffering some shadow calculation.
    *LV_SHADOW_CACHE_SIZE is the max. shadow size to buffer, where shadow size is `shadow_width + radius`
    *Caching has LV_SHADOW_CACHE_SIZE^2 RAM cost*/
#ifndef LV_SHADOW_CACHE_SIZE
    #define LV_SHADOW_CACHE_SIZE 0
#endif

    /* Set number of maximally cached circle data.
    * The circumference of 1/4 circle are saved for anti-aliasing
    * radius * 4 bytes are used per circle (the most often used radiuses are saved)
    * 0: to disable caching */
#ifndef LV_CIRCLE_CACHE_SIZE
    #define LV_CIRCLE_CACHE_SIZE 4
#endif
#endif /*LV_DRAW_COMPLEX*/

/**
 * "Simple layers" are used when a widget has `style_opa < 255` to buffer the widget into a layer
 * and blend it as an image with the given opacity.
 * Note that `bg_opa`, `text_opa` etc don't require buffering into layer)
 * The widget can be buffered in smaller chunks to avoid using large buffers.
 *
 * - LV_LAYER_SIMPLE_BUF_SIZE: [bytes] the optimal target buffer size. LVGL will try to allocate it
 * - LV_LAYER_SIMPLE_FALLBACK_BUF_SIZE: [bytes]  used if `LV_LAYER_SIMPLE_BUF_SIZE` couldn't be allocated.
 *
 * Both buffer sizes are in bytes.
 * "Transformed layers" (where transform_angle/zoom properties are used) use larger buffers
 * and can't be drawn in chunks. So these settings affects only widgets with opacity.
 */
#ifndef LV_LAYER_SIMPLE_BUF_SIZE
#define LV_LAYER_SIMPLE_BUF_SIZE          (24 * 1024)
#endif
#ifndef LV_LAYER_SIMPLE_FALLBACK_BUF_SIZE
#define LV_LAYER_SIMPLE_FALLBACK_BUF_SIZE (3 * 1024)
#endif

/*Default image cache size. Image caching keeps the images opened.
 *If only the built-in image formats are used there is no real advantage of caching. (I.e. if no new image decoder is added)
 *With complex image decoders (e.g. PNG or JPG) caching can save the continuous open/decode of images.
 *However the opened images might consume additional RAM.
 *0: to disable caching*/
#ifndef LV_IMG_CACHE_DEF_SIZE
#define LV_IMG_CACHE_DEF_SIZE 0
#endif

/*Number of stops allowed per gradient. Increase this to allow more stops.
 *This adds (sizeof(lv_color_t) + 1) bytes per additional stop*/
#ifndef LV_GRADIENT_MAX_STOPS
#define LV_GRADIENT_MAX_STOPS 2
#endif

/*Default gradient buffer size.
 *When LVGL calculates the gradient "maps" it can save them into a cache to avoid calculating them again.
 *LV_GRAD_CACHE_DEF_SIZE sets the size of this cache in bytes.
 *If the cache is too small the map will be allocated only while it's required for the drawing.
 *0 mean no caching.*/
#ifndef LV_GRAD_CACHE_DEF_SIZE
#define LV_GRAD_CACHE_DEF_SIZE 0
#endif

/*Allow dithering the gradients (to achieve visual smooth color gradients on limited color depth display)
 *LV_DITHER_GRADIENT implies allocating one or two more lines of the object's rendering surface
 *The increase in memory consumption is (32 bits * object width) plus 24 bits * object width if using error diffusion */
#ifndef LV_DITHER_GRADIENT
#define LV_DITHER_GRADIENT 0
#endif
#if LV_DITHER_GRADIENT
    /*Add support for error diffusion dithering.
     *Error diffusion dithering gets a much better visual result, but implies more CPU consumption and memory when drawing.
     *The increase in memory consumption is (24 bits * object's width)*/
#ifndef LV_DITHER_ERROR_DIFFUSION
    #define LV_DITHER_ERROR_DIFFUSION 0
#endif
#endif

/*Maximum buffer size to allocate for rotation.
 *Only used if software rotation is enabled in the display driver.*/
#ifndef LV_DISP_ROT_MAX_BUF
#define LV_DISP_ROT_MAX_BUF (10*1024)
#endif

/*-------------
 * GPU
 *-----------*/

/*Use Arm's 2D acceleration library Arm-2D */
#ifndef LV_USE_GPU_ARM2D
#define LV_USE_GPU_ARM2D 0
#endif

/*Use STM32's DMA2D (aka Chrom Art) GPU*/
#ifndef LV_USE_GPU_STM32_DMA2D
#define LV_USE_GPU_STM32_DMA2D 0
#endif
#if LV_USE_GPU_STM32_DMA2D
    /*Must be defined to include path of CMSIS header of target processor
    e.g. "stm32f7xx.h" or "stm32f4xx.h"*/
#ifndef LV_GPU_DMA2D_CMSIS_INCLUDE
    #define LV_GPU_DMA2D_CMSIS_INCLUDE
#endif
#endif

/*Enable RA6M3 G2D GPU*/
#ifndef LV_USE_GPU_RA6M3_G2D
#define LV_USE_GPU_RA6M3_G2D 0
#endif
#if LV_USE_GPU_RA6M3_G2D
    /*include path of target processor
    e.g. "hal_data.h"*/
#ifndef LV_GPU_RA6M3_G2D_INCLUDE
    #define LV_GPU_RA6M3_G2D_INCLUDE "hal_data.h"
#endif
#endif

/*Use SWM341's DMA2D GPU*/
#ifndef LV_USE_GPU_SWM341_DMA2D
#define LV_USE_GPU_SWM341_DMA2D 0
#endif
#if LV_USE_GPU_SWM341_DMA2D
#ifndef LV_GPU_SWM341_DMA2D_INCLUDE
    #define LV_GPU_SWM341_DMA2D_INCLUDE "SWM341.h"
#endif
#endif

/*Use NXP's PXP GPU iMX RTxxx platforms*/
#ifndef LV_USE_GPU_NXP_PXP
#define LV_USE_GPU_NXP_PXP 0
#endif
#if LV_USE_GPU_NXP_PXP
    /*1: Add default bare metal and FreeRTOS interrupt handling routines for PXP (lv_gpu_nxp_pxp_osa.c)
    *   and call lv_gpu_nxp_pxp_init() automatically during lv_init(). Note that symbol SDK_OS_FREE_RTOS
    *   has to be defined in order to use FreeRTOS OSA, otherwise bare-metal implementation is selected.
    *0: lv_gpu_nxp_pxp_init() has to be called manually before lv_init()
    */
#ifndef LV_USE_GPU_NXP_PXP_AUTO_INIT
    #define LV_USE_GPU_NXP_PXP_AUTO_INIT 0
#endif
#endif

/*Use NXP's VG-Lite GPU iMX RTxxx platforms*/
#ifndef LV_USE_GPU_NXP_VG_LITE
#define LV_USE_GPU_NXP_VG_LITE 0
#endif

/*Use SDL renderer API*/
#ifndef LV_USE_GPU_SDL
#define LV_USE_GPU_SDL 0
#endif
#if LV_USE_GPU_SDL
#ifndef LV_GPU_SDL_INCLUDE_PATH
    #define LV_GPU_SDL_INCLUDE_PATH <SDL2/SDL.h>
#endif
    /*Texture cache size, 8MB by default*/
#ifndef LV_GPU_SDL_LRU_SIZE
    #define LV_GPU_SDL_LRU_SIZE (1024 * 1024 * 8)
#endif
    /*Custom blend mode for mask drawing, disable if you need to link with older SDL2 lib*/
#ifndef LV_GPU_SDL_CUSTOM_BLEND_MODE
    #define LV_GPU_SDL_CUSTOM_BLEND_MODE (SDL_VERSION_ATLEAST(2, 0, 6))
#endif
#endif

/*-------------
 * Logging
 *-----------*/

/*Enable the log module*/
#ifndef LV_USE_LOG
#define LV_USE_LOG 0
#endif
#if LV_USE_LOG

    /*How important log should be added:
    *LV_LOG_LEVEL_TRACE       A lot of logs to give detailed information
    *LV_LOG_LEVEL_INFO        Log important events
    *LV_LOG_LEVEL_WARN        Log if something unwanted happened but didn't cause a problem
    *LV_LOG_LEVEL_ERROR       Only critical issue, when the system may fail
    *LV_LOG_LEVEL_USER        Only logs added by the user
    *LV_LOG_LEVEL_NONE        Do not log anything*/
#ifndef LV_LOG_LEVEL
    #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#endif

    /*1: Print the log with 'printf';
    *0: User need to register a callback with `lv_log_register_print_cb()`*/
#ifndef LV_LOG_PRINTF
    #define LV_LOG_PRINTF 0
#endif

    /*Enable/disable LV_LOG_TRACE in modules that produces a huge number of logs*/
#ifndef LV_LOG_TRACE_MEM
    #define LV_LOG_TRACE_MEM        1
#endif
#ifndef LV_LOG_TRACE_TIMER
    #define LV_LOG_TRACE_TIMER      1
#endif
#ifndef LV_LOG_TRACE_INDEV
    #define LV_LOG_TRACE_INDEV      1
#endif
#ifndef LV_LOG_TRACE_DISP_REFR
    #define LV_LOG_TRACE_DISP_REFR  1
#endif
#ifndef LV_LOG_TRACE_EVENT
    #define LV_LOG_TRACE_EVENT      1
#endif
#ifndef LV_LOG_TRACE_OBJ_CREATE
    #define LV_LOG_TRACE_OBJ_CREATE 1
#endif
#ifndef LV_LOG_TRACE_LAYOUT
    #define LV_LOG_TRACE_LAYOUT     1
#endif
#ifndef LV_LOG_TRACE_ANIM
    #define LV_LOG_TRACE_ANIM       1
#endif

#endif  /*LV_USE_LOG*/

/*-------------
 * Asserts
 *-----------*/

/*Enable asserts if an operation is failed or an invalid data is found.
 *If LV_USE_LOG is enabled an error message will be printed on failure*/
#ifndef LV_USE_ASSERT_NULL
#define LV_USE_ASSERT_NULL          1   /*Check if the parameter is NULL. (Very fast, recommended)*/
#endif
#ifndef LV_USE_ASSERT_MALLOC
#define LV_USE_ASSERT_MALLOC        1   /*Checks is the memory is successfully allocated or no. (Very fast, recommended)*/
#endif
#ifndef LV_USE_ASSERT_STYLE
#define LV_USE_ASSERT_STYLE         0   /*Check if the styles are properly initialized. (Very fast, recommended)*/
#endif
#ifndef LV_USE_ASSERT_MEM_INTEGRITY
#define LV_USE_ASSERT_MEM_INTEGRITY 0   /*Check the integrity of `lv_mem` after critical operations. (Slow)*/
#endif
#ifndef LV_USE_ASSERT_OBJ
#define LV_USE_ASSERT_OBJ           0   /*Check the object's type and existence (e.g. not deleted). (Slow)*/
#endif

/*Add a custom handler when assert happens e.g. to restart the MCU*/
#ifndef LV_ASSERT_HANDLER_INCLUDE
#define LV_ASSERT_HANDLER_INCLUDE <stdint.h>
#endif
#ifndef LV_ASSERT_HANDLER
#define LV_ASSERT_HANDLER while(1);   /*Halt by default*/
#endif

/*-------------
 * Others
 *-----------*/

/*1: Show CPU usage and FPS count*/
#ifndef LV_USE_PERF_MONITOR
#define LV_USE_PERF_MONITOR 0
#endif
#if LV_USE_PERF_MONITOR
#ifndef LV_USE_PERF_MONITOR_POS
    #define LV_USE_PERF_MONITOR_POS LV_ALIGN_BOTTOM_RIGHT
#endif
#endif

/*1: Show the used memory and the memory fragmentation
 * Requires LV_MEM_CUSTOM = 0*/
#ifndef LV_USE_MEM_MONITOR
#define LV_USE_MEM_MONITOR 0
#endif
#if LV_USE_MEM_MONITOR
#ifndef LV_USE_MEM_MONITOR_POS
    #define LV_USE_MEM_MONITOR_POS LV_ALIGN_BOTTOM_LEFT
#endif
#endif

/*1: Draw random colored rectangles over the redrawn areas*/
#ifndef LV_USE_REFR_DEBUG
#define LV_USE_REFR_DEBUG 0
#endif

/*Change the built in (v)snprintf functions*/
#ifndef LV_SPRINTF_CUSTOM
#define LV_SPRINTF_CUSTOM 0
#endif
#if LV_SPRINTF_CUSTOM
#ifndef LV_SPRINTF_INCLUDE
    #define LV_SPRINTF_INCLUDE <stdio.h>
#endif
    #define lv_snprintf  snprintf
    #define lv_vsnprintf vsnprintf
#else   /*LV_SPRINTF_CUSTOM*/
#ifndef LV_SPRINTF_USE_FLOAT
    #define LV_SPRINTF_USE_FLOAT 0
#endif
#endif  /*LV_SPRINTF_CUSTOM*/

#ifndef LV_USE_USER_DATA
#define LV_USE_USER_DATA 1
#endif

/*Garbage Collector settings
 *Used if lvgl is bound to higher level language and the memory is managed by that language*/
#ifndef LV_ENABLE_GC
#define LV_ENABLE_GC 0
#endif
#if LV_ENABLE_GC != 0
#ifndef LV_GC_INCLUDE
    #define LV_GC_INCLUDE "gc.h"                           /*Include Garbage Collector related things*/
#endif
#endif /*LV_ENABLE_GC*/

/*=====================
 *  COMPILER SETTINGS
 *====================*/

/*For big endian systems set to 1*/
#ifndef LV_BIG_ENDIAN_SYSTEM
#define LV_BIG_ENDIAN_SYSTEM 0
#endif

/*Define a custom attribute to `lv_tick_inc` function*/
#ifndef LV_ATTRIBUTE_TICK_INC
#define LV_ATTRIBUTE_TICK_INC
#endif

/*Define a custom attribute to `lv_timer_handler` function*/
#ifndef LV_ATTRIBUTE_TIMER_HANDLER
#define LV_ATTRIBUTE_TIMER_HANDLER
#endif

/*Define a custom attribute to `lv_disp_flush_ready` function*/
#ifndef LV_ATTRIBUTE_FLUSH_READY
#define LV_ATTRIBUTE_FLUSH_READY
#endif

/*Required alignment size for buffers*/
#ifndef LV_ATTRIBUTE_MEM_ALIGN_SIZE
#define LV_ATTRIBUTE_MEM_ALIGN_SIZE 1
#endif

/*Will be added where memories needs to be aligned (with -Os data might not be aligned to boundary by default).
 * E.g. __attribute__((aligned(4)))*/
#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

/*Attribute to mark large constant arrays for example font's bitmaps*/
#ifndef LV_ATTRIBUTE_LARGE_CONST
#define LV_ATTRIBUTE_LARGE_CONST
#endif

/*Compiler prefix for a big array declaration in RAM*/
#ifndef LV_ATTRIBUTE_LARGE_RAM_ARRAY
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY
#endif

/*Place performance critical functions into a faster memory (e.g RAM)*/
#ifndef LV_ATTRIBUTE_FAST_MEM
#define LV_ATTRIBUTE_FAST_MEM
#endif

/*Prefix variables that are used in GPU accelerated operations, often these need to be placed in RAM sections that are DMA accessible*/
#ifndef LV_ATTRIBUTE_DMA
#define LV_ATTRIBUTE_DMA
#endif

/*Export integer constant to binding. This macro is used with constants in the form of LV_<CONST> that
 *should also appear on LVGL binding API such as Micropython.*/
#ifndef LV_EXPORT_CONST_INT
#define LV_EXPORT_CONST_INT(int_value) struct _silence_gcc_warning /*The default value just prevents GCC warning*/
#endif

/*Extend the default -32k..32k coordinate range to -4M..4M by using int32_t for coordinates instead of int16_t*/
#ifndef LV_USE_LARGE_COORD
#define LV_USE_LARGE_COORD 0
#endif

/*==================
 *   FONT USAGE
 *===================*/

/*Montserrat fonts with ASCII range and some symbols using bpp = 4
 *https://fonts.google.com/specimen/Montserrat*/
#ifndef LV_FONT_MONTSERRAT_8
#define LV_FONT_MONTSERRAT_8  0
#endif
#ifndef LV_FONT_MONTSERRAT_10
#define LV_FONT_MONTSERRAT_10 0
#endif
#ifndef LV_FONT_MONTSERRAT_12
#define LV_FONT_MONTSERRAT_12 0
#endif
#ifndef LV_FONT_MONTSERRAT_14
#define LV_FONT_MONTSERRAT_14 1
#endif
#ifndef LV_FONT_MONTSERRAT_16
#define LV_FONT_MONTSERRAT_16 0
#endif
#ifndef LV_FONT_MONTSERRAT_18
#define LV_FONT_MONTSERRAT_18 0
#endif
#ifndef LV_FONT_MONTSERRAT_20
#define LV_FONT_MONTSERRAT_20 0
#endif
#ifndef LV_FONT_MONTSERRAT_22
#define LV_FONT_MONTSERRAT_22 0
#endif
#ifndef LV_FONT_MONTSERRAT_24
#define LV_FONT_MONTSERRAT_24 0
#endif
#ifndef LV_FONT_MONTSERRAT_26
#define LV_FONT_MONTSERRAT_26 0
#endif
#ifndef LV_FONT_MONTSERRAT_28
#define LV_FONT_MONTSERRAT_28 0
#endif
#ifndef LV_FONT_MONTSERRAT_30
#define LV_FONT_MONTSERRAT_30 0
#endif
#ifndef LV_FONT_MONTSERRAT_32
#define LV_FONT_MONTSERRAT_32 0
#endif
#ifndef LV_FONT_MONTSERRAT_34
#define LV_FONT_MONTSERRAT_34 0
#endif
#ifndef LV_FONT_MONTSERRAT_36
#define LV_FONT_MONTSERRAT_36 0
#endif
#ifndef LV_FONT_MONTSERRAT_38
#define LV_FONT_MONTSERRAT_38 0
#endif
#ifndef LV_FONT_MONTSERRAT_40
#define LV_FONT_MONTSERRAT_40 0
#endif
#ifndef LV_FONT_MONTSERRAT_42
#define LV_FONT_MONTSERRAT_42 0
#endif
#ifndef LV_FONT_MONTSERRAT_44
#define LV_FONT_MONTSERRAT_44 0
#endif
#ifndef LV_FONT_MONTSERRAT_46
#define LV_FONT_MONTSERRAT_46 0
#endif
#ifndef LV_FONT_MONTSERRAT_48
#define LV_FONT_MONTSERRAT_48 0
#endif

/*Demonstrate special features*/
#ifndef LV_FONT_MONTSERRAT_12_SUBPX
#define LV_FONT_MONTSERRAT_12_SUBPX      0
#endif
#ifndef LV_FONT_MONTSERRAT_28_COMPRESSED
#define LV_FONT_MONTSERRAT_28_COMPRESSED 0  /*bpp = 3*/
#endif
#ifndef LV_FONT_DEJAVU_16_PERSIAN_HEBREW
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW 0  /*Hebrew, Arabic, Persian letters and all their forms*/
#endif
#ifndef LV_FONT_SIMSUN_16_CJK
#define LV_FONT_SIMSUN_16_CJK            0  /*1000 most common CJK radicals*/
#endif

/*Pixel perfect monospace fonts*/
#ifndef LV_FONT_UNSCII_8
#define LV_FONT_UNSCII_8  0
#endif
#ifndef LV_FONT_UNSCII_16
#define LV_FONT_UNSCII_16 0
#endif

/*Optionally declare custom fonts here.
 *You can use these fonts as default font too and they will be available globally.
 *E.g. #define LV_FONT_CUSTOM_DECLARE   LV_FONT_DECLARE(my_font_1) LV_FONT_DECLARE(my_font_2)*/
#ifndef LV_FONT_CUSTOM_DECLARE
#define LV_FONT_CUSTOM_DECLARE
#endif

/*Always set a default font*/
#ifndef LV_FONT_DEFAULT
#define LV_FONT_DEFAULT &lv_font_montserrat_14
#endif

/*Enable handling large font and/or fonts with a lot of characters.
 *The limit depends on the font size, font face and bpp.
 *Compiler error will be triggered if a font needs it.*/
#ifndef LV_FONT_FMT_TXT_LARGE
#define LV_FONT_FMT_TXT_LARGE 0
#endif

/*Enables/disables support for compressed fonts.*/
#ifndef LV_USE_FONT_COMPRESSED
#define LV_USE_FONT_COMPRESSED 0
#endif

/*Enable subpixel rendering*/
#ifndef LV_USE_FONT_SUBPX
#define LV_USE_FONT_SUBPX 0
#endif
#if LV_USE_FONT_SUBPX
    /*Set the pixel order of the display. Physical order of RGB channels. Doesn't matter with "normal" fonts.*/
#ifndef LV_FONT_SUBPX_BGR
    #define LV_FONT_SUBPX_BGR 0  /*0: RGB; 1:BGR order*/
#endif
#endif

/*Enable drawing placeholders when glyph dsc is not found*/
#ifndef LV_USE_FONT_PLACEHOLDER
#define LV_USE_FONT_PLACEHOLDER 1
#endif

/*=================
 *  TEXT SETTINGS
 *=================*/

/**
 * Select a character encoding for strings.
 * Your IDE or editor should have the same character encoding
 * - LV_TXT_ENC_UTF8
 * - LV_TXT_ENC_ASCII
 */
#ifndef LV_TXT_ENC
#define LV_TXT_ENC LV_TXT_ENC_UTF8
#endif

/*Can break (wrap) texts on these chars*/
#ifndef LV_TXT_BREAK_CHARS
#define LV_TXT_BREAK_CHARS " ,.;:-_"
#endif

/*If a word is at least this long, will break wherever "prettiest"
 *To disable, set to a value <= 0*/
#ifndef LV_TXT_LINE_BREAK_LONG_LEN
#define LV_TXT_LINE_BREAK_LONG_LEN 0
#endif

/*Minimum number of characters in a long word to put on a line before a break.
 *Depends on LV_TXT_LINE_BREAK_LONG_LEN.*/
#ifndef LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN 3
#endif

/*Minimum number of characters in a long word to put on a line after a break.
 *Depends on LV_TXT_LINE_BREAK_LONG_LEN.*/
#ifndef LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3
#endif

/*The control character to use for signalling text recoloring.*/
#ifndef LV_TXT_COLOR_CMD
#define LV_TXT_COLOR_CMD "#"
#endif

/*Support bidirectional texts. Allows mixing Left-to-Right and Right-to-Left texts.
 *The direction will be processed according to the Unicode Bidirectional Algorithm:
 *https://www.w3.org/International/articles/inline-bidi-markup/uba-basics*/
#ifndef LV_USE_BIDI
#define LV_USE_BIDI 0
#endif
#if LV_USE_BIDI
    /*Set the default direction. Supported values:
    *`LV_BASE_DIR_LTR` Left-to-Right
    *`LV_BASE_DIR_RTL` Right-to-Left
    *`LV_BASE_DIR_AUTO` detect texts base direction*/
#ifndef LV_BIDI_BASE_DIR_DEF
    #define LV_BIDI_BASE_DIR_DEF LV_BASE_DIR_AUTO
#endif
#endif

/*Enable Arabic/Persian processing
 *In these languages characters should be replaced with an other form based on their position in the text*/
#ifndef LV_USE_ARABIC_PERSIAN_CHARS
#define LV_USE_ARABIC_PERSIAN_CHARS 0
#endif

/*==================
 *  WIDGET USAGE
 *================*/

/*Documentation of the widgets: https://docs.lvgl.io/latest/en/html/widgets/index.html*/

#ifndef LV_USE_ARC
#define LV_USE_ARC        1
#endif

#ifndef LV_USE_BAR
#define LV_USE_BAR        1
#endif

#ifndef LV_USE_BTN
#define LV_USE_BTN        1
#endif

#ifndef LV_USE_BTNMATRIX
#define LV_USE_BTNMATRIX  1
#endif

#ifndef LV_USE_CANVAS
#define LV_USE_CANVAS     1
#endif

#ifndef LV_USE_CHECKBOX
#define LV_USE_CHECKBOX   1
#endif

#ifndef LV_USE_DROPDOWN
#define LV_USE_DROPDOWN   1   /*Requires: lv_label*/
#endif

#ifndef LV_USE_IMG
#define LV_USE_IMG        1   /*Requires: lv_label*/
#endif

#ifndef LV_USE_LABEL
#define LV_USE_LABEL      1
#endif
#if LV_USE_LABEL
#ifndef LV_LABEL_TEXT_SELECTION
    #define LV_LABEL_TEXT_SELECTION 1 /*Enable selecting text of the label*/
#endif
#ifndef LV_LABEL_LONG_TXT_HINT
    #define LV_LABEL_LONG_TXT_HINT 1  /*Store some extra info in labels to speed up drawing of very long texts*/
#endif
#endif

#ifndef LV_USE_LINE
#define LV_USE_LINE       1
#endif

#ifndef LV_USE_ROLLER
#define LV_USE_ROLLER     1   /*Requires: lv_label*/
#endif
#if LV_USE_ROLLER
#ifndef LV_ROLLER_INF_PAGES
    #define LV_ROLLER_INF_PAGES 7 /*Number of extra "pages" when the roller is infinite*/
#endif
#endif

#ifndef LV_USE_SLIDER
#define LV_USE_SLIDER     1   /*Requires: lv_bar*/
#endif

#ifndef LV_USE_SWITCH
#define LV_USE_SWITCH     1
#endif

#ifndef LV_USE_TEXTAREA
#define LV_USE_TEXTAREA   1   /*Requires: lv_label*/
#endif
#if LV_USE_TEXTAREA != 0
#ifndef LV_TEXTAREA_DEF_PWD_SHOW_TIME
    #define LV_TEXTAREA_DEF_PWD_SHOW_TIME 1500    /*ms*/
#endif
#endif

#ifndef LV_USE_TABLE
#define LV_USE_TABLE      1
#endif

/*==================
 * EXTRA COMPONENTS
 *==================*/

/*-----------
 * Widgets
 *----------*/
#ifndef LV_USE_ANIMIMG
#define LV_USE_ANIMIMG    1
#endif

#ifndef LV_USE_CALENDAR
#define LV_USE_CALENDAR   1
#endif
#if LV_USE_CALENDAR
#ifndef LV_CALENDAR_WEEK_STARTS_MONDAY
    #define LV_CALENDAR_WEEK_STARTS_MONDAY 0
#endif
    #if LV_CALENDAR_WEEK_STARTS_MONDAY
#ifndef LV_CALENDAR_DEFAULT_DAY_NAMES
        #define LV_CALENDAR_DEFAULT_DAY_NAMES {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"}
#endif
    #else
#ifndef LV_CALENDAR_DEFAULT_DAY_NAMES
        #define LV_CALENDAR_DEFAULT_DAY_NAMES {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"}
#endif
    #endif

#ifndef LV_CALENDAR_DEFAULT_MONTH_NAMES
    #define LV_CALENDAR_DEFAULT_MONTH_NAMES {"January", "February", "March",  "April", "May",  "June", "July", "August", "September", "October", "November", "December"}
#endif
#ifndef LV_USE_CALENDAR_HEADER_ARROW
    #define LV_USE_CALENDAR_HEADER_ARROW 1
#endif
#ifndef LV_USE_CALENDAR_HEADER_DROPDOWN
    #define LV_USE_CALENDAR_HEADER_DROPDOWN 1
#endif
#endif  /*LV_USE_CALENDAR*/

#ifndef LV_USE_CHART
#define LV_USE_CHART      1
#endif

#ifndef LV_USE_COLORWHEEL
#define LV_USE_COLORWHEEL 1
#endif

#ifndef LV_USE_IMGBTN
#define LV_USE_IMGBTN     1
#endif

#ifndef LV_USE_KEYBOARD
#define LV_USE_KEYBOARD   1
#endif

#ifndef LV_USE_LED
#define LV_USE_LED        1
#endif

#ifndef LV_USE_LIST
#define LV_USE_LIST       1
#endif

#ifndef LV_USE_MENU
#define LV_USE_MENU       1
#endif

#ifndef LV_USE_METER
#define LV_USE_METER      1
#endif

#ifndef LV_USE_MSGBOX
#define LV_USE_MSGBOX     1
#endif

#ifndef LV_USE_SPAN
#define LV_USE_SPAN       1
#endif
#if LV_USE_SPAN
    /*A line text can contain maximum num of span descriptor */
#ifndef LV_SPAN_SNIPPET_STACK_SIZE
    #define LV_SPAN_SNIPPET_STACK_SIZE 64
#endif
#endif

#ifndef LV_USE_SPINBOX
#define LV_USE_SPINBOX    1
#endif

#ifndef LV_USE_SPINNER
#define LV_USE_SPINNER    1
#endif

#ifndef LV_USE_TABVIEW
#define LV_USE_TABVIEW    1
#endif

#ifndef LV_USE_TILEVIEW
#define LV_USE_TILEVIEW   1
#endif

#ifndef LV_USE_WIN
#define LV_USE_WIN        1
#endif

/*-----------
 * Themes
 *----------*/

/*A simple, impressive and very complete theme*/
#ifndef LV_USE_THEME_DEFAULT
#define LV_USE_THEME_DEFAULT 1
#endif
#if LV_USE_THEME_DEFAULT

    /*0: Light mode; 1: Dark mode*/
#ifndef LV_THEME_DEFAULT_DARK
    #define LV_THEME_DEFAULT_DARK 0
#endif

    /*1: Enable grow on press*/
#ifndef LV_THEME_DEFAULT_GROW
    #define LV_THEME_DEFAULT_GROW 1
#endif

    /*Default transition time in [ms]*/
#ifndef LV_THEME_DEFAULT_TRANSITION_TIME
    #define LV_THEME_DEFAULT_TRANSITION_TIME 80
#endif
#endif /*LV_USE_THEME_DEFAULT*/

/*A very simple theme that is a good starting point for a custom theme*/
#ifndef LV_USE_THEME_BASIC
#define LV_USE_THEME_BASIC 1
#endif

/*A theme designed for monochrome displays*/
#ifndef LV_USE_THEME_MONO
#define LV_USE_THEME_MONO 1
#endif

/*-----------
 * Layouts
 *----------*/

/*A layout similar to Flexbox in CSS.*/
#ifndef LV_USE_FLEX
#define LV_USE_FLEX 1
#endif

/*A layout similar to Grid in CSS.*/
#ifndef LV_USE_GRID
#define LV_USE_GRID 1
#endif

/*---------------------
 * 3rd party libraries
 *--------------------*/

/*File system interfaces for common APIs */

/*API for fopen, fread, etc*/
#ifndef LV_USE_FS_STDIO
#define LV_USE_FS_STDIO 0
#endif
#if LV_USE_FS_STDIO
#ifndef LV_FS_STDIO_LETTER
    #define LV_FS_STDIO_LETTER '\0'     /*Set an upper cased letter on which the drive will accessible (e.g. 'A')*/
#endif
#ifndef LV_FS_STDIO_PATH
    #define LV_FS_STDIO_PATH ""         /*Set the working directory. File/directory paths will be appended to it.*/
#endif
#ifndef LV_FS_STDIO_CACHE_SIZE
    #define LV_FS_STDIO_CACHE_SIZE 0    /*>0 to cache this number of bytes in lv_fs_read()*/
#endif
#endif

/*API for open, read, etc*/
#ifndef LV_USE_FS_POSIX
#define LV_USE_FS_POSIX 0
#endif
#if LV_USE_FS_POSIX
#ifndef LV_FS_POSIX_LETTER
    #define LV_FS_POSIX_LETTER '\0'     /*Set an upper cased letter on which the drive will accessible (e.g. 'A')*/
#endif
#ifndef LV_FS_POSIX_PATH
    #define LV_FS_POSIX_PATH ""         /*Set the working directory. File/directory paths will be appended to it.*/
#endif
#ifndef LV_FS_POSIX_CACHE_SIZE
    #define LV_FS_POSIX_CACHE_SIZE 0    /*>0 to cache this number of bytes in lv_fs_read()*/
#endif
#endif

/*API for CreateFile, ReadFile, etc*/
#ifndef LV_USE_FS_WIN32
#define LV_USE_FS_WIN32 0
#endif
#if LV_USE_FS_WIN32
#ifndef LV_FS_WIN32_LETTER
    #define LV_FS_WIN32_LETTER '\0'     /*Set an upper cased letter on which the drive will accessible (e.g. 'A')*/
#endif
#ifndef LV_FS_WIN32_PATH
    #define LV_FS_WIN32_PATH ""         /*Set the working directory. File/directory paths will be appended to it.*/
#endif
#ifndef LV_FS_WIN32_CACHE_SIZE
    #define LV_FS_WIN32_CACHE_SIZE 0    /*>0 to cache this number of bytes in lv_fs_read()*/
#endif
#endif

/*API for FATFS (needs to be added separately). Uses f_open, f_read, etc*/
#ifndef LV_USE_FS_FATFS
#define LV_USE_FS_FATFS 0
#endif
#if LV_USE_FS_FATFS
#ifndef LV_FS_FATFS_LETTER
    #define LV_FS_FATFS_LETTER '\0'     /*Set an upper cased letter on which the drive will accessible (e.g. 'A')*/
#endif
#ifndef LV_FS_FATFS_CACHE_SIZE
    #define LV_FS_FATFS_CACHE_SIZE 0    /*>0 to cache this number of bytes in lv_fs_read()*/
#endif
#endif

/*API for LittleFS (library needs to be added separately). Uses lfs_file_open, lfs_file_read, etc*/
#ifndef LV_USE_FS_LITTLEFS
#define LV_USE_FS_LITTLEFS 0
#endif
#if LV_USE_FS_LITTLEFS
#ifndef LV_FS_LITTLEFS_LETTER
    #define LV_FS_LITTLEFS_LETTER '\0'     /*Set an upper cased letter on which the drive will accessible (e.g. 'A')*/
#endif
#ifndef LV_FS_LITTLEFS_CACHE_SIZE
    #define LV_FS_LITTLEFS_CACHE_SIZE 0    /*>0 to cache this number of bytes in lv_fs_read()*/
#endif
#endif

/*PNG decoder library*/
#ifndef LV_USE_PNG
#define LV_USE_PNG 0
#endif

/*BMP decoder library*/
#ifndef LV_USE_BMP
#define LV_USE_BMP 0
#endif

/* JPG + split JPG decoder library.
 * Split JPG is a custom format optimized for embedded systems. */
#ifndef LV_USE_SJPG
#define LV_USE_SJPG 0
#endif

/*GIF decoder library*/
#ifndef LV_USE_GIF
#define LV_USE_GIF 0
#endif

/*QR code library*/
#ifndef LV_USE_QRCODE
#define LV_USE_QRCODE 0
#endif

/*FreeType library*/
#ifndef LV_USE_FREETYPE
#define LV_USE_FREETYPE 0
#endif
#if LV_USE_FREETYPE
    /*Memory used by FreeType to cache characters [bytes] (-1: no caching)*/
#ifndef LV_FREETYPE_CACHE_SIZE
    #define LV_FREETYPE_CACHE_SIZE (16 * 1024)
#endif
    #if LV_FREETYPE_CACHE_SIZE >= 0
        /* 1: bitmap cache use the sbit cache, 0:bitmap cache use the image cache. */
        /* sbit cache:it is much more memory efficient for small bitmaps(font size < 256) */
        /* if font size >= 256, must be configured as image cache */
#ifndef LV_FREETYPE_SBIT_CACHE
        #define LV_FREETYPE_SBIT_CACHE 0
#endif
        /* Maximum number of opened FT_Face/FT_Size objects managed by this cache instance. */
        /* (0:use system defaults) */
#ifndef LV_FREETYPE_CACHE_FT_FACES
        #define LV_FREETYPE_CACHE_FT_FACES 0
#endif
#ifndef LV_FREETYPE_CACHE_FT_SIZES
        #define LV_FREETYPE_CACHE_FT_SIZES 0
#endif
    #endif
#endif

/*Tiny TTF library*/
#ifndef LV_USE_TINY_TTF
#define LV_USE_TINY_TTF 0
#endif
#if LV_USE_TINY_TTF
    /*Load TTF data from files*/
#ifndef LV_TINY_TTF_FILE_SUPPORT
    #define LV_TINY_TTF_FILE_SUPPORT 0
#endif
#endif

/*Rlottie library*/
#ifndef LV_USE_RLOTTIE
#define LV_USE_RLOTTIE 0
#endif

/*FFmpeg library for image decoding and playing videos
 *Supports all major image formats so do not enable other image decoder with it*/
#ifndef LV_USE_FFMPEG
#define LV_USE_FFMPEG 0
#endif
#if LV_USE_FFMPEG
    /*Dump input information to stderr*/
#ifndef LV_FFMPEG_DUMP_FORMAT
    #define LV_FFMPEG_DUMP_FORMAT 0
#endif
#endif

/*-----------
 * Others
 *----------*/

/*1: Enable API to take snapshot for object*/
#ifndef LV_USE_SNAPSHOT
#define LV_USE_SNAPSHOT 0
#endif

/*1: Enable Monkey test*/
#ifndef LV_USE_MONKEY
#define LV_USE_MONKEY 0
#endif

/*1: Enable grid navigation*/
#ifndef LV_USE_GRIDNAV
#define LV_USE_GRIDNAV 0
#endif

/*1: Enable lv_obj fragment*/
#ifndef LV_USE_FRAGMENT
#define LV_USE_FRAGMENT 0
#endif

/*1: Support using images as font in label or span widgets */
#ifndef LV_USE_IMGFONT
#define LV_USE_IMGFONT 0
#endif

/*1: Enable a published subscriber based messaging system */
#ifndef LV_USE_MSG
#define LV_USE_MSG 0
#endif

/*1: Enable Pinyin input method*/
/*Requires: lv_keyboard*/
#ifndef LV_USE_IME_PINYIN
#define LV_USE_IME_PINYIN 0
#endif
#if LV_USE_IME_PINYIN
    /*1: Use default thesaurus*/
    /*If you do not use the default thesaurus, be sure to use `lv_ime_pinyin` after setting the thesauruss*/
#ifndef LV_IME_PINYIN_USE_DEFAULT_DICT
    #define LV_IME_PINYIN_USE_DEFAULT_DICT 1
#endif
    /*Set the maximum number of candidate panels that can be displayed*/
    /*This needs to be adjusted according to the size of the screen*/
#ifndef LV_IME_PINYIN_CAND_TEXT_NUM
    #define LV_IME_PINYIN_CAND_TEXT_NUM 6
#endif

    /*Use 9 key input(k9)*/
#ifndef LV_IME_PINYIN_USE_K9_MODE
    #define LV_IME_PINYIN_USE_K9_MODE      1
#endif
    #if LV_IME_PINYIN_USE_K9_MODE == 1
#ifndef LV_IME_PINYIN_K9_CAND_TEXT_NUM
        #define LV_IME_PINYIN_K9_CAND_TEXT_NUM 3
#endif
    #endif // LV_IME_PINYIN_USE_K9_MODE
#endif

/*==================
* EXAMPLES
*==================*/

/*Enable the examples to be built with the library*/
#ifndef LV_BUILD_EXAMPLES
#define LV_BUILD_EXAMPLES 1
#endif

/*===================
 * DEMO USAGE
 ====================*/

/*Show some widget. It might be required to increase `LV_MEM_SIZE` */
#ifndef LV_USE_DEMO_WIDGETS
#define LV_USE_DEMO_WIDGETS 0
#endif
#if LV_USE_DEMO_WIDGETS
#ifndef LV_DEMO_WIDGETS_SLIDESHOW
#define LV_DEMO_WIDGETS_SLIDESHOW 0
#endif
#endif

/*Demonstrate the usage of encoder and keyboard*/
#ifndef LV_USE_DEMO_KEYPAD_AND_ENCODER
#define LV_USE_DEMO_KEYPAD_AND_ENCODER 0
#endif

/*Benchmark your system*/
#ifndef LV_USE_DEMO_BENCHMARK
#define LV_USE_DEMO_BENCHMARK 0
#endif
#if LV_USE_DEMO_BENCHMARK
/*Use RGB565A8 images with 16 bit color depth instead of ARGB8565*/
#ifndef LV_DEMO_BENCHMARK_RGB565A8
#define LV_DEMO_BENCHMARK_RGB565A8 0
#endif
#endif

/*Stress test for LVGL*/
#ifndef LV_USE_DEMO_STRESS
#define LV_USE_DEMO_STRESS 0
#endif

/*Music player demo*/
#ifndef LV_USE_DEMO_MUSIC
#define LV_USE_DEMO_MUSIC 0
#endif
#if LV_USE_DEMO_MUSIC
#ifndef LV_DEMO_MUSIC_SQUARE
    #define LV_DEMO_MUSIC_SQUARE    0
#endif
#ifndef LV_DEMO_MUSIC_LANDSCAPE
    #define LV_DEMO_MUSIC_LANDSCAPE 0
#endif
#ifndef LV_DEMO_MUSIC_ROUND
    #define LV_DEMO_MUSIC_ROUND     0
#endif
#ifndef LV_DEMO_MUSIC_LARGE
    #define LV_DEMO_MUSIC_LARGE     0
#endif
#ifndef LV_DEMO_MUSIC_AUTO_PLAY
    #define LV_DEMO_MUSIC_AUTO_PLAY 0
#endif
#endif

/*--END OF LV_CONF_H--*/

#endif /*LV_CONF_COMMON_H*/


