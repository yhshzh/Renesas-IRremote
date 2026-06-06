# Please see README.md for build instructions

cmake_minimum_required(VERSION 3.16.4)

set(CMAKE_SYSTEM_NAME Generic)

set(CMAKE_SYSTEM_PROCESSOR arm)

# Use add_library() with the STATIC option to name the source file in the generated project.
# This avoids running the linker and is intended for use with cross-compiling toolchains that
# cannot link without custom flags or linker scripts
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo MinSizeRel)

if(BUILD_KIT)
	message(NOTICE "Build Kit is defined as ${BUILD_KIT}")
endif()

message(NOTICE "CMAKE_CURRENT_BINARY_DIR is defined as ${CMAKE_CURRENT_BINARY_DIR}")

get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(isMultiConfig)
    message(NOTICE "Multi-config generator(${CMAKE_GENERATOR}) is active")
else()
    message(NOTICE "Single-config generator(${CMAKE_GENERATOR}) is active")
	if(DEFINED CMAKE_BUILD_TYPE)
	 	message(NOTICE "CMAKE_BUILD_TYPE is defined as ${CMAKE_BUILD_TYPE}")
	elseif(DEFINED BUILD_TYPE)
		set(CMAKE_BUILD_TYPE ${BUILD_TYPE})
		message(NOTICE "CMAKE_BUILD_TYPE is set to BUILD_TYPE ENV ${CMAKE_BUILD_TYPE}")
	else()
		message(FATAL_ERROR "Please specify the CMAKE_BUILD_TYPE with -DCMAKE_BUILD_TYPE=")
	endif()
    #set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
	message(NOTICE "CMAKE_RUNTIME_OUTPUT_DIRECTORY is defined as ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

cmake_minimum_required(VERSION 3.16.4)

# Project configuration
project(Renesas_IRremote
	VERSION 1.0.0
	LANGUAGES C CXX ASM)

set(CMAKE_CROSSCOMPILING "TRUE")
set(CMAKE_EXPORT_COMPILE_COMMANDS "TRUE")

include(${CMAKE_CURRENT_LIST_DIR}/Config.cmake)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/GeneratedCfg.cmake)

# If required user can amend RASC generated flags here, e.g. list(REMOVE_ITEM RASC_CMAKE_C_FLAGS "-Wall")

include(${CMAKE_CURRENT_LIST_DIR}/cmake/GeneratedSrc.cmake)

set(IRREMOTEESP8266_DIR "${CMAKE_CURRENT_LIST_DIR}/third_party/IRremoteESP8266" CACHE PATH
    "Path to a local IRremoteESP8266 checkout.")

if(NOT EXISTS "${IRREMOTEESP8266_DIR}/src/IRremoteESP8266.h")
    message(FATAL_ERROR
        "IRremoteESP8266 was not found at '${IRREMOTEESP8266_DIR}'.\n"
        "Clone it into this project with:\n"
        "  git clone https://github.com/crankyoldgit/IRremoteESP8266.git "
        "${CMAKE_CURRENT_LIST_DIR}/third_party/IRremoteESP8266\n"
        "Or configure with -DIRREMOTEESP8266_DIR=/path/to/IRremoteESP8266")
endif()

option(IRREMOTE_RA4M2_SEND_DEMO "Send a NEC demo frame from hal_entry()." OFF)
option(IRREMOTE_RA4M2_RECV_DEMO "Decode IR frames from hal_entry()." OFF)
option(IRREMOTE_RA4M2_USE_IRQ_RECV "Use an FSP External IRQ instance for IR receive edge capture." ON)
option(IRREMOTE_RA4M2_USE_SYSTICK_RECV "Use SysTick interrupt sampling for IR receive instead of blocking polling." OFF)
option(IRREMOTE_RA4M2_USE_GPT_SEND "Use an FSP GPT PWM instance for IR carrier generation." ON)
set(IRREMOTE_RA4M2_RECV_SYSTICK_US "50" CACHE STRING
    "SysTick receive sampling period in microseconds when IRREMOTE_RA4M2_USE_SYSTICK_RECV is ON.")
set(IRREMOTE_RA4M2_RECV_BUFFER_SIZE "1024" CACHE STRING
    "IR receive demo capture buffer entries. Use a large value for A/C protocols.")
set(IRREMOTE_RA4M2_RECV_TIMEOUT_MS "50" CACHE STRING
    "IR receive demo frame timeout in milliseconds. Use a larger value for A/C protocols.")
set(IRREMOTE_RA4M2_DEBUG_RAW_DUMP_SIZE "1024" CACHE STRING
    "Maximum number of captured raw timing entries exposed through GDB debug globals.")
set(IRREMOTE_RA4M2_GPT_SEND_INSTANCE "g_ir_tx_gpt" CACHE STRING
    "FSP timer_instance_t symbol used for the IR carrier GPT PWM output.")
set(IRREMOTE_RA4M2_GPT_SEND_IO_PIN "GPT_IO_PIN_GTIOCA" CACHE STRING
    "FSP gpt_io_pin_t value used for the IR carrier PWM output.")

set(IRREMOTEESP8266_UPSTREAM_SOURCES
    ${IRREMOTEESP8266_DIR}/src/IRtext.cpp
    ${IRREMOTEESP8266_DIR}/src/IRtimer.cpp
    ${IRREMOTEESP8266_DIR}/src/IRutils.cpp
    ${IRREMOTEESP8266_DIR}/src/IRac.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Aiwa.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Airton.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Airwell.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Amcor.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Argo.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Arris.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_BluestarHeavy.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Bosch.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Bose.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Carrier.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_ClimaButler.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Coolix.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Corona.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Daikin.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Denon.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Dish.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Delonghi.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Doshisha.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Ecoclim.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Electra.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_EliteScreens.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Epson.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Eurom.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Fujitsu.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_GICable.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_GlobalCache.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Goodweather.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Gree.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Gorenje.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Haier.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Hitachi.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Inax.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_JVC.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Kelon.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Kelvinator.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Lasertag.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Lego.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_LG.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Lutron.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Magiquest.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Metz.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Midea.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Mirage.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_MilesTag2.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Mitsubishi.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_MitsubishiHeavy.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Multibrackets.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_MWM.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Nikai.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_NEC.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Neoclima.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Panasonic.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Pioneer.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Pronto.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_RC5_RC6.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_RCMM.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Rhoss.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Samsung.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Sanyo.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Sharp.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Sherwood.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Sony.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Symphony.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Tcl.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Technibel.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Teco.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Teknopoint.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Toto.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Toshiba.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Transcold.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Trotec.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Truma.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Vestel.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Voltas.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Whirlpool.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Whynter.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Wowwee.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Xmp.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_York.cpp
    ${IRREMOTEESP8266_DIR}/src/ir_Zepeal.cpp
)

set(IRREMOTEESP8266_RA_SOURCES
    ${IRREMOTEESP8266_UPSTREAM_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/src/ir_ra4m2/IRsend_RA4M2.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ir_ra4m2/IRrecv_RA4M2.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ir_ra4m2/ir_ra4m2_gpt_carrier.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ir_ra4m2/Arduino.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ir_ra4m2/newlib_stubs.c
)

set_source_files_properties(
    ${IRREMOTEESP8266_UPSTREAM_SOURCES}
    PROPERTIES COMPILE_OPTIONS "-Wno-conversion;-Wno-shadow;-Wno-unused-parameter;-Wno-aggregate-return;-Wno-float-conversion;-Wno-float-equal;-Wno-missing-declarations"
)

set_source_files_properties(
    ${CMAKE_CURRENT_LIST_DIR}/src/ir_ra4m2/IRsend_RA4M2.cpp
    PROPERTIES COMPILE_OPTIONS "-Wno-conversion;-Wno-shadow;-Wno-unused-parameter;-Wno-aggregate-return;-Wno-float-conversion;-Wno-float-equal;-Wno-missing-declarations"
)

set_source_files_properties(
    ${CMAKE_CURRENT_LIST_DIR}/src/ir_ra4m2/IRrecv_RA4M2.cpp
    PROPERTIES COMPILE_OPTIONS "-Wno-aggregate-return"
)

set_source_files_properties(
    ${CMAKE_CURRENT_LIST_DIR}/src/hal_entry.cpp
    PROPERTIES COMPILE_OPTIONS "-Wno-aggregate-return"
)

set_source_files_properties(
    ${CMAKE_CURRENT_LIST_DIR}/src/app/aircon_ir_service.cpp
    PROPERTIES COMPILE_OPTIONS "-Wno-aggregate-return"
)

target_sources(${PROJECT_NAME}.elf PRIVATE ${IRREMOTEESP8266_RA_SOURCES})

target_include_directories(${PROJECT_NAME}.elf
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/ir_ra4m2
    ${IRREMOTEESP8266_DIR}/src
)

target_compile_definitions(${PROJECT_NAME}.elf
    PRIVATE
    ARDUINO=100
    STRCASECMP=irremote_ra_strcasecmp
    _IR_ENABLE_DEFAULT_=false
    SEND_RAW=true
    SEND_AIWA_RC_T501=true
    SEND_AIRTON=true
    SEND_AIRWELL=true
    SEND_AMCOR=true
    SEND_ARGO=true
    SEND_ARRIS=true
    SEND_BLUESTARHEAVY=true
    SEND_BOSCH144=true
    SEND_BOSE=true
    SEND_CARRIER_AC=true
    SEND_CARRIER_AC40=true
    SEND_CARRIER_AC64=true
    SEND_CARRIER_AC84=true
    SEND_CARRIER_AC128=true
    SEND_CLIMABUTLER=true
    SEND_COOLIX=true
    SEND_COOLIX48=true
    SEND_CORONA_AC=true
    SEND_DAIKIN=true
    SEND_DAIKIN2=true
    SEND_DAIKIN64=true
    SEND_DAIKIN128=true
    SEND_DAIKIN152=true
    SEND_DAIKIN160=true
    SEND_DAIKIN176=true
    SEND_DAIKIN200=true
    SEND_DAIKIN216=true
    SEND_DAIKIN312=true
    SEND_DENON=true
    SEND_DISH=true
    SEND_DELONGHI_AC=true
    SEND_DOSHISHA=true
    SEND_ECOCLIM=true
    SEND_ELECTRA_AC=true
    SEND_ELITESCREENS=true
    SEND_EPSON=true
    SEND_EUROM=true
    SEND_FUJITSU_AC=true
    SEND_GICABLE=true
    SEND_GLOBALCACHE=true
    SEND_GOODWEATHER=true
    SEND_GREE=true
    SEND_GORENJE=true
    SEND_HAIER_AC=true
    SEND_HAIER_AC_YRW02=true
    SEND_HAIER_AC160=true
    SEND_HAIER_AC176=true
    SEND_HITACHI_AC=true
    SEND_HITACHI_AC1=true
    SEND_HITACHI_AC2=true
    SEND_HITACHI_AC3=true
    SEND_HITACHI_AC264=true
    SEND_HITACHI_AC296=true
    SEND_HITACHI_AC344=true
    SEND_HITACHI_AC424=true
    SEND_INAX=true
    SEND_JVC=true
    SEND_KELON=true
    SEND_KELON168=true
    SEND_KELVINATOR=true
    SEND_LASERTAG=true
    SEND_LEGOPF=true
    SEND_LG=true
    SEND_LUTRON=true
    SEND_MAGIQUEST=true
    SEND_METZ=true
    SEND_MIDEA=true
    SEND_MIDEA24=true
    SEND_MIRAGE=true
    SEND_MILESTAG2=true
    SEND_MITSUBISHI=true
    SEND_MITSUBISHI2=true
    SEND_MITSUBISHI_AC=true
    SEND_MITSUBISHI112=true
    SEND_MITSUBISHI136=true
    SEND_MITSUBISHIHEAVY=true
    SEND_MULTIBRACKETS=true
    SEND_MWM=true
    SEND_NIKAI=true
    SEND_NEC=true
    SEND_NEOCLIMA=true
    SEND_PANASONIC=true
    SEND_PANASONIC_AC=true
    SEND_PANASONIC_AC32=true
    SEND_PIONEER=true
    SEND_PRONTO=true
    SEND_RC5=true
    SEND_RC6=true
    SEND_RCMM=true
    SEND_RHOSS=true
    SEND_SAMSUNG=true
    SEND_SAMSUNG36=true
    SEND_SAMSUNG_AC=true
    SEND_SANYO=true
    SEND_SANYO_AC=true
    SEND_SANYO_AC88=true
    SEND_SANYO_AC152=true
    SEND_SHARP=true
    SEND_SHARP_AC=true
    SEND_SHERWOOD=true
    SEND_SONY=true
    SEND_SYMPHONY=true
    SEND_TCL96AC=true
    SEND_TCL112AC=true
    SEND_TECHNIBEL_AC=true
    SEND_TECO=true
    SEND_TEKNOPOINT=true
    SEND_TOTO=true
    SEND_TOSHIBA_AC=true
    SEND_TRANSCOLD=true
    SEND_TROTEC=true
    SEND_TROTEC_3550=true
    SEND_TRUMA=true
    SEND_VESTEL_AC=true
    SEND_VOLTAS=true
    SEND_WHIRLPOOL_AC=true
    SEND_WHYNTER=true
    SEND_WOWWEE=true
    SEND_XMP=true
    SEND_YORK=true
    SEND_ZEPEAL=true
    DECODE_HASH=true
    DECODE_AIWA_RC_T501=true
    DECODE_AIRTON=true
    DECODE_AIRWELL=true
    DECODE_AMCOR=true
    DECODE_ARGO=true
    DECODE_ARRIS=true
    DECODE_BLUESTARHEAVY=true
    DECODE_BOSCH144=true
    DECODE_BOSE=true
    DECODE_CARRIER_AC=true
    DECODE_CARRIER_AC40=true
    DECODE_CARRIER_AC64=true
    DECODE_CARRIER_AC84=true
    DECODE_CARRIER_AC128=true
    DECODE_CLIMABUTLER=true
    DECODE_COOLIX=true
    DECODE_COOLIX48=true
    DECODE_CORONA_AC=true
    DECODE_DAIKIN=true
    DECODE_DAIKIN2=true
    DECODE_DAIKIN64=true
    DECODE_DAIKIN128=true
    DECODE_DAIKIN152=true
    DECODE_DAIKIN160=true
    DECODE_DAIKIN176=true
    DECODE_DAIKIN200=true
    DECODE_DAIKIN216=true
    DECODE_DAIKIN312=true
    DECODE_DENON=true
    DECODE_DISH=true
    DECODE_DELONGHI_AC=true
    DECODE_DOSHISHA=true
    DECODE_ECOCLIM=true
    DECODE_ELECTRA_AC=true
    DECODE_ELITESCREENS=true
    DECODE_EPSON=true
    DECODE_EUROM=true
    DECODE_FUJITSU_AC=true
    DECODE_GICABLE=true
    DECODE_GOODWEATHER=true
    DECODE_GREE=true
    DECODE_GORENJE=true
    DECODE_HAIER_AC=true
    DECODE_HAIER_AC_YRW02=true
    DECODE_HAIER_AC160=true
    DECODE_HAIER_AC176=true
    DECODE_HITACHI_AC=true
    DECODE_HITACHI_AC1=true
    DECODE_HITACHI_AC2=true
    DECODE_HITACHI_AC3=true
    DECODE_HITACHI_AC264=true
    DECODE_HITACHI_AC296=true
    DECODE_HITACHI_AC344=true
    DECODE_HITACHI_AC424=true
    DECODE_INAX=true
    DECODE_JVC=true
    DECODE_KELON=true
    DECODE_KELON168=true
    DECODE_KELVINATOR=true
    DECODE_LASERTAG=true
    DECODE_LEGOPF=true
    DECODE_LG=true
    DECODE_LUTRON=true
    DECODE_MAGIQUEST=true
    DECODE_METZ=true
    DECODE_MIDEA=true
    DECODE_MIDEA24=true
    DECODE_MIRAGE=true
    DECODE_MILESTAG2=true
    DECODE_MITSUBISHI=true
    DECODE_MITSUBISHI2=true
    DECODE_MITSUBISHI_AC=true
    DECODE_MITSUBISHI112=true
    DECODE_MITSUBISHI136=true
    DECODE_MITSUBISHIHEAVY=true
    DECODE_MULTIBRACKETS=true
    DECODE_MWM=true
    DECODE_NIKAI=true
    DECODE_NEC=true
    DECODE_NEOCLIMA=true
    DECODE_PANASONIC=true
    DECODE_PANASONIC_AC=true
    DECODE_PANASONIC_AC32=true
    DECODE_PIONEER=true
    DECODE_RC5=true
    DECODE_RC6=true
    DECODE_RCMM=true
    DECODE_RHOSS=true
    DECODE_SAMSUNG=true
    DECODE_SAMSUNG36=true
    DECODE_SAMSUNG_AC=true
    DECODE_SANYO=true
    DECODE_SANYO_AC=true
    DECODE_SANYO_AC88=true
    DECODE_SANYO_AC152=true
    DECODE_SHARP=true
    DECODE_SHARP_AC=true
    DECODE_SONY=true
    DECODE_SYMPHONY=true
    DECODE_TCL96AC=true
    DECODE_TCL112AC=true
    DECODE_TECHNIBEL_AC=true
    DECODE_TECO=true
    DECODE_TEKNOPOINT=true
    DECODE_TOTO=true
    DECODE_TOSHIBA_AC=true
    DECODE_TRANSCOLD=true
    DECODE_TROTEC=true
    DECODE_TROTEC_3550=true
    DECODE_TRUMA=true
    DECODE_VESTEL_AC=true
    DECODE_VOLTAS=true
    DECODE_WHIRLPOOL_AC=true
    DECODE_WHYNTER=true
    DECODE_WOWWEE=true
    DECODE_XMP=true
    DECODE_YORK=true
    DECODE_ZEPEAL=true
    IRREMOTE_RA4M2_RECV_DEMO=$<BOOL:${IRREMOTE_RA4M2_RECV_DEMO}>
    IRREMOTE_RA4M2_SEND_DEMO=$<BOOL:${IRREMOTE_RA4M2_SEND_DEMO}>
    IRREMOTE_RA4M2_USE_IRQ_RECV=$<BOOL:${IRREMOTE_RA4M2_USE_IRQ_RECV}>
    IRREMOTE_RA4M2_USE_SYSTICK_RECV=$<BOOL:${IRREMOTE_RA4M2_USE_SYSTICK_RECV}>
    IRREMOTE_RA4M2_USE_GPT_SEND=$<BOOL:${IRREMOTE_RA4M2_USE_GPT_SEND}>
    IRREMOTE_RA4M2_GPT_SEND_INSTANCE=${IRREMOTE_RA4M2_GPT_SEND_INSTANCE}
    IRREMOTE_RA4M2_GPT_SEND_IO_PIN=${IRREMOTE_RA4M2_GPT_SEND_IO_PIN}
    IRREMOTE_RA4M2_RECV_SYSTICK_US=${IRREMOTE_RA4M2_RECV_SYSTICK_US}
    IRREMOTE_RA4M2_RECV_BUFFER_SIZE=${IRREMOTE_RA4M2_RECV_BUFFER_SIZE}
    IRREMOTE_RA4M2_RECV_TIMEOUT_MS=${IRREMOTE_RA4M2_RECV_TIMEOUT_MS}
    IRREMOTE_RA4M2_DEBUG_RAW_DUMP_SIZE=${IRREMOTE_RA4M2_DEBUG_RAW_DUMP_SIZE}
)
