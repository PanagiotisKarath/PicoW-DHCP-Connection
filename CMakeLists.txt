#Setting minimum version of CMake that can be used
cmake_minimum_required(VERSION 3.13)

#Define project's and executable's name
set(PROJECT_NAME wifi-blink)

include(pico_sdk_import.cmake) #Include pico-sdk files
project(${PROJECT_NAME} C CXX ASM) #Required languages
set(CMAKE_C_STANDARD 11) #Define C standard
set(CMAKE_CXX_STANDARD 17) #Define C++ standard
pico_sdk_init() #Initialise the pico-sdk

# ==========================================================================
# Settings that are common for both the Access Point and the Station pico
# ==========================================================================
set(WIFI_SSID "picoNetwork") #Define WIFI_SSID variable
set(PASSWORD "pass1234") #Define PASSWORD variable
set(WIFI_PORT "8080") #Define WIFI_PORT variable

# ==========================================================================
# 1st pico - Access point
# ==========================================================================
set(A "pico_ap") #Variable A is defined as the access point

#Define project files
add_executable(${A}
    pico_ap.c
    dhcpserver/dhcpserver.c
)
pico_enable_stdio_usb(${A} 0) #Not using USB
pico_enable_stdio_uart(${A} 1) #Using UART

#Included directories for the access point
target_include_directories(${A} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/dhcpserver
)

#Adding preprocessor definitions to the access point (Macros)
target_compile_definitions(${A} PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    PASSWORD=\"${PASSWORD}\"
    WIFI_PORT=${WIFI_PORT}
)

#Linking required libraries
target_link_libraries(${A}
    pico_cyw43_arch_lwip_poll
    pico_stdlib
)

#Creates .map / .bin / .hex / .uf2 / etc files
pico_add_extra_outputs(${A})

# ==========================================================================
# 2nd pico - Station
# ==========================================================================
set(B "pico_sta") #Variable B is defined as the station

#Define project files
add_executable(${B}
    pico_sta.c
)
pico_enable_stdio_usb(${B} 0) #Not using USB
pico_enable_stdio_uart(${B} 1) #Using UART

#Included directories for the access point
target_include_directories(${B} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

#Adding preprocessor definitions to the access point (Macros)
target_compile_definitions(${B} PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    PASSWORD=\"${PASSWORD}\"
    WIFI_PORT=${WIFI_PORT}
)

#Linking required libraries
target_link_libraries(${B}
    pico_cyw43_arch_lwip_threadsafe_background
    pico_stdlib
)

#Creates .map / .bin / .hex / .uf2 / etc files
pico_add_extra_outputs(${B})
