/*******************************************************************************************
*
*   rpc v2.0 - A simple and easy-to-use raylib projects creator
*
*   FEATURES:
*       - Generate complete build systems: Makefile, VSCode, VS2022
*       - Generate complete GitHub project, ready to upload
-       - Generate preconfigured GitHub Actions, ready to run
*       - WEB: Download generated template as a .zip file
*
*   LIMITATIONS:
*       - Script: build.bat requires Makefile, it could be a cmd/shell script instead
*       - VSCode: Requires compiler tools (make.exe) in the system path
*
*   CONFIGURATION:
*       #define CUSTOM_MODAL_DIALOGS
*           Use custom raygui generated modal dialogs instead of native OS ones
*           NOTE: Avoids including tinyfiledialogs depencency library
*
*   VERSIONS HISTORY:
*       2.0  (xx-Nov-2025)  ADDED: Load and save project configuration data
*                           ADDED: Resource scan from code files
*                           REVIEWED: Resource management, copy to generated project
*
*       1.1  (30-Sep-2024)  ADDED: Support raylib path as property on VS2022 projects
*                           ADDED: Support for HighDPI/4K monitors, scaling UI automatically
*                           REVIEWED: Issue with browser files filter (*.c;*.h) not working properly
*                           REVIEWED: Issue while building Basic Sample
*
*       1.0  (26-Sep-2024)  First release
*
*   DEPENDENCIES:
*       raylib 5.6-dev          - Windowing/input management and drawing
*       raygui 4.5-dev          - Immediate-mode GUI controls with custom styling and icons
*       tinyfiledialogs 3.20.0  - Open/save file dialogs, it requires linkage with comdlg32 and ole32 libs
*       miniz 2.2.0             - Save .zip package file (required for multiple images export)
*
*   COMPILATION (Windows - MinGW):
*       gcc -o $(NAME_PART).exe $(FILE_NAME) -I../../src -lraylib -lopengl32 -lgdi32 -std=c99
*
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2024-2025 Ramon Santamaria (@raysan5)
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

#define TOOL_NAME               "raylib project creator"
#define TOOL_SHORT_NAME         "rpc"
#define TOOL_VERSION            "2.0"
#define TOOL_DESCRIPTION        "A simple and easy-to-use raylib projects creator"
#define TOOL_DESCRIPTION_BREAK  "A simple and easy-to-use\nraylib projects creator"
#define TOOL_RELEASE_DATE       "Sep.2025"
#define TOOL_LOGO_COLOR         0x000000ff
#define TOOL_CONFIG_FILENAME    "rpc.ini"

#include "raylib.h"

#if defined(PLATFORM_WEB)
    #define CUSTOM_MODAL_DIALOGS            // Force custom modal dialogs usage
    #include <emscripten/emscripten.h>      // Emscripten library - LLVM to JavaScript compiler
    #include <emscripten/html5.h>           // Emscripten HTML5 browser functionality (emscripten_set_beforeunload_callback)
#endif

#define RAYGUI_IMPLEMENTATION
#include "external/raygui.h"

#undef RAYGUI_IMPLEMENTATION                // Avoid including raygui implementation again

#define GUI_MAIN_TOOLBAR_IMPLEMENTATION
#include "gui_main_toolbar.h"               // GUI: Main toolbar

#define GUI_WINDOW_HELP_IMPLEMENTATION
#include "gui_window_help.h"                // GUI: Help Window

#define GUI_WINDOW_ABOUT_IMPLEMENTATION
#include "gui_window_about_welcome.h"       // GUI: About Window

#define GUI_FILE_DIALOGS_IMPLEMENTATION
#include "gui_file_dialogs.h"               // GUI: File Dialogs

// raygui embedded styles
// NOTE: Included in the same order as selector
#define MAX_GUI_STYLES_AVAILABLE   5
#include "styles/style_genesis.h"           // raygui style: genesis
#include "styles/style_cyber.h"             // raygui style: cyber
#include "styles/style_lavanda.h"           // raygui style: lavanda
#include "styles/style_terminal.h"          // raygui style: terminal
#include "styles/style_amber.h"             // raygui style: amber


// miniz: Single C source file zlib-replacement library
// Ref: https://github.com/richgel999/miniz
#include "external/miniz.h"                 // ZIP packaging functions definition
#include "external/miniz.c"                 // ZIP packaging implementation

//#include "template.zip.h"                   // Project template to embed into executable (zipped)

#define RPNG_IMPLEMENTATION
#include "external/rpng.h"                  // PNG chunks management

#define RINI_IMPLEMENTATION
#include "external/rini.h"                  // Config file values reader/writer

// Standard C libraries
#include <stdio.h>                          // Required for: fopen(), fclose(), fread()...
#include <stdlib.h>                         // Required for: NULL, calloc(), free()
#include <string.h>                         // Required for: memcpy()

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#if (!defined(_DEBUG) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)))
bool __stdcall FreeConsole(void);   // Close console from code (kernel32.lib)
#endif

// Simple log system to avoid printf() calls if required
// NOTE: Avoiding those calls, also avoids const strings memory usage
#define SUPPORT_LOG_INFO
#if defined(SUPPORT_LOG_INFO)// && defined(_DEBUG)
    #define LOG(...) printf(__VA_ARGS__)
#else
    #define LOG(...)
#endif

//#define BUILD_TEMPLATE_INTO_EXE

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

// Project configuration data
// WARNING: Create only dynamic objects for this type
typedef struct ProjectConfig {
    struct {
        int type;                   // Project type to generate: Basic, Advanced, Custom
        char name[64];              // Project name
        char internalName[64];      // Project internal name (used for GitHub, if not provided using name)
        char productName[64];       // Project product name
        char version[16];           // Project version
        char description[256];      // Project description
        char developer[64];         // Project developer/company
        char developerWeb[64];      // Project developer webpage
        char developerEmail[64];    // Project developer email (info/support?)

        char iconPath[256];         // Project icon file path (.ico/.icns), for application
        char logoPath[256];         // Project logo file path, for imagery (itchio/Steam)
        char readmePath[256];       // Project README file path
        char eulaPath[256];         // Project EULA path

        int srcFileCount;           // Project source files count
        char srcFilePaths[64][256]; // Project source files path(s) -> MAX_SOURCE_FILES=64
        int resFileCount;           // Project resource files count
        char resFilePaths[256][256]; // Project resource files paths -> MAX_RESOURCE_FILES=256
        char resBasePath[256];      // Project resources base directory path (including all resources)

        char outputPath[256];       // Project generation output path
    } Project;
    struct {
        int buildSystemFlags;       // Building systems required: Script, Makefile, VSCode, VS2022

        char raylibSrcPath[256];    // Building: raylib source path
        char compilerPath[256];     // Building: Desktop (Windows/Linux/macOS): GCC/w64devkit compiler path
        char emsdkPath[256];        // Building: WebAssembly: Emscripten SDK path

        char includePath[256];      // Building additional include path
        char libPath[256];          // Building additional library path
        char buildPath[256];        // Building path (for VS2022 defaults to 'build' directory)
    } Building;
} ProjectConfig;

// Packed file entry
// NOTE: Used for template packing to be attached to executable
typedef struct PackFileEntry {
    int fileSize;                   // Package entry file uncompressed size
    int compFileSize;               // Package entry file compressed size
    char filePath[256];             // Package entry file path and name
} PackFileEntry;

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static const char *toolName = TOOL_NAME;
static const char *toolVersion = TOOL_VERSION;
static const char *toolDescription = TOOL_DESCRIPTION;

static const int screenWidth = 960;        // Default screen width (at initialization)
static const int screenHeight = 660;        // Default screen height (at initialization)

// NOTE: Max length depends on OS, in Windows MAX_PATH = 256
static char inFileName[512] = { 0 };        // Input file name (required in case of drag & drop over executable)
static char outFileName[512] = { 0 };       // Output file name (required for file save/export)

static int framesCounter = 0;               // General pourpose frames counter (not used)
static Vector2 mousePoint = { 0 };          // Mouse position
static bool lockBackground = false;         // Toggle lock background (controls locked)
static bool saveChangesRequired = false;    // Flag to notice save changes are required

static RenderTexture2D screenTarget = { 0 }; // Render texture to render the tool (if required)

static Vector2 panelScroll = { 0 };
static Rectangle panelView = { 0 };

static bool showLoadSourceFilesDialog = false;
static bool showLoadResourcePathDialog = false;
static bool showLoadRaylibSourcePathDialog = false;
static bool showLoadCompilerPathDialog = false;
static bool showLoadOutputPathDialog = false;
static bool showExportProjectProgress = false;
static bool showInfoMessagePanel = false;
static const char *infoTitle = NULL;
static const char *infoMessage = NULL;
static const char *infoButton = NULL;

static char **srcFileNameList = NULL;

static float exportProjectProgress = 0.0f;

static int monitorWidth = 0;
static int monitorHeight = 0;
static bool screenSizeDouble = false;       // Flag to request screen size x2, in case of HighDPI

// Basic program variables
//----------------------------------------------------------------------------------
//static rpbConfigData project = { 0 };       // rpb project config data
static ProjectConfig *config = NULL;

static bool showMessageReset = false;       // Show message: reset
static bool showMessageExit = false;        // Show message: exit (quit)

static double baseTime = 0;                 // Base time in seconds to start counting
static double currentTime = 0;              // Current time counter in seconds
//-----------------------------------------------------------------------------------

// Support Message Box
//-----------------------------------------------------------------------------------
#if defined(SPLASH_SUPPORT_MESSAGE)
static bool showSupportMessage = true;      // Support message box splash message at startup
#else
static bool showSupportMessage = false;
#endif
static int supportMessageRandBtn = 0;       // Support message buttons random position
//-----------------------------------------------------------------------------------

// GUI: Main Layout
//-----------------------------------------------------------------------------------
static Vector2 anchorProject = { 12, 104 };
static Vector2 anchorBuilding = { 12, 298 };

static bool projectNameEditMode = false;
static bool productNameEditMode = false;
static bool projectDeveloperEditMode = false;
static bool projectDeveloperWebEditMode = false;
static bool projectDescriptionEditMode = false;
static bool projectSourceFilePathEditMode = false;
static bool projectResourcePathEditMode = false;
static bool buildingRaylibPathEditMode = false;
static bool buildingCompilerPathEditMode = false;
static bool buildingOutputPathEditMode = false;
//-----------------------------------------------------------------------------------

// GUI: Main toolbar panel
//-----------------------------------------------------------------------------------
static GuiMainToolbarState mainToolbarState = { 0 };
//-----------------------------------------------------------------------------------

// GUI: Help Window
//-----------------------------------------------------------------------------------
static GuiWindowHelpState windowHelpState = { 0 };
//-----------------------------------------------------------------------------------

// GUI: About Window
//-----------------------------------------------------------------------------------
static GuiWindowAboutState windowAboutState = { 0 };
//-----------------------------------------------------------------------------------

// GUI: Issue Report Window
//-----------------------------------------------------------------------------------
static bool showIssueReportWindow = false;
//-----------------------------------------------------------------------------------

// GUI: User Window
//-----------------------------------------------------------------------------------
//static GuiWindowUserState windowUserState = { 0 };
//-----------------------------------------------------------------------------------

// GUI: Export Window
//-----------------------------------------------------------------------------------
static bool windowExportActive = false;
static int exportFormatActive = 0;         // ComboBox file type selection
//-----------------------------------------------------------------------------------

// GUI: Exit Window
//-----------------------------------------------------------------------------------
static bool closeWindow = false;
static bool windowExitActive = false;
//-----------------------------------------------------------------------------------

// GUI: Custom file dialogs
//-----------------------------------------------------------------------------------
static bool showLoadFileDialog = false;
static bool showSaveFileDialog = false;
static bool showExportFileDialog = false;
//-----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
#if defined(PLATFORM_DESKTOP)
// Command line functionality
static void ShowCommandLineInfo(void);                      // Show command line usage info
static void ProcessCommandLine(int argc, char *argv[]);     // Process command line input
#endif

static void UpdateDrawFrame(void);                          // Update and draw one frame

// Load/Save/Export data functions
static ProjectConfig *LoadProjectData(const char *fileName); // Load project config data data from .rpc file
static void UnloadProjectData(ProjectConfig *data);         // Unload project data
static void SaveProjectData(ProjectConfig *data, const char *fileName); // Save project config data to .rpc file

// Auxiliar functions
static void SetupProject(ProjectConfig *config);            // Setup project, using template

// Packing and unpacking of template files (NOT USED)
static char *PackDirectoryData(const char *baseDirPath, int *packSize);
static void UnpackDirectoryData(const char *outputDirPath, const unsigned char *data, int *dataSize, PackFileEntry *entries, int fileCount);

// Split string into multiple strings
// NOTE: No memory is dynamically allocated
static const char **GetSubtextPtrs(char *text, char delimiter, int *count);
//------------------------------------------------------------------------------------

// Load/Save application configuration
// NOTE: Functions operate over global variables
//------------------------------------------------------------------------------------
static void LoadApplicationConfig(void);
static void SaveApplicationConfig(void);
#if defined(PLATFORM_WEB)
// Load/Save data on web LocalStorage (persistent between sessions)
static void SaveWebLocalStorage(const char *key, const char *value);
static char *LoadWebLocalStorage(const char *key);
// Web function to be called before page unload/close
static const char *RunBeforeWebUnload(int eventType, const void *reserved, void *userData) { SaveApplicationConfig(); return NULL; }
#endif
//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    // Template.zip export as code
    //--------------------------------------------------------------------------------
    //int templateZipDataSize = 0;
    //unsigned char *templateZipData = LoadFileData("template.zip", &templateZipDataSize);
    //ExportDataAsCode(templateZipData, templateZipDataSize, "template.zip.h");
    //UnloadFileData(templateZipData);
    //--------------------------------------------------------------------------------

#if defined(BUILD_TEMPLATE_INTO_EXE)
    // Attach template data into generated executable on first run
    int exeFileDataSize = 0;
    unsigned char *exeFileData = LoadFileData(argv[0], &exeFileDataSize);

    // Check if template already added to not add it again
    char fourcc[5] = { 0 };
    memcpy(fourcc, exeFileData + exeFileDataSize - 4, 4);

    if ((fourcc[0] != 'r') || (fourcc[1] != 'p') || (fourcc[2] != 'c') || (fourcc[3] != 'h'))
    {
        // No template data attached to exe, so we attach it
        int packDataSize = 0;
        char *packData = PackDirectoryData(TextFormat("%s/template", GetApplicationDirectory()), &packDataSize);

        int outExeFileDataSize = exeFileDataSize + packDataSize;
        char *outExeFileData = (char *)RL_CALLOC(outExeFileDataSize, 1);
        memcpy(outExeFileData, exeFileData, exeFileDataSize);
        memcpy(outExeFileData + exeFileDataSize, packData, packDataSize);

        SaveFileData(TextFormat("%s.template.exe", GetFileNameWithoutExt(argv[0])), outExeFileData, outExeFileDataSize);

        RL_FREE(outExeFileData);
        RL_FREE(packData);
    }
#endif

#if !defined(_DEBUG)
    SetTraceLogLevel(LOG_NONE);         // Disable raylib trace log messsages
#endif
#if defined(COMMAND_LINE_ONLY)
    ProcessCommandLine(argc, argv);
#else
#if defined(PLATFORM_DESKTOP)
    // Command-line usage mode
    //--------------------------------------------------------------------------------------
    if (argc > 1)
    {
        if ((argc == 2) &&
            (strcmp(argv[1], "-h") != 0) &&
            (strcmp(argv[1], "--help") != 0))       // One argument (file dropped over executable?)
        {
            if (IsFileExtension(argv[1], ".rpc"))
            {
                // TODO: Load .rpc config file

            }
            if (IsFileExtension(argv[1], ".c"))
            {
                ProjectConfig *config = (ProjectConfig *)RL_CALLOC(1, sizeof(ProjectConfig));

                config->Project.type = 2;  // Custom files
                strcpy(config->Project.name, GetFileNameWithoutExt(argv[1]));
                strcpy(config->Project.productName, GetFileNameWithoutExt(argv[1]));
                strcpy(config->Project.description, "My cool project");
                strcpy(config->Project.developer, "raylibtech");
                strcpy(config->Project.developerWeb, "www.raylibtech.com");
                strcpy(config->Project.srcFilePaths[0], argv[1]);
                config->Project.srcFileCount = 1;
                strcpy(config->Project.outputPath, GetDirectoryPath(argv[1]));

                strcpy(config->Building.compilerPath, "C:\\raylib\\w64devkit\\bin");
                strcpy(config->Building.raylibSrcPath, "C:\\raylib\\raylib\\src");

                SetupProject(config);

                RL_FREE(config);

                return 0;
            }
        }
        else
        {
            // Get multiple input code files?
            /*
            for (int i = 1; i < argc; i++)
            {
                strcpy(config->Project.srcFilePaths[config->Project.srcFileCount], argv[i]);
                config->Project.srcFileCount++;
            }
            */
            ProcessCommandLine(argc, argv);
            return 0;
        }
    }
#endif  // PLATFORM_DESKTOP
#if (!defined(_DEBUG) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)))
    // WARNING (Windows): If program is compiled as Window application (instead of console),
    // no console is available to show output info... solution is compiling a console application
    // and closing console (FreeConsole()) when changing to GUI interface
    //FreeConsole();
#endif

    // GUI usage mode - Initialization
    //--------------------------------------------------------------------------------------
    //SetConfigFlags(FLAG_WINDOW_RESIZABLE);      // Window configuration flags
    InitWindow(screenWidth, screenHeight, TextFormat("%s v%s | %s", toolName, toolVersion, toolDescription));
    //SetWindowMinSize(1280, 720);
    SetExitKey(0);

    // Create a RenderTexture2D to be used for render to texture
    screenTarget = LoadRenderTexture(screenWidth, screenHeight);
    SetTextureFilter(screenTarget.texture, TEXTURE_FILTER_POINT);

    // Initialize project config default
    config = (ProjectConfig *)RL_CALLOC(1, sizeof(ProjectConfig));
    config->Project.type = 2;  // Custom files
    strcpy(config->Project.name, "cool_project");
    strcpy(config->Project.productName, "Cool Project");
    strcpy(config->Project.description, "my new cool project");
    strcpy(config->Project.developer, "raylib technologies");
    strcpy(config->Project.developerWeb, "www.raylibtech.com");
    //strcpy(config->Project.srcFilePaths[0], argv[1]);
    //config->Project.srcFileCount = 1;
    strcpy(config->Project.outputPath, ".");

    strcpy(config->Building.compilerPath, "C:\\raylib\\w64devkit\\bin");
    strcpy(config->Building.raylibSrcPath, "C:\\raylib\\raylib\\src");

    // Source file names (without path) are used for display on source textbox
    srcFileNameList = (char **)RL_CALLOC(256, sizeof(char *)); // Max number of input source files supported
    for (int i = 0; i < 256; i++) srcFileNameList[i] = (char *)RL_CALLOC(256, sizeof(char));

#if !defined(PLATFORM_WEB)
    monitorWidth = GetMonitorWidth(GetCurrentMonitor());
    monitorHeight = GetMonitorHeight(GetCurrentMonitor());
    if ((GetWindowScaleDPI().x > 1.0f) || (monitorWidth > (screenWidth*2)))
    {
        // NOTE: We need to consider app window title bar and possible OS bottom bar
        if ((monitorHeight - 24 - 40)  > (screenHeight*2))
        {
            screenSizeDouble = true;
            SetWindowSize(screenWidth*2, screenHeight*2);
            SetMouseScale(0.5f, 0.5f);
            SetWindowPosition(monitorWidth/2 - screenWidth, monitorHeight/2 - screenHeight);
        }
    }
#endif

    // Set default project values
    strcpy(config->Project.name, "cool_project");
    strcpy(config->Project.productName, "Cool Project");
    strcpy(config->Project.developer, "raylibtech");
    strcpy(config->Project.developerWeb, "www.raylibtech.com");
    strcpy(config->Project.description, "my cool new project");

    // Welcome panel data
    infoTitle = "WELCOME! LET'S CREATE A PROJECT!";
    infoMessage = "Provide some source code files (.c) to generate project!";// \nOr choose a default project type!";
    infoButton = "Sure! Let's start!";
    showInfoMessagePanel = true;

    LOG("INIT: Ready to show project generation info...\n");
    //-----------------------------------------------------------------------------------

    // GUI: Main toolbar panel (file and visualization)
    //-----------------------------------------------------------------------------------
    mainToolbarState = InitGuiMainToolbar();

    // Set raygui style to start with
    // WARNING: It must be aligned with mainToolbarState.visualStyleActive
    GuiLoadStyleGenesis();

    GuiEnableTooltip();     // Enable tooltips by default
    //-----------------------------------------------------------------------------------

    // GUI: Help Window
    //-----------------------------------------------------------------------------------
    windowHelpState = InitGuiWindowHelp();
    //-----------------------------------------------------------------------------------

    // GUI: About Window
    //-----------------------------------------------------------------------------------
    windowAboutState = InitGuiWindowAbout();
    //-----------------------------------------------------------------------------------

    // GUI: User Window
    //-----------------------------------------------------------------------------------
    //windowUserState = InitGuiWindowUser();
    //-----------------------------------------------------------------------------------

    // Trial message(s) and Support Message Box
    //-----------------------------------------------------------------------------------
#if defined(SPLASH_SUPPORT_MESSAGE)
    supportMessageRandBtn = GetRandomValue(0, 1); // Used for the support message button order
#endif
    //-----------------------------------------------------------------------------------

    // Load application init configuration (if available)
    //-------------------------------------------------------------------------------------
    LoadApplicationConfig();
#if defined(PLATFORM_WEB)
    // Set callback to automatically save app config on page closing
    emscripten_set_beforeunload_callback(NULL, RunBeforeWebUnload);
#endif
    //-------------------------------------------------------------------------------------

#if !defined(PLATFORM_WEB)
    // File dropped over executable / command line input file
    //-------------------------------------------------------------------------------------
    if ((inFileName[0] != '\0') && (IsFileExtension(inFileName, ".rpc")))
    {
        // TODO: Load tool data from file
        //rpbConfigData data = LoadProjectData(inFileName);

        // TODO: Do something with loaded data
    }
    else
    {
        // TODO: Set some default values
    }
    //-------------------------------------------------------------------------------------
#endif

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    SetTargetFPS(60);       // Set our game frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!closeWindow)    // Program must finish
    {
        UpdateDrawFrame();
    }
#endif

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadProjectData(config);

    UnloadRenderTexture(screenTarget); // Unload render texture

    // Save application init configuration for next run
    //--------------------------------------------------------------------------------------
    SaveApplicationConfig();
    //--------------------------------------------------------------------------------------

    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

#endif  // !COMMAND_LINE_ONLY
    return 0;
}

//--------------------------------------------------------------------------------------------
// Module Functions Definition
//--------------------------------------------------------------------------------------------
// Update and draw one frame
static void UpdateDrawFrame(void)
{
    // WARNING: ASINCIFY requires this line,
    // it contains the call to emscripten_sleep() for PLATFORM_WEB
    if (WindowShouldClose()) closeWindow = true;

    // Dropped files logic
    //----------------------------------------------------------------------------------
    if (IsFileDropped())
    {
        FilePathList droppedFiles = LoadDroppedFiles();

        for (int i = 0; i < droppedFiles.count; i++)
        {
            if (IsFileExtension(droppedFiles.paths[i], ".c;.h"))
            {
                // Add files to source list
                strcpy(config->Project.srcFilePaths[config->Project.srcFileCount], droppedFiles.paths[i]);
                strcpy(srcFileNameList[config->Project.srcFileCount], GetFileName(droppedFiles.paths[i]));
                config->Project.srcFileCount++;
            }
        }

        if ((droppedFiles.count == 1) && IsFileExtension(droppedFiles.paths[0], ".rgs"))
        {
            // Reset to default internal style
            // NOTE: Required to unload any previously loaded font texture
            GuiLoadStyleDefault();
            GuiLoadStyle(droppedFiles.paths[0]);
        }

        UnloadDroppedFiles(droppedFiles);    // Unload filepaths from memory
    }
    //----------------------------------------------------------------------------------

    // Keyboard shortcuts
    //------------------------------------------------------------------------------------
    // New style file, previous in/out files registeres are reseted
    if ((IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_N)) || mainToolbarState.btnNewFilePressed)
    {
        // TODO: Create new project
    }

    // Show dialog: load input project config file (.rpc)
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_O)) showLoadFileDialog = true;

    // Show dialog: export project
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S))
    {
        strcpy(outFileName, TextToLower(config->Project.name));
        showExportProjectProgress = true;
    }

    // Show dialog: load source files
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_O)) showLoadSourceFilesDialog = true;

    // Toggle window: help
    if (IsKeyPressed(KEY_F1)) windowHelpState.windowActive = !windowHelpState.windowActive;

    // Toggle window: about
    if (IsKeyPressed(KEY_F2)) windowAboutState.windowActive = !windowAboutState.windowActive;

    // Toggle window: issue report
    if (IsKeyPressed(KEY_F3)) showIssueReportWindow = !showIssueReportWindow;

    // Show closing window on ESC
    if (IsKeyPressed(KEY_ESCAPE))
    {
        if (windowHelpState.windowActive) windowHelpState.windowActive = false;
        else if (windowAboutState.windowActive) windowAboutState.windowActive = false;
        else if (showIssueReportWindow) showIssueReportWindow = false;
        else if (windowExportActive) windowExportActive = false;
#if defined(PLATFORM_DESKTOP)
        else if (showInfoMessagePanel) showInfoMessagePanel = false;
        else windowExitActive = !windowExitActive;
#else
        else if (showLoadSourceFilesDialog) showLoadSourceFilesDialog = false;
        else if (showLoadResourcePathDialog) showLoadResourcePathDialog = false;
        else if (showLoadRaylibSourcePathDialog) showLoadRaylibSourcePathDialog = false;
        else if (showLoadCompilerPathDialog) showLoadCompilerPathDialog = false;
        else if (showLoadOutputPathDialog) showLoadCompilerPathDialog = false;
        else if (showExportProjectProgress) showExportProjectProgress = false;
#endif
    }
    //----------------------------------------------------------------------------------

    // Main toolbar logic
    //----------------------------------------------------------------------------------
    // File options logic
    if (mainToolbarState.btnLoadFilePressed) showLoadFileDialog = true;
    else if (mainToolbarState.btnSaveFilePressed)
    {
        memset(outFileName, 0, 512);
        strcpy(outFileName, TextFormat("%s.rpc", config->Project.name));
        showExportFileDialog = true;
    }

    // Visual options logic
    if (mainToolbarState.visualStyleActive != mainToolbarState.prevVisualStyleActive)
    {
        // Reset to default internal style
        // NOTE: Required to unload any previously loaded font texture
        GuiLoadStyleDefault();

        switch (mainToolbarState.visualStyleActive)
        {
        case 0: GuiLoadStyleGenesis(); break;
        case 1: GuiLoadStyleCyber(); break;
        case 2: GuiLoadStyleLavanda(); break;
        case 3: GuiLoadStyleTerminal(); break;
        case 4: GuiLoadStyleAmber(); break;
        default: break;
        }

        mainToolbarState.prevVisualStyleActive = mainToolbarState.visualStyleActive;
    }

    // Help options logic
    if (mainToolbarState.btnHelpPressed) windowHelpState.windowActive = true;
    if (mainToolbarState.btnAboutPressed) windowAboutState.windowActive = true;
    if (mainToolbarState.btnIssuePressed) showIssueReportWindow = true;
    //----------------------------------------------------------------------------------

    // Basic program flow logic
    //----------------------------------------------------------------------------------
#if !defined(PLATFORM_WEB)
    if (WindowShouldClose())
    {
        if (saveChangesRequired) showMessageExit = true;
        else closeWindow = true;
    }

    // Window scale logic to support 4K/HighDPI monitors
    if (IsKeyPressed(KEY_F10))
    {
        screenSizeDouble = !screenSizeDouble;
        if (screenSizeDouble)
        {
            // Screen size x2
            if (GetScreenWidth() < screenWidth*2)
            {
                SetWindowSize(screenWidth*2, screenHeight*2);
                SetMouseScale(0.5f, 0.5f);
                SetWindowPosition(monitorWidth/2 - screenWidth, monitorHeight/2 - screenHeight);
            }
        }
        else
        {
            // Screen size x1
            if (screenWidth*2 >= GetScreenWidth())
            {
                SetWindowSize(screenWidth, screenHeight);
                SetMouseScale(1.0f, 1.0f);
                SetWindowPosition(monitorWidth/2 - screenWidth/2, monitorHeight/2 - screenHeight/2);
            }
        }
    }
#endif

    if (windowExitActive ||
        windowHelpState.windowActive ||
        windowAboutState.windowActive ||
        showIssueReportWindow ||
        showInfoMessagePanel ||
        showLoadSourceFilesDialog ||
        showLoadResourcePathDialog ||
        showLoadRaylibSourcePathDialog ||
        showLoadCompilerPathDialog ||
        showLoadOutputPathDialog ||
        showExportProjectProgress) lockBackground = true;
    else lockBackground = false;

    if (lockBackground) GuiLock();
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    // Render all screen to texture (for scaling)
    BeginTextureMode(screenTarget);
    ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

    // GUI: Main Window
    //----------------------------------------------------------------------------------
    int prevProjectType = config->Project.type;
    GuiLabel((Rectangle){ 16, 52, 104, 32 }, "PROJECT TYPE:");
    //GuiSetTooltip("Choose from a default project template or custom source files");
    GuiToggleGroup((Rectangle){ 120, 52, 223, 32 }, "Basic Sample;Screen Manager;Custom files", &config->Project.type);
    GuiSetTooltip(NULL);

    if (config->Project.type != prevProjectType)
    {
        if (config->Project.type == 0)
        {
            strcpy(srcFileNameList[0], TextFormat("%s.c", TextToLower(config->Project.name)));
            config->Project.srcFileCount = 1;
        }
        else if (config->Project.type == 1)
        {
            strcpy(srcFileNameList[0], TextFormat("%s.c", TextToLower(config->Project.name)));
            strcpy(srcFileNameList[1], "screens.h");
            strcpy(srcFileNameList[2], "screen_logo.c");
            strcpy(srcFileNameList[3], "screen_title.c");
            strcpy(srcFileNameList[4], "screen_options.c");
            strcpy(srcFileNameList[5], "screen_gameplay.c");
            strcpy(srcFileNameList[6], "screen_ending.c");
            config->Project.srcFileCount = 7;
        }
        else if (config->Project.type == 2)
        {
            config->Project.srcFileCount = 0;
        }
    }

    GuiGroupBox((Rectangle){ anchorProject.x + 0, anchorProject.y + 0, GetScreenWidth() - 24, 172 }, "PROJECT SETTINGS");
    GuiLabel((Rectangle){ anchorProject.x + 8, anchorProject.y + 24, 104, 24 }, "PROJECT NAME:");
    //GuiSetTooltip("Define project name, note that every project\nblablablaballsadlksad");
    if (GuiTextBox((Rectangle){ anchorProject.x + 112, anchorProject.y + 24, 280, 24 }, config->Project.name, 128, projectNameEditMode)) projectNameEditMode = !projectNameEditMode;
    GuiLabel((Rectangle){ anchorProject.x + 408, anchorProject.y + 24, 120, 24 }, "PRODUCT NAME:");
    if (GuiTextBox((Rectangle){ anchorProject.x + 496, anchorProject.y + 24, 280, 24 }, config->Project.productName, 128, productNameEditMode)) productNameEditMode = !productNameEditMode;
    GuiLabel((Rectangle){ anchorProject.x + 8, anchorProject.y + 56, 104, 24 }, "DESCRIPTION:");
    if (GuiTextBox((Rectangle){ anchorProject.x + 112, anchorProject.y + 56, 664, 24 }, config->Project.description, 128, projectDescriptionEditMode)) projectDescriptionEditMode = !projectDescriptionEditMode;
    GuiLabel((Rectangle){ anchorProject.x + 8, anchorProject.y + 88, 104, 24 }, "DEVELOPER:");
    if (GuiTextBox((Rectangle){ anchorProject.x + 112, anchorProject.y + 88, 280, 24 }, config->Project.developer, 128, projectDeveloperEditMode)) projectDeveloperEditMode = !projectDeveloperEditMode;
    GuiLabel((Rectangle){ anchorProject.x + 408, anchorProject.y + 88, 120, 24 }, "DEV. WEBPAGE:");
    if (GuiTextBox((Rectangle){ anchorProject.x + 496, anchorProject.y + 88, 280, 24 }, config->Project.developerWeb, 128, projectDeveloperWebEditMode)) projectDeveloperWebEditMode = !projectDeveloperWebEditMode;

    if (config->Project.type != 2) GuiDisable();
    GuiLabel((Rectangle){ anchorProject.x + 8, anchorProject.y + 128, 104, 24 }, "SOURCE FILES:");
    GuiSetStyle(TEXTBOX, TEXT_READONLY, 1);
    GuiTextBox((Rectangle){ anchorProject.x + 112, anchorProject.y + 128, 536, 24 }, TextJoin(srcFileNameList, config->Project.srcFileCount, ";"), 256, projectSourceFilePathEditMode);//) projectSourceFilePathEditMode = !projectSourceFilePathEditMode;
    GuiSetStyle(TEXTBOX, TEXT_READONLY, 0);
    if (GuiButton((Rectangle){ anchorProject.x + 656, anchorProject.y + 128, 120, 24 }, "Browse")) showLoadSourceFilesDialog = true;
    //GuiLabel((Rectangle){ anchorProject.x + 8, anchorProject.y + 160, 104, 24 }, "RESOURCE PATH:");
    //if (GuiTextBox((Rectangle){ anchorProject.x + 112, anchorProject.y + 160, 536, 24 }, config->Project.resBasePath, 128, projectResourcePathEditMode)) projectResourcePathEditMode = !projectResourcePathEditMode;
#if defined(PLATFORM_WEB)
    //GuiDisable();
#endif
    //if (GuiButton((Rectangle){ anchorProject.x + 656, anchorProject.y + 160, 120, 24 }, "Browse")) showLoadResourcePathDialog = true;
    GuiEnable();

    GuiGroupBox((Rectangle){ anchorBuilding.x + 0, anchorBuilding.y + 0, 784, 136 }, "BUILD SETTINGS");
    GuiLabel((Rectangle){ anchorBuilding.x + 8, anchorBuilding.y + 16, 104, 24 }, "raylib SRC PATH:");
    if (GuiTextBox((Rectangle){ anchorBuilding.x + 112, anchorBuilding.y + 16, 536, 24 }, config->Building.raylibSrcPath, 128, buildingRaylibPathEditMode)) buildingRaylibPathEditMode = !buildingRaylibPathEditMode;
#if defined(PLATFORM_WEB)
    GuiDisable();
#endif
    if (GuiButton((Rectangle){ anchorBuilding.x + 656, anchorBuilding.y + 16, 120, 24 }, "Browse")) showLoadRaylibSourcePathDialog = true;
    GuiEnable();

#if !defined(PLATFORM_WEB) && (defined(__linux__) || defined(__FreeBSD__))
    GuiDisable();
#endif
    GuiLabel((Rectangle){ anchorBuilding.x + 8, anchorBuilding.y + 48, 104, 24 }, "COMPILER PATH:");
    if (GuiTextBox((Rectangle){ anchorBuilding.x + 112, anchorBuilding.y + 48, 536, 24 }, config->Building.compilerPath, 128, buildingCompilerPathEditMode)) buildingCompilerPathEditMode = !buildingCompilerPathEditMode;
    GuiEnable();

#if defined(PLATFORM_WEB)
    GuiDisable();
#endif
    if (GuiButton((Rectangle){ anchorBuilding.x + 656, anchorBuilding.y + 48, 120, 24 }, "Browse")) showLoadCompilerPathDialog = true;
    GuiEnable();
    GuiLabel((Rectangle){ anchorBuilding.x + 8, anchorBuilding.y + 88, 104, 32 }, "BUILD SYSTEMS:");

    if (!lockBackground) GuiLock();
    bool buildSystem = true;
    GuiToggle((Rectangle){ anchorBuilding.x + 112, anchorBuilding.y + 88, 164, 32 }, "Script", &buildSystem);
    GuiToggle((Rectangle){ anchorBuilding.x + 112 + 166, anchorBuilding.y + 88, 164, 32 }, "Makefile", &buildSystem);
    GuiToggle((Rectangle){ anchorBuilding.x + 112 + 166*2, anchorBuilding.y + 88, 164, 32 }, "VSCode", &buildSystem);
    GuiToggle((Rectangle){ anchorBuilding.x + 112 + 166*3, anchorBuilding.y + 88, 164, 32 }, "VS2022", &buildSystem);
    if (!lockBackground) GuiUnlock();

#if defined(PLATFORM_WEB)
    GuiDisable();
#endif
    GuiLabel((Rectangle){ anchorBuilding.x + 8, anchorBuilding.y + 152, 104, 24 }, "OUTPUT PATH:");
    if (GuiTextBox((Rectangle){ anchorBuilding.x + 112, anchorBuilding.y + 152, 536, 24 }, config->Project.outputPath, 128, buildingOutputPathEditMode)) buildingOutputPathEditMode = !buildingOutputPathEditMode;
    if (GuiButton((Rectangle){ anchorBuilding.x + 656, anchorBuilding.y + 152, 120, 24 }, "Browse")) showLoadOutputPathDialog = true;
    GuiEnable();

    if (config->Project.srcFileCount == 0) GuiDisable();
    if (GuiButton((Rectangle){ 8, 450, 784, 40 }, "GENERATE PROJECT STRUCTURE"))
    {
        SetupProject(config);
        showExportProjectProgress = true;
    }
    GuiEnable();

    if (!lockBackground && CheckCollisionPointRec(GetMousePosition(), (Rectangle){ 0, GetScreenHeight() - 64, screenWidth, 32 })) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    else SetMouseCursor(MOUSE_CURSOR_DEFAULT);

    if (GuiButton((Rectangle){ 0, screenHeight - 64, screenWidth, 32 },
        "Did you find this tool useful? Please, consider a donation! Thanks! <3"))
    {
        OpenURL("https://github.com/sponsors/raysan5");
    }
    //----------------------------------------------------------------------------------

    // GUI: Main toolbar panel
    //----------------------------------------------------------------------------------
    GuiMainToolbar(&mainToolbarState);
    //----------------------------------------------------------------------------------

    // GUI: Status bar
    //----------------------------------------------------------------------------------
    int textPadding = GuiGetStyle(STATUSBAR, TEXT_PADDING);
    GuiSetStyle(STATUSBAR, TEXT_PADDING, 0);
    GuiSetStyle(STATUSBAR, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiStatusBar((Rectangle){ 0, screenHeight - 24, screenWidth, 24 }, "PROJECT INFO");
    GuiSetStyle(STATUSBAR, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(STATUSBAR, TEXT_PADDING, textPadding);
    //----------------------------------------------------------------------------------

    // NOTE: If some overlap window is open and main window is locked, we draw a background rectangle
    if (lockBackground) DrawRectangle(0, 0, screenWidth, screenHeight, Fade(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)), 0.85f));

    // WARNING: Before drawing the windows, we unlock them
    GuiUnlock();

    // GUI: Show info message panel
    //----------------------------------------------------------------------------------------
    if (showInfoMessagePanel)
    {
        Vector2 textSize = MeasureTextEx(GuiGetFont(), infoMessage, GuiGetFont().baseSize*2, 3);
        GuiPanel((Rectangle){ -10, screenHeight/2 - 180, screenWidth + 20, 290 }, NULL);

        int textSpacing = GuiGetStyle(DEFAULT, TEXT_SPACING);
        GuiSetStyle(DEFAULT, TEXT_SIZE, GuiGetFont().baseSize*3);
        GuiSetStyle(DEFAULT, TEXT_SPACING, 0);
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
        GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, GuiGetStyle(DEFAULT, TEXT_COLOR_FOCUSED));
        GuiLabel((Rectangle){ -10, screenHeight/2 - 140, screenWidth + 20, 30 }, infoTitle);
        GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL));
        GuiSetStyle(DEFAULT, TEXT_SIZE, GuiGetFont().baseSize*2);
        GuiLabel((Rectangle){ -10, screenHeight/2 - textSize.y - 30, screenWidth + 20, 30 }, infoMessage);

        if (GuiButton((Rectangle){ screenWidth/4, screenHeight/2 + 40, screenWidth/2, 40 }, infoButton))
        {
            showInfoMessagePanel = false;

            infoTitle = "WARNING! READ CAREFULLY!";
            infoMessage = NULL;
            infoButton = "I understand implications";
        }

        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
        GuiSetStyle(DEFAULT, TEXT_SIZE, GuiGetFont().baseSize);
        GuiSetStyle(DEFAULT, TEXT_SPACING, textSpacing);
    }
    //----------------------------------------------------------------------------------------

    // GUI: Help Window
    //----------------------------------------------------------------------------------------
    windowHelpState.windowBounds.x = (float)screenWidth/2 - windowHelpState.windowBounds.width/2;
    windowHelpState.windowBounds.y = (float)screenHeight/2 - windowHelpState.windowBounds.height/2;
    GuiWindowHelp(&windowHelpState);
    //----------------------------------------------------------------------------------------

    // GUI: About Window
    //----------------------------------------------------------------------------------------
    windowAboutState.windowBounds.x = (float)screenWidth/2 - windowAboutState.windowBounds.width/2;
    windowAboutState.windowBounds.y = (float)screenHeight/2 - windowAboutState.windowBounds.height/2;
    GuiWindowAbout(&windowAboutState);
    //----------------------------------------------------------------------------------------

    // GUI: Issue Report Window
    //----------------------------------------------------------------------------------------
    if (showIssueReportWindow)
    {
        Rectangle messageBox = { (float)screenWidth/2 - 300/2, (float)screenHeight/2 - 190/2 - 20, 300, 190 };
        int result = GuiMessageBox(messageBox, "#220#Report Issue",
            "Do you want to report any issue or\nfeature request for this program?\n\ngithub.com/raysan5/raylib-project-creator", "#186#Report on GitHub");

        if (result == 1)    // Report issue pressed
        {
            OpenURL("https://github.com/raysan5/raylib-project-creator/issues");
            showIssueReportWindow = false;
        }
        else if (result == 0) showIssueReportWindow = false;
    }
    //----------------------------------------------------------------------------------------

    // GUI: Export Window
    //----------------------------------------------------------------------------------------
    if (windowExportActive)
    {
        Rectangle messageBox = { (float)screenWidth/2 - 248/2, (float)screenHeight/2 - 200/2, 248, 112 };
        int result = GuiMessageBox(messageBox, "#7#Export Icon File", " ", "#7#Export Icon");

        //GuiLabel((Rectangle){ messageBox.x + 12, messageBox.y + 12 + 24, 106, 24 }, "Icon Format:");

        // NOTE: If current platform is macOS, we support .icns file export
        //GuiComboBox((Rectangle){ messageBox.x + 12 + 88, messageBox.y + 12 + 24, 136, 24 }, (mainToolbarState.platformActive == 1)? "Icon (.ico);Images (.png);Icns (.icns)" : "Icon (.ico);Images (.png)", &exportFormatActive);

        // WARNING: exportTextChunkChecked is used as a global variable required by SaveICO() and SaveICNS() functions
        //GuiCheckBox((Rectangle){ messageBox.x + 20, messageBox.y + 48 + 24, 16, 16 }, "Export text poem with icon", &exportTextChunkChecked);

        if (result == 1)    // Export button pressed
        {
            windowExportActive = false;
            showExportFileDialog = true;
        }
        else if (result == 0) windowExportActive = false;
    }
    //----------------------------------------------------------------------------------

    // GUI: Load File Dialog (and loading logic)
    //----------------------------------------------------------------------------------------
    if (showLoadFileDialog)
    {
#if defined(CUSTOM_MODAL_DIALOGS)
        int result = GuiFileDialog(DIALOG_MESSAGE, "Load project file", inFileName, "Ok", "Just drag and drop your file!");
#else
        int result = GuiFileDialog(DIALOG_OPEN_FILE, "Load project file...", inFileName, "*.rpc", "raylib project creator Files");
#endif
        if (result == 1)
        {
            // Load project file
        }

        if (result >= 0) showLoadFileDialog = false;
    }
    //----------------------------------------------------------------------------------------

    // GUI: Exit Window
    //----------------------------------------------------------------------------------------
    if (windowExitActive)
    {
        int result = GuiMessageBox((Rectangle){ (float)screenWidth/2 - 125, (float)screenHeight/2 - 50, 250, 100 }, "#159#Closing raylib project creator", "Do you really want to exit?", "Yes;No");

        if ((result == 0) || (result == 2)) windowExitActive = false;
        else if (result == 1) closeWindow = true;
    }
    //----------------------------------------------------------------------------------------

    // GUI: Load Source Files Dialog
    //----------------------------------------------------------------------------------------
    if (showLoadSourceFilesDialog)
    {
        // Multiple files selected are supported, tinyfiledialogs returns them as: path01|path02|path03|path04
        // We must reserve enough memory to deal with the maximum amount of data that tinyfiledialogs can provide
        // tinifiledialogs limits defined:
        //#define MAX_MULTIPLE_FILES 1024
        //#define MAX_PATH_OR_CMD 1024
#if defined(PLATFORM_WEB)
        char *multiFileNames = NULL;
#else
        char *multiFileNames = (char *)RL_CALLOC(1024*1024, 1); // Let's reserve for 1024 paths, 1024 bytes each
#endif
#if defined(CUSTOM_MODAL_DIALOGS)
        int result = GuiFileDialog(DIALOG_MESSAGE, "Load source file(s)...", inFileName, "Ok", "Just drag and drop your code file(s)!");
#else
        int result = GuiFileDialog(DIALOG_OPEN_FILE_MULTI, "Load source file(s)...", multiFileNames, "*.c;*.h", "Code Files (*.c,*.h)");
#endif
        if (result == 1)
        {
            int multiFileCount = 0;
            const char **multiFileList = GetSubtextPtrs(multiFileNames, '|', &multiFileCount); // Split text into multiple strings

            for (int i = 0; i < multiFileCount; i++)
            {
                if (IsFileExtension(multiFileList[i], ".c;.h") && (config->Project.srcFileCount < 256))
                {
                    // Add files to source list
                    strcpy(config->Project.srcFilePaths[config->Project.srcFileCount], multiFileList[i]);
                    strcpy(srcFileNameList[config->Project.srcFileCount], GetFileName(multiFileList[i]));
                    config->Project.srcFileCount++;

                    if (config->Project.srcFileCount >= 256) break;
                }
            }
        }

        if (result >= 0) showLoadSourceFilesDialog = false;
    }
    //----------------------------------------------------------------------------------------

    // GUI: Load Resource Path Dialog
    //----------------------------------------------------------------------------------------
    if (showLoadResourcePathDialog)
    {
#if defined(CUSTOM_MODAL_DIALOGS)
        int result = GuiFileDialog(DIALOG_MESSAGE, "Select resource directory...", inFileName, "Ok", "Edit the path in text box");
#else
        int result = GuiFileDialog(DIALOG_OPEN_DIRECTORY, "Select resources directory...", inFileName, NULL, NULL);
#endif
        if (result == 1)
        {
            if (DirectoryExists(inFileName)) strcpy(config->Project.resBasePath, inFileName);
            else
            {
                infoMessage = "Provided resource path does not exist!";
                showInfoMessagePanel = true;
            }
        }

        if (result >= 0) showLoadResourcePathDialog = false;
    }
    //----------------------------------------------------------------------------------------

    // GUI: Load raylib Source Path Dialog
    //----------------------------------------------------------------------------------------
    if (showLoadRaylibSourcePathDialog)
    {
#if defined(CUSTOM_MODAL_DIALOGS)
        int result = GuiFileDialog(DIALOG_MESSAGE, "Select raylib src directory...", inFileName, "Ok", "Edit the path in text box");
#else
        int result = GuiFileDialog(DIALOG_OPEN_DIRECTORY, "Select raylib src directory...", inFileName, NULL, NULL);
#endif
        if (result == 1)
        {
            // Check raylib path
            if (FileExists(TextFormat("%s/raylib.h", inFileName))) strcpy(config->Building.raylibSrcPath, inFileName);
            else
            {
                infoMessage = "Provided raylib source path does not included raylib.h!";
                showInfoMessagePanel = true;
                strcpy(config->Building.raylibSrcPath, inFileName);
            }
        }

        if (result >= 0) showLoadRaylibSourcePathDialog = false;
    }
    //----------------------------------------------------------------------------------------

    // GUI: Load Compiler Path Dialog
    //----------------------------------------------------------------------------------------
    if (showLoadCompilerPathDialog)
    {
#if defined(CUSTOM_MODAL_DIALOGS)
        int result = GuiFileDialog(DIALOG_MESSAGE, "Select GCC compiler directory...", inFileName, "Ok", "Edit the path in text box");
#else
        int result = GuiFileDialog(DIALOG_OPEN_DIRECTORY, "Select GCC compiler directory...", inFileName, NULL, NULL);
#endif
        if (result == 1)
        {
            // Check compiler path
            if (FileExists(TextFormat("%s/gcc.exe", inFileName))) strcpy(config->Building.compilerPath, inFileName);
            else
            {
                infoMessage = "Provided compiler path does not included gcc.exe!";
                showInfoMessagePanel = true;
                strcpy(config->Building.compilerPath, inFileName);
            }
        }

        if (result >= 0) showLoadCompilerPathDialog = false;
    }
    //----------------------------------------------------------------------------------------

    // GUI: Load Output Path Dialog
    //----------------------------------------------------------------------------------------
    if (showLoadOutputPathDialog)
    {
#if defined(CUSTOM_MODAL_DIALOGS)
        int result = GuiFileDialog(DIALOG_MESSAGE, "Select generation output directory...", inFileName, "Ok", "Edit the path in text box");
#else
        int result = GuiFileDialog(DIALOG_OPEN_DIRECTORY, "Select generation output directory...", inFileName, NULL, NULL);
#endif
        if (result == 1)
        {
            strcpy(config->Project.outputPath, inFileName);
        }

        if (result >= 0) showLoadOutputPathDialog = false;
    }
    //----------------------------------------------------------------------------------------

    // GUI: Export Project Dialog (and saving logic)
    //----------------------------------------------------------------------------------------
    if (showExportProjectProgress)
    {
        GuiPanel((Rectangle){ -10, screenHeight/2 - 100, screenWidth + 20, 200 }, NULL);

        int textSpacing = GuiGetStyle(DEFAULT, TEXT_SPACING);
        GuiSetStyle(DEFAULT, TEXT_SIZE, GuiGetFont().baseSize*3);
        GuiSetStyle(DEFAULT, TEXT_SPACING, 3);
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
        GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, GuiGetStyle(DEFAULT, TEXT_COLOR_FOCUSED));
        GuiLabel((Rectangle){ -10, screenHeight/2 - 60, screenWidth + 20, 30 }, ((int)exportProjectProgress >= 100)? "PROJECT GENERATED SUCCESSFULLY" : "GENERATING PROJECT...");
        GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL));
        GuiSetStyle(DEFAULT, TEXT_SIZE, GuiGetFont().baseSize*2);

        exportProjectProgress += 2.0f;
        GuiProgressBar((Rectangle){ 12, screenHeight/2, screenWidth - 24, 20 }, NULL, NULL, &exportProjectProgress, 0, 100);

        if (exportProjectProgress < 100.0f) GuiDisable();
        if (GuiButton((Rectangle){ screenWidth/4, screenHeight/2 + 40, screenWidth/2, 40 }, "GREAT!"))
        {
            showExportProjectProgress = false;
        }
        GuiEnable();

        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
        GuiSetStyle(DEFAULT, TEXT_SIZE, GuiGetFont().baseSize);
        GuiSetStyle(DEFAULT, TEXT_SPACING, textSpacing);

        if (!showExportProjectProgress)
        {
#if defined(PLATFORM_WEB)
            strcpy(outFileName, TextFormat("%s/%s", config->Project.outputPath, TextToLower(config->Project.name)));

            // Package all created files (in browser MEMFS) into a .zip to be exported
            mz_zip_archive zip = { 0 };
            mz_bool mz_ret = mz_zip_writer_init_file(&zip, TextFormat("%s.zip", outFileName), 0);
            if (!mz_ret) LOG("WARNING: Could not initialize zip archive\n");

            FilePathList files = LoadDirectoryFilesEx(outFileName, NULL, true);

            // Add all template updated files to zip
            for (int i = 0; i < files.count; i++)
            {
                // WARNING: We need to move the directory path a bit to skip "././" and "./"
                mz_ret = mz_zip_writer_add_file(&zip,
                    TextFormat("%s/%s", GetDirectoryPath(files.paths[i]) + 4, GetFileName(files.paths[i])),
                    TextFormat("%s/%s", GetDirectoryPath(files.paths[i]) + 2, GetFileName(files.paths[i])),
                    "Comment", (mz_uint16)strlen("Comment"), MZ_BEST_SPEED);
                if (!mz_ret) printf("Could not add file to zip archive\n");
            }

            mz_ret = mz_zip_writer_finalize_archive(&zip);
            if (!mz_ret) LOG("WARNING: Could not finalize zip archive\n");

            mz_ret = mz_zip_writer_end(&zip);
            if (!mz_ret) LOG("WARNING: Could not finalize zip writer\n");

            UnloadDirectoryFiles(files);

            char tempFileName[512] = { 0 };
            strcpy(tempFileName, TextFormat("%s.zip", outFileName));
            emscripten_run_script(TextFormat("saveFileFromMEMFSToDisk('%s','%s')", tempFileName, GetFileName(tempFileName)));

            // Download file from MEMFS (emscripten memory filesystem)
            // NOTE: Second argument must be a simple filename (we can't use directories)
            // NOTE: Included security check to (partially) avoid malicious code on PLATFORM_WEB
            //if (strchr(outFileName, '\'') == NULL) emscripten_run_script(TextFormat("saveFileFromMEMFSToDisk('%s','%s')", outFileName, GetFileName(outFileName)));
#endif
        }
    }
    //----------------------------------------------------------------------------------------
    EndTextureMode();

    BeginDrawing();
    ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

    // Draw render texture to screen
    if (screenSizeDouble) DrawTexturePro(screenTarget.texture, (Rectangle){ 0, 0, (float)screenTarget.texture.width, -(float)screenTarget.texture.height }, (Rectangle){ 0, 0, (float)screenTarget.texture.width*2, (float)screenTarget.texture.height*2 }, (Vector2){ 0, 0 }, 0.0f, WHITE);
    else DrawTextureRec(screenTarget.texture, (Rectangle){ 0, 0, (float)screenTarget.texture.width, -(float)screenTarget.texture.height }, (Vector2){ 0, 0 }, WHITE);
    EndDrawing();
    //----------------------------------------------------------------------------------
}

#if defined(PLATFORM_DESKTOP)
// Show command line usage info
static void ShowCommandLineInfo(void)
{
    printf("\n//////////////////////////////////////////////////////////////////////////////////\n");
    printf("//                                                                              //\n");
    printf("// %s v%s - %s     //\n", TOOL_NAME, TOOL_VERSION, TOOL_DESCRIPTION);
    printf("// powered by raylib v%s and raygui v%s                               //\n", RAYLIB_VERSION, RAYGUI_VERSION);
    printf("//                                                                              //\n");
    printf("// Copyright (c) 2024-2025 Ramon Santamaria (@raysan5)                          //\n");
    printf("//                                                                              //\n");
    printf("//////////////////////////////////////////////////////////////////////////////////\n\n");

    printf("USAGE:\n\n");
    printf("    > raylib_project_creator [--help] --name <project_name> --src <source_file01.c>,<source_file02.c>\n");
    printf("             [--product <product_name>] [--desc <project_description>]\n");
    printf("             [--dev <developer_name>] [--devweb <developer_webpage>]\n");
    printf("             [--raylib <raylib_src_path>] [--comp <compiler_path>]\n");
    printf("             [--out <output_path>]\n");

    printf("\nOPTIONS:\n\n");
    printf("    -h, --help                      : Show tool version and command line usage help\n\n");
    printf("    -n, --name                      : Define project name\n");
    printf("    -i, --src <source_file01.c>,<source_file02.c>\n");
    printf("                                    : Define input source files(s), comma separated.\n");
    printf("    -p, --product <product_name>    : Define product name, commercial name\n");
    printf("    --desc <project_description>    : Define product description, use \"desc\"\n");
    printf("    --dev <developer_name>          : Define developer name\n");
    printf("    --devweb <developer_webpage>    : Define developer webpage\n");
    printf("    --raylib <raylib_src_path>      : Define raylib src path (raylib.h)\n");
    printf("    --comp <compiler_path>          : Define compiler path (ggc.exe)\n");
    printf("    -o, --out <output_path>         : Define output path for generated project.\n");

    printf("\nEXAMPLES:\n\n");
    printf("    > raylib_project_creator -n rfxgen -p rFXGen --src rfxgen.c\n");
    printf("        Generate <rfxgen> project build system\n");
}

// Process command line input
static void ProcessCommandLine(int argc, char *argv[])
{
    // CLI required variables
    bool showUsageInfo = false;         // Toggle command line usage info

    if (argc == 1) showUsageInfo = true;

    ProjectConfig *config = (ProjectConfig *)RL_CALLOC(1, sizeof(ProjectConfig));

    // Process command line arguments
    for (int i = 1; i < argc; i++)
    {
        if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0))
        {
            showUsageInfo = true;
        }
        else if ((strcmp(argv[i], "-n") == 0) || (strcmp(argv[i], "--name") == 0))
        {
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                strcpy(config->Project.name, argv[i + 1]);
            }
            else LOG("WARNING: Name parameter provided not valid\n");
        }
        else if ((strcmp(argv[i], "-i") == 0) || (strcmp(argv[i], "--src") == 0))
        {
            // Check for valid argument and valid file extension
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                // Split provided arg by ',' to get multiple source input files
                int fileCount = 0;
                char **files = TextSplit(argv[i + 1], ',', &fileCount);

                for (int j = 0; j < fileCount; j++)
                {
                    if (IsFileExtension(files[i], ".h;.c"))
                    {
                        strcpy(config->Project.srcFilePaths[config->Project.srcFileCount], files[i]);
                        config->Project.srcFileCount++;
                    }
                    else LOG("WARNING: [%s] File not recognized as source file (Use: .c,.h)\n", files[i]);
                }
            }
            else LOG("WARNING: No input file provided\n");
        }
        else if ((strcmp(argv[i], "-p") == 0) || (strcmp(argv[i], "--product") == 0))
        {
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                strcpy(config->Project.productName, argv[i + 1]);
            }
            else LOG("WARNING: Product name parameters provided not valid\n");
        }
        else if (strcmp(argv[i], "--desc") == 0)
        {
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                strcpy(config->Project.description, argv[i + 1]);
            }
            else LOG("WARNING: Description parameters provided not valid\n");
        }
        else if (strcmp(argv[i], "--dev") == 0)
        {
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                strcpy(config->Project.developer, argv[i + 1]);
            }
            else LOG("WARNING: Developer parameters provided not valid\n");
        }
        else if (strcmp(argv[i], "--devweb") == 0)
        {
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                strcpy(config->Project.developerWeb, argv[i + 1]);
            }
            else LOG("WARNING: Developer web parameters provided not valid\n");
        }
        else if (strcmp(argv[i], "--raylib") == 0)
        {
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                strcpy(config->Building.raylibSrcPath, argv[i + 1]);
            }
            else LOG("WARNING: raylib source path parameters provided not valid\n");
        }
        else if (strcmp(argv[i], "--comp") == 0)
        {
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                strcpy(config->Building.compilerPath, argv[i + 1]);
            }
            else LOG("WARNING: Compiler path parameters provided not valid\n");
        }
        else if ((strcmp(argv[i], "-o") == 0) || (strcmp(argv[i], "--out") == 0))
        {
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                strcpy(config->Project.outputPath, argv[i + 1]);
            }
            else LOG("WARNING: Output path provided not valid\n");
        }
    }

    // Generate build projects
    SetupProject(config);

    RL_FREE(config);

    if (showUsageInfo) ShowCommandLineInfo();
}
#endif      // PLATFORM_DESKTOP

//--------------------------------------------------------------------------------------------
// Load/Save/Export functions
//--------------------------------------------------------------------------------------------
// Load project config data data from .rpc file
static ProjectConfig *LoadProjectData(const char *fileName)
{
    ProjectConfig *data = { 0 };

    if (FileExists(fileName))
    {
        rini_data config = { 0 };
        config = rini_load(fileName);

        // Process/organize config data for our application
        data = (ProjectConfig *)RL_CALLOC(1, sizeof(ProjectConfig));

        // TODO: Map config file data to project config

        rini_unload(&config);
    }

    return data;
}

// Unload project data
static void UnloadProjectData(ProjectConfig *data)
{
    RL_FREE(data);
}

// Save project config data to .rpc file
// NOTE: Same function as [rpc] tool but but adding more data
static void SaveProjectData(ProjectConfig *data, const char *fileName)
{
    rini_data config = rini_load(NULL);   // Create empty config with 32 entries (RINI_MAX_CONFIG_CAPACITY)

    // Define header comment lines
    rini_set_comment_line(&config, NULL);   // Empty comment line, but including comment prefix delimiter
    rini_set_comment_line(&config, "raylib project creator - project definition file");
    rini_set_comment_line(&config, NULL);
    rini_set_comment_line(&config, "This definition file contains all required info to descrive a project");
    rini_set_comment_line(&config, "and allow building it for multiple platforms");
    rini_set_comment_line(&config, NULL);
    rini_set_comment_line(&config, "This file follow certain conventions to be able to display the information in");
    rini_set_comment_line(&config, "an easy-configurable UI manner when loaded through [raylib project builder]");
    rini_set_comment_line(&config, "CONVENTIONS:");
    rini_set_comment_line(&config, "   - ID containing [_FLAG_]: Value is considered a boolean, it displays with a [GuiCheckBox]");
    rini_set_comment_line(&config, "   - ID do not contain "": Value is considered as an integer, it displays as [GuiValueBox]");
    rini_set_comment_line(&config, "   - ID ends with _FILE or _FILES: Value is considered as a text file path, it displays as [GuiTextBox] with a [BROWSE-File] button");
    rini_set_comment_line(&config, "   - ID ends with _PATH: Value is considered as a text directory path, it displays as [GuiTextBox] with a [BROWSE-Dir] button");
    rini_set_comment_line(&config, NULL);
    rini_set_comment_line(&config, "NOTE: The comments/description for each entry is used as tooltip when editing the entry on [rpb]");
    rini_set_comment_line(&config, "\n");

    // TODO: Save project data to config file

    rini_save(config, fileName);
    rini_unload(&config);
}

//--------------------------------------------------------------------------------------------
// Auxiliar functions
//--------------------------------------------------------------------------------------------

// Generate tool project files
// Project input files required to update:
//  - src/project_name.c
//  - src/project_name.rc
//  - src/project_name.ico
//  - src/project_name.icns
//  - src/Info.plist
//  - src/minshell.html
//  - src/Makefile
//  - projects/scripts/*
//  - projects/VS2022/*
//  - projects/VSCode/*
//  - README.md
//  - LICENSE
static void SetupProject(ProjectConfig *config)
{
    char *fileText = NULL;
    char *fileTextUpdated[6] = { 0 };

    // Get template directory
    // TODO: Use embedded template into executable?
    char templatePath[256] = { 0 };
    strcpy(templatePath, TextFormat("%s/template", GetApplicationDirectory()));
    //strcpy(templatePath, "./template"); // NOTE: Template directory should be in same directory as application, usually working directory

    //mz_bool mz_zip_reader_init_mem(mz_zip_archive *pZip, const void *pMem, size_t size, mz_uint flags); // Read file from memory zip data
    // TODO: Replace LoadFileText(), from template path, by reading text file from zip data, decompressing it...

    // Remove spaces from directories/files names
    char outProjectName[256] = { 0 };
    TextCopy(outProjectName, TextToLower(config->Project.name));//TextRemoveSpaces(TextToLower(config->Project.name)));

    LOG("INFO: Output path: %s\n", TextFormat("%s/%s", config->Project.outputPath, outProjectName));

    // Copy project source file(s) provided
    //--------------------------------------------------------------------------
    // Create required output directories
    MakeDirectory(TextFormat("%s/%s/src/external", config->Project.outputPath, outProjectName));
    if (config->Project.type == 0)  // Use base sample (one source file)
    {
        FileCopy(TextFormat("%s/src/project_name.c", templatePath),
            TextFormat("%s/%s/src/%s.c", config->Project.outputPath, outProjectName, TextToLower(config->Project.name)));

        LOG("INFO: Copied src/%s.c successfully\n", TextToLower(config->Project.name));
    }
    else if (config->Project.type == 1) // Use advance sample (screen manager, multiple source files)
    {
        FileCopy(TextFormat("%s/src/raylib_advanced.c", templatePath),
            TextFormat("%s/%s/src/%s.c", config->Project.outputPath, outProjectName, TextToLower(config->Project.name)));

        FileCopy(TextFormat("%s/src/screens.h", templatePath),
            TextFormat("%s/%s/src/screens.h", config->Project.outputPath, outProjectName));
        FileCopy(TextFormat("%s/src/screen_logo.c", templatePath),
            TextFormat("%s/%s/src/screen_logo.c", config->Project.outputPath, outProjectName));
        FileCopy(TextFormat("%s/src/screen_title.c", templatePath),
            TextFormat("%s/%s/src/screen_title.c", config->Project.outputPath, outProjectName));
        FileCopy(TextFormat("%s/src/screen_options.c", templatePath),
            TextFormat("%s/%s/src/screen_options.c", config->Project.outputPath, outProjectName));
        FileCopy(TextFormat("%s/src/screen_gameplay.c", templatePath),
            TextFormat("%s/%s/src/screen_gameplay.c", config->Project.outputPath, outProjectName));
        FileCopy(TextFormat("%s/src/screen_ending.c", templatePath),
            TextFormat("%s/%s/src/screen_ending.c", config->Project.outputPath, outProjectName));

        LOG("INFO: Copied advance project with src/%s.c successfully\n", TextToLower(config->Project.name));
    }
    else if (config->Project.type == 2) // Use provided source files
    {
        for (int i = 0; i < config->Project.srcFileCount; i++)
        {
            FileCopy(config->Project.srcFilePaths[i],
                TextFormat("%s/%s/src/%s", config->Project.outputPath, outProjectName, GetFileName(config->Project.srcFilePaths[i])));

            LOG("INFO: Copied src/%s successfully\n", GetFileName(config->Project.srcFilePaths[i]));
        }
    }
    //--------------------------------------------------------------------------

    // Project build system: Script
    //-------------------------------------------------------------------------------------
    MakeDirectory(TextFormat("%s/%s/projects/scripts", config->Project.outputPath, outProjectName));
    // Update src/build.bat (Windows only)
    // TODO: Use CMD/Shell calls directly, current script uses Makefile
    fileText = LoadFileText(TextFormat("%s/projects/scripts/build.bat", templatePath));
    fileTextUpdated[0] = TextReplace(fileText, "project_name", TextToLower(config->Project.name));
    fileTextUpdated[1] = TextReplace(fileTextUpdated[0], "ProjectDescription", config->Project.description);
    fileTextUpdated[2] = TextReplace(fileTextUpdated[1], "C:\\raylib\\w64devkit\\bin", config->Building.compilerPath);
    SaveFileText(TextFormat("%s/%s/projects/scripts/build.bat", config->Project.outputPath, outProjectName), fileTextUpdated[2]);
    for (int i = 0; i < 6; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
    UnloadFileText(fileText);

    LOG("INFO: Updated build system successfully: Script (src/build.bat)\n");
    //-------------------------------------------------------------------------------------

    // Project build system: Makefile
    //-------------------------------------------------------------------------------------
    // Update src/Makefile
    fileText = LoadFileText(TextFormat("%s/src/Makefile", templatePath));
    if (config->Project.type == 0) // Using basic template (one file)
    {
        fileTextUpdated[0] = TextReplace(fileText, "project_name.c", TextFormat("%s.c", TextToLower(config->Project.name)));
    }
    else if (config->Project.type == 1) // Using advance template (multiple files)
    {
        fileTextUpdated[0] = TextReplace(fileText, "project_name.c", TextFormat("%s.c screen_logo.c screen_title.c screen_options.c screen_gameplay.c screen_ending.c", TextToLower(config->Project.name)));
    }
    else if (config->Project.type == 2) // Using custom provided source files
    {
        char **srcFileNames = (char **)RL_CALLOC(256, sizeof(char *)); // Max number of input source files supported
        for (int i = 0; i < 256; i++) srcFileNames[i] = (char *)RL_CALLOC(256, sizeof(char));

        int codeFileCount = 0;
        for (int j = 0; j < config->Project.srcFileCount; j++)
        {
            if (IsFileExtension(config->Project.srcFilePaths[j], ".c"))
            {
                strcpy(srcFileNames[codeFileCount], GetFileName(config->Project.srcFilePaths[j]));
                codeFileCount++;
            }
        }

        fileTextUpdated[0] = TextReplace(fileText, "project_name.c", TextJoin(srcFileNames, codeFileCount, " "));

        for (int i = 0; i < 256; i++) RL_FREE(srcFileNames[i]);
        RL_FREE(srcFileNames);
    }
    fileTextUpdated[1] = TextReplace(fileText, "project_name", TextToLower(config->Project.name));
    fileTextUpdated[2] = TextReplace(fileTextUpdated[0], "C:\\raylib\\w64devkit\\bin", config->Building.compilerPath);
    fileTextUpdated[3] = TextReplace(fileTextUpdated[1], "C:/raylib/raylib/src", config->Building.raylibSrcPath);
    SaveFileText(TextFormat("%s/%s/src/Makefile", config->Project.outputPath, outProjectName), fileTextUpdated[3]);

    for (int i = 0; i < 6; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
    UnloadFileText(fileText);

    LOG("INFO: Updated build system successfully: Makefile (src/Makefile)\n");
    //-------------------------------------------------------------------------------------

    // Project build system: VS2022
    //-------------------------------------------------------------------------------------
    // Create required output directories
    MakeDirectory(TextFormat("%s/%s/projects/VS2022/raylib", config->Project.outputPath, outProjectName));
    MakeDirectory(TextFormat("%s/%s/projects/VS2022/%s", config->Project.outputPath, outProjectName, outProjectName));
    // Copy projects/VS2022/raylib/raylib.vcxproj
    fileText = LoadFileText(TextFormat("%s/projects/VS2022/raylib/raylib.vcxproj", templatePath));
    fileTextUpdated[0] = TextReplace(fileText, "C:\\raylib\\raylib\\src", config->Building.raylibSrcPath);
    SaveFileText(TextFormat("%s/%s/projects/VS2022/raylib/raylib.vcxproj", config->Project.outputPath, outProjectName, outProjectName), fileTextUpdated[0]);
    for (int i = 0; i < 6; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
    UnloadFileText(fileText);

    // Copy projects/VS2022/raylib/Directory.Build.props
    //fileText = LoadFileText(TextFormat("%s/projects/VS2022/raylib/Directory.Build.props", templatePath));
    //SaveFileText(TextFormat("%s/%s/projects/VS2022/raylib/Directory.Build.props", config->Project.outputPath, outProjectName, outProjectName), fileText);
    //UnloadFileText(fileText);

    // Update projects/VS2022/project_name/config->project_name.vcproj
    fileText = LoadFileText(TextFormat("%s/projects/VS2022/project_name/project_name.vcxproj", templatePath));
    if (config->Project.type == 0) // Using basic template (one file)
    {
        fileTextUpdated[0] = TextReplace(fileText, "project_name.c", TextFormat("%s.c", TextToLower(config->Project.name)));
        fileTextUpdated[1] = TextReplace(fileTextUpdated[0], "project_name", "project_name"); // WARNING: Only used to force a second buffer usage!
    }
    else if (config->Project.type == 1) // Using advance template (multiple files)
    {
        fileTextUpdated[0] = TextReplace(fileText, "project_name.c", TextFormat("%s.c", TextToLower(config->Project.name)));
        fileTextUpdated[1] = TextReplace(fileTextUpdated[0], "<!--Additional Compile Items-->",
        "<ClCompile Include=\"..\\..\\..\\src\\screen_logo.c\" />\n    \
         <ClCompile Include=\"..\\..\\..\\src\\screen_title.c\" />\n    \
         <ClCompile Include=\"..\\..\\..\\src\\screen_options.c\" />\n    \
         <ClCompile Include=\"..\\..\\..\\src\\screen_gameplay.c\" />\n    \
         <ClCompile Include=\"..\\..\\..\\src\\screen_ending.c\" />\n");
    }
    else if (config->Project.type == 2) // Using custom provided source files
    {
        char **srcFileNames = (char **)RL_CALLOC(256, sizeof(char *)); // Max number of input source files supported
        for (int i = 0; i < 256; i++) srcFileNames[i] = (char *)RL_CALLOC(256, sizeof(char));

        int codeFileCount = 0;
        for (int j = 0; j < config->Project.srcFileCount; j++)
        {
            if (IsFileExtension(config->Project.srcFilePaths[j], ".c"))
            {
                strcpy(srcFileNames[codeFileCount], GetFileName(config->Project.srcFilePaths[j]));
                codeFileCount++;
            }
        }

        fileTextUpdated[0] = TextReplace(fileText, "project_name.c", srcFileNames[0]);
        char srcFilesBlock[1024] = { 0 };
        int nextPosition = 0;
        for (int k = 1; k < codeFileCount; k++)
        {
            TextAppend(srcFilesBlock, TextFormat("<ClCompile Include=\"..\\..\\..\\src\\%s\" />\n    ", srcFileNames[k]), &nextPosition);
        }

        fileTextUpdated[1] = TextReplace(fileTextUpdated[0], "<!--Additional Compile Items-->", srcFilesBlock);

        for (int i = 0; i < 256; i++) RL_FREE(srcFileNames[i]);
        RL_FREE(srcFileNames);
    }
    fileTextUpdated[2] = TextReplace(fileTextUpdated[1], "project_name", outProjectName);
    fileTextUpdated[3] = TextReplace(fileTextUpdated[2], "C:\\raylib\\raylib\\src", config->Building.raylibSrcPath);
    SaveFileText(TextFormat("%s/%s/projects/VS2022/%s/%s.vcxproj", config->Project.outputPath, outProjectName, outProjectName, outProjectName), fileTextUpdated[3]);
    for (int i = 0; i < 6; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
    UnloadFileText(fileText);

    // Update projects/VS2022/project_name.sln
    fileText = LoadFileText(TextFormat("%s/projects/VS2022/project_name.sln", templatePath));
    fileTextUpdated[0] = TextReplace(fileText, "project_name", outProjectName);
    SaveFileText(TextFormat("%s/%s/projects/VS2022/%s.sln", config->Project.outputPath, outProjectName, outProjectName), fileTextUpdated[0]);
    for (int i = 0; i < 6; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
    UnloadFileText(fileText);

    LOG("INFO: Updated build system successfully: VS2022 (projects/VS2022)\n");
    //-------------------------------------------------------------------------------------

    // Project build system: VSCode
    //-------------------------------------------------------------------------------------
    // Create required output directories
    MakeDirectory(TextFormat("%s/%s/projects/VSCode/.vscode", config->Project.outputPath, outProjectName));
    // Update projects/VSCode/.vscode/launch.json
    fileText = LoadFileText(TextFormat("%s/projects/VSCode/.vscode/launch.json", templatePath));
    fileTextUpdated[0] = TextReplace(fileText, "project_name", TextToLower(config->Project.name));
    fileTextUpdated[1] = TextReplace(fileTextUpdated[0], "C:/raylib/w64devkit/bin", config->Building.compilerPath);
    SaveFileText(TextFormat("%s/%s/projects/VSCode/.vscode/launch.json", config->Project.outputPath, outProjectName), fileTextUpdated[1]);
    for (int i = 0; i < 6; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
    UnloadFileText(fileText);

    // Update projects/VSCode/.vscode/c_cpp_properties.json
    fileText = LoadFileText(TextFormat("%s/projects/VSCode/.vscode/c_cpp_properties.json", templatePath));
    fileTextUpdated[0] = TextReplace(fileText, "C:/raylib/raylib/src", config->Building.raylibSrcPath);
    fileTextUpdated[1] = TextReplace(fileTextUpdated[0], "C:/raylib/w64devkit/bin", config->Building.compilerPath);
    SaveFileText(TextFormat("%s/%s/projects/VSCode/.vscode/c_cpp_properties.json", config->Project.outputPath, outProjectName), fileTextUpdated[1]);
    for (int i = 0; i < 6; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
    UnloadFileText(fileText);

    // Update projects/VSCode/.vscode/tasks.json
    fileText = LoadFileText(TextFormat("%s/projects/VSCode/.vscode/tasks.json", templatePath));

    // Update source code files
    if (config->Project.type == 0) // Using basic template (one file)
    {
        fileTextUpdated[0] = TextReplace(fileText, "project_name.c", TextFormat("%s.c", TextToLower(config->Project.name)));
    }
    else if (config->Project.type == 1) // Using advance template (multiple files)
    {
        fileTextUpdated[0] = TextReplace(fileText, "project_name.c", TextFormat("%s.c screen_logo.c screen_title.c screen_options.c screen_gameplay.c screen_ending.c", TextToLower(config->Project.name)));
    }
    else if (config->Project.type == 2) // Using custom provided source files
    {
        char **srcFileNames = (char **)RL_CALLOC(256, sizeof(char *)); // Max number of input source files supported
        for (int i = 0; i < 256; i++) srcFileNames[i] = (char *)RL_CALLOC(256, sizeof(char));

        int codeFileCount = 0;
        for (int j = 0; j < config->Project.srcFileCount; j++)
        {
            if (IsFileExtension(config->Project.srcFilePaths[j], ".c"))
            {
                strcpy(srcFileNames[codeFileCount], GetFileName(config->Project.srcFilePaths[j]));
                codeFileCount++;
            }
        }

        fileTextUpdated[0] = TextReplace(fileText, "project_name.c", TextJoin(srcFileNames, codeFileCount, " "));

        for (int i = 0; i < 256; i++) RL_FREE(srcFileNames[i]);
        RL_FREE(srcFileNames);
    }

    fileTextUpdated[1] = TextReplace(fileText, "project_name", TextToLower(config->Project.name));
    fileTextUpdated[2] = TextReplace(fileTextUpdated[1], "C:/raylib/raylib/src", config->Building.raylibSrcPath);
    fileTextUpdated[3] = TextReplace(fileTextUpdated[2], "C:/raylib/w64devkit/bin", config->Building.compilerPath);

    SaveFileText(TextFormat("%s/%s/projects/VSCode/.vscode/tasks.json", config->Project.outputPath, outProjectName), fileTextUpdated[3]);
    for (int i = 0; i < 6; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
    UnloadFileText(fileText);

    // Copy projects/VSCode/.vscode/settings.json
    FileCopy(TextFormat("%s/projects/VSCode/.vscode/settings.json", templatePath),
        TextFormat("%s/%s/projects/VSCode/.vscode/settings.json", config->Project.outputPath, outProjectName));

    // Copy projects/VSCode/main.code-workspace
    FileCopy(TextFormat("%s/projects/VSCode/main.code-workspace", templatePath),
        TextFormat("%s/%s/projects/VSCode/main.code-workspace", config->Project.outputPath, outProjectName));

    // Copy projects/VSCode/README.md
    FileCopy(TextFormat("%s/projects/VSCode/README.md", templatePath),
        TextFormat("%s/%s/projects/VSCode/README.md", config->Project.outputPath, outProjectName));

    LOG("INFO: Updated build system successfully: VSCode (projects/VSCode)\n");
    //-------------------------------------------------------------------------------------

    // Project build system: GitHub Actions
    // - Windows: Uses VS2022 project
    // - Linux, macOS, WebAssembly: Uses Makefile project
    // WARNING: Expects the PROJECT_NAME to be the repository-name (as generated by default)
    //-------------------------------------------------------------------------------------
    // Create required output directories
    MakeDirectory(TextFormat("%s/%s/.github/workflows", config->Project.outputPath, outProjectName));
    // Copy GitHub workflows: linux.yml, webassembly.yml, windows.yml
    fileText = LoadFileText(TextFormat("%s/.github/workflows/windows.yml", templatePath));
    SaveFileText(TextFormat("%s/%s/.github/workflows/windows.yml", config->Project.outputPath, outProjectName), fileText);
    UnloadFileText(fileText);
    fileText = LoadFileText(TextFormat("%s/.github/workflows/linux.yml", templatePath));
    SaveFileText(TextFormat("%s/%s/.github/workflows/linux.yml", config->Project.outputPath, outProjectName), fileText);
    UnloadFileText(fileText);
    fileText = LoadFileText(TextFormat("%s/.github/workflows/macos.yml", templatePath));
    SaveFileText(TextFormat("%s/%s/.github/workflows/macos.yml", config->Project.outputPath, outProjectName), fileText);
    UnloadFileText(fileText);
    fileText = LoadFileText(TextFormat("%s/.github/workflows/webassembly.yml", templatePath));
    SaveFileText(TextFormat("%s/%s/.github/workflows/webassembly.yml", config->Project.outputPath, outProjectName), fileText);
    UnloadFileText(fileText);

    LOG("INFO: Updated build system successfully: GitHub Actions CI/CD workflows (.github)\n");
    //-------------------------------------------------------------------------------------

    // Update additional files required for product building
    //  - src/project_name.rc   -> Windows: Executable resource file, includes .ico and metadata
    //  - src/project_name.ico  -> Product icon, required for Window resource file
    //  - src/project_name.icns -> macOS: Product icon, required by Info.plist
    //  - src/Info.plist        -> macOS application resource file, includes .icns and metadata
    //  - src/minshell.html     -> Web: Html minimum shell for WebAssembly application, preconfigured
    //-------------------------------------------------------------------------------------
    // Update src/project_name.rc
    fileText = LoadFileText(TextFormat("%s/src/project_name.rc", templatePath));
    fileTextUpdated[0] = TextReplace(fileText, "CommercialName", config->Project.productName);
    fileTextUpdated[1] = TextReplace(fileTextUpdated[0], "project_name", TextToLower(config->Project.name));
    fileTextUpdated[2] = TextReplace(fileTextUpdated[1], "ProjectDescription", config->Project.description);
    fileTextUpdated[3] = TextReplace(fileTextUpdated[2], "ProjectDev", config->Project.developer);
    SaveFileText(TextFormat("%s/%s/src/%s.rc", config->Project.outputPath, outProjectName, outProjectName), fileTextUpdated[3]);
    for (int i = 0; i < 6; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
    UnloadFileText(fileText);
    LOG("INFO: Updated src/%s.rc successfully\n", outProjectName);

    // Copy src/project_name.ico to src/project_name.ico
    FileCopy(TextFormat("%s/src/project_name.ico", templatePath),
        TextFormat("%s/%s/src/%s.ico", config->Project.outputPath, outProjectName, outProjectName));
    LOG("INFO: Copied src/%s.ico successfully\n", TextToLower(config->Project.name));

    // Copy src/project_name.icns to src/project_name.icns
    FileCopy(TextFormat("%s/src/project_name.icns", templatePath),
        TextFormat("%s/%s/src/%s.icns", config->Project.outputPath, outProjectName, outProjectName));
    LOG("INFO: Copied src/%s.icns successfully\n", TextToLower(config->Project.name));

    // Update src/Info.plist
    fileText = LoadFileText(TextFormat("%s/src/Info.plist", templatePath));
    fileTextUpdated[0] = TextReplace(fileText, "ProductName", config->Project.productName);
    fileTextUpdated[1] = TextReplace(fileTextUpdated[0], "project_name", TextToLower(config->Project.name));
    fileTextUpdated[2] = TextReplace(fileTextUpdated[1], "ProjectDescription", config->Project.description);
    fileTextUpdated[3] = TextReplace(fileTextUpdated[2], "ProjectDev", config->Project.developer);
    fileTextUpdated[4] = TextReplace(fileTextUpdated[3], "project_dev", TextToLower(config->Project.developer));
    fileTextUpdated[5] = TextReplace(fileTextUpdated[4], "developer_web", TextToLower(config->Project.developerWeb));
    SaveFileText(TextFormat("%s/%s/src/Info.plist", config->Project.outputPath, outProjectName), fileTextUpdated[5]);
    for (int i = 0; i < 6; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
    UnloadFileText(fileText);
    LOG("INFO: Updated src/Info.plist successfully\n");

    // Update src/minshell.html
    // Review Webpage, links, OpenGraph/X card, keywords...
    fileText = LoadFileText(TextFormat("%s/src/minshell.html", templatePath));
    fileTextUpdated[0] = TextReplace(fileText, "ProductName", config->Project.productName);
    fileTextUpdated[1] = TextReplace(fileTextUpdated[0], "project_name", TextToLower(config->Project.name));
    fileTextUpdated[2] = TextReplace(fileTextUpdated[1], "ProjectDescription", config->Project.description);
    fileTextUpdated[3] = TextReplace(fileTextUpdated[2], "ProjectDev", config->Project.developer);
    fileTextUpdated[4] = TextReplace(fileTextUpdated[3], "project_dev", TextToLower(config->Project.developer));
    fileTextUpdated[5] = TextReplace(fileTextUpdated[4], "developer_web", TextToLower(config->Project.developerWeb));
    SaveFileText(TextFormat("%s/%s/src/minshell.html", config->Project.outputPath, outProjectName), fileTextUpdated[5]);
    for (int i = 0; i < 6; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
    UnloadFileText(fileText);
    LOG("INFO: Updated src/minshell.html successfully\n");
    //-------------------------------------------------------------------------------------

    // TODO: Process resource directory --> Copy to provided output resource path?

    // Update README.md
    fileText = LoadFileText(TextFormat("%s/README.md", templatePath));
    fileTextUpdated[0] = TextReplace(fileText, "ProductName", config->Project.productName);
    fileTextUpdated[1] = TextReplace(fileTextUpdated[0], "project_name", TextToLower(config->Project.name));
    fileTextUpdated[2] = TextReplace(fileTextUpdated[1], "ProjectDescription", config->Project.description);
    fileTextUpdated[3] = TextReplace(fileTextUpdated[2], "ProjectDev", config->Project.developer);
    SaveFileText(TextFormat("%s/%s/README.md", config->Project.outputPath, outProjectName), fileTextUpdated[3]);
    for (int i = 0; i < 6; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
    UnloadFileText(fileText);
    LOG("INFO: Updated README.md successfully\n");

    // Update LICENSE, including ProjectDev
    fileText = LoadFileText(TextFormat("%s/LICENSE", templatePath));
    fileTextUpdated[0] = TextReplace(fileText, "ProjectDev", config->Project.developer);
    SaveFileText(TextFormat("%s/%s/LICENSE", config->Project.outputPath, outProjectName), fileTextUpdated[0]);
    for (int i = 0; i < 6; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
    UnloadFileText(fileText);
    LOG("INFO: Updated LICENSE successfully\n");

    // Copy from template files that do not require customization: CONVENTIONS.md, .gitignore
    FileCopy(TextFormat("%s/CONVENTIONS.md", templatePath),
        TextFormat("%s/%s/CONVENTIONS.md", config->Project.outputPath, outProjectName));
    FileCopy(TextFormat("%s/.gitignore", templatePath),
        TextFormat("%s/%s/.gitignore", config->Project.outputPath, outProjectName));

    LOG("INFO: GitHub %s project generated successfully!\n", config->Project.name);
}

// Packing of directory files into a binary blob
static char *PackDirectoryData(const char *baseDirPath, int *packSize)
{
    #define MAX_PACKED_DATA_SIZE    10*1024*1024    // 10 MB

    int fullPackSize = 0;
    char *data = NULL;

    FilePathList files = LoadDirectoryFilesEx(baseDirPath, NULL, true);

    if (files.count > 0)
    {
        int filesDataSize = 0;
        data = (char *)RL_CALLOC(MAX_PACKED_DATA_SIZE, 1);
        PackFileEntry *entries = (PackFileEntry *)RL_CALLOC(files.count, sizeof(PackFileEntry));

        for (unsigned int i = 0; i < files.count; i++)
        {
            strcpy(entries[i].filePath, files.paths[i]);
            unsigned char *fileData = LoadFileData(files.paths[i], &entries[i].fileSize);
            unsigned char *compFileData = CompressData(fileData, entries[i].fileSize, &entries[i].compFileSize);
            UnloadFileData(fileData);

            printf("Packing file: %s\n", files.paths[i]);

            memcpy(data + filesDataSize, compFileData, entries[i].compFileSize);
            filesDataSize += entries[i].compFileSize;
            MemFree(compFileData);
        }

        // OPTION: Compress entries data for optimization
        int compEntriesDataSize = 0;
        unsigned char *compEntriesData = CompressData((unsigned char *)entries, files.count*sizeof(PackFileEntry), &compEntriesDataSize);

        // Append entries data + compEntriesDataSize + filesDataSize + files.count + CCFOUR (at the end of file)
        fullPackSize = filesDataSize;
        memcpy(data + fullPackSize, compEntriesData, compEntriesDataSize);
        fullPackSize += compEntriesDataSize;

        memcpy(data + fullPackSize, &compEntriesDataSize, sizeof(int));
        fullPackSize += sizeof(int);
        memcpy(data + fullPackSize, &filesDataSize, sizeof(int));
        fullPackSize += sizeof(int);
        memcpy(data + fullPackSize, &files.count, sizeof(int));
        fullPackSize += sizeof(int);
        char fourcc[5] = "rpch";
        memcpy(data + fullPackSize, fourcc, 4);
        fullPackSize += 4;
    }

    UnloadDirectoryFiles(files);

    *packSize = fullPackSize;
    return data;
}

// Unpacking of directory files from a binary blob
static void UnpackDirectoryData(const char *outputDirPath, const unsigned char *data, int *dataSize, PackFileEntry *entries, int fileCount)
{
    int nextFileDataOffset = 0;

    for (int i = 0; i < fileCount; i++)
    {
        // Decompress entry from data
        int fileDataSize = 0;
        unsigned char *fileData = DecompressData(data + nextFileDataOffset, entries[i].compFileSize, &fileDataSize);

        // Verify process worked as expected
        if ((fileData != NULL) && (fileDataSize == entries[i].fileSize))
        {
            SaveFileData(TextFormat("%s/%s", outputDirPath, entries[i].filePath), fileData, fileDataSize);
        }
        else
        {
            LOG("WARNING: File data could not be decompressed!\n");
            break;
        }

        MemFree(fileData);
        nextFileDataOffset += entries[i].compFileSize;
    }
}

// Load a text file data from memory packed data
char *LoadFileTextPack(const char *fileName, const char *packData, PackFileEntry *entries, int fileCount)
{
    int fileDataSize = 0;
    char *fileData = NULL;

    // Find data offset in package and decompress it
    for (int i = 0, dataOffset = 0; i < fileCount; i++)
    {
        if (TextIsEqual(fileName, entries[i].filePath))
        {
            unsigned char *uncompFileData = DecompressData(packData + dataOffset, entries[i].compFileSize, &fileDataSize);

            if ((fileData != NULL) && (fileDataSize == entries[i].fileSize))
            {
                // NOTE: We make sure the text data ends with /0
                fileData = (char *)RL_CALLOC(entries[i].fileSize + 1, 1);
                memcpy(fileData, uncompFileData, fileDataSize);
                MemFree(uncompFileData);
            }
            else LOG("WARNING: File not loaded properly from pack\n");

            break;
        }
        else dataOffset += entries[i].compFileSize;
    }

    return fileData;
}

// Split string into multiple strings
// NOTE: No memory is dynamically allocated
static const char **GetSubtextPtrs(char *text, char delimiter, int *count)
{
    #define MAX_SUBTEXTPTRS_COUNT    1024

    // WARNING: Input string is modified, '\0' is added in the delimiter and the resulting strings
    // are returned as a static array of pointers
    // Maximum number of pointed strings is set by MAX_TEXTSPLIT_COUNT
    static const char *result[MAX_SUBTEXTPTRS_COUNT] = { NULL };

    result[0] = text;
    int counter = 1;
    int textSize = TextLength(text);

    if (text != NULL)
    {
        // Count how many substrings we have on text and point to every one
        for (int i = 0; i < textSize; i++)
        {
            if (text[i] == '\0') break;
            else if (text[i] == delimiter)
            {
                text[i] = '\0';   // Set an end of string at this point
                result[counter] = text + i + 1;
                counter++;

                if (counter == MAX_SUBTEXTPTRS_COUNT) break;
            }
        }
    }

    *count = counter;
    return result;
}

// Load/Save application configuration functions
//------------------------------------------------------------------------------------
// Load aplication init configuration
static void LoadApplicationConfig(void)
{
    int windowMaximized = 0;
#if defined(PLATFORM_WEB)
    bool loadConfigData = true;
#else
    bool loadConfigData = FileExists(TextFormat("%s/%s", GetApplicationDirectory(), TOOL_CONFIG_FILENAME));
#endif

    if (loadConfigData)
    {
        rini_data config = { 0 };
#if defined(PLATFORM_WEB)
        int outputSize = 0;
        char *configDataBase64 = LoadWebLocalStorage(TOOL_CONFIG_FILENAME);
        char *configText = DecodeDataBase64(configDataBase64, &outputSize);
        config = rini_load_from_memory(configText);
        MemFree(configText);
#else
        config = rini_load(TextFormat("%s/%s", GetApplicationDirectory(), TOOL_CONFIG_FILENAME));
#endif
        // Load required config variables
        // NOTE: Keys not found default to 0 value, unless fallback is requested
        windowAboutState.showSplash = rini_get_value(config, "SHOW_WINDOW_WELCOME");
        windowMaximized = rini_get_value(config, "INIT_WINDOW_MAXIMIZED");
        mainToolbarState.visualStyleActive = rini_get_value(config, "GUI_VISUAL_STYLE");

        rini_unload(&config);

        // NOTE: Config is automatically saved when application is closed
    }

    // Setup application using config values (or default)
    if (windowAboutState.showSplash) { windowAboutState.welcomeMode = true; windowAboutState.windowActive = true; }
    else { windowAboutState.welcomeMode = false; windowAboutState.windowActive = false; }
    //if (mainToolbarState.showTooltips) GuiEnableTooltip();
    //else GuiDisableTooltip();

    if (windowMaximized == 1) MaximizeWindow();
}

// Save application configuration
static void SaveApplicationConfig(void)
{
    rini_data config = rini_load(NULL);   // Create empty config with 32 entries (RINI_MAX_CONFIG_CAPACITY)

    // Define header comment lines
    rini_set_comment_line(&config, NULL);   // Empty comment line, but including comment prefix delimiter
    rini_set_comment_line(&config, TextFormat("%s initialization configuration options", TOOL_NAME));
    rini_set_comment_line(&config, NULL);
    rini_set_comment_line(&config, "NOTE: This file is loaded at application startup,");
    rini_set_comment_line(&config, "if file is not found, default values are applied");
    rini_set_comment_line(&config, NULL);

#if defined(PLATFORM_DESKTOP)
    int windowMaximized = (int)IsWindowMaximized();
#endif
    rini_set_value(&config, "SHOW_WINDOW_WELCOME", (int)windowAboutState.showSplash, "Show welcome window at initialization");
#if defined(PLATFORM_DESKTOP)
    rini_set_value(&config, "INIT_WINDOW_MAXIMIZED", (int)windowMaximized, "Initialize window maximized");
#endif
    rini_set_value(&config, "GUI_VISUAL_STYLE", (int)mainToolbarState.visualStyleActive, "UI visual style selected");

#if defined(PLATFORM_WEB)
    int outputSize = 0;
    char *configText = rini_save_to_memory(config);
    char *configBase64 = EncodeDataBase64(configText, strlen(configText), &outputSize);
    SaveWebLocalStorage(TOOL_CONFIG_FILENAME, configBase64);
    MemFree(configBase64);
#else
    rini_save(config, TextFormat("%s/%s", GetApplicationDirectory(), TOOL_CONFIG_FILENAME));
#endif
    rini_unload(&config);
}

#if defined(PLATFORM_WEB)
// Save data to web LocalStorage (persistent between sessions)
// WARNING: Despite line-breaks are supposedly supported in value,
// emscripten interprets them as separate execution lines and fails -> Use Base64 string
static void SaveWebLocalStorage(const char *key, const char *value)
{
    char script[2048] = { 0 };
    snprintf(script, 2048, "localStorage.setItem(\"%s\", \"%s\")", key, value);

    // Run script to save config to local storage
    // WARNING: TextFormat() can not be used because defaults to MAX 1024 chars
    emscripten_run_script(script);
}

// Load data from web LocalStorage (persistent between sessions)
static char *LoadWebLocalStorage(const char *key)
{
    // NOTE: Make sure result has enough space for the retrieved data!
    static char result[2048] = { 0 };
    memset(result, 0, 2048);

    // Run the script and get the result as a string
    const char *loadedData = emscripten_run_script_string(TextFormat("localStorage.getItem('%s')", key));
    strncpy(result, loadedData, sizeof(result) - 1);
    result[sizeof(result) - 1] = '\0';

    return result;
}
#endif
//------------------------------------------------------------------------------------
