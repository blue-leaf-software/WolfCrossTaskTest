idf_component_register(
                    SRCS main.cpp
                    INCLUDE_DIRS "."
                    REQUIRES
                      esp_littlefs
                      WolfSSL
                      )

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
