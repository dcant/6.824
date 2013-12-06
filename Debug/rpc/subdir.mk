################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../rpc/connection.o \
../rpc/jsl_log.o \
../rpc/pollmgr.o \
../rpc/rpc.o \
../rpc/thr_pool.o 

CC_SRCS += \
../rpc/connection.cc \
../rpc/jsl_log.cc \
../rpc/pollmgr.cc \
../rpc/rpc.cc \
../rpc/rpctest.cc \
../rpc/thr_pool.cc 

OBJS += \
./rpc/connection.o \
./rpc/jsl_log.o \
./rpc/pollmgr.o \
./rpc/rpc.o \
./rpc/rpctest.o \
./rpc/thr_pool.o 

CC_DEPS += \
./rpc/connection.d \
./rpc/jsl_log.d \
./rpc/pollmgr.d \
./rpc/rpc.d \
./rpc/rpctest.d \
./rpc/thr_pool.d 


# Each subdirectory must supply rules for building sources it contributes
rpc/%.o: ../rpc/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


