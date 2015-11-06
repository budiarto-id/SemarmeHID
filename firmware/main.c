/* Name: main.h
 * Project: SemarmeHID
 * Author: Arif Budiarto
 * Creation Date: 2015-11-04
 * Copyright: (c) 2015 by Arif Budiarto (semarme.com)
 * License: GNU GPL v2 (see License.txt)
 */
 
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

  // Input Report
  0x09, 0x02,       // Usage ID - vendor defined
  0x15, 0x00,       // Logical Minimum (0)
  0x26, 0xFF, 0x00, // Logical Maximum (255)
  0x75, 0x08,       // Report Size (8 bits)
  0x95, 0x01,       // Report Count (1 fields)
  0x81, 0x02,       // Input (Data, Variable, Absolute)

  // Output report
  0x09, 0x03,       // Usage ID - vendor defined
  0x15, 0x00,       // Logical Minimum (0)
  0x26, 0xFF, 0x00, // Logical Maximum (255)
  0x75, 0x08,       // Report Size (8 bits)
  0x95, 0x01,       // Report Count (1 fields)
  0x91, 0x02,       // Output (Data, Variable, Absolute)

  0xc0              // END_COLLECTION
};

static unsigned bytesRemaining;
static uchar inbuf[8], outbuf[8], outlen, reportId;

uchar   usbFunctionRead(uchar *data, uchar len)
{
  uchar i;
  if (len > bytesRemaining) len = bytesRemaining;
  bytesRemaining -= len;
  for (i=0; i<len; i++){
	data[i] = 0xf0;
  }
  return bytesRemaining == 0;  // return 1 when done
}

uchar usbFunctionWrite (uchar *data, uchar len)
{
  uchar i;
  if (len > bytesRemaining) len = bytesRemaining;
  bytesRemaining -= len;
  for (i=0; i<len; i++){
	outbuf[i] = data[i];
	if(reportId==0) PORTB = data[i];
	if(reportId==1) PORTB = ~data[i];
  }
  outlen = len;
  return bytesRemaining == 0;  // return 1 when done
}

usbMsgLen_t usbFunctionSetup (uchar *data)
{
  usbRequest_t *rq = (void *) data;
  //PORTA = rq->bRequest;
  reportId = rq->wValue.bytes[0];
  if(rq->bRequest == USBRQ_HID_SET_REPORT) {
    bytesRemaining = rq->wLength.word;
    return USB_NO_MSG;
  }else if(rq->bRequest == USBRQ_HID_GET_REPORT) {
    bytesRemaining = rq->wLength.word;
    return USB_NO_MSG;
  }
  return 0;
}

void main_loop (void)
{
  uchar i;
  if (outlen) {
    if (usbInterruptIsReady()) {
      for (i=0; i<outlen; i++) inbuf[i] = outbuf[i];
      usbSetInterrupt(inbuf, i);
      outlen = 0;
    }
  }
}

int main (void)
{
  DDRB &= ~0x1F;
  DDRC |= 0x3F;
  DDRD &= ~0xe0;
  //DDRD |= 0x03;
  
  wdt_disable();
  //odDebugInit();
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

