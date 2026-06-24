#-------------------------------------------------------------------------------
# Configure variable
#-------------------------------------------------------------------------------
TOP_DIR:=$(TOP_DIR)
ROOT_DIR:=$(TOP_DIR)/../..
MAKEFILE_PATH :=$(subst $(strip \),/,$(abspath $(lastword $(MAKEFILE_LIST))))

#-------------------------------------------------------------------------------
# Configure source code files
#-------------------------------------------------------------------------------
SRC_FILES+= \
  src/sb_app_main.c \
  src/sb_error.c \
  src/sb_event.c \
  src/sb_event_bus.c \
  src/sb_log.c \
  src/sb_supervisor.c \
  src/sb_bsp_kae8_sq1.c \
  src/sb_hal_adc.c \
  src/sb_hal_audio_pa.c \
  src/sb_hal_gpio.c \
  src/sb_hal_key.c \
  src/sb_hal_led.c \
  src/sb_led_status.c \
  src/sb_crc32.c \
  src/sb_storage_fs.c \
  src/sb_ext_nor_flash.c \
  src/sb_audio_types.c \
  src/sb_amount_tokenizer.c \
  src/sb_audio_asset.c \
  src/sb_audio_asset_store.c \
  src/sb_audio_asset_manifest.c \
  src/sb_audio_prompt_logic.c \
  src/sb_audio_script.c \
  src/sb_audio_codec_es8311.c \
  src/sb_audio_hal.c \
  src/sb_audio_service.c \
  src/sb_config.c \
  src/sb_sim_service.c \
  src/sb_time_service.c \
  src/sb_network_service.c \
  src/sb_cloud_utils.c \
  src/sb_default_certs.c \
  src/sb_demo_profile.c \
  src/sb_mqtt_service.c \
  src/sb_http_service.c \
  src/sb_json.c \
  src/sb_transaction_ledger.c \
  src/sb_payment_processor.c \
  src/sb_command_dispatcher.c \
  src/sb_business_service.c \
  src/sb_mode_service.c \
  src/sb_factory_diag.c \
  src/sb_serial_service.c \
  src/sb_sms_service.c \

#-------------------------------------------------------------------------------
# the include directories
#-------------------------------------------------------------------------------
INC_DIRS+= \
  -I${TOP_DIR}/ \
  -I${TOP_DIR}/common/include \
  -I${TOP_DIR}/common/include/fs \
  -I${TOP_DIR}/common/include/mqtt \
  -I${TOP_DIR}/common/include/lwipv4v6 \
  -I${TOP_DIR}/interface/soundbox/include \

#-------------------------------------------------------------------------------
# Configure compile flag for C
#-------------------------------------------------------------------------------
U_FLAGS:=

#-------------------------------------------------------------------------------
# Configure compile flag for C++
#-------------------------------------------------------------------------------
U_CPPFLAGS:=

#-------------------------------------------------------------------------------
# Configure default macro define
#-------------------------------------------------------------------------------
DFLAGS+= \
  -D__TM_ZONE=tm_zone \
  -D__TM_GMTOFF=tm_gmtoff \
  -DSB_PHASE_NUMBER=23 \
  -DSB_BUILD_TARGET_EG800AK=1 \
  -DSB_MQTT_CERT_FROM_FS=1 \
  -DSB_EXTERNAL_NOR_ENABLED=1 \
  -DSB_HTTP_SERVICE_ENABLED=1 \
  -DSB_SERIAL_USE_MAIN_UART=1 \
  -DSB_SERIAL_FILE_PROVISIONING_ENABLED=1 \

#-------------------------------------------------------------------------------
# Configure link library
#-------------------------------------------------------------------------------
U_LIBS:=

#-------------------------------------------------------------------------------
# include mk file
#-------------------------------------------------------------------------------
include ${TOP_DIR}/config/common/makefile.mk
