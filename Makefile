#!/bin/bash
#****************************************************
# FILE: Makefile
#
# DESCRIPTION
# Makefile for CSE434 Socket Programming Project
# Josh Moore
# Jon Yocky
#
# http://www.cs.bgu.ac.il/~sadetsky/openu/myMakeTutorial.txt
#
#****************************************************
OBJS1 = client.o
OBJS2 = server.o
CC = gcc
DEBUG = -g
CFLAGS = -Wall -c $(DEBUG) -std=c99 -Wall
LFLAGS = -Wall $(DEBUG)

all: client server clean

client: $(OBJS1)
	$(CC) $(LFLAGS) $(OBJS1) -o $@

server: $(OBJS2)
	$(CC) $(LFLAGS) $(OBJS2) -o $@

client.o: client.c client.h
	$(CC) $(CFLAGS) client.c

server.o: server.c server.h
	$(CC) $(CFLAGS) server.c

clean:
	\rm -f *.o