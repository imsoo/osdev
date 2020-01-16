#include "Types.h"
#include "AssemblyUtility.h"
#include "Keyboard.h"
#include "Queue.h"
#include "Mouse.h"

/*
  About Control Keyboard
*/

/*
  check Output Buffer (0x60) has received data
*/
BOOL kIsOutputBufferFull(void)
{
  // read status register(0x64) 
  // and check output buffer status bit(0)
  if (kInPortByte(0x64) & 0x01)
    return TRUE;
  return FALSE;
}

/*
  check input buffer (0x64) has written data by processor
*/
BOOL kIsInputBufferFull(void)
{
  // read status register(0x64) 
  // and check input buffer status bit(1)
  if (kInPortByte(0x64) & 0x02)
    return TRUE;
  return FALSE;
}

/*
  ready ACK, other scancodes put KeyQueue
*/
BOOL kWaitForAckAndPutOtherScanCode(void)
{
  int i, j;
  BYTE bData;
  BOOL bResult = FALSE;
  BOOL bMouseData;

  for (j = 0; j < 100; j++) {
    for (i = 0; i < 0xFFFF; i++) {
      if (kIsOutputBufferFull() == TRUE) {
        break;
      }
    }

    // Check Mouse Data OR KeyBoard Data
    if (kIsMouseDataInOutputBuffer() == TRUE) {
      bMouseData = TRUE;
    }
    else {
      bMouseData = FALSE;
    }

    // READ
    bData = kInPortByte(0x60);
    // ACK
    if (bData == 0xFA) {
      bResult = TRUE;
      break;
    }
    // OTHER (Mouse OR KeyBorad Data)
    else {
      if (bMouseData == FALSE) {
        kConvertScanCodeAndPutQueue(bData);
      }
      else {
        kAccumulatedMouseDataAndPutQueue(bData);
      }
    }
  }
  return bResult;
}

/*
  activate keyboard
*/
BOOL kActivateKeyboard(void)
{
  int i, j;
  BOOL bPreviousInterrupt;
  BOOL bResult;

  // interrup disable
  bPreviousInterrupt = kSetInterruptFlag(FALSE);

  // control register
  // keyborad command enable (0xAE)
  kOutPortByte(0x64, 0xAE);

  // wait
  for (i = 0; i < 0xFFFF; i++) {
    if (kIsInputBufferFull() == FALSE)
      break;
  }

  // input buffer
  // keyboard enable (0xF4)
  kOutPortByte(0x60, 0xF4);

  // wait ack
  bResult = kWaitForAckAndPutOtherScanCode();

  // interrupt set previous value
  kSetInterruptFlag(bPreviousInterrupt);

  return bResult;
}

/*
  read key from output buffer(0x60)
*/
BYTE kGetKeyboardScanCode(void)
{
  while (kIsOutputBufferFull() == FALSE);

  return kInPortByte(0x60);
}

/*
  turn on/off key board led
*/
BOOL kChangeKeyboardLED(BOOL bCapsLockOn, BOOL bNumLockOn, BOOL bScrollLockOn)
{
  int i, j;
  BOOL bPreviousInterrupt;
  BOOL bResult;
  BYTE bData;

  // interrup disable
  bPreviousInterrupt = kSetInterruptFlag(FALSE);

  // wait
  for (i = 0; i < 0xFFFF; i++) {
    if (kIsInputBufferFull() == FALSE)
      break;
  }

  // send LED change command
  kOutPortByte(0x60, 0xED);
  // wait
  for (i = 0; i < 0xFFFF; i++) {
    if (kIsInputBufferFull() == FALSE)
      break;
  }

  // wait ack
  bResult = kWaitForAckAndPutOtherScanCode();

  if (bResult == FALSE) {
    // interrupt set previous value
    kSetInterruptFlag(bPreviousInterrupt);
    return FALSE;
  }

  kOutPortByte(0x60, (bCapsLockOn << 2)
    | (bNumLockOn << 1)
    | bScrollLockOn);

  for (i = 0; i < 0xFFFF; i++) {
    if (kIsInputBufferFull() == FALSE)
      break;
  }

  // wait ack
  bResult = kWaitForAckAndPutOtherScanCode();

  // interrupt set previous value
  kSetInterruptFlag(bPreviousInterrupt);
  return bResult;
}

/*
  Enable A20 gate
*/
void kEnableA20Gate(void)
{
  BYTE bOutputPortData;
  int i;

  // CR 0x64, Read Output port command (0xD0)
  kOutPortByte(0x64, 0xD0);

  // wait
  for (i = 0; i < 0xFFFF; i++) {
    if (kIsOutputBufferFull() == TRUE)
      break;
  }

  // read from output port
  bOutputPortData = kInPortByte(0x60);

  // set A20 gate bit (bit 1)
  bOutputPortData |= 0x02;

  // wait
  for (i = 0; i < 0xFFFF; i++) {
    if (kIsInputBufferFull() == FALSE)
      break;
  }

  // CR 0x64, write Output port command (0xD1)
  kOutPortByte(0x64, 0xD1);
  kOutPortByte(0x60, bOutputPortData);
}

/*
  process reboot
*/
void kReboot(void)
{
  int i;

  // wait
  for (i = 0; i < 0xFFFF; i++) {
    if (kIsInputBufferFull() == FALSE)
      break;
  }

  // CR 0x64, write Output port command (0xD1)
  kOutPortByte(0x64, 0xD1);
  kOutPortByte(0x60, 0x00);
  while (1);
}

/*
  Convert Scancode to ASCII
*/

static KEYBOARDMANAGER gs_stKeyBoardManager = { 0, };

/*
  Scancode to ASCII table
*/
static KEYMAPPINGENTRY gs_vstKeyMappingTable[KEY_MAPPINGTABLEMAXCOUNT] =
{
  /*  index       normal code     |   combine code    */
  /*  0   */  {   KEY_NONE        ,   KEY_NONE        },
  /*  1   */  {   KEY_ESC         ,   KEY_ESC         },
  /*  2   */  {   '1'             ,   '!'             },
  /*  3   */  {   '2'             ,   '@'             },
  /*  4   */  {   '3'             ,   '#'             },
  /*  5   */  {   '4'             ,   '$'             },
  /*  6   */  {   '5'             ,   '%'             },
  /*  7   */  {   '6'             ,   '^'             },
  /*  8   */  {   '7'             ,   '&'             },
  /*  9   */  {   '8'             ,   '*'             },
  /*  10  */  {   '9'             ,   '('             },
  /*  11  */  {   '0'             ,   ')'             },
  /*  12  */  {   '-'             ,   '_'             },
  /*  13  */  {   '='             ,   '+'             },
  /*  14  */  {   KEY_BACKSPACE   ,   KEY_BACKSPACE   },
  /*  15  */  {   KEY_TAB         ,   KEY_TAB         },
  /*  16  */  {   'q'             ,   'Q'             },
  /*  17  */  {   'w'             ,   'W'             },
  /*  18  */  {   'e'             ,   'E'             },
  /*  19  */  {   'r'             ,   'R'             },
  /*  20  */  {   't'             ,   'T'             },
  /*  21  */  {   'y'             ,   'Y'             },
  /*  22  */  {   'u'             ,   'U'             },
  /*  23  */  {   'i'             ,   'I'             },
  /*  24  */  {   'o'             ,   'O'             },
  /*  25  */  {   'p'             ,   'P'             },
  /*  26  */  {   '['             ,   '{'             },
  /*  27  */  {   ']'             ,   '}'             },
  /*  28  */  {   '\n'            ,   '\n'            },
  /*  29  */  {   KEY_CTRL        ,   KEY_CTRL        },
  /*  30  */  {   'a'             ,   'A'             },
  /*  31  */  {   's'             ,   'S'             },
  /*  32  */  {   'd'             ,   'D'             },
  /*  33  */  {   'f'             ,   'F'             },
  /*  34  */  {   'g'             ,   'G'             },
  /*  35  */  {   'h'             ,   'H'             },
  /*  36  */  {   'j'             ,   'J'             },
  /*  37  */  {   'k'             ,   'K'             },
  /*  38  */  {   'l'             ,   'L'             },
  /*  39  */  {   ';'             ,   ':'             },
  /*  40  */  {   '\''            ,   '\"'            },
  /*  41  */  {   '`'             ,   '~'             },
  /*  42  */  {   KEY_LSHIFT      ,   KEY_LSHIFT      },
  /*  43  */  {   '\\'            ,   '|'             },
  /*  44  */  {   'z'             ,   'Z'             },
  /*  45  */  {   'x'             ,   'X'             },
  /*  46  */  {   'c'             ,   'C'             },
  /*  47  */  {   'v'             ,   'V'             },
  /*  48  */  {   'b'             ,   'B'             },
  /*  49  */  {   'n'             ,   'N'             },
  /*  50  */  {   'm'             ,   'M'             },
  /*  51  */  {   ','             ,   '<'             },
  /*  52  */  {   '.'             ,   '>'             },
  /*  53  */  {   '/'             ,   '?'             },
  /*  54  */  {   KEY_RSHIFT      ,   KEY_RSHIFT      },
  /*  55  */  {   '*'             ,   '*'             },
  /*  56  */  {   KEY_LALT        ,   KEY_LALT        },
  /*  57  */  {   ' '             ,   ' '             },
  /*  58  */  {   KEY_CAPSLOCK    ,   KEY_CAPSLOCK    },
  /*  59  */  {   KEY_F1          ,   KEY_F1          },
  /*  60  */  {   KEY_F2          ,   KEY_F2          },
  /*  61  */  {   KEY_F3          ,   KEY_F3          },
  /*  62  */  {   KEY_F4          ,   KEY_F4          },
  /*  63  */  {   KEY_F5          ,   KEY_F5          },
  /*  64  */  {   KEY_F6          ,   KEY_F6          },
  /*  65  */  {   KEY_F7          ,   KEY_F7          },
  /*  66  */  {   KEY_F8          ,   KEY_F8          },
  /*  67  */  {   KEY_F9          ,   KEY_F9          },
  /*  68  */  {   KEY_F10         ,   KEY_F10         },
  /*  69  */  {   KEY_NUMLOCK     ,   KEY_NUMLOCK     },
  /*  70  */  {   KEY_SCROLLLOCK  ,   KEY_SCROLLLOCK  },

  /*  71  */  {   KEY_HOME        ,   '7'             },
  /*  72  */  {   KEY_UP          ,   '8'             },
  /*  73  */  {   KEY_PAGEUP      ,   '9'             },
  /*  74  */  {   '-'             ,   '-'             },
  /*  75  */  {   KEY_LEFT        ,   '4'             },
  /*  76  */  {   KEY_CENTER      ,   '5'             },
  /*  77  */  {   KEY_RIGHT       ,   '6'             },
  /*  78  */  {   '+'             ,   '+'             },
  /*  79  */  {   KEY_END         ,   '1'             },
  /*  80  */  {   KEY_DOWN        ,   '2'             },
  /*  81  */  {   KEY_PAGEDOWN    ,   '3'             },
  /*  82  */  {   KEY_INS         ,   '0'             },
  /*  83  */  {   KEY_DEL         ,   '.'             },
  /*  84  */  {   KEY_NONE        ,   KEY_NONE        },
  /*  85  */  {   KEY_NONE        ,   KEY_NONE        },
  /*  86  */  {   KEY_NONE        ,   KEY_NONE        },
  /*  87  */  {   KEY_F11         ,   KEY_F11         },
  /*  88  */  {   KEY_F12         ,   KEY_F12         }
};

/*
  KeyQueue
*/
static QUEUE gs_stKeyQueue;
static KEYDATA gs_vstKeyQueueBuffer[KEY_MAXQUEUECOUNT];

/*
  check scancode is alphabet
*/
BOOL kIsAlphabetScanCode(BYTE bScanCode)
{
  if (('a' <= gs_vstKeyMappingTable[bScanCode].bNormalCode) &&
    (gs_vstKeyMappingTable[bScanCode].bNormalCode <= 'z'))
    return TRUE;
  return FALSE;
}

/*
  check scancode is number or symbol
*/
BOOL kIsNumberOrSymbolScanCode(BYTE bScanCode)
{
  if ((2 <= bScanCode) && (bScanCode <= 53) &&
    (kIsAlphabetScanCode(bScanCode) == FALSE))
    return TRUE;
  return FALSE;
}

/*
  check scancode in numpad
*/
BOOL kIsNumberPadScanCode(BYTE bScanCode)
{
  if ((71 <= bScanCode) && (bScanCode <= 83))
    return TRUE;
  return FALSE;
}

/*
  check use combine code
*/
BOOL kIsUseCombinedCode(BYTE bScanCode)
{
  BYTE bDownScanCode;
  BOOL bUseCombineKey = FALSE;

  bDownScanCode = bScanCode & 0x7F;

  // alpha
  if (kIsAlphabetScanCode(bDownScanCode) == TRUE)
  {
    if (gs_stKeyBoardManager.bShiftDown ^ gs_stKeyBoardManager.bCapsLockOn)
      bUseCombineKey = TRUE;
    else
      bUseCombineKey = FALSE;
  }

  // number or symbol
  if (kIsNumberOrSymbolScanCode(bDownScanCode) == TRUE)
  {
    if (gs_stKeyBoardManager.bShiftDown == TRUE)
      bUseCombineKey = TRUE;
    else
      bUseCombineKey = FALSE;
  }

  // num pad
  if ((kIsNumberPadScanCode(bDownScanCode) == TRUE) &&
    (gs_stKeyBoardManager.bExtendedCodeIn == FALSE))
  {
    if (gs_stKeyBoardManager.bNumLockOn == TRUE)
      bUseCombineKey = TRUE;
    else
      bUseCombineKey = FALSE;
  }

  return bUseCombineKey;
}


void UpdateCombinationKeyStatusAndLED(BYTE bScanCode)
{
  BOOL bDown;
  BYTE bDownScanCode;
  BOOL bLEDStatusChanged = FALSE;

  if (bScanCode & 0x80) {
    bDown = FALSE;
    bDownScanCode = bScanCode & 0x7F;
  }
  else {
    bDown = TRUE;
    bDownScanCode = bScanCode;
  }

  // shift
  if ((bDownScanCode == 42) || (bDownScanCode == 54))
    gs_stKeyBoardManager.bShiftDown = bDown;
  // caps lock
  else if ((bDownScanCode == 58) && (bDown == TRUE)) {
    gs_stKeyBoardManager.bCapsLockOn ^= TRUE;
    bLEDStatusChanged = TRUE;
  }
  // num lock
  else if ((bDownScanCode == 69) && (bDown == TRUE)) {
    gs_stKeyBoardManager.bNumLockOn ^= TRUE;
    bLEDStatusChanged = TRUE;
  }
  // scroll lock
  else if ((bDownScanCode == 70) && (bDown == TRUE)) {
    gs_stKeyBoardManager.bScrollLockOn ^= TRUE;
    bLEDStatusChanged = TRUE;
  }

  // led change
  if (bLEDStatusChanged == TRUE) {
    kChangeKeyboardLED(gs_stKeyBoardManager.bCapsLockOn,
      gs_stKeyBoardManager.bNumLockOn,
      gs_stKeyBoardManager.bScrollLockOn);
  }
}

BOOL kConvertScanCodeToASCIICode(BYTE bScanCode, BYTE* pbASCIICode, BOOL* pbFlags)
{
  BOOL bUseCombineKey;

  // after pause down skip 
  if (gs_stKeyBoardManager.iSkipCountForPause > 0) {
    gs_stKeyBoardManager.iSkipCountForPause--;
    return FALSE;
  }

  // pause first down
  if (bScanCode == 0xE1) {
    *pbASCIICode = KEY_PAUSE;
    *pbFlags = KEY_FLAGS_DOWN;
    gs_stKeyBoardManager.iSkipCountForPause = KEY_SKIPCOUNTFORPAUSE;
    return TRUE;
  }
  // ext key
  else if (bScanCode == 0xE0) {
    gs_stKeyBoardManager.bExtendedCodeIn = TRUE;
    return FALSE;
  }

  // check using combine key or not
  bUseCombineKey = kIsUseCombinedCode(bScanCode);

  if (bUseCombineKey == TRUE) {
    *pbASCIICode = gs_vstKeyMappingTable[bScanCode & 0x7F].bCombinedCode;
  }
  else {
    *pbASCIICode = gs_vstKeyMappingTable[bScanCode & 0x7F].bNormalCode;
  }

  // check using ext key or not
  if (gs_stKeyBoardManager.bExtendedCodeIn == TRUE) {
    *pbFlags = KEY_FLAGS_EXTENDEDKEY;
    gs_stKeyBoardManager.bExtendedCodeIn = FALSE;
  }
  else {
    *pbFlags = 0;
  }

  if ((bScanCode & 0x80) == 0)
    *pbFlags |= KEY_FLAGS_DOWN;

  UpdateCombinationKeyStatusAndLED(bScanCode);
  return TRUE;
}

/*
  Init Keyboard
*/
BOOL kInitializeKeyboard(void)
{
  // KeyQueue init
  kInitializeQueue(&gs_stKeyQueue, &gs_vstKeyQueueBuffer, KEY_MAXQUEUECOUNT, sizeof(KEYDATA));

  // SpinLock Init
  kInitializeSpinLock(&(gs_stKeyBoardManager.stSpinLock));

  // activate keyboard
  return kActivateKeyboard();
}

/*
  Scan code convert to KEYDATA and Put into KeyQueue
*/
BOOL kConvertScanCodeAndPutQueue(BYTE bScanCode)
{
  KEYDATA stData;
  BOOL bResult;

  stData.bScanCode = bScanCode;

  if (kConvertScanCodeToASCIICode(bScanCode, &(stData.bASCIICode), &(stData.bFlags)) == TRUE) {
    
    // --- CRITCAL SECTION BEGIN ---
    kLockForSpinLock(&(gs_stKeyBoardManager.stSpinLock));

    // Put into KeyQueue
    bResult = kPutQueue(&gs_stKeyQueue, &stData);

    kUnlockForSpinLock(&(gs_stKeyBoardManager.stSpinLock));
    // --- CRITCAL SECTION END ---
  }

  return bResult;
}

/*
  get from KeyQueue
*/
BOOL kGetKeyFromKeyQueue(KEYDATA *pstData)
{
  BOOL bResult;

  // --- CRITCAL SECTION BEGIN ---
  kLockForSpinLock(&(gs_stKeyBoardManager.stSpinLock));

  // Get from KeyQueue
  bResult = kGetQueue(&gs_stKeyQueue, pstData);

  kUnlockForSpinLock(&(gs_stKeyBoardManager.stSpinLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}
