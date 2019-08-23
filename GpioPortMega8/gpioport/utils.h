/*
 * utils.h
 *
 * Created: 23.08.2019 14:54:21
 *  Author: Dmitry
 */ 


#ifndef UTILS_H_
#define UTILS_H_

#define CONCAT(a, b)            a ## b
#define OUTPORT(name)           CONCAT(PORT, name)
#define INPORT(name)            CONCAT(PIN, name)
#define DDRPORT(name)           CONCAT(DDR, name)


#endif /* UTILS_H_ */