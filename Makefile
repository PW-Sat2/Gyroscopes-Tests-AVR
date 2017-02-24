APP_NAME = GT

INCLUDES = \
  -I . \
  -I libs/SD

SRCS = \
  main.cpp \
  libs/SD/ff.cpp \
  libs/SD/sdmm.cpp

include $(HAL_PATH)/build.make
