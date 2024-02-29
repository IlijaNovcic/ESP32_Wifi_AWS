#Added extra component - wifiCustom
set(EXTRA_COMPONENT_DIRS $EXTRA_COMPONENT_DIRS "wifiCustom")
file(GLOB WIFI_CUSTOM_SOURCES "wifiCustom/*.c")

#Added extra component - nvsCustom
set(EXTRA_COMPONENT_DIRS $EXTRA_COMPONENT_DIRS "nvsCustom")
file(GLOB NVS_CUSTOM_SOURCES "nvsCustom/*.c")

#Added extra component - httpServerCustom
set(EXTRA_COMPONENT_DIRS $EXTRA_COMPONENT_DIRS "httpServerCustom")
file(GLOB HTTP_SERVER_CUSTOM_SOURCES "httpServerCustom/*.c")

#Added extra component - awsCustom
set(EXTRA_COMPONENT_DIRS $EXTRA_COMPONENT_DIRS "awsCustom")
file(GLOB AWS_CUSTOM_SOURCES "awsCustom/*.c")

#Added extra component - sntpTimeSyncCustom
set(EXTRA_COMPONENT_DIRS $EXTRA_COMPONENT_DIRS "sntpTimeSyncCustom")
file(GLOB SNTP_CUSTOM_SOURCES "sntpTimeSyncCustom/*.c")

#Added extra component - dht22Custom
set(EXTRA_COMPONENT_DIRS $EXTRA_COMPONENT_DIRS "dht22Custom")
file(GLOB DHT22_CUSTOM_SOURCES "dht22Custom/*.c")

#Added extra component - rgbLedCustom
set(EXTRA_COMPONENT_DIRS $EXTRA_COMPONENT_DIRS "rgbLedCustom")
file(GLOB RGB_LED_CUSTOM_SOURCES "rgbLedCustom/*.c")

# Edit following two lines to set component requirements (see docs)
idf_component_register(SRCS ${WIFI_CUSTOM_SOURCES} ${NVS_CUSTOM_SOURCES} ${HTTP_SERVER_CUSTOM_SOURCES}
							${AWS_CUSTOM_SOURCES} ${SNTP_CUSTOM_SOURCES} ${DHT22_CUSTOM_SOURCES} ${RGB_LED_CUSTOM_SOURCES} main.c
						INCLUDE_DIRS "wifiCustom" "nvsCustom" "httpServerCustom" "awsCustom" "sntpTimeSyncCustom" "dht22Custom" "rgbLedCustom" "." 
						EMBED_FILES webpage/app.css webpage/app.js webpage/favicon.ico webpage/index.html webpage/jquery-3.3.1.min.js)


target_add_binary_data(${COMPONENT_TARGET} "certs/aws_root_ca_pem" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "certs/certificate_pem_crt" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "certs/private_pem_key" TEXT)