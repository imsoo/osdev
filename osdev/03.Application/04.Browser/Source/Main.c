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

// Entry Point
int Main(char* pcArgument)
{
  QWORD qwWindowID;
  int i, j;
  int iX;
  int iY;
  int iWidth;
  int iHeight;
  EVENT stReceivedEvent;
  KEYEVENT* pstKeyEvent;
  WINDOWEVENT* pstWindowEvent;
  RECT stScreenArea;

  if (IsGraphicMode() == FALSE)
  {
    printf("This task can run only GUI Mode...\n");
    return -1;
  }

  // HTML ENGINE
  rc_init();
  const char* html = "t.html";
  const char* css = "d.css";

  Stream *r = file_stream(html);
  // Stream *r = string_stream("<html><head>HEAD</head><body>BODY</body></html>");
  if (!r) {
    return 1;
  }
  Node *n = parse(r);
  strm_done(r);
  if (!n) {
    return 1;
  }

  r = file_stream(css);
  if (!r) {
    return 1;
  }
  Stylesheet *ss = parse_rules(r);
  strm_done(r);
  if (!ss) {
    return 1;
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

  // Create Window
  GetScreenArea(&stScreenArea);
  iWidth = 800;
  iHeight = 600;
  iX = (GetRectangleWidth(&stScreenArea) - iWidth) / 2;
  iY = (GetRectangleHeight(&stScreenArea) - iHeight) / 2;
  qwWindowID = CreateWindow(iX, iY, iWidth, iHeight, WINDOW_FLAGS_DEFAULT, "Web Browser");

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

      DrawRect(qwWindowID, rect.x, rect.y,
        rect.x + rect.width, rect.y + rect.height,
        RGB(atoi(t_buf, 16), atoi(t_buf + 3, 16), atoi(t_buf + 6, 16)),
        TRUE);

    } break;
    case Text: {
      Rect rect = item->Text.rect;
      DrawText(qwWindowID, rect.x, rect.y, RGB(255, 255, 255), RGB(0, 0, 0),
        item->Text.text, strlen(item->Text.text));
    } break;
    default: break;
    }
  }

  rc_release(n);
  rc_release(ss);
  rc_release(sn);
  rc_release(lb);
  rc_release(displayList);

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
      if (pstKeyEvent->bFlags & KEY_FLAGS_DOWN)
      {
      }
      break;

    case EVENT_WINDOW_CLOSE:
      DeleteWindow(qwWindowID);
      return 0;
      break;

    default:
      break;
    }
  }

  return 0;
}
