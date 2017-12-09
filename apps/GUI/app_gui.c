#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "app_gui.h"
#include "fsl_debug_console.h"
#include "string.h"

#define CLEAR_BUTTON_ID (GUI_ID_BUTTON0)
#define COLOR_BUTTONS 8
#define COLOR_BUTTON_FIRST_ID (GUI_ID_USER)
#define COLOR_BUTTON_LAST_ID (COLOR_BUTTON_FIRST_ID + COLOR_BUTTONS - 1)

static GUI_COLOR button_color[COLOR_BUTTONS] = {GUI_WHITE,   GUI_YELLOW, GUI_ORANGE, GUI_RED,
                                                GUI_MAGENTA, GUI_BLUE,   GUI_GREEN,  GUI_BLACK};


/*******************************************************************************
 * Application implemented functions required by emWin library
 ******************************************************************************/
void LCD_X_Config(void)
{
    GUI_DEVICE_CreateAndLink(GUIDRV_LIN_16, GUICC_565, 0, 0);

    LCD_SetSizeEx(0, LCD_WIDTH, LCD_HEIGHT);
    LCD_SetVSizeEx(0, LCD_WIDTH, LCD_HEIGHT);

    LCD_SetVRAMAddrEx(0, (void *)VRAM_ADDR);
}

int LCD_X_DisplayDriver(unsigned LayerIndex, unsigned Cmd, void *pData)
{
    return 0;
}

void GUI_X_Config(void)
{
    /* Assign work memory area to emWin */
    GUI_ALLOC_AssignMemory((void *)GUI_MEMORY_ADDR, GUI_NUMBYTES);

    /* Select default font */
    GUI_SetDefaultFont(APP_GUI_FONT);
}

void GUI_X_Init(void)
{
}

/* Dummy RTOS stub required by emWin */
void GUI_X_InitOS(void)
{
}

/* Dummy RTOS stub required by emWin */
void GUI_X_Lock(void)
{
}

/* Dummy RTOS stub required by emWin */
void GUI_X_Unlock(void)
{
}

/* Dummy RTOS stub required by emWin */
U32 GUI_X_GetTaskId(void)
{
    return 0;
}

void GUI_X_ExecIdle(void)
{
}

GUI_TIMER_TIME GUI_X_GetTime(void)
{
    return 0;
}

void GUI_X_Delay(int Period)
{
}

void *emWin_memcpy(void *pDst, const void *pSrc, long size)
{
    return memcpy(pDst, pSrc, size);
}
/*******************************************************************************/
static void cbBackgroundWin(WM_MESSAGE *pMsg)
{
    int widget_id;

    switch (pMsg->MsgId)
    {
        case WM_NOTIFY_PARENT:
            widget_id = WM_GetId(pMsg->hWinSrc);
            if (widget_id >= COLOR_BUTTON_FIRST_ID && widget_id <= COLOR_BUTTON_LAST_ID)
            {
                GUI_SetColor(button_color[widget_id - COLOR_BUTTON_FIRST_ID]);
            }
            else if (widget_id == CLEAR_BUTTON_ID && pMsg->Data.v == WM_NOTIFICATION_CLICKED)
            {
                GUI_Clear();
            }
            break;
        default:
            WM_DefaultProc(pMsg);
    }
}

static void cbCanvasWin(WM_MESSAGE *pMsg)
{
    GUI_PID_STATE *pid_state;
    static GUI_PID_STATE pid_state_0;

    switch (pMsg->MsgId)
    {
        case WM_TOUCH:
            pid_state = (GUI_PID_STATE *)pMsg->Data.p;
            if (pid_state->Pressed)
            {
                if (pid_state_0.Pressed)
                {
                    GUI_DrawLine(pid_state_0.x, pid_state_0.y, pid_state->x, pid_state->y);
                }
                else
                {
                    GUI_DrawPoint(pid_state->x, pid_state->y);
                }
            }
            pid_state_0 = *pid_state;
            break;
        default:
            WM_DefaultProc(pMsg);
    }
}

void APP_GUI_Init()
{
    status_t status;
//    int i;
    /* init LCD controller */
//    PRINTF("Initialize LCD\r\n");
    status = APP_LCDC_Init();
//    if (status != kStatus_Success)
//    {
//        PRINTF("LCD init failed\r\n");
//    }
//    else
//    {
//        PRINTF("LCD initialized\r\n");
//    }
    /* emWin start */
    GUI_Init();
    /* Set size and default color for the background window */
    WM_SetSize(WM_HBKWIN, LCD_WIDTH, LCD_HEIGHT);
    WM_SetDesktopColor(GUI_BLACK);

    /* Set callback for the backgroung window */
    WM_SetCallback(WM_HBKWIN, cbBackgroundWin);

    /* Solid color display */
    GUI_SetBkColor(GUI_BLACK);
    GUI_Clear();
//    BUTTON_SetReactOnLevel();
//
//    /* Create the 'clear' button */
//    BUTTON_Handle hButtonClear;
//    hButtonClear = BUTTON_CreateEx(4, 2, 30, 25, 0, WM_CF_SHOW, 0, CLEAR_BUTTON_ID);
//    BUTTON_SetText(hButtonClear, "CLR");
//
//    /* Create color selection buttons */
//    BUTTON_Handle hButtonColor[COLOR_BUTTONS];
//    for (i = 0; i < COLOR_BUTTONS; i++)
//    {
//        hButtonColor[i] = BUTTON_CreateEx(4, i * 30 + 32, 30, 25, 0, WM_CF_SHOW, 0, COLOR_BUTTON_FIRST_ID + i);
//        BUTTON_SetSkinClassic(hButtonColor[i]);
//        BUTTON_SetBkColor(hButtonColor[i], BUTTON_CI_UNPRESSED, button_color[i]);
//    }
//
//    /* Create canvas */
//    WM_HWIN hCanvas;
//    hCanvas = WM_CreateWindowAsChild(35, 0, WM_GetWindowSizeX(WM_HBKWIN) - 35, WM_GetWindowSizeY(WM_HBKWIN), 0,
//                                     WM_CF_SHOW, cbCanvasWin, 0);
//    /* Select canvas window and leave it selected forever */
//    WM_SelectWindow(hCanvas);
    GUI_SetColor(GUI_YELLOW);
    GUI_SetDrawMode(GUI_DM_NORMAL);
    GUI_SetPenSize(6);
//    GUI_DispStringHCenterAt("Touch and drag to draw.\nClick the CLR button to clear the canvas.",
//                            WM_GetWindowSizeX(hCanvas) / 2, WM_GetWindowSizeY(hCanvas) / 2);
    //GUI_DispNextLine();
    GUI_DispStringHCenterAt("ChauNM Pilot Project - LPC54608\n", LCD_WIDTH / 2, 0);
    GUI_DispNextLine();
    WM_Exec();
}

int APP_GUI_printf(const char* string, ...)
{
    char* displayString = (char *)malloc(1024);
    va_list ap;
    va_start(ap, string);
    vsprintf(displayString, string, ap);
    va_end(ap);
    int line;
    line = GUI_GetDispPosY();
    if (line >= LCD_HEIGHT - APP_GUI_FONT_H)
    {
        WM_Exec();
        GUI_Clear();
        GUI_GotoXY(0,0);
    }    
    GUI_DispString(displayString);
    WM_Exec();
    free(displayString);
    return 0;
}