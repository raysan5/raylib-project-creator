/*******************************************************************************************
*
*   Window About/Welcome
*
*   MODULE USAGE:
*       #define GUI_WINDOW_ABOUT_IMPLEMENTATION
*       #include "gui_window_about.h"
*
*   On game init call:  GuiWindowAboutState state = InitGuiWindowAbout();
*   On game draw call:  GuiWindowAbout(&state);
*
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2022-2025 raylib technologies (@raylibtech) / Ramon Santamaria (@raysan5)
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/

#include "raylib.h"

// WARNING: raygui implementation is expected to be defined before including this header

#ifndef GUI_WINDOW_ABOUT_H
#define GUI_WINDOW_ABOUT_H

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// Gui window structure declaration
typedef struct {
    bool windowActive;
    bool supportDrag;
    bool borderless;

    Rectangle windowBounds;
    Vector2 panOffset;
    bool dragMode;

    bool showSplash;
    bool welcomeMode;

} GuiWindowAboutState;

#ifdef __cplusplus
extern "C" {            // Prevents name mangling of functions
#endif

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
GuiWindowAboutState InitGuiWindowAbout(void);
void GuiWindowAbout(GuiWindowAboutState *state);

#ifdef __cplusplus
}
#endif

#endif // GUI_WINDOW_ABOUT_H

/***********************************************************************************
*
*   GUI_WINDOW_ABOUT IMPLEMENTATION
*
************************************************************************************/

#if defined(GUI_WINDOW_ABOUT_IMPLEMENTATION)

#include "raygui.h"

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
// The following define is required to customize the window info
#if !defined(TOOL_NAME)
    #define TOOL_NAME           "rTool"
#endif
#if !defined(TOOL_SHORT_NAME)
    #define TOOL_SHORT_NAME     "rTN"
#endif
#if !defined(TOOL_VERSION)
    #define TOOL_VERSION        "1.0"
#endif
#if !defined(TOOL_DESCRIPTION)
    #define TOOL_DESCRIPTION    "A simple and easy-to-use tool to do something"
#endif
#if !defined(TOOL_RELEASE_DATE)
    #define TOOL_RELEASE_DATE   "Jan.2025"
#endif
#if !defined(TOOL_LOGO_COLOR)
    #define TOOL_LOGO_COLOR      0x000000ff
#endif

#if defined(NO_ALPHA_BLENDING)
    #define FADE(c,a)   c
#else
    #define FADE(c,a)   Fade(c,a)
#endif

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// ...

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static const char *lblCopyrightText = "Copyright (c) 2025 raylib technologies";
static const char *linkraylibtechText = "[@raylibtech]";
static const char *lblMoreInfoText = "More info:";
static const char *linkMailText = "ray@raylibtech.com";
static const char *lblSupportText = "Support:";
//static const char *btnWebText = "#185#Webpage";  // TODO: Show #23#EULA?

//----------------------------------------------------------------------------------
// Internal Functions Declaration
//----------------------------------------------------------------------------------
// Tech button control, returns true when clicked
static bool GuiTechButton(Rectangle bounds, const char *text, int textSize, bool corner, Color colors[4]);

// Draw rTool generated icon
static void DrawTechIcon(int posX, int posY, int size, const char *text, int textSize, bool corner, Color colors[4]);

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------

// Init Window Sponsor
GuiWindowAboutState InitGuiWindowAbout(void)
{
    GuiWindowAboutState state = { 0 };

    state.windowActive = true;
    state.supportDrag = false;
    state.borderless = false;
    
    state.windowBounds = (Rectangle){ (float)GetScreenWidth()/2 - 440/2, (float)GetScreenHeight()/2 - 440/2, 440, 440 };
    state.panOffset = (Vector2){ 0, 0 };
    state.dragMode = false;

    state.showSplash = true;
    state.welcomeMode = true;

    return state;
}

// Sponsor Window update & draw
void GuiWindowAbout(GuiWindowAboutState *state)
{
    if (state->windowActive)
    {
        Color rtoolCols[4] = { RAYWHITE, GetColor(TOOL_LOGO_COLOR), GetColor(TOOL_LOGO_COLOR), GetColor(TOOL_LOGO_COLOR) };
        Color raylibCols[4] = { RAYWHITE, BLACK, BLACK, BLANK };
        Color raylibtechCols[4] = { RAYWHITE, BLACK, BLACK, GetColor(0xdc1e28ff) };
        Color rayguiCols[4] = { GetColor(0xe2e2e2ff), GetColor(0xacacacff), GetColor(0x949494ff), BLANK };
        //Color rpngCols[4] = { GetColor(0xe1d7b4ff), GetColor(0x967d32ff), GetColor(0x7d642dff), BLANK };
        Color riniCols[4] = { GetColor(0xf0ebcdff), GetColor(0xd1b72fff), GetColor(0xc3a91fff), BLANK };

        // Update window dragging
        //----------------------------------------------------------------------------------------
        if (state->supportDrag)
        {
            Vector2 mousePosition = GetMousePosition();

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                // Window can be dragged from the top window bar
                if (CheckCollisionPointRec(mousePosition, (Rectangle){ state->windowBounds.x, state->windowBounds.y, (float)state->windowBounds.width, RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT }))
                {
                    state->dragMode = true;
                    state->panOffset.x = mousePosition.x - state->windowBounds.x;
                    state->panOffset.y = mousePosition.y - state->windowBounds.y;
                }
            }

            if (state->dragMode)
            {
                state->windowBounds.x = (mousePosition.x - state->panOffset.x);
                state->windowBounds.y = (mousePosition.y - state->panOffset.y);

                // Check screen limits to avoid moving out of screen
                if (state->windowBounds.x < 0) state->windowBounds.x = 0;
                else if (state->windowBounds.x > (GetScreenWidth() - state->windowBounds.width)) state->windowBounds.x = GetScreenWidth() - state->windowBounds.width;

                if (state->windowBounds.y < 40) state->windowBounds.y = 40;
                else if (state->windowBounds.y > (GetScreenHeight() - state->windowBounds.height - 24)) state->windowBounds.y = GetScreenHeight() - state->windowBounds.height - 24;

                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) state->dragMode = false;
            }
        }

        if (CheckCollisionPointRec(GetMousePosition(), (Rectangle){ state->windowBounds.x + 12, state->windowBounds.y + 24 + 10, 96, 96 }) ||   // Tool icon
            CheckCollisionPointRec(GetMousePosition(), (Rectangle){ state->windowBounds.x + 12 + (96 + 10)*0, state->windowBounds.y + 24 + 116 + 30 + 10, 96, 96 }) ||
            CheckCollisionPointRec(GetMousePosition(), (Rectangle){ state->windowBounds.x + 12 + (96 + 10)*1, state->windowBounds.y + 24 + 116 + 30 + 10, 96, 96 }) ||
            CheckCollisionPointRec(GetMousePosition(), (Rectangle){ state->windowBounds.x + 12 + (96 + 10)*2, state->windowBounds.y + 24 + 116 + 30 + 10, 96, 96 }) ||
            CheckCollisionPointRec(GetMousePosition(), (Rectangle){ state->windowBounds.x + 12 + (96 + 8)*3, state->windowBounds.y + 24 + 116 + 30 + 10, 96, 96 }) ||
            CheckCollisionPointRec(GetMousePosition(), (Rectangle){ state->windowBounds.x + 12, state->windowBounds.y + 24 + 116 + 34 + 8 + 100 + 13, 96, 96 }))
        {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        }
        else SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        //----------------------------------------------------------------------------------------

        // Draw window and controls
        //----------------------------------------------------------------------------------------
        state->windowBounds = (Rectangle){ (float)GetScreenWidth()/2 - 440/2, (float)GetScreenHeight()/2 - 440/2, 440, 440 };

        state->windowActive = !GuiWindowBox(state->windowBounds, TextFormat((state->welcomeMode? "#186#Welcome to %s!" : "#191#About %s"), TOOL_NAME));

        // Draw top line info: tool logo, name and description
        DrawRectangleRec((Rectangle){ state->windowBounds.x + 1, state->windowBounds.y + 24, state->windowBounds.width - 2, 116 }, FADE(GetColor(GuiGetStyle(DEFAULT, BASE_COLOR_NORMAL)), 0.5f));
        if (GuiTechButton((Rectangle){ state->windowBounds.x + 12, state->windowBounds.y + 24 + 10, 96, 96 }, TOOL_SHORT_NAME, 30, true, rtoolCols))
        {
            OpenURL("https://raylibtech.itch.io/rpb");
        }
#if defined(TOOL_DESCRIPTION_BREAK)
        GuiLabel((Rectangle){ state->windowBounds.x + 116, state->windowBounds.y + 48, 300, 30 }, TextFormat("%s %s (%s)", TOOL_NAME, TOOL_VERSION, TOOL_RELEASE_DATE));
        GuiLabel((Rectangle){ state->windowBounds.x + 116, state->windowBounds.y + 88, (float)state->windowBounds.width, 40 }, TOOL_DESCRIPTION_BREAK);
#else
        GuiLabel((Rectangle){ state->windowBounds.x + 116, state->windowBounds.y + 68, 200, 30 }, TextFormat("%s %s (%s)", TOOL_NAME, TOOL_VERSION, TOOL_RELEASE_DATE));
        GuiLabel((Rectangle){ state->windowBounds.x + 116, state->windowBounds.y + 94, (float)state->windowBounds.width, 40 }, TOOL_DESCRIPTION);
#endif
        // Powered by section
        GuiLine((Rectangle){ state->windowBounds.x, state->windowBounds.y + 24 + 116, (float)state->windowBounds.width, 1 }, NULL);
        GuiLabel((Rectangle){ state->windowBounds.x + 12, state->windowBounds.y + 24 + 116 + 2, state->windowBounds.width - 24, 24 }, TextFormat("%s is powered by:", TOOL_NAME));
        GuiLine((Rectangle){ state->windowBounds.x, state->windowBounds.y + 24 + 116 + 4 + 24, (float)state->windowBounds.width, 2 }, NULL);
       
        // Powered by icons and links
        //----------------------------------------------------------------------------------------
        DrawRectangleRec((Rectangle){ state->windowBounds.x + 1, state->windowBounds.y + 24 + 116 + 28 + 2, state->windowBounds.width - 2, 118 }, FADE(GetColor(GuiGetStyle(DEFAULT, BASE_COLOR_NORMAL)), 0.5f));
        GuiSetStyle(BUTTON, BORDER_WIDTH, 1);
        if (GuiTechButton((Rectangle){ state->windowBounds.x + 12 + (96 + 10)*0, state->windowBounds.y + 24 + 116 + 30 + 10, 96, 96 }, "raylib", 20, true, raylibCols)) { OpenURL("https://github.com/raysan5/raylib"); }
        if (GuiTechButton((Rectangle){ state->windowBounds.x + 12 + (96 + 10)*1, state->windowBounds.y + 24 + 116 + 30 + 10, 96, 96 }, "raygui", 20, true, rayguiCols)) { OpenURL("https://github.com/raysan5/raygui"); }
        if (GuiTechButton((Rectangle){ state->windowBounds.x + 12 + (96 + 10)*2, state->windowBounds.y + 24 + 116 + 30 + 10, 96, 96 }, "rini", 20, true, riniCols)) { OpenURL("https://github.com/raysan5/rini"); }
        if (GuiButton((Rectangle){ state->windowBounds.x + 12 + (96 + 10)*3, state->windowBounds.y + 24 + 116 + 30 + 10, 96, 96 }, "more...")) { OpenURL("https://github.com/raylibtech/rtools"); }
        //----------------------------------------------------------------------------------------

        // raylibtech section
        //----------------------------------------------------------------------------------------
        GuiSetStyle(BUTTON, BORDER_WIDTH, 2);
        GuiLine((Rectangle){ state->windowBounds.x, state->windowBounds.y + 24 + 116 + 36 + 100 + 8, (float)state->windowBounds.width, 2 }, NULL);

        if (GuiTechButton((Rectangle){ state->windowBounds.x + 12, state->windowBounds.y + 24 + 116 + 34 + 8 + 100 + 13, 96, 96 }, "raylib", 20, true, raylibtechCols)) { OpenURL("https://www.raylibtech.com"); }

        GuiLabel((Rectangle){ state->windowBounds.x + 12 + 98 + 12, state->windowBounds.y + 320, 289, 20 }, lblCopyrightText);
        GuiLabel((Rectangle){ state->windowBounds.x + 12 + 98 + 12, state->windowBounds.y + 320 + 30, 85, 16 }, lblMoreInfoText);

        float linkMailTextWidth = MeasureTextEx(GuiGetFont(), linkMailText, (float)GuiGetStyle(DEFAULT, TEXT_SIZE), (float)GuiGetStyle(DEFAULT, TEXT_SPACING)).x;
        if (GuiLabelButton((Rectangle){ state->windowBounds.x + 12 + 98 + 12 + 80, state->windowBounds.y + 320 + 30, 165, 16 }, TextFormat("www.raylibtech.com/%s", TextToLower(TOOL_SHORT_NAME)))) { OpenURL(TextFormat("https://www.raylibtech.com/%s", TextToLower(TOOL_SHORT_NAME))); }
        if (GuiLabelButton((Rectangle){ state->windowBounds.x + 12 + 98 + 12 + 80, state->windowBounds.y + 320 + 50, linkMailTextWidth, 16 }, linkMailText)) { OpenURL("mailto:ray@raylibtech.com"); }
        if (GuiLabelButton((Rectangle){ state->windowBounds.x + 12 + 98 + 12 + 80 + linkMailTextWidth + 4, state->windowBounds.y + 320 + 50, MeasureTextEx(GuiGetFont(), linkraylibtechText, (float)GuiGetStyle(DEFAULT, TEXT_SIZE), (float)GuiGetStyle(DEFAULT, TEXT_SPACING)).x, 16 }, linkraylibtechText)) { OpenURL("https://github.com/raylibtech"); }

        GuiLabel((Rectangle){ state->windowBounds.x + 12 + 98 + 12, state->windowBounds.y + 320 + 50, 65, 16 }, lblSupportText);
        //----------------------------------------------------------------------------------------

        // Bottom section with buttons
        //----------------------------------------------------------------------------------------
        DrawRectangleRec((Rectangle){ state->windowBounds.x + 1, state->windowBounds.y + state->windowBounds.height - 40, state->windowBounds.width - 2, 39 }, FADE(GetColor(GuiGetStyle(DEFAULT, BASE_COLOR_NORMAL)), 0.5f));
        GuiLine((Rectangle){ state->windowBounds.x, state->windowBounds.y + state->windowBounds.height - 41, (float)state->windowBounds.width, 2 }, NULL);

        GuiCheckBox((Rectangle){ state->windowBounds.x + 12, state->windowBounds.y + state->windowBounds.height - 31 + 4, 16, 16 }, "Show welcome window at startup", &state->showSplash);

        int buttonTextAlign = GuiGetStyle(BUTTON, TEXT_ALIGNMENT);
        GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
        //if (GuiButton((Rectangle){ state->windowBounds.x + state->windowBounds.width - 82 - 90, state->windowBounds.y + state->windowBounds.height - 31, 80, 24 }, "#186#Webpage")) { OpenURL("https://github.com/sponsors/raysan5"); }
        if (GuiButton((Rectangle){ state->windowBounds.x + state->windowBounds.width - 98 - 12, state->windowBounds.y + state->windowBounds.height - 31, 98, 24 }, "#159#Close") || !state->windowActive)
        {
            state->welcomeMode = false;
            state->windowActive = false;
        }
        GuiSetStyle(BUTTON, TEXT_ALIGNMENT, buttonTextAlign);
        //----------------------------------------------------------------------------------------
    }
}

// Image button control, returns true when clicked
static bool GuiTechButton(Rectangle bounds, const char *text, int textSize, bool corner, Color colors[4])
{
    GuiState state = guiState;
    bool pressed = false;

    // Update control
    //--------------------------------------------------------------------
    if ((state != STATE_DISABLED) && !guiLocked)
    {
        Vector2 mousePoint = GetMousePosition();

        // Check button state
        if (CheckCollisionPointRec(mousePoint, bounds))
        {
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) state = STATE_PRESSED;
            else state = STATE_FOCUSED;

            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) pressed = true;

            //SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        }
    }
    //--------------------------------------------------------------------

    // Draw control
    //--------------------------------------------------------------------
    DrawTechIcon((int)bounds.x, (int)bounds.y, (int)bounds.width, text, textSize, corner, colors);

    GuiDrawRectangle(bounds, 1,
        Fade(GetColor(GuiGetStyle(BUTTON, BORDER + (state*3))), guiAlpha), 
        (state == 0)? BLANK : Fade(GetColor(GuiGetStyle(BUTTON, BASE + (state*3))), 0.5f));
    //------------------------------------------------------------------

    return pressed;
}

// Draw rTool generated icon
// TODO: Icon can actually have up to 4 colors: BASE, BORDER, TEXT, CORNER
static void DrawTechIcon(int posX, int posY, int size, const char *text, int textSize, bool corner, Color colors[4])
{
    float borderSize = ceilf((float)size/16.0f);
    bool offsetY = true;

    // Make sure there is no character with pixels down the text baseline for a perfect y-aligned icon
    for (int i = 0; text[i] != '\0'; i++)
    {
        if ((text[i] == 'q') || (text[i] == 'y') || (text[i] == 'p') || (text[i] == 'j') || (text[i] == 'g'))
        {
            offsetY = false;
            break;
        }
    }

    int textPosX = posX + size - (int)(2.0f*borderSize) - MeasureText(text, textSize);
    int textPosY = posY + size - (int)(2.0f*borderSize) - textSize + (offsetY? (2*textSize/10) : 0);

    //DrawRectangle(posX - 1, posY - 1, size + 2, size + 2, GetColor(GuiGetStyle(DEFAULT, LINE_COLOR)));
    DrawRectangle(posX, posY, size, size, colors[0]);
    DrawRectangleLinesEx((Rectangle){ (float)posX, (float)posY, (float)size, (float)size }, borderSize, colors[1]);
    DrawText(text, textPosX, textPosY, textSize, colors[2]);

    if (corner)
    {
        DrawTriangle((Vector2){ (float)posX + (float)size - 2*borderSize - (float)size/4, (float)posY + 2*borderSize },
            (Vector2){ (float)posX + (float)size - 2*borderSize, (float)posY + 2*borderSize + (float)size/4 },
            (Vector2){ (float)posX + (float)size - 2*borderSize, (float)posY + 2*borderSize }, colors[3]);
    }
}

#endif // GUI_WINDOW_SPONSOR_IMPLEMENTATION