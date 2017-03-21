/* Copyright (c) 2016/2017, FlySorter LLC *
 *                                        *
 *                                        */ 

#ifndef __PHOTOFUNCS_H__
#define __PHOTOFUNCS_H__

#define  INLET_GATE_OPEN   7100
#define  INLET_GATE_CLOSED 5400

#define OUTLET_GATE_OPEN   4500
#define OUTLET_GATE_CLOSED 6500

int maestroGetPosition(int fd, unsigned char channel);
int maestroSetTarget(int fd, unsigned char channel, unsigned short target);

int openSerialPort(const char *dev);
int serialport_read_until(int fd, char* buf, char until, int buf_max, int timeout);
int sendSerialCmd(int fd, char *msg, const char *reply, int waitTime = 500000);

int initDispenser(int fd);
int dispenseFly(int fd);
int enableLights(int fd);
int disableLights(int fd);
int pumpOn(int fd);
int pumpOff(int fd);
int stepVanes(int fd);
int stepperOff(int fd);

#endif // __PHOTOFUNCS_H__
