#!/bin/bash

export NOW=$(shell date)
# export NOW=SEKARANG

build: build-receiver build-sender
	@echo "${NOW} ====Done==="
	@echo


build-receiver: 
	@echo "${NOW} === BUILDING RECEIVER ==="
	@echo
	@g++ src/recvfile.cpp -pthread -o recv src/ack.cpp src/packet.cpp -w
	

run-receiver: build-receiver
	@echo "${NOW} RUNNING RECEIVER..."
	@./recv ${output} ${ws} ${bs} ${port}



build-sender: 
	@echo "${NOW} === BUILDING SENDER ==="
	@echo
	@g++ src/sendfile.cpp -pthread -o send src/ack.cpp src/packet.cpp -w

run-sender: build-sender
	@echo "${NOW} RUNNING SENDER..."
	@./send ${input} ${ws} ${bs} ${address} ${port}