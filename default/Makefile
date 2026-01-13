################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL := cmd.exe
RM := rm -rf

USER_OBJS :=

LIBS := 
PROJ := 

O_SRCS := 
C_SRCS := 
S_SRCS := 
S_UPPER_SRCS := 
OBJ_SRCS := 
ASM_SRCS := 
PREPROCESSING_SRCS := 
OBJS := 
OBJS_AS_ARGS := 
C_DEPS := 
C_DEPS_AS_ARGS := 
EXECUTABLES := 
OUTPUT_FILE_PATH :=
OUTPUT_FILE_PATH_AS_ARGS :=
AVR_APP_PATH :=$$$AVR_APP_PATH$$$
QUOTE := "
ADDITIONAL_DEPENDENCIES:=
OUTPUT_FILE_DEP:=

# Every subdirectory with source files must be described here
SUBDIRS := 


# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS +=  \
../STRCB.c \
../timer_t0.c \
../uart.c \
../uart1.c \
../ds1307.c \
../key2.c \
../glcd.c \
../TDisplay.c \
../TFont_PixelMix_5x8.c \
../ADA600.c \
../SDLs.c \
../STR.c \
../TempNTC.c


PREPROCESSING_SRCS += 


ASM_SRCS += 


OBJS +=  \
STRCB.o \
timer_t0.o \
uart.o \
uart1.o \
ds1307.o \
key2.o \
glcd.o \
TDisplay.o \
TFont_PixelMix_5x8.o \
ADA600.o \
SDLs.o \
STR.o \
TempNTC.o


OBJS_AS_ARGS +=  \
STRCB.o \
timer_t0.o \
uart.o \
uart1.o \
ds1307.o \
key2.o \
glcd.o \
TDisplay.o \
TFont_PixelMix_5x8.o \
ADA600.o \
SDLs.o \
STR.o \
TempNTC.o


C_DEPS +=  \
STRCB.d \
timer_t0.d \
uart.d \
uart1.d \
ds1307.d \
key2.d \
glcd.d \
TDisplay.d \
TFont_PixelMix_5x8.d \
ADA600.d \
SDLs.d \
STR.d \
TempNTC.d


C_DEPS_AS_ARGS +=  \
STRCB.d \
timer_t0.d \
uart.d \
uart1.d \
ds1307.d \
key2.d \
glcd.d \
TDisplay.d \
TFont_PixelMix_5x8.d \
ADA600.d \
SDLs.d \
STR.d \
TempNTC.d


OUTPUT_FILE_PATH +=STRCB.elf

OUTPUT_FILE_PATH_AS_ARGS +=STRCB.elf

ADDITIONAL_DEPENDENCIES:=

OUTPUT_FILE_DEP:= ./makedep.mk

# AVR32/GNU C Compiler



























./%.o: .././%.c
	@echo Building file: $<
	@echo Invoking: AVR/GNU C Compiler
	$(QUOTE)C:\Program Files (x86)\Atmel\AVR Studio 5.1\extensions\Atmel\AVRGCC\3.3.1.27\AVRToolchain\bin\avr-gcc.exe$(QUOTE)  -funsigned-char -funsigned-bitfields -DF_CPU=16000000UL  -Os -fpack-struct -fshort-enums -Wall -c -gdwarf-2 -std=gnu99  -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"  -mmcu=atmega128  -o"$@" "$<" 
	@echo Finished building: $<



# AVR32/GNU Preprocessing Assembler



# AVR32/GNU Assembler




ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: $(OUTPUT_FILE_PATH) $(ADDITIONAL_DEPENDENCIES)

$(OUTPUT_FILE_PATH): $(OBJS) $(USER_OBJS) $(OUTPUT_FILE_DEP)
	@echo Building target: $@
	@echo Invoking: AVR/GNU C Linker
	$(QUOTE)C:\Program Files (x86)\Atmel\AVR Studio 5.1\extensions\Atmel\AVRGCC\3.3.1.27\AVRToolchain\bin\avr-gcc.exe$(QUOTE) -o$(OUTPUT_FILE_PATH_AS_ARGS) $(OBJS_AS_ARGS) $(USER_OBJS) $(LIBS) -Wl,-Map="STRCB.map" -Wl,-lm   -mmcu=atmega128  
	@echo Finished building target: $@
	"C:\Program Files (x86)\Atmel\AVR Studio 5.1\extensions\Atmel\AVRGCC\3.3.1.27\AVRToolchain\bin\avr-objcopy.exe" -O ihex -R .eeprom -R .fuse -R .lock -R .signature  "STRCB.elf" "STRCB.hex"
	"C:\Program Files (x86)\Atmel\AVR Studio 5.1\extensions\Atmel\AVRGCC\3.3.1.27\AVRToolchain\bin\avr-objcopy.exe" -j .eeprom  --set-section-flags=.eeprom=alloc,load --change-section-lma .eeprom=0  --no-change-warnings -O ihex "STRCB.elf" "STRCB.eep" || exit 0
	"C:\Program Files (x86)\Atmel\AVR Studio 5.1\extensions\Atmel\AVRGCC\3.3.1.27\AVRToolchain\bin\avr-objdump.exe" -h -S "STRCB.elf" > "STRCB.lss"
	"C:\Program Files (x86)\Atmel\AVR Studio 5.1\extensions\Atmel\AVRGCC\3.3.1.27\AVRToolchain\bin\avr-size.exe" -C --mcu=atmega128  "STRCB.elf"
	




# Other Targets
clean:
	-$(RM) $(OBJS_AS_ARGS)$(C_DEPS_AS_ARGS) $(EXECUTABLES) 
	rm -rf "STRCB.hex" "STRCB.lss" "STRCB.eep" "STRCB.map"
	