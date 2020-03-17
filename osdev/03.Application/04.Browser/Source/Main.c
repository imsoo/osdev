#include "Main.h"
#include "OSLibrary.h"
#include "refcnt.h"
#include "refhash.h"
#include "refalist.h"
#include "dom.h"
#include "stream.h"
#include "html.h"
#include "css.h"
#include "style.h"
#include "layout.h"
#include "display.h"
#include "print.h"

/*
  Draw Filename (imageView Task)
*/
static void DrawURLName(QWORD qwWindowID, RECT* pstArea, char *pcURL,
  int iNameLength)
{
  DrawRect(qwWindowID, pstArea->iX1 + 1, pstArea->iY1 + 1, pstArea->iX2 - 1,
    pstArea->iY2 - 1, WINDOW_COLOR_BACKGROUND, TRUE);

  // Draw File name
  DrawText(qwWindowID, pstArea->iX1 + 2, pstArea->iY1 + 2, WINDOW_COLOR_WHITE,
    WINDOW_COLOR_BACKGROUND, pcURL, iNameLength);

  // Draw Cursor
  if (iNameLength < MAX_URL_LENGTH)
  {
    DrawText(qwWindowID, pstArea->iX1 + 2 + FONT_ENGLISHWIDTH * iNameLength,
      pstArea->iY1 + 2, WINDOW_COLOR_WHITE, WINDOW_COLOR_BACKGROUND, "_", 1);
  }

  // Update Screen
  UpdateScreenByWindowArea(qwWindowID, pstArea);
}

static BOOL DrawImage(QWORD qwWindowID, int iX, int iY, const char* pcFileName)
{
  int i, j;
  DIR* pstDirectory;
  struct dirent* pstEntry;
  DWORD dwFileSize;
  BYTE* pbFileBuffer;
  COLOR* pstOutputBuffer;
  int iWindowWidth;
  FILE* fp;
  JPEG* pstJpeg;

  // Init
  fp = NULL;
  pbFileBuffer = NULL;
  pstOutputBuffer = NULL;

  // Open Root Dir and Find file
  pstDirectory = opendir("/");
  dwFileSize = 0;
  while (1)
  {
    // Read Entry
    pstEntry = readdir(pstDirectory);
    if (pstEntry == NULL)
    {
      break;
    }

    // Comapare filename and length
    if ((strlen(pstEntry->d_name) == strlen(pcFileName)) &&
      (memcmp(pstEntry->d_name, pcFileName, strlen(pcFileName))
        == 0))
    {
      dwFileSize = pstEntry->dwFileSize;
      break;
    }
  }
  closedir(pstDirectory);

  if (dwFileSize == 0)
  {
    printf("[Browser-Image] %s file doesn't exist or size is zero\n", pcFileName);
    return FALSE;
  }

  // open file
  fp = fopen(pcFileName, "rb");
  if (fp == NULL)
  {
    printf("[Browser-Image] %s file open fail\n", pcFileName);
    return FALSE;
  }

  // Allocate Buffer for File, JPEG 
  pbFileBuffer = (BYTE*)malloc(dwFileSize);
  pstJpeg = (JPEG*)malloc(sizeof(JPEG));
  if ((pbFileBuffer == NULL) || (pstJpeg == NULL))
  {
    printf("[Browser-Image] Buffer allocation fail\n");
    free(pbFileBuffer);
    free(pstJpeg);
    fclose(fp);
    return FALSE;
  }

  // read file and check is file JPEG Format
  if ((fread(pbFileBuffer, 1, dwFileSize, fp) != dwFileSize) ||
    (JPEGInit(pstJpeg, pbFileBuffer, dwFileSize) == FALSE))
  {
    printf("[Browser-Image] Read fail or file is not JPEG format\n");
    free(pbFileBuffer);
    free(pstJpeg);
    fclose(fp);
    return FALSE;
  }

  // Allocate Result Buffer
  pstOutputBuffer = malloc(pstJpeg->width * pstJpeg->height *
    sizeof(COLOR));

  // Decode
  if ((pstOutputBuffer != NULL) &&
    (JPEGDecode(pstJpeg, pstOutputBuffer) == TRUE))
  {
    ; //Success
  }

  // if Decode fail
  if (pstOutputBuffer == NULL)
  {
    printf("[Browser-Image] Window create fail or output buffer allocation fail\n");
    free(pbFileBuffer);
    free(pstJpeg);
    free(pstOutputBuffer);
    return FALSE;
  }

  // copy Image to Window buffer
  BitBlt(qwWindowID, iX, iY, pstOutputBuffer, pstJpeg->width, pstJpeg->height);

  // Free Buffer
  free(pbFileBuffer);
  ShowWindow(qwWindowID, TRUE);

  // Free Buffer
  free(pstJpeg);
  free(pstOutputBuffer);

  return TRUE;
}

static BOOL RequestHTTP(char* pcURL, char* file_name) {
  int i, j;
  char hostname[20];
  char path[30];
  char http[100];
  int url_len = strlen(pcURL);
  printf("RequestHTTP : %s\n", pcURL);
  // URL 파싱 : TOOD 개선, DNS 처리 추가
  for (i = 0; i < url_len; i++) {
    if (pcURL[i] == '/')
      break;
  }
  memcpy(hostname, pcURL, i); hostname[i] = '\0';
  memcpy(path, pcURL + i, url_len - i); path[url_len - i] = '\0';
  //printf("hostname : %s %s\n", hostname, path);

  // HTTP 요청 메시지 생성
  sprintf(http, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", path, hostname);
  //printf("http : %s\n", http);

  QWORD dwIP;
  char* pcTemp;
  BYTE bTemp;
  // IP 주소 정수로 변환
  dwIP = 0;
  pcTemp = hostname;
  bTemp = 0;
  while (*pcTemp != '\0') {
    if (*pcTemp == '.') {
      dwIP = (dwIP << 8) + bTemp;
      bTemp = 0;
    }
    else {
      bTemp = (bTemp * 10) + (*pcTemp - '0');
    }
    pcTemp++;
  }
  dwIP = (dwIP << 8) + bTemp;

  // TCP 연결
  printf("TCP | Connection Try...\n");
  TCP_TCB* pstTCB;
  pstTCB = TCP_Open((GetTickCount() * rand()) % 52525 + 1024, (dwIP << 16) | 80, TCP_ACTIVE);
  if (pstTCB == NULL) {
    printf("TCP | Connection failed...\n");
    return FALSE;
  }

  while (TCP_Status(pstTCB) != TCP_ESTABLISHED);
  printf("TCP | Connection established...\n");
  // HTTP GET 전송
  TCP_Send(pstTCB, http, strlen(http), TCP_PUSH);

  int dwRecvLen;
  int index = 0;
  BYTE vbBuf[100000];
  while (1) {
    dwRecvLen = TCP_Recv(pstTCB, vbBuf + index, 1380, TCP_NONBLOCK);
    if (dwRecvLen < 0) {  // 연결 종로
      printf("TCP | Connection closed by the remote end...\n");
      break;
    }
    else if (dwRecvLen != 0) {  // 수신 받은 데이터가 있는 경우
      index += dwRecvLen;
    }
  }
  // 응답 메시지 모두 받은 경우 TCP 연결 종료
  TCP_Close(pstTCB);
  printf("TCP | Connection closed...\n");

  // 응답 성공 확인 : TODO 에러 처리 300 400 ....
  if (memcmp(vbBuf, "HTTP/1.1 200 OK\r\n", 17) != 0) {
    printf("HTTP | Fail\n");
    return FALSE;
  }

  // 엔티티 찾기
  for (i = 0; i < index; i++) {
    if (memcmp(vbBuf + i, "\r\n\r\n", 4) == 0)
      break;
  }
  i += 4;

  // 엔티티 파일로 저장
  FILE* pstFile;
  pstFile = fopen(file_name, "w");
  if (pstFile == NULL)
  {
    printf("File Create Fail\n");
    return FALSE;
  }
  fwrite(vbBuf + i, 1, index - i, pstFile);
  fclose(pstFile);

  printf("HTTP | File Save\n");
  return TRUE;
}

static void DrawContent(QWORD qwWindowID, RECT* pstArea, char *pcURL,
  int iNameLength)
{
  int i, j;

  // HTML ENGINE
  char buf[100];
  const char* html = "t.html";
  const char* css = "t.css";
  const char* default_css = "* { display: block; padding: 12px; }";
  // HTML 요청 실패
  printf("1\n");
  if (RequestHTTP(pcURL, "t.html") != TRUE) {
    DrawText(qwWindowID, pstArea->iX1 + 5, pstArea->iY1 + 5, RGB(0, 0, 0), RGB(255, 255, 255),
      "Could not connected", 20);
  }
  // 성공
  else {

    Stream *r = file_stream(html);
    if (!r) {
      DrawText(qwWindowID, pstArea->iX1 + 5, pstArea->iY1 + 5, RGB(0, 0, 0), RGB(255, 255, 255),
        "Could not open HTML", 20);
      goto error;
    }
    Node *n = parse(r);
    printf("4\n");
    strm_done(r);
    if (!n) {
      DrawText(qwWindowID, pstArea->iX1 + 5, pstArea->iY1 + 5, RGB(0, 0, 0), RGB(255, 255, 255),
        "Could not parse HTML", 20);
      goto error;
    }

    memcpy(buf, pcURL, strlen(pcURL) + 1);
    for (i = 0; i < strlen(buf); i++) {
      if (buf[i] == '.')
        j = i;
    }

    memcpy(buf + j, ".css", 5);
    if (RequestHTTP(buf, "t.css") != TRUE) r = string_stream(default_css);
    else r = file_stream(css);

    if (!r) {
      DrawText(qwWindowID, pstArea->iX1 + 5, pstArea->iY1 + 5, RGB(0, 0, 0), RGB(255, 255, 255),
        "Could not open CSS", 20);
      goto error;
    }

    Stylesheet *ss = parse_rules(r);
    strm_done(r);
    if (!ss) {
      DrawText(qwWindowID, pstArea->iX1 + 5, pstArea->iY1 + 5, RGB(0, 0, 0), RGB(255, 255, 255),
        "Could not parse CSS", 20);
      goto error;
    }
    // print_styles(ss);

    StyledNode *sn = style_tree(n, ss);

    Dimensions viewport;
    memset(&viewport, 0, sizeof viewport);
    viewport.content.width = 800.0f;
    viewport.content.height = 600.0f;

    LayoutBox *lb = layout_tree(sn, viewport);
    // print_layoutbox(lb, 0);

    ArrayList *displayList = build_display_list(lb);
    // print_displaylist(displayList);

    // 영역 초기화
    DrawRect(qwWindowID, pstArea->iX1, pstArea->iY1,
      pstArea->iX2, pstArea->iY2, WINDOW_COLOR_WHITE, TRUE);

    for (i = 0; i < al_size(displayList); i++) {
      DisplayCommand *item = al_get(displayList, i);
      switch (item->type) {
      case SolidColor: {
        Rect rect = item->solidColor.rect;
        char* color = item->solidColor.color;
        char t_buf[9] = { 0 };
        for (j = 0; j < 3; j++) {
          t_buf[j * 3] = color[j * 2];
          t_buf[j * 3 + 1] = color[j * 2 + 1];
        }

        DrawRect(qwWindowID, pstArea->iX1 + rect.x, pstArea->iY1 + rect.y,
          pstArea->iX1 + rect.x + rect.width, pstArea->iY1 + rect.y + rect.height,
          RGB(atoi(t_buf, 16), atoi(t_buf + 3, 16), atoi(t_buf + 6, 16)),
          TRUE);

      } break;

      case Text: {
        Rect rect = item->Text.rect;
        DrawText(qwWindowID, pstArea->iX1 + rect.x, pstArea->iY1 + rect.y, RGB(0, 0, 0), RGB(255, 255, 255),
          item->Text.text, strlen(item->Text.text));
      } break;

      case Img: {
        Rect rect = item->Img.rect;
        const char* src = item->Img.src;
        const char* alt = item->Img.alt;

        // HTTP 요청 URL 생성
        memcpy(buf, pcURL, strlen(pcURL) + 1);
        for (i = 0; i < strlen(buf); i++) {
          if (buf[i] == '/')
            j = i;
        }
        memcpy(buf + j + 1, src, strlen(src) + 1);
        // HTTP 요청 전송 
        if (RequestHTTP(buf, src) != TRUE) goto alt;

        if (DrawImage(qwWindowID, pstArea->iX1 + rect.x, pstArea->iY1 + rect.y, src) == FALSE) {
        alt:
          // 실패시 alt 출력
          DrawText(qwWindowID, pstArea->iX1 + rect.x, pstArea->iY1 + rect.y, RGB(0, 0, 0), RGB(255, 255, 255),
            alt, strlen(alt));
        }
      } break;

      default:
        break;
      }
    }
    error:
      rc_release(n);
      rc_release(ss);
      rc_release(sn);
      rc_release(lb);
      rc_release(displayList);
  }
  // Update Screen
  UpdateScreenByWindowArea(qwWindowID, pstArea);
}

// Entry Point
int Main(char* pcArgument)
{
  char vcURL[MAX_URL_LENGTH + 1];
  int iURLNameLength;

  QWORD qwWindowID;

  int iX;
  int iY;
  int iWidth;
  int iHeight;

  EVENT stSendEvent;
  EVENT stReceivedEvent;
  KEYEVENT* pstKeyEvent;
  WINDOWEVENT* pstWindowEvent;

  RECT stURLBoxArea;
  RECT stContentArea;
  RECT stScreenArea;

  if (IsGraphicMode() == FALSE)
  {
    printf("This task can run only GUI Mode...\n");
    return -1;
  }

  // Create Window
  GetScreenArea(&stScreenArea);
  iWidth = 800 + UI_MARGIN * 2;
  iHeight = 600 + UI_URL_BOX_HEIGHT + UI_MARGIN * 4;
  iX = (GetRectangleWidth(&stScreenArea) - iWidth) / 2;
  iY = (GetRectangleHeight(&stScreenArea) - iHeight) / 2;
  qwWindowID = CreateWindow(iX, iY, iWidth, iHeight, WINDOW_FLAGS_DEFAULT, "Web Browser");

  // Draw URL Box Frame
  SetRectangleData(UI_MARGIN, WINDOW_TITLEBAR_HEIGHT + UI_MARGIN, iWidth - UI_MARGIN,
    WINDOW_TITLEBAR_HEIGHT + UI_URL_BOX_HEIGHT, &stURLBoxArea);
  DrawRect(qwWindowID, stURLBoxArea.iX1, stURLBoxArea.iY1,
    stURLBoxArea.iX2, stURLBoxArea.iY2, WINDOW_COLOR_WHITE, FALSE);

  // Draw URL Box Text
  iURLNameLength = 0;
  memset(vcURL, 0, sizeof(vcURL));
  DrawURLName(qwWindowID, &stURLBoxArea, vcURL, iURLNameLength);

  // Draw Content Frame
  SetRectangleData(UI_MARGIN, WINDOW_TITLEBAR_HEIGHT + GetRectangleHeight(&stURLBoxArea) + UI_MARGIN,
    iWidth - UI_MARGIN, iHeight - UI_MARGIN, &stContentArea);
  DrawRect(qwWindowID, stContentArea.iX1, stContentArea.iY1,
    stContentArea.iX2, stContentArea.iY2, WINDOW_COLOR_WHITE, TRUE);

  // Update Window
  ShowWindow(qwWindowID, TRUE);

  while (1)
  {
    if (ReceiveEventFromWindowQueue(qwWindowID, &stReceivedEvent) == FALSE)
    {
      Sleep(10);
      continue;
    }

    switch (stReceivedEvent.qwType)
    {
    case EVENT_KEY_DOWN:
      pstKeyEvent = &(stReceivedEvent.stKeyEvent);

      // Backspace : remove one character that in input buffer
      if ((pstKeyEvent->bASCIICode == KEY_BACKSPACE) &&
        (iURLNameLength > 0))
      {
        vcURL[iURLNameLength] = '\0';
        iURLNameLength--;

        DrawURLName(qwWindowID, &stURLBoxArea, vcURL, iURLNameLength);
      }
      // Enter = Button click, Make Button Click Event
      else if ((pstKeyEvent->bASCIICode == KEY_ENTER) &&
        (iURLNameLength > 0))
      {
        // Parse
        DrawContent(qwWindowID, &stContentArea, vcURL, iURLNameLength);
      }
      // ESC = Close Window, Make Window Close Event
      else if (pstKeyEvent->bASCIICode == KEY_ESC)
      {
        SetWindowEvent(qwWindowID, EVENT_WINDOW_CLOSE, &stSendEvent);
        SendEventToWindow(qwWindowID, &stSendEvent);
      }
      // Etc = filename, insert char to input buffer
      else if ((pstKeyEvent->bASCIICode <= 128) &&
        (pstKeyEvent->bASCIICode != KEY_BACKSPACE) &&
        (iURLNameLength < MAX_URL_LENGTH))
      {
        vcURL[iURLNameLength] = pstKeyEvent->bASCIICode;
        iURLNameLength++;

        DrawURLName(qwWindowID, &stURLBoxArea, vcURL, iURLNameLength);
      }
      break;
      break;

    case EVENT_WINDOW_CLOSE:
      if (stReceivedEvent.qwType == EVENT_WINDOW_CLOSE)
      {
        DeleteWindow(qwWindowID);
        return;
      }
      break;

    default:
      break;
    }
  }

  return 0;
}
