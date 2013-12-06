################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../extent_client.o \
../extent_server.o \
../inode_manager.o \
../lab1_tester.o 

CC_SRCS += \
../extent_client.cc \
../extent_server.cc \
../extent_smain.cc \
../gettime.cc \
../inode_manager.cc \
../lab1_tester.cc 

OBJS += \
./extent_client.o \
./extent_server.o \
./extent_smain.o \
./gettime.o \
./inode_manager.o \
./lab1_tester.o 

CC_DEPS += \
./extent_client.d \
./extent_server.d \
./extent_smain.d \
./gettime.d \
./inode_manager.d \
./lab1_tester.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


