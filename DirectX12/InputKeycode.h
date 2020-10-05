#pragma once
//*******************************
//		    DirectInput
//*******************************

/*同時押しの判定を取りたいときはビット和演算をすれば取れます*/

//version
#define DINPUT_VERTION 0x80		//バージョン指定マクロ

//BOTTON						[Xbox]			[PS4]
#define DINPUT_BOTTON_0		0	//A				//□
#define DINPUT_BOTTON_1		1	//B				//×
#define DINPUT_BOTTON_2		2	//X				//〇
#define DINPUT_BOTTON_3		3	//Y				//△
#define DINPUT_BOTTON_4		4	//L1			//L1
#define DINPUT_BOTTON_5		5	//R1			//R1
#define DINPUT_BOTTON_6		6	//view			//L2
#define DINPUT_BOTTON_7		7	//menu			//R2
#define DINPUT_BOTTON_8		8	//L3			//SHARE
#define DINPUT_BOTTON_9		9	//R3			//START
#define DINPUT_BOTTON_10	10	//割り当てなし   //L3
#define DINPUT_BOTTON_11	11	//割り当てなし   //R3
#define DINPUT_BOTTON_12	12	//割り当てなし	//タッチパッド

//POV
#define DINPUT_POV_UP		0	//POV-UP
#define DINPUT_POV_RIGHT	1	//POV-RIGHT
#define DINPUT_POV_DOWN		2	//POV-DOWN
#define DINPUT_POV_LEFT		3	//POV-LEFT

//KeyBorad
//DirectInput8標準のキーマクロを使用


//*******************************
//		    XInput
//*******************************

//botton
#define XINPUT_BOTTON_A		0	//A			
#define XINPUT_BOTTON_B		1	//B			
#define XINPUT_BOTTON_X		2	//X			
#define XINPUT_BOTTON_Y		3	//Y			
#define XINPUT_BOTTON_L1	4	//L1		
#define XINPUT_BOTTON_R1	5	//R1		
#define XINPUT_BOTTON_VIEW	6	//view		
#define XINPUT_BOTTON_MENU	7	//menu		
#define XINPUT_BOTTON_L3	8	//L3		
#define XINPUT_BOTTON_R3	9	//R3		

//POV
#define XINPUT_POV_UP		DINPUT_POV_UP		//POV-UP
#define XINPUT_POV_RIGHT	DINPUT_POV_RIGHT	//POV-RIGHT
#define XINPUT_POV_DOWN		DINPUT_POV_DOWN		//POV-DOWN
#define XINPUT_POV_LEFT		DINPUT_POV_LEFT		//POV-LEFT