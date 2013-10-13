/*
	PIC18F2550 あそ～ぶUSB用USBキーボードサンプル
		ver1.00	2010/11/03	初版(MCHPFSUSBのサンプルから作成)
	
	RA0にスイッチを繋ぎ、押すと、[ENTER]キーが押されます。
	[Caps lock][Num lock]の状態によってLEDが点灯します。
	
*/

#ifndef KEYBOARD_C
#define KEYBOARD_C

/** INCLUDES *******************************************************/
#include "Microchip/Include/USB/usb.h"
#include "HardwareProfile.h"
#include "Microchip/Include/USB/usb_function_hid.h"
#include "bootload.h"
#include <timers.h>

/** VARIABLES ******************************************************/
#pragma udata
// char buffer[8];
// unsigned char OutBuffer[8];
USB_HANDLE lastINTransmission;
USB_HANDLE lastOUTTransmission;
// BOOL Keyboard_out;
// DWORD CountdownTimerToShowUSBStatusOnLEDs;

/** 
 * キーテーブル
 * 1次元目のインデックスがそのまま内部キーコードになる。
 * 2次元目のインデックス0がFnキー無押下状態のUSBキーコード、インデックス1がFnキー押下時状態のUSBキーコード
 */
unsigned char KEY_TABLE[][2] = {
	{0x00, 0x00,}, //  0:reserve						 0:reserve

	{0x29, 0x00,}, //  1:ESCAPE							 1:
	{0x2B, 0x00,}, //  2:Tab							 2:
	{0xE0, 0x00,}, //  3:Left Control					 3:
	{0xE1, 0x00,}, //  4:Left Shift						 4:
	{0xE3, 0x00,}, //  5:Left GUI						 5:
	{0xE2, 0x00,}, //  6:Left Alt						 6:
	{0x00, 0x00,}, //  7:Left Fn						 7:
	{0x2C, 0x00,}, //  8:Left Spacebar					 8:

	{0x2C, 0x00,}, //  9:Right Spacebar					 9:
	{0x00, 0x00,}, // 10:Right Fn						10:
	{0xE6, 0x00,}, // 11:Right Alt						11:
	{0xE7, 0x00,}, // 12:Right GUI						12:
	{0xE5, 0x00,}, // 13:Right Shift					13:
	{0x28, 0x00,}, // 14:Return							14:
	{0x2A, 0x4C,}, // 15:DELETE							15:Delete Forward
	{0x35, 0x00,}, // 16:Grave Accent and Tilde // ~`	16:

	{0x1E, 0x3A,}, // 17:1 and !						17:F1
	{0x14, 0x00,}, // 18:q and Q						18:
	{0x04, 0x50,}, // 19:a and A						19:LeftArrow
	{0x1D, 0x00,}, // 20:z and Z						20:
	{0x1B, 0x00,}, // 21:x and X						21:
	{0x06, 0x00,}, // 22:c and C						22:
	{0x19, 0x00,}, // 23:v and V						23:
	{0x05, 0x00,}, // 24:b and B						24:

	{0x11, 0x00,}, // 25:n and N						25:
	{0x10, 0x00,}, // 26:m and M						26:
	{0x36, 0x4D,}, // 27:, and <						27:End
	{0x37, 0x4E,}, // 28:. and >						28:PageDown
	{0x38, 0x00,}, // 29:/ and ?						29:
	{0x34, 0x00,}, // 30:' and "						30:
	{0x30, 0x00,}, // 31:] and }						31:
	{0x31, 0x49,}, // 32:\ and |						32:Insert

	{0x1F, 0x3B,}, // 33:2 and @						33:F2
	{0x1A, 0x52,}, // 34:w and W						34:UpArrow
	{0x16, 0x51,}, // 35:s and S						35:DownArrow
	{0x07, 0x4F,}, // 36:d and D						35:RightArrow
	{0x09, 0x00,}, // 37:f and F						37:
	{0x0A, 0x00,}, // 38:g and G						38:
	{0x0B, 0x00,}, // 39:h and H						39:
	{0x0D, 0x00,}, // 40:j and J						40:

	{0x0E, 0x4A,}, // 41:k and K						41:Home
	{0x0F, 0x4B,}, // 42:l and L						42:PageUp
	{0x33, 0x00,}, // 43:; and :						43:
	{0x13, 0x48,}, // 44:p and P						44:Pause
	{0x2F, 0x00,}, // 45:[ and {						45:
	{0x2E, 0x45,}, // 46:= and +						46:F12
	{0x00, 0x00,}, // 47:								47:
	{0x00, 0x00,}, // 48:								48:

	{0x20, 0x3C,}, // 49:3 and #						49:F3
	{0x08, 0x00,}, // 50:e and E						50:
	{0x15, 0x00,}, // 51:r and R						51:
	{0x17, 0x00,}, // 52:t and T						52:
	{0x1C, 0x00,}, // 53:y and Y						53:
	{0x18, 0x00,}, // 54:u and U						54:
	{0x0C, 0x46,}, // 55:i and I						55:PrintScreen
	{0x12, 0x47,}, // 56:o and O						56:Scroll Lock

	{0x21, 0x3D,}, // 57:4 and $						57:F4
	{0x22, 0x3E,}, // 58:5 and %						58:F5
	{0x23, 0x3F,}, // 59:6 and ^						59:F6
	{0x24, 0x40,}, // 60:7 and &						60:F7
	{0x25, 0x41,}, // 61:8 and *						61:F8
	{0x26, 0x42,}, // 62:9 and (						62:F9
	{0x27, 0x43,}, // 63:0 and )						63:F10
	{0x2D, 0x44,}, // 64:- and _(underscore)			64:F10
};


// ===== 特殊な処理を行うキーの内部キーコード =====
// 左Fnキー
#define FN_LEFT_SCAN_CODE		7
// 右Fnキー
#define FN_RIGHT_SCAN_CODE		10
// 左スペースキー
#define SPACE_LEFT_SCAN_CODE	8
// 右スペースキー
#define SPACE_RIGHT_SCAN_CODE	9

// ===== 修飾キーのUSBキーコード範囲 =====
#define DV_FROM 0xE0
#define DV_TO	0xE7

// チャタリング抑制用閾値。1あたりの時間は謎。
#define CHATTAERING_THRESHOLD	20

#define KEY_SCAN_MAX_INDEX	64

unsigned char dynamicFlags = 0;
unsigned char report[6];
unsigned char reportIndex = 0;
unsigned short scanBuff[KEY_SCAN_MAX_INDEX];

/** PRIVATE PROTOTYPES *********************************************/
static void InitializeSystem(void);
void ProcessIO(void);
void UserInit(void);
void Keyboard(void);

void USBHIDCBSetReportComplete(void);

/** VECTOR REMAPPING ***********************************************/
#pragma code
	
void isr_high()
{
	#if defined(USB_INTERRUPT)
	USBDeviceTasks();
	#endif
}	//This return will be a "retfie fast", since this is in a #pragma interrupt section 
void isr_low()
{
}	//This return will be a "retfie", since this is in a #pragma interruptlow section 


void main(void)
{
	InitializeSystem();

	#if defined(USB_INTERRUPT)
		USBDeviceAttach();
	#endif

	while(1)
	{
		#if defined(USB_POLLING)
			USBDeviceTasks(); // Interrupt or polling method.  If using polling, must call
		#endif
		ProcessIO();
	}//end while
}//end main

static void InitializeSystem(void)
{
	ADCON1 |= 0x0F;					// Default all pins to digital

	#if defined(USE_USB_BUS_SENSE_IO)
	tris_usb_bus_sense = INPUT_PIN; // See HardwareProfile.h
	#endif
	#if defined(USE_SELF_POWER_SENSE_IO)
	tris_self_power = INPUT_PIN;	// See HardwareProfile.h
	#endif
	
	UserInit();

	USBDeviceInit();	//usb_device.c.	 Initializes USB module SFRs and firmware
}//end InitializeSystem



/******************************************************************************
 * Function:		void UserInit(void)
 *****************************************************************************/
void UserInit(void)
{
	int i;

	LATA = 0;
	LATB = 0;
	LATC = 0;

	TRISA = 0;		// RAを出力
	TRISB = 0x3F;	// RB0~5を入力
	TRISC = 0xC0;	// RC0,1を出力、RC6,7を入力

	lastINTransmission	= 0;
	lastOUTTransmission	= 0;

	for(i = 0; i <= KEY_SCAN_MAX_INDEX; i++) {
		scanBuff[i] = 0;
	}

}//end UserInit


void myKbPinOut(unsigned char pinNo, unsigned char onoff);
BOOL myKbChkIn(unsigned char pinNo);
void KeyScan(void);


/********************************************************************
 * Function:		void ProcessIO(void)
 *******************************************************************/
void ProcessIO(void)
{	
	// User Application USB tasks
	if((USBDeviceState < CONFIGURED_STATE)||(USBSuspendControl==1)) return;

	// キースキャン
	KeyScan();
	
	//Call the function that behaves like a keyboard  
	Keyboard();
	 
}//end ProcessIO

/**
 * キースキャン処理
 */
void KeyScan(void) {
	unsigned char i = 0;
	unsigned char j = 0;
	unsigned char scanCode = 0;
	unsigned char fnDetected = 0;
	unsigned char spaceDetected = 0;
	unsigned char c = 0;
	// 
	reportIndex = 0;
	dynamicFlags = 0;
	
	for(i = 0; i <= 7; i++) {

		myKbPinOut(i, (unsigned char)1);

		for(j = 0; j <= 7; j++) {
			scanCode++;
			if(myKbChkIn(j)) {
				if(scanBuff[scanCode] < CHATTAERING_THRESHOLD) {
					scanBuff[scanCode]++;
				}
				
				if(scanCode == FN_LEFT_SCAN_CODE || scanCode == FN_RIGHT_SCAN_CODE) {
					fnDetected = 1;
				} 
			} else {
				scanBuff[scanCode] = 0;
			}
		}

		myKbPinOut(i, (unsigned char)0);
	}

	
	for(i = 1; i <= KEY_SCAN_MAX_INDEX; i++) {
		if(scanBuff[i] >= CHATTAERING_THRESHOLD) {
			c = KEY_TABLE[i][fnDetected];
			if(fnDetected && c == 0) {
				// Fnキー押下状態でキーコードが無かった場合
				c = KEY_TABLE[i][0]; // Fnキー無押下状態のUSBキーコード
			}
			if((c == SPACE_LEFT_SCAN_CODE || c == SPACE_RIGHT_SCAN_CODE) && spaceDetected) {
				// 既にスペースキーを処理している場合
				c = 0; // 捨てる
			}
			if(c > 0) {
				if(DV_FROM <= c && c <= DV_TO) {
					dynamicFlags |= (1 << (c & 0x0F));
				} else {
					report[reportIndex] = c;
					if(c == SPACE_LEFT_SCAN_CODE || c == SPACE_RIGHT_SCAN_CODE) {
						spaceDetected = 1; // スペースキー追加したよ
					}
					reportIndex++;
				}
				if(reportIndex >= 6) {
					break;
				}
			}
		}
	}
}

void Keyboard(void)
{
	unsigned char i = 0;

	if(!HIDTxHandleBusy(lastINTransmission))
	{
		hid_report_in[0] = 0;
		hid_report_in[1] = 0;
		hid_report_in[2] = 0;
		hid_report_in[3] = 0;
		hid_report_in[4] = 0;
		hid_report_in[5] = 0;
		hid_report_in[6] = 0;
		hid_report_in[7] = 0;

		if(dynamicFlags > 0) {
			hid_report_in[0] = dynamicFlags;
		} else {
			hid_report_in[0] = 0;
		}

		for(i = 0; i < reportIndex; i++) {
			hid_report_in[i + 2] = report[i];
		}

/*
		if(!PORTAbits.RA0)
			hid_report_in[2] = 0x28;	//[ENTER]
		else
			hid_report_in[2] = 0;
*/
		//Send the 8 byte packet over USB to the host.
		lastINTransmission = HIDTxPacket(HID_EP, (BYTE*)hid_report_in, 0x08);
	}
	
	if(!HIDRxHandleBusy(lastOUTTransmission))
	{
/*
		if(hid_report_out[0] & 0x02)	//[CAPS]lock時に点灯
			LATBbits.LATB4 = 1;
		else
			LATBbits.LATB4 = 0;

		if(hid_report_out[0] & 0x01)	//[NUM]lock時に点灯
			LATBbits.LATB5 = 1;
		else
			LATBbits.LATB5 = 0;
*/
		lastOUTTransmission = HIDRxPacket(HID_EP,(BYTE*)&hid_report_out,1);
	} 

	return;
}//end keyboard()



// ******************************************************************************************************
// ************** USB Callback Functions ****************************************************************
// ******************************************************************************************************

/******************************************************************************
 * Function:		void USBCBSuspend(void)
 *****************************************************************************/
void USBCBSuspend(void)
{
}

/******************************************************************************
 * Function:		void USBCBWakeFromSuspend(void)
 *****************************************************************************/
void USBCBWakeFromSuspend(void)
{
}

/********************************************************************
 * Function:		void USBCB_SOF_Handler(void)
 *******************************************************************/
void USBCB_SOF_Handler(void)
{
}

/*******************************************************************
 * Function:		void USBCBErrorHandler(void)
 *******************************************************************/
void USBCBErrorHandler(void)
{
}

/*******************************************************************
 * Function:		void USBCBCheckOtherReq(void)
 *******************************************************************/
void USBCBCheckOtherReq(void)
{
	USBCheckHIDRequest();
}//end


/*******************************************************************
 * Function:		void USBCBStdSetDscHandler(void)
 *******************************************************************/
void USBCBStdSetDscHandler(void)
{
}//end


/*******************************************************************
 * Function:		void USBCBInitEP(void)
 *******************************************************************/
void USBCBInitEP(void)
{
	//enable the HID endpoint
	USBEnableEndpoint(HID_EP,USB_IN_ENABLED|USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);

	lastOUTTransmission = HIDRxPacket(HID_EP,(BYTE*)&hid_report_out,1);

}

/********************************************************************
 * Function:		void USBCBSendResume(void)
 *******************************************************************/
void USBCBSendResume(void)
{
	static WORD delay_count;
	
	USBResumeControl = 1;				 // Start RESUME signaling
	
	delay_count = 1800U;				// Set RESUME line for 1-13 ms
	do
	{
		delay_count--;
	}while(delay_count);
	USBResumeControl = 0;
}


/*******************************************************************
 * Function:		BOOL USER_USB_CALLBACK_EVENT_HANDLER(
 *						  USB_EVENT event, void *pdata, WORD size)
 *******************************************************************/
BOOL USER_USB_CALLBACK_EVENT_HANDLER(USB_EVENT event, void *pdata, WORD size)
{
	switch(event)
	{
		case EVENT_CONFIGURED: 
			USBCBInitEP();
			break;
		case EVENT_SET_DESCRIPTOR:
			USBCBStdSetDscHandler();
			break;
		case EVENT_EP0_REQUEST:
			USBCBCheckOtherReq();
			break;
		case EVENT_SOF:
			USBCB_SOF_Handler();
			break;
		case EVENT_SUSPEND:
			USBCBSuspend();
			break;
		case EVENT_RESUME:
			USBCBWakeFromSuspend();
			break;
		case EVENT_BUS_ERROR:
			USBCBErrorHandler();
			break;
		case EVENT_TRANSFER:
			Nop();
			break;
		default:
			break;
	}
	return TRUE; 
}


// *****************************************************************************
// ************** USB Class Specific Callback Function(s) **********************
// *****************************************************************************

/********************************************************************
 * Function:		void USBHIDCBSetReportHandler(void)
 *******************************************************************/
void USBHIDCBSetReportHandler(void)
{
	USBEP0Receive((BYTE*)&CtrlTrfData, USB_EP0_BUFF_SIZE, USBHIDCBSetReportComplete);
}

void USBHIDCBSetReportComplete(void)
{
}

void myKbPinOut(unsigned char pinNo, unsigned char onoff) {
	switch(pinNo) {
	case 0: LATAbits.LATA0 = onoff; break;
	case 1: LATAbits.LATA1 = onoff; break;
	case 2: LATAbits.LATA2 = onoff; break;
	case 3: LATAbits.LATA3 = onoff; break;
	case 4: LATAbits.LATA4 = onoff; break;
	case 5: LATAbits.LATA5 = onoff; break;
	case 6: LATCbits.LATC0 = onoff; break;
	case 7: LATCbits.LATC1 = onoff; break;
	}
} 

BOOL myKbChkIn(unsigned char pinNo) {
	switch(pinNo) {
	case 0: return PORTBbits.RB0;
	case 1: return PORTBbits.RB1;
	case 2: return PORTBbits.RB2;
	case 3: return PORTBbits.RB3;
	case 4: return PORTBbits.RB4;
	case 5: return PORTBbits.RB5;
	case 6: return PORTCbits.RC6;
	case 7: return PORTCbits.RC7;
	}
}

/** EOF Keyboard.c **********************************************/
#endif
