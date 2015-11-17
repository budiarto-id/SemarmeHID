// Use subject to GPLv3.  Copyright 2011 George Magiros
#include <avr/io.h>
#include <avr/wdt.h>         // for wdt routines
#include <avr/interrupt.h>   // for sei() 
#include <util/delay.h>      // for _delay_ms()

#include <avr/pgmspace.h>    // required by usbdrv.h
#include "usbdrv.h"

/* USB report descriptor */
PROGMEM char usbHidReportDescriptor[] = {
  0x06, 0xa0, 0xff, // USAGE_PAGE (Vendor Defined Page 1)
  0x09, 0x01,       // USAGE (Vendor Usage 1)
  0xa1, 0x01,       // COLLECTION (Application)
  0x15, 0x00,       // Logical Minimum (0)
  0x26, 0xFF, 0x00, // Logical Maximum (255)
  0x75, 0x08,       // Report Size (8 bits)

  // Input Report
  0x95, 0x01,       // Report Count (8 fields)
  0x09, 0x00,       // USAGE (Undefined)
  0x81, 0x02,       // Input (Data, Variable, Absolute)

  // Output report
  0x95, 0x01,       // Report Count (8 fields)
  0x09, 0x00,       // USAGE (Undefined)
  0x91, 0x02,       // Output (Data, Variable, Absolute)

  0xc0              // END_COLLECTION
};

static unsigned bytesRemaining;
static uchar replyBuf[1];

uchar usbFunctionWrite (uchar *data, uchar len)
{
  uchar i;
  if (len > bytesRemaining) len = bytesRemaining;
  bytesRemaining -= len;
  for (i=0; i<len; i++){
	PORTC &= 0b11000000;
	PORTD &= 0b11111100;
	if((data[i] & 0x80) == 0x80) PORTC |= 1; 
	if((data[i] & 0x40) == 0x40) PORTC |= (1<<1);
	if((data[i] & 0x20) == 0x20) PORTC |= (1<<2);
	if((data[i] & 0x10) == 0x10) PORTC |= (1<<3);
	if((data[i] & 0x08) == 0x08) PORTC |= (1<<4);
	if((data[i] & 0x04) == 0x04) PORTC |= (1<<5);
	if((data[i] & 0x02) == 0x02) PORTD |= (1<<1);
	if((data[i] & 0x01) == 0x01) PORTD |= 1;
  }
  return bytesRemaining == 0;  // return 1 when done
}

usbMsgLen_t usbFunctionSetup (uchar *data)
{
  usbRequest_t *rq = (void *) data;
  if(rq->bRequest == USBRQ_HID_SET_REPORT) {
    bytesRemaining = rq->wLength.word;
    return USB_NO_MSG;
  }
  return 0;
}

void main_loop (void)
{
    if (usbInterruptIsReady()) {
	  replyBuf[0] = ((PINB <<3 ) | (PIND >> 5));
      usbSetInterrupt(replyBuf, 1);
    }
}

int main (void)
{
  DDRB &= 0b11100000;
  PORTB |= 0b00011111;
  DDRB |= (1<<5);
  PORTB |= (1<<5);
  DDRC |= 0b00111111;
  DDRD &= 0b00011111;
  PORTD |= 0b11100000;
  DDRD |= 0b00000011;
  wdt_disable();
  usbInit();

  usbDeviceDisconnect();  // force re-enumeration
  _delay_ms(250);
  usbDeviceConnect();

  sei();                  // now enable interrupts
  for(;;){
    usbPoll();
    main_loop();
  }
}

