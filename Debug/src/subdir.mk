################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/BranchBuffer.cpp \
../src/main.cpp \
../src/pipeline.cpp 

OBJS += \
./src/BranchBuffer.o \
./src/main.o \
./src/pipeline.o 

CPP_DEPS += \
./src/BranchBuffer.d \
./src/main.d \
./src/pipeline.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


