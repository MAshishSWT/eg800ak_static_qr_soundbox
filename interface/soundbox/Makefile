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
  src/sb_crc32.c \
  src/sb_storage_fs.c \
  src/sb_extnor.c \
  src/sb_config.c \

#-------------------------------------------------------------------------------
# the include directories
#-------------------------------------------------------------------------------
INC_DIRS+= \
  -I${TOP_DIR}/ \
  -I${TOP_DIR}/common/include \
  -I${TOP_DIR}/common/include/fs \
  -I${TOP_DIR}/common/include/mqtt \
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
  -DSB_PHASE_NUMBER=3 \
  -DSB_BUILD_TARGET_EG800AK=1 \

#-------------------------------------------------------------------------------
# Configure link library
#-------------------------------------------------------------------------------
U_LIBS:=

#-------------------------------------------------------------------------------
# include mk file
#-------------------------------------------------------------------------------
include ${TOP_DIR}/config/common/makefile.mk
