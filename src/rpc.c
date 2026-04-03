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
*       2.0  (xx-Nov-2026)  ADDED: Load and save project configuration data
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
*       raylib 6.0              - Windowing/input management and drawing
*       raygui 5.0              - Immediate-mode GUI controls with custom styling and icons
*       tinyfiledialogs 3.20.0  - Open/save file dialogs, it requires linkage with comdlg32 and ole32 libs
*       miniz 2.2.0             - Save .zip package file (required for multiple images export)
*
*   COMPILATION (Windows - MinGW):
*       gcc -o $(NAME_PART).exe $(FILE_NAME) -I../../src -lraylib -lopengl32 -lgdi32 -std=c99
*
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2024-2026 Ramon Santamaria (@raysan5)
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

#define RPCONFIG_IMPLEMENTATION
#include "rpconfig.h"                // Data types and functionality (shared by [rpc] and [rpb] tools)

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
// REF: https://github.com/richgel999/miniz
#include "external/miniz.h"                 // ZIP packaging functions definition
#include "external/miniz.c"                 // ZIP packaging implementation

//#include "template.zip.h"                   // Project template to embed into executable (zipped)

#define RPNG_IMPLEMENTATION
#include "external/rpng.h"                  // PNG chunks management

#define RINI_MAX_VALUE_CAPACITY     256
#define RINI_MAX_TEXT_SIZE          256
#define RINI_KEY_SPACING             37
#define RINI_VALUE_SPACING           35
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

#define RPC_MAX_SOURCE_FILES        128
#define RPC_MAX_ASSET_FILES         256
#define RPC_SOURCE_PATH_LENGTH      256     // Source file path length
#define RPC_ASSET_PATH_LENGTH       256     // Asset file path length

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

// NOTE: [rpc] and [rpb] tools shared data types and functions are provided by rpcdata.h

// Project input data required for .rpc generation
typedef struct rpcProjectInput {
    int srcFileCount;               // Project: source files count
    char **srcFilePaths;            // Project: source files path(s) -> RPC_MAX_SOURCE_FILES
    int assetFileCount;             // Project: assets files count
    char **assetFilePaths;          // Project: assets files paths -> RPC_MAX_ASSET_FILES

    bool requestedBuildSystems[6];  // Build: Flags: systems required: 0-Script, 1-Makefile, 2-VSCode, 3-VS2022, 4-CMake
} rpcProjectInput;

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

// Basic program variables
//----------------------------------------------------------------------------------
static const int screenWidth = 1070;        // Default screen width (at initialization)
static const int screenHeight = 820;        // Default screen height (at initialization)

// NOTE: Max length depends on OS, in Windows MAX_PATH = 256
static char inFileName[256] = { 0 };            // Input file name (required in case of drag & drop over executable)
static char outFileName[256] = { 0 };           // Output file name (required for file save/export)

static char inDirectoryPath[256] = { 0 };       // Input directory path
static char outProjectPath[256] = { 0 };        // Output project path, initializeed to working directory

static int framesCounter = 0;                   // General pourpose frames counter (not used)
static Vector2 mousePoint = { 0 };              // Mouse position
static bool lockBackground = false;             // Toggle lock background (controls locked)
static bool saveChangesRequired = false;        // Flag to notice save changes are required

static RenderTexture2D screenTarget = { 0 };    // Render texture to render the tool (if required)

static Vector2 propPanelScroll = { 0, -10 };    // Project properties panel scroll offset
static Rectangle propPanelView = { 0 };         // Project properties panel view (for scissoring)
static Vector2 filesPanelScroll = { 0 };
static Rectangle filesPanelView = { 0 };

// Info panel customizable variables
static bool showInfoMessagePanel = false;       // Flag: request info message panel
static const char *infoTitle = NULL;            // Info panel: title
static const char *infoMessage = NULL;          // Info panel: message
static const char *infoButton = NULL;           // Info panel: button text

// Screen scaling variables
static int monitorWidth = 0;                    // Monitor width
static int monitorHeight = 0;                   // Monitor height
static bool screenSizeDouble = false;           // Flag to screen size x2, in case of HighDPI

static bool showMessageReset = false;           // Show message: reset
static bool showMessageExit = false;            // Show message: exit (quit)
static bool showGenProjectProgress = false;     // Show project generation progress bar
static float generateProjectProgress = 0.0f;    // Project export progress bar (fake)

static double baseTime = 0;                     // Base time in seconds to start counting
static double currentTime = 0;                  // Current time counter in seconds
static int currentYear = 2026;                  // Current year for project, retrieved at init

// Project variables
static rpcProjectConfig project = { 0 };     // Project configuration (generic-raw structure)
static rpcProjectInput input = { 0 };           // Project input data (requried for generation)

static int selectedTemplate = 0;                // Project selected template, defines input data
static char generationOutPath[256] = { 0 };     // Project generation output path

// TODO: Support icons images viewing
static Texture2D *texProjectIcons = { 0 };      // Project icon textures
static int projectIconCount = 0;
//-----------------------------------------------------------------------------------

// GUI: Custom file dialogs
//-----------------------------------------------------------------------------------
static bool showLoadProjectDialog = false;
static bool showSaveProjectDialog = false;

static bool showAddInputFilesDialog = false;
static bool showAddInputDirectoryDialog = false;

static bool showProjectGenPathDialog = false;

static bool showLoadFileDialog = false;
static bool showLoadDirectoryDialog = false;
static int projectEditProperty = -1;

static bool showLoadIconDialog = false;

static bool showLoadSourceFilesDialog = false;
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

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
#if defined(PLATFORM_DESKTOP)
// Command line functionality
static void ShowCommandLineInfo(void);                      // Show command line usage info
static void ProcessCommandLine(int argc, char *argv[]);     // Process command line input
#endif

static void UpdateDrawFrame(void);                          // Update and draw one frame

static rpcProjectInput rpcLoadProjectInput(void);           // Load project source/asset file paths bucket
static void rpcUnloadProjectInput(rpcProjectInput input);   // Unload project source/asset file paths bucket
static void rpcUpdateProjectInput(rpcProjectInput *input, int selTemplate); // Update input data by selected template

static char **LoadSourceAssetPaths(const char *filePath, int *assetPathCount); // Scan resource paths in example file
static void UnloadSourceAssetPaths(char **assetPaths);      // Unload resource paths scanned

// Generate output project structure
static void GenerateProject(rpcProjectConfig project, rpcProjectInput input, const char *outPath);

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
static const char *CallBeforeWebUnload(int eventType, const void *reserved, void *userData) { SaveApplicationConfig(); return NULL; }
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

    // Get current year
    time_t now = time(NULL);
    struct tm *nowTime = localtime(&now);
    currentYear = nowTime->tm_year + 1900;

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
                // Load .rpc config file and open tool UI
                project = rpcLoadProjectConfig(argv[1]);
            }
            else if (IsFileExtension(argv[1], ".c"))
            {
                // Process automatically the C file and setup a project using source file name
                // WARNING: Really? Is this the desired behaviour?
                project = rpcLoadProjectConfig("template/project_name.rpc"); // Load base template data

                rpcSetText(project, "PROJECT_INTERNAL_NAME", TextToSnake(GetFileNameWithoutExt(argv[1])));
                rpcSetText(project, "PROJECT_REPO_NAME", GetFileNameWithoutExt(argv[1]));
                rpcSetText(project, "PROJECT_COMMERCIAL_NAME", GetFileNameWithoutExt(argv[1]));
                rpcSetText(project, "PROJECT_SHORT_NAME", GetFileNameWithoutExt(argv[1]));
                rpcSetText(project, "PROJECT_VERSION", "1.0");
                rpcSetText(project, "PROJECT_DESCRIPTION","A very cool project");
                rpcSetText(project, "PROJECT_PUBLISHER_NAME", "raylib technologies");
                rpcSetText(project, "PROJECT_DEVELOPER_NAME", "Ramon Santamaria");
                rpcSetText(project, "PROJECT_DEVELOPER_URL", "www.raylibtech.com");
                rpcSetText(project, "PROJECT_DEVELOPER_EMAIL", "ray@raylibtech.com");
                rpcSetText(project, "PROJECT_ICON_FILE", "template/src/project_name.ico");
                rpcSetText(project, "PLATFORM_WINDOWS_W64DEVKIT_PATH", "C:\\raylib\\w64devkit\\bin");
                rpcSetText(project, "RAYLIB_SRC_PATH", "C:\\raylib\\raylib\\src");

                selectedTemplate = 0;  // Custom input file
                input.srcFileCount = 1;
                strcpy(input.srcFilePaths[0], argv[1]);
                strcpy(generationOutPath, ".");

                // NOTE: Project requires input data to generate output .rpc file
                GenerateProject(project, input, generationOutPath);

                rpcUnloadProjectConfig(project);

                return 0;
            }
        }
        else
        {
            ProcessCommandLine(argc, argv);
            return 0;
        }
    }
#endif // PLATFORM_DESKTOP
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

    input = rpcLoadProjectInput();

    if (project.entries == NULL)
    {
        // Initialize project config default
        selectedTemplate = 1;   // Basic window
        project = rpcLoadProjectConfig("template/project_name.rpc");
        rpcSetText(project, "PROJECT_INTERNAL_NAME", "cool_project");
        rpcSetText(project, "PROJECT_REPO_NAME", "cool_project");
        rpcSetText(project, "PROJECT_COMMERCIAL_NAME", "Cool Project");
        rpcSetText(project, "PROJECT_SHORT_NAME", "CoolProject");
        rpcSetText(project, "PROJECT_VERSION", "1.0");
        rpcSetText(project, "PROJECT_DESCRIPTION", "A very cool project");
        rpcSetText(project, "PROJECT_PUBLISHER_NAME", "raylib technologies");
        rpcSetText(project, "PROJECT_DEVELOPER_NAME", "Ramon Santamaria");
        rpcSetText(project, "PROJECT_DEVELOPER_URL", "www.raylibtech.com");
        rpcSetText(project, "PROJECT_DEVELOPER_EMAIL", "ray@raylibtech.com");
        rpcSetText(project, "PROJECT_ICON_FILE", "template/src/project_name.ico");
        rpcSetText(project, "PLATFORM_WINDOWS_W64DEVKIT_PATH", "C:\\raylib\\w64devkit\\bin");
        rpcSetText(project, "RAYLIB_SRC_PATH", "C:\\raylib\\raylib\\src");

        // TODO: Using default template source file, it should be renamed on generation!
        input.srcFileCount = 1;
        strcpy(input.srcFilePaths[0], TextFormat("template/src/project_name.c"));//, rpcGetText(project, "PROJECT_INTERNAL_NAME")));
        strcpy(generationOutPath, ".");
    }

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

    // Welcome panel data
    infoTitle = "WELCOME! LET'S CREATE A PROJECT!";
    infoMessage = "Provide some source code files (.c) to generate project!";// \nOr choose a default project type!";
    infoButton = "Sure! Let's start!";
    showInfoMessagePanel = true;

    strcpy(outFileName, TextFormat("%s/%s", GetWorkingDirectory(), rpcGetText(project, "PROJECT_INTERNAL_NAME")));

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
    emscripten_set_beforeunload_callback(NULL, CallBeforeWebUnload);
#endif
    //-------------------------------------------------------------------------------------

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
    //rpcUnloadProjectConfigTyped(project);

    UnloadRenderTexture(screenTarget); // Unload render texture

    // Save application init configuration for next run
    //--------------------------------------------------------------------------------------
    SaveApplicationConfig();
    //--------------------------------------------------------------------------------------

    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

#endif // !COMMAND_LINE_ONLY
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

        if ((droppedFiles.count == 1) && IsFileExtension(droppedFiles.paths[0], ".rgs"))
        {
            // Reset to default internal style
            // NOTE: Required to unload any previously loaded font texture
            GuiLoadStyleDefault();
            GuiLoadStyle(droppedFiles.paths[0]);
        }
        else
        {
            for (unsigned int i = 0; i < droppedFiles.count; i++)
            {
                if (IsPathFile(droppedFiles.paths[i]))
                {
                    
                    if (IsFileExtension(droppedFiles.paths[i], ".c;.h;.cpp;.hpp"))
                    {
                        // Add files to source list
                        strcpy(input.srcFilePaths[input.srcFileCount], droppedFiles.paths[i]);
                        input.srcFileCount++;
                    }
                    else if (IsFileExtension(droppedFiles.paths[i], ".png;.bmp;.jpg;.qoi;.gif;.raw;.hdr;.ktx;.dxt;.astc;.pvr;.ttf;.otf;.fnt;.wav;.ogg;.mp3;.flac;.mod;.xm;.qoa;.obj;.iqm;.glb;.gltf;.m3d;.vox;.vs;.fs;.txt"))
                    {
                        // Add assets to assets list
                        // TODO: Filtering for recognized assets extensions but, really required?
                        strcpy(input.assetFilePaths[input.assetFileCount], droppedFiles.paths[i]);
                        input.assetFileCount++;
                    }
                }
                else // Path is a directory
                {
                    FilePathList list = LoadDirectoryFilesEx(droppedFiles.paths[i], "FILES*", true);

                    for (int i = 0; i < list.count; i++)
                    {
                        if (IsFileExtension(droppedFiles.paths[i], ".c;.h;.cpp;.hpp"))
                        {
                            // Add files to source list
                            strcpy(input.srcFilePaths[input.srcFileCount], droppedFiles.paths[i]);
                            input.srcFileCount++;
                        }
                        else if (IsFileExtension(droppedFiles.paths[i], ".png;.bmp;.jpg;.qoi;.gif;.raw;.hdr;.ktx;.dxt;.astc;.pvr;.ttf;.otf;.fnt;.wav;.ogg;.mp3;.flac;.mod;.xm;.qoa;.obj;.iqm;.glb;.gltf;.m3d;.vox;.vs;.fs;.txt"))
                        {
                            // Add assets to assets list
                            // TODO: Filtering for recognized assets extensions but, really required?
                            strcpy(input.assetFilePaths[input.assetFileCount], droppedFiles.paths[i]);
                            input.assetFileCount++;
                        }
                    }
                }
            }
        }

        UnloadDroppedFiles(droppedFiles);    // Unload filepaths from memory
    }
    //----------------------------------------------------------------------------------

    // Keyboard shortcuts
    //------------------------------------------------------------------------------------
    // New style file, previous in/out files registeres are reseted
    if ((IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_N)) || mainToolbarState.btnNewFilePressed)
    {
        rpcSetText(project, "PROJECT_INTERNAL_NAME", "cool_project");
        rpcSetText(project, "PROJECT_REPO_NAME", "cool_project");
        rpcSetText(project, "PROJECT_COMMERCIAL_NAME", "Cool Project");
        rpcSetText(project, "PROJECT_SHORT_NAME", "CoolProject");
        rpcSetText(project, "PROJECT_VERSION","1.0");
        rpcSetText(project, "PROJECT_DESCRIPTION","A very cool project");
        rpcSetText(project, "PROJECT_PUBLISHER_NAME", "raylib technologies");
        rpcSetText(project, "PROJECT_DEVELOPER_NAME", "Ramon Santamaria");
        rpcSetText(project, "PROJECT_DEVELOPER_URL", "www.raylibtech.com");
        rpcSetText(project, "PROJECT_DEVELOPER_EMAIL", "ray@raylibtech.com");
        rpcSetText(project, "PROJECT_ICON_FILE", "template/src/project_name.ico");
        //strcpy(config->Project.sourceFilePaths[0], argv[1]);
        //config->Project.srcFileCount = 1;
        rpcSetText(project, "PLATFORM_WINDOWS_W64DEVKIT_PATH", "C:\\raylib\\w64devkit\\bin");
        rpcSetText(project, "RAYLIB_SRC_PATH", "C:\\raylib\\raylib\\src");
    }

    // Show dialog: load project config file (.rpc)
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_O)) showLoadProjectDialog = true;

    // Show dialog: save project config file (.rpc)
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)) showSaveProjectDialog = true;

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
        else if (showLoadProjectDialog) showLoadProjectDialog = false;
        else if (showSaveProjectDialog) showLoadProjectDialog = false;
        else if (showProjectGenPathDialog) showProjectGenPathDialog = false;
        else if (showGenProjectProgress) showGenProjectProgress = false;
        else if (showLoadFileDialog) showLoadFileDialog = false;
        else if (showLoadDirectoryDialog) showLoadDirectoryDialog = false;
        else if (showLoadSourceFilesDialog) showLoadSourceFilesDialog = false;
        //else if (showLoadResourcePathDialog) showLoadResourcePathDialog = false;
        //else if (showLoadRaylibSourcePathDialog) showLoadRaylibSourcePathDialog = false;
        //else if (showLoadCompilerPathDialog) showLoadCompilerPathDialog = false;
#endif
    }
    //----------------------------------------------------------------------------------

    // Main toolbar logic
    //----------------------------------------------------------------------------------
    // File options logic
    if (mainToolbarState.btnLoadFilePressed) showLoadProjectDialog = true;
    else if (mainToolbarState.btnSaveFilePressed)
    {
        memset(outFileName, 0, 256);
        strcpy(outFileName, TextFormat("%s.rpc", rpcGetText(project, "PROJECT_INTERNAL_NAME")));
        showSaveProjectDialog = true;
    }
    else if (mainToolbarState.btnAddInputFilePressed) showAddInputFilesDialog = true;
    else if (mainToolbarState.btnAddInputFolderPressed) showAddInputDirectoryDialog = true;
    else if (mainToolbarState.btnClearFilesPressed)
    {
        rpcUnloadProjectInput(input);
        input = rpcLoadProjectInput();
        selectedTemplate = 0;
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
        showLoadProjectDialog ||
        showSaveProjectDialog ||
        showLoadFileDialog ||
        showLoadDirectoryDialog ||
        showProjectGenPathDialog ||
        showGenProjectProgress ||
        showLoadSourceFilesDialog) lockBackground = true;
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
    int prevProjectType = selectedTemplate;

    GuiLabel((Rectangle){ 16, 44, 200, 24 }, "CHOOSE PROJECT TEMPLATE:");
    GuiSetStyle(TOGGLE, BORDER_WIDTH, 4);
    GuiToggleGroup((Rectangle){ 16, 72, 206, 80 }, "#142#Custom;#198#Basic Window;#227#Screen Manager;#150#Platform 2D;#162#First Person 3D", &selectedTemplate);
    GuiSetStyle(TOGGLE, BORDER_WIDTH, 1);

    // Update project data required files depending on selected template
    if (selectedTemplate != prevProjectType) rpcUpdateProjectInput(&input, selectedTemplate);

    //if (GuiTextBox((Rectangle){ 24 + 180, 52 + 96 + 12 + 36 + (24 + 8)*0 + propPanelScroll.y, GetScreenWidth() - (24 + 180 + 12 + 460 + 24), 24 },
    //    rpcGetText(project, "PROJECT_INTERNAL_NAME"), 255, rpcGetPropertyEntry(project, "PROJECT_INTERNAL_NAME")->editMode))
    //    rpcGetPropertyEntry(project, "PROJECT_INTERNAL_NAME")->editMode = !rpcGetPropertyEntry(project, "PROJECT_INTERNAL_NAME")->editMode;

    // Draw project configuration fields
    for (int i = 0, k = 0; i < project.entryCount; i++)
    {
        if ((project.entries[i].category == RPC_CAT_PROJECT) && // Only project category
            (!TextIsEqual(project.entries[i].key, "PROJECT_SOURCE_PATH")) && // Project source path is generated from template
            (!TextIsEqual(project.entries[i].key, "PROJECT_ASSETS_PATH")) && // Project assets path is generated from template
            (!TextIsEqual(project.entries[i].key, "PROJECT_ASSETS_OUTPUT_PATH")))
        {
            if (project.entries[i].type != RPC_TYPE_BOOL)
                GuiLabel((Rectangle){ 24, 180 + (24 + 8)*k + propPanelScroll.y, 160, 24 }, TextFormat("%s:", project.entries[i].name));

            int descWidth = 440;
            int textWidth = 1080 - (24 + 160 + 12 + descWidth + 24);

            GuiSetStyle(TEXTBOX, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
            switch (project.entries[i].type)
            {
                case RPC_TYPE_BOOL:
                {
                    bool checked = (bool)project.entries[i].value;
                    GuiCheckBox((Rectangle){ 24 + 2, 180 + (24 + 8)*k + 2 + propPanelScroll.y, 20, 20 }, project.entries[i].name + 5, &checked);
                    project.entries[i].value = (checked? 1 : 0);
                } break;
                case RPC_TYPE_VALUE:
                {
                    if (GuiValueBox((Rectangle){ 24 + 160, 180 + (24 + 8)*k + propPanelScroll.y, 160, 24 },
                        NULL, &project.entries[i].value, 0, 1024, project.entries[i].editMode)) project.entries[i].editMode = !project.entries[i].editMode;
                } break;
                case RPC_TYPE_TEXT:
                {
                    if (GuiTextBox((Rectangle){ 24 + 160, 180 + (24 + 8)*k + propPanelScroll.y, textWidth, 24 },
                        project.entries[i].text, 255, project.entries[i].editMode)) project.entries[i].editMode = !project.entries[i].editMode;
                } break;
                case RPC_TYPE_TEXT_FILE:
                {
                    if (TextIsEqual(project.entries[i].key, "PROJECT_ICON_FILE"))
                    {
                        GuiTextBox((Rectangle){ 24 + 160, 180 + (24 + 8)*k + propPanelScroll.y, textWidth - 90, 24 },
                            TextReplace(project.entries[i].text, "project_name", rpcGetText(project, "PROJECT_INTERNAL_NAME")), 255, false);
                        if (GuiButton((Rectangle){ 24 + 160 + textWidth - 86, 180 + (24 + 8)*k + propPanelScroll.y, 86, 24 }, "#6#Browse")) showLoadIconDialog = true;
                    }
                    else
                    {
                        // Generic solution
                        // TODO: Requires file loading dialog filters and text
                        if (GuiTextBox((Rectangle){ 24 + 160, 180 + (24 + 8)*k + propPanelScroll.y, textWidth - 90, 24 },
                            project.entries[i].text, 255, project.entries[i].editMode)) project.entries[i].editMode = !project.entries[i].editMode;
                        if (GuiButton((Rectangle){ 24 + 160 + textWidth - 86, 180 + (24 + 8)*k + propPanelScroll.y, 86, 24 }, "#6#Browse"))
                        {
                            showLoadFileDialog = true;
                            projectEditProperty = i;
                        }
                    }

                } break;
                case RPC_TYPE_TEXT_PATH:
                {
                    if (GuiTextBox((Rectangle){ 24 + 160, 180 + (24 + 8)*k + propPanelScroll.y, textWidth - 90, 24 },
                        project.entries[i].text, 255, project.entries[i].editMode)) project.entries[i].editMode = !project.entries[i].editMode;
                    if (GuiButton((Rectangle){ 24 + 160 + textWidth - 86, 180 + (24 + 8)*k + propPanelScroll.y, 86, 24 }, "#173#Browse"))
                    {
                        showLoadDirectoryDialog = true;
                        projectEditProperty = i;
                    }
                } break;
                default: break;
            }
            GuiSetStyle(TEXTBOX, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);

            // Draw field description
            if (project.entries[i].type == RPC_TYPE_BOOL)
                GuiStatusBar((Rectangle){ 24 + 160, 180 + (24 + 8)*k + propPanelScroll.y, (textWidth + descWidth + 12), 24 }, project.entries[i].desc);
            else GuiStatusBar((Rectangle){ 24 + 160 + textWidth + 12, 180 + (24 + 8)*k + propPanelScroll.y, descWidth, 24 }, project.entries[i].desc);

            k++;
        }
    }

    GuiScrollPanel((Rectangle){ 12, 536, GetScreenWidth() - 24, 200 }, TextFormat("#16#PROJECT INPUT FILES: SOURCE (%i) + ASSETS (%i)", input.srcFileCount, input.assetFileCount),
        (Rectangle){ 0, 0, GetScreenWidth() - 24 - 20, 4 + (input.srcFileCount + input.assetFileCount)*(28 + 2) }, &filesPanelScroll, &filesPanelView);

    if ((input.srcFileCount == 0) && (selectedTemplate == 0))
    {
        GuiSetIconScale(2);
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
        GuiSetStyle(DEFAULT, TEXT_SIZE, GuiGetFont().baseSize*2);
        GuiLabel((Rectangle){ 12, 536, GetScreenWidth() - 24, 200 }, "#10#Drag & Drop you code files here!");
        GuiSetStyle(DEFAULT, TEXT_SIZE, GuiGetFont().baseSize);
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
        GuiSetIconScale(1);
    }

    BeginScissorMode(filesPanelView.x, filesPanelView.y, filesPanelView.width, filesPanelView.height);
        for (int i = 0; i < input.srcFileCount; i++)
        {
            /*
            DrawRectangleLinesEx((Rectangle){ 16, 536 + 26 + (28 + 2)*i + filesPanelScroll.y, GetScreenWidth() - 24 - 24, 28 }, 
                1.0f, GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL)));
            GuiSetStyle(LABEL, TEXT_PADDING, 8);
            GuiLabel((Rectangle){ 16, 536 + 26 + (28 + 2)*i + filesPanelScroll.y, GetScreenWidth() - 24 - 24, 28 }, 
                TextFormat("#10#%s", TextReplace(input.srcFilePaths[i], "project_name", rpcGetText(project, "PROJECT_INTERNAL_NAME"))));
            GuiSetStyle(LABEL, TEXT_PADDING, 0);
            */
            
            GuiSetStyle(TOGGLE, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
            GuiSetStyle(TOGGLE, TEXT_PADDING, 8);
            GuiToggle((Rectangle){ 16, 536 + 26 + (28 + 2)*i + filesPanelScroll.y, GetScreenWidth() - 24 - 24, 28 }, 
                TextFormat("#10#%s", TextReplace(input.srcFilePaths[i], "project_name", rpcGetText(project, "PROJECT_INTERNAL_NAME"))), NULL);
            GuiSetStyle(TOGGLE, TEXT_PADDING, 0);
            GuiSetStyle(TOGGLE, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
        }
        for (int i = 0; i < input.assetFileCount; i++)
        {
            /*
            DrawRectangleLinesEx((Rectangle){ 16, 536 + 26 + (28 + 2)*i + filesPanelScroll.y, GetScreenWidth() - 24 - 24, 28 }, 
            1.0f, GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL)));
            GuiSetStyle(LABEL, TEXT_PADDING, 8);
            GuiLabel((Rectangle){ 16, 536 + 26 + (28 + 2)*i + filesPanelScroll.y, GetScreenWidth() - 24 - 24, 28 }, 
                TextFormat("#200#%s", input.assetFilePaths[i]))));
            GuiSetStyle(LABEL, TEXT_PADDING, 0);
            */

            GuiSetStyle(TOGGLE, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
            GuiSetStyle(TOGGLE, TEXT_PADDING, 8);
            GuiToggle((Rectangle){ 16, 536 + 26 + (28 + 2)*(i + input.srcFileCount) + filesPanelScroll.y, GetScreenWidth() - 24 - 24, 28 }, 
                TextFormat("#200#%s", input.assetFilePaths[i]), NULL);
            GuiSetStyle(TOGGLE, TEXT_PADDING, 0);
            GuiSetStyle(TOGGLE, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);

        }
    EndScissorMode();

#if defined(PLATFORM_WEB)
    GuiDisable();
#endif
    if (input.srcFileCount == 0) GuiDisable();
    if (GuiButton((Rectangle){ 8, GetScreenHeight() - 24 - 8 - 40, GetScreenWidth() - 16, 40 }, "#13#GENERATE PROJECT STRUCTURE")) showProjectGenPathDialog = true;
    GuiEnable();

    if (!lockBackground && CheckCollisionPointRec(GetMousePosition(), (Rectangle){ 0, GetScreenHeight() - 64, screenWidth, 32 })) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    else SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    //----------------------------------------------------------------------------------

    // GUI: Main toolbar panel
    //----------------------------------------------------------------------------------
    GuiMainToolbar(&mainToolbarState);
    //----------------------------------------------------------------------------------

    // GUI: Status bar
    //----------------------------------------------------------------------------------
    int textPadding = GuiGetStyle(STATUSBAR, TEXT_PADDING);
    GuiSetStyle(STATUSBAR, TEXT_PADDING, 12);
    GuiStatusBar((Rectangle){ 0, screenHeight - 24, screenWidth, 24 }, 
        TextFormat("SOURCE FILES: %i  | ASSET FILES: %i", input.srcFileCount, input.assetFileCount));
    GuiSetStyle(STATUSBAR, TEXT_PADDING, textPadding);
    //----------------------------------------------------------------------------------

    // NOTE: If some overlap window is open and main window is locked, draw a background rectangle
    if (lockBackground) DrawRectangle(0, 0, screenWidth, screenHeight, Fade(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)), 0.85f));

    // WARNING: Before drawing the windows, unlock them
    GuiUnlock();

    // GUI: Show info message panel
    //----------------------------------------------------------------------------------------
    if (showInfoMessagePanel)
    {
        Vector2 textSize = MeasureTextEx(GuiGetFont(), infoMessage, (float)GuiGetFont().baseSize*2, 3);
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
        int result = GuiMessageBox(messageBox, "#7#Export File", " ", "#7#Export File");

        // TODO: Fill export window data if required

        if (result == 1)    // Export button pressed
        {
            windowExportActive = false;
            showProjectGenPathDialog = true;
        }
        else if (result == 0) windowExportActive = false;
    }
    //----------------------------------------------------------------------------------

    // GUI: Load File Dialog (and loading logic)
    //----------------------------------------------------------------------------------------
    if (showLoadProjectDialog)
    {
#if defined(CUSTOM_MODAL_DIALOGS)
        int result = GuiFileDialog(DIALOG_MESSAGE, "Load project config file", inFileName, "Ok", "Just drag and drop your .rpc file!");
#else
        int result = GuiFileDialog(DIALOG_OPEN_FILE, "Load project config file...", inFileName, "*.rpc", "raylib project config files (.rpc)");
#endif
        if (result == 1)
        {
            // WARNING: Maybe not all data required is provided by custom .rpc project
            // TODO: Load project default data from template and sync with loaded project data
            //rpcProjectConfig template = rpcLoadProjectConfig("template/project_name.rpc");

            rpcProjectConfig temp = rpcLoadProjectConfig(inFileName);

            if ((temp.entryCount > 0) && (temp.entries != NULL))
            {
                rpcUnloadProjectConfig(project);
                project = temp;

                SetWindowTitle(TextFormat("%s v%s - %s", toolName, toolVersion, GetFileName(inFileName)));
            }
        }

        if (result >= 0) showLoadProjectDialog = false;
    }
    //----------------------------------------------------------------------------------------

    // GUI: Save File Dialog (and saving logic)
    //----------------------------------------------------------------------------------------
    if (showSaveProjectDialog)
    {
#if defined(CUSTOM_MODAL_DIALOGS)
        int result = GuiFileDialog(DIALOG_TEXTINPUT, "Save project config file...", outFileName, "Ok;Cancel", NULL);
#else
        int result = GuiFileDialog(DIALOG_SAVE_FILE, "Save projeect config file...", outFileName, "*.rpc", "raylib project config files (*.rpc)");
#endif
        if (result == 1)
        {
            // Save file: outFileName
            // Check for valid extension and make sure it is
            if ((GetFileExtension(outFileName) == NULL) || !IsFileExtension(outFileName, ".rpc")) strcat(outFileName, ".rpc\0");

            //rpcSyncProjectConfigTyped(project, project); // TODO: Currently UI modifies [project], review if changed to project
            rpcSaveProjectConfig(project, outFileName, 0);

#if defined(PLATFORM_WEB)
            // Download file from MEMFS (emscripten memory filesystem)
            // NOTE: Second argument must be a simple filename (can't use directories)
            // NOTE: Included security check to (partially) avoid malicious code on PLATFORM_WEB
            if (strchr(outFileName, '\'') == NULL) emscripten_run_script(TextFormat("saveFileFromMEMFSToDisk('%s','%s')", outFileName, GetFileName(outFileName)));
#endif
        }

        if (result >= 0) showSaveProjectDialog = false;
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

    // GUI: Add Input Files Dialog
    //----------------------------------------------------------------------------------------
    if (showAddInputFilesDialog)
    {
        // Multiple files selected are supported, tinyfiledialogs returns them as: path01|path02|path03|path04
        // We must reserve enough memory to deal with the maximum amount of data that tinyfiledialogs can provide
        // tinifiledialogs limits defined:
        //#define MAX_MULTIPLE_FILES 1024
        //#define MAX_PATH_OR_CMD 1024
#if defined(PLATFORM_WEB)
        char *multiFileNames = NULL;
#else
        char *multiFileNames = (char *)RL_CALLOC(1024*256, 1); // Let's reserve for 1024 paths, 256 bytes each
#endif
#if defined(CUSTOM_MODAL_DIALOGS)
        int result = GuiFileDialog(DIALOG_MESSAGE, "Load input file(s)...", inFileName, "Ok", "Just drag and drop your file(s)!");
#else
        int result = GuiFileDialog(DIALOG_OPEN_FILE_MULTI, "Load input file(s)...", multiFileNames, "", "Code/Asset Files");
#endif
        if (result == 1)
        {
            int multiFileCount = 0;
            const char **multiFileList = GetSubtextPtrs(multiFileNames, '|', &multiFileCount); // Split text into multiple strings

            // Temp project input paths for scanned assets
            // WARNING: Storing in srcFilePaths the source path for every asset added,
            // so the asset can be related to equivalent source path
            rpcProjectInput temp = rpcLoadProjectInput();

            for (int i = 0; i < multiFileCount; i++)
            {
                if (IsFileExtension(multiFileList[i], ".c;.h;.cpp;.hpp"))
                {
                    // Scan code file looking for assets
                    int assetCount = 0;
                    char **assetPaths = LoadSourceAssetPaths(multiFileList[i], &assetCount);

                    for (int a = 0; a < assetCount; a++)
                    {
                        // NOTE: Not verifying at the moment if asset exist, just adding it to assets list
                        // const char *fullPath = TextFormat("%s/%s", GetDirectoryPath(multiFileList[i]), assetPaths[a]);
                        //if (FileExists(TextFormat(fullPath))) { }

                        strcpy(temp.srcFilePaths[temp.srcFileCount], multiFileList[i]);
                        strcpy(temp.assetFilePaths[temp.assetFileCount], assetPaths[a]);
                        temp.srcFileCount++;
                        temp.assetFileCount++;
                    }

                    // Add files to source list
                    strcpy(input.srcFilePaths[input.srcFileCount], multiFileList[i]);
                    input.srcFileCount++;

                    if (input.srcFileCount >= RPC_MAX_SOURCE_FILES) break;
                }
                else if (IsFileExtension(multiFileList[i], ".png;.bmp;.jpg;.qoi;.gif;.raw;.hdr;.ktx;.dxt;.astc;.pvr;.ttf;.otf;.fnt;.wav;.ogg;.mp3;.flac;.mod;.xm;.qoa;.obj;.iqm;.glb;.gltf;.m3d;.vox;.vs;.fs;.txt"))
                {
                    // Add assets to assets list
                    // TODO: Filtering for recognized assets extensions but, really required?
                    strcpy(input.assetFilePaths[input.assetFileCount], multiFileList[i]);
                    input.assetFileCount++;

                    if (input.assetFileCount >= RPC_MAX_ASSET_FILES) break;
                }
            }

            // TODO: What should be done with temp assets scanned?
            // It should probably be validated that they exist and there are no dups on the list

            rpcUnloadProjectInput(temp);
        }

        if (result >= 0) showAddInputFilesDialog = false;
    }
    //----------------------------------------------------------------------------------------

    // GUI: Add Input Directory Dialog
    //----------------------------------------------------------------------------------------
    if (showAddInputDirectoryDialog)
    {
#if defined(CUSTOM_MODAL_DIALOGS)
        int result = GuiFileDialog(DIALOG_MESSAGE, "Load path...", inDirectoryPath, "Ok", "Drag and drop your files");
#else
        int result = GuiFileDialog(DIALOG_OPEN_DIRECTORY, "Load path...", inDirectoryPath, "", "");
#endif
        if (result == 1)
        {
            if (DirectoryExists(inDirectoryPath))
            {
                FilePathList pathList = LoadDirectoryFilesEx(inDirectoryPath, "FILES*", true);

                // Temp project input paths for scanned assets
                // WARNING: Storing in srcFilePaths the source path for every asset added,
                // so the asset can be related to equivalent source path
                rpcProjectInput temp = rpcLoadProjectInput();

                for (unsigned int i = 0; i < pathList.count; i++)
                {
                    if (IsFileExtension(pathList.paths[i], ".c;.h;.cpp;.hpp"))
                    {
                        // Scan code file looking for assets
                        int assetCount = 0;
                        char **assetPaths = LoadSourceAssetPaths(pathList.paths[i], &assetCount);

                        for (int a = 0; a < assetCount; a++)
                        {
                            // NOTE: Not verifying at the moment if asset exist, just adding it to assets list
                            // const char *fullPath = TextFormat("%s/%s", GetDirectoryPath(pathList.paths[i]), assetPaths[a]);
                            //if (FileExists(TextFormat(fullPath))) { }

                            strcpy(temp.srcFilePaths[temp.srcFileCount], pathList.paths[i]);
                            strcpy(temp.assetFilePaths[temp.assetFileCount], assetPaths[a]);
                            temp.srcFileCount++;
                            temp.assetFileCount++;
                        }

                        // Add files to source list
                        strcpy(input.srcFilePaths[input.srcFileCount], pathList.paths[i]);
                        input.srcFileCount++;

                        if (input.srcFileCount >= RPC_MAX_SOURCE_FILES) break;
                    }
                    else if (IsFileExtension(pathList.paths[i], ".png;.bmp;.jpg;.qoi;.gif;.raw;.hdr;.ktx;.dxt;.astc;.pvr;.ttf;.otf;.fnt;.wav;.ogg;.mp3;.flac;.mod;.xm;.qoa;.obj;.iqm;.glb;.gltf;.m3d;.vox;.vs;.fs;.txt"))
                    {
                        // Add assets to assets list
                        // TODO: Filtering for recognized assets extensions but, really required?
                        strcpy(input.assetFilePaths[input.assetFileCount], pathList.paths[i]);
                        input.assetFileCount++;

                        if (input.assetFileCount >= RPC_MAX_ASSET_FILES) break;
                    }
                }

                // TODO: What should be done with temp assets scanned?
                // It should probably be validated that they exist and there are no dups on the list

                rpcUnloadProjectInput(temp);

                UnloadDirectoryFiles(pathList);
            }
            else
            {
                // TODO: Show right message depending on projectEditProperty
                infoMessage = "Provided resource path does not exist!";
                showInfoMessagePanel = true;
            }
        }

        if (result >= 0) showAddInputDirectoryDialog = false;
    }
    //----------------------------------------------------------------------------------------


    // GUI: Load File Dialog (and loading logic) - GENERIC
    //----------------------------------------------------------------------------------------
    if (showLoadFileDialog && !showLoadDirectoryDialog)
    {
#if defined(CUSTOM_MODAL_DIALOGS)
        int result = GuiFileDialog(DIALOG_MESSAGE, "Load file...", inFileName, "Ok", "Just drag and drop your .rpc file!");
#else
        int result = GuiFileDialog(DIALOG_OPEN_FILE, "Load file...", inFileName, "", "File Type (*.rpc)");
#endif
        if (result == 1)
        {
            if (FileExists(inFileName))
            {
                // Update required property with selected path
                memset(project.entries[projectEditProperty].text, 0, 256);
                strcpy(project.entries[projectEditProperty].text, inFileName);
            }
            else
            {
                // TODO: Show right message depending on projectEditProperty
                infoMessage = "Provided resource path does not exist!";
                showInfoMessagePanel = true;
            }
        }

        if (result >= 0) showLoadFileDialog = false;
    }
    //----------------------------------------------------------------------------------------

    // GUI: Load Directory Dialog (and loading logic) - GENERIC
    //----------------------------------------------------------------------------------------
    if (showLoadDirectoryDialog && !showLoadFileDialog)
    {
#if defined(CUSTOM_MODAL_DIALOGS)
        int result = GuiFileDialog(DIALOG_MESSAGE, "Load path...", inDirectoryPath, "Ok", "Drag and drop your files");
#else
        int result = GuiFileDialog(DIALOG_OPEN_DIRECTORY, "Load path...", inDirectoryPath, "", "");
#endif
        if (result == 1)
        {
            if (DirectoryExists(inDirectoryPath))
            {
                // Update required property with selected path
                memset(project.entries[projectEditProperty].text, 0, 256);
                strcpy(project.entries[projectEditProperty].text, inDirectoryPath);
            }
            else
            {
                // TODO: Show right message depending on projectEditProperty
                infoMessage = "Provided resource path does not exist!";
                showInfoMessagePanel = true;
            }
        }

        if (result >= 0) showLoadDirectoryDialog = false;
    }
    //----------------------------------------------------------------------------------------

    // GUI: Load Icon Dialog (and loading logic)
    //----------------------------------------------------------------------------------------
    if (showLoadIconDialog)
    {
#if defined(CUSTOM_MODAL_DIALOGS)
        int result = GuiFileDialog(DIALOG_MESSAGE, "Load icon file...", inFileName, "Ok", "Just drag and drop your .ico file!");
#else
        int result = GuiFileDialog(DIALOG_OPEN_FILE, "Load icon file...", inFileName, "*.png;*.ico", "File Type (*.ico)");
#endif
        if (result == 1)
        {
            if (FileExists(inFileName)) rpcSetText(project, "PROJECT_ICON_FILE", inFileName);
        }

        if (result >= 0) showLoadIconDialog = false;
    }
    //----------------------------------------------------------------------------------------

    // GUI: Load Project Generation Path Dialog
    //----------------------------------------------------------------------------------------
    if (showProjectGenPathDialog)
    {
#if defined(CUSTOM_MODAL_DIALOGS)
        int result = GuiFileDialog(DIALOG_MESSAGE, "Select generation output directory...", outProjectPath, "Ok", "Edit the path in text box");
#else
        int result = GuiFileDialog(DIALOG_OPEN_DIRECTORY, "Select generation output directory...", outProjectPath, NULL, NULL);
#endif
        if (result == 1)
        {
            strcpy(generationOutPath, outProjectPath);
            GenerateProject(project, input, generationOutPath);
            showGenProjectProgress = true;
        }

        if (result >= 0) showProjectGenPathDialog = false;
    }
    //----------------------------------------------------------------------------------------

    // GUI: Export Project Dialog (and saving logic)
    //----------------------------------------------------------------------------------------
    if (showGenProjectProgress)
    {
        GuiPanel((Rectangle){ -10, screenHeight/2 - 100, screenWidth + 20, 200 }, NULL);

        int textSpacing = GuiGetStyle(DEFAULT, TEXT_SPACING);
        GuiSetStyle(DEFAULT, TEXT_SIZE, GuiGetFont().baseSize*3);
        GuiSetStyle(DEFAULT, TEXT_SPACING, 3);
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
        GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, GuiGetStyle(DEFAULT, TEXT_COLOR_FOCUSED));
        GuiLabel((Rectangle){ -10, screenHeight/2 - 60, screenWidth + 20, 30 }, ((int)generateProjectProgress >= 100)? "PROJECT GENERATED SUCCESSFULLY" : "GENERATING PROJECT...");
        GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL));
        GuiSetStyle(DEFAULT, TEXT_SIZE, GuiGetFont().baseSize*2);

        generateProjectProgress += 2.0f;
        GuiProgressBar((Rectangle){ 12, screenHeight/2, screenWidth - 24, 20 }, NULL, NULL, &generateProjectProgress, 0, 100);

        if (generateProjectProgress < 100.0f) GuiDisable();
        if (GuiButton((Rectangle){ screenWidth/4, screenHeight/2 + 40, screenWidth/2, 40 }, "GREAT!")) showGenProjectProgress = false;
        GuiEnable();

        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
        GuiSetStyle(DEFAULT, TEXT_SIZE, GuiGetFont().baseSize);
        GuiSetStyle(DEFAULT, TEXT_SPACING, textSpacing);

        if (!showGenProjectProgress)
        {
#if defined(PLATFORM_WEB)
            strcpy(outFileName, TextFormat("%s/%s", generationOutPath, TextToLower(rpcGetText(project, "PROJECT_REPO_NAME"))));

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
            // NOTE: Second argument must be a simple filename (can't use directories)
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
        if (screenSizeDouble) DrawTexturePro(screenTarget.texture,
            (Rectangle){ 0, 0, (float)screenTarget.texture.width, -(float)screenTarget.texture.height },
            (Rectangle){ 0, 0, (float)screenTarget.texture.width*2, (float)screenTarget.texture.height*2 },
            (Vector2){ 0, 0 }, 0.0f, WHITE);
        else DrawTextureRec(screenTarget.texture,
            (Rectangle){ 0, 0, (float)screenTarget.texture.width, -(float)screenTarget.texture.height },
            (Vector2){ 0, 0 }, WHITE);
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
    printf("// Copyright (c) 2024-%i Ramon Santamaria (@raysan5)                        //\n", currentYear);
    printf("//                                                                              //\n");
    printf("//////////////////////////////////////////////////////////////////////////////////\n\n");

    printf("USAGE:\n\n");
    printf("    > rpc [--help] --pn <project_name> --src <source_file01.c>,<source_file02.c>\n");
    printf("             [--rn <repo-name>] [--cn <commercial_name>] [--pv <version>]\n");
    printf("             [--desc <project_description>] [--dev <developer_name>]\n");
    printf("             [--devurl <developer_webpage>] [--devmail <developer_email>]\n");
    printf("             [--raylib <raylib_src_path>] [--comp <compiler_path>]\n");
    printf("             [--out <output_path>]\n");

    printf("\nOPTIONS:\n\n");
    printf("    -h, --help                          : Show tool version and command line usage help\n\n");
    printf("    -i, --src <source_file01.c>,<source_file02.c>\n");
    printf("                                        : Define input source files(s), comma separated\n");
    printf("    -rpc <config_file.rpc>              : Define raylib project configuration file\n");

    printf("    -pn, --project-name <project_name>  : Define project internal name\n");
    printf("    -rn, --repo-name <repository_name>  : Define project repository name\n");
    printf("    -cn, --commercial-name <commercial_name>  : Define project commercial name\n");
    printf("    -pv, --project-version <version>    : Define project version\n");
    printf("    --desc <project_description>        : Define project description, use \"Project Description\"\n");
    printf("    --dev <developer_name>              : Define developer name\n");
    printf("    --devurl <developer_webpage>        : Define developer webpage\n");
    printf("    --devmail <developer_email>         : Define developer email\n");
    printf("    --raylib <raylib_src_path>          : Define raylib src path (raylib.h)\n");
    printf("    --comp <compiler_path>              : Define compiler path (ggc.exe)\n");
    printf("    -o, --out <output_path>             : Define output path for project generation\n");

    printf("\nEXAMPLES:\n\n");
    printf("    > rpc -pn cool_game -rn cool-game-repo -cn \"Cool Game\" -pv 1.0\n");
    printf("        Generates project <cool_game> in output directory <cool-game-repo>\n");
}

// Process command line input
// TODO: REVIEW: Global used: input
static void ProcessCommandLine(int argc, char *argv[])
{
    // CLI required variables
    bool showUsageInfo = false;     // Toggle command line usage info

    if (argc == 1) showUsageInfo = true;

    // Load default project config file
    // NOTE 1: If a different .rpc is provided, defined properties will be overriden
    // NOTE 2: Properties defined on command-line will also override default ones
    rpcProjectConfig config = rpcLoadProjectConfig("template/project_name.rpc");
    rpcProjectInput input = rpcLoadProjectInput();

    // Process command line arguments
    for (int i = 1; i < argc; i++)
    {
        if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0))
        {
            showUsageInfo = true;
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
                        strcpy(input.srcFilePaths[input.srcFileCount], files[i]);
                        input.srcFileCount++;
                    }
                    else LOG("WARNING: [%s] File not recognized as source file (Use: .c,.h)\n", files[i]);
                }
            }
            else LOG("WARNING: No input file provided\n");
        }
        else if (strcmp(argv[i], "-rpc") == 0)
        {
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                if (FileExists(argv[i + 1]) && IsFileExtension(argv[i + 1], ".rpc"))
                {
                    rpcProjectConfig tempConfig = rpcLoadProjectConfig(argv[i + 1]);

                    // TODO: WARNING: Provided .rpc could not have all required fields defined,
                    // so it's better to sync available ones with already loaded config from template
                    //rpcSyncProjectConfigTyped(config, raw);

                    rpcUnloadProjectConfig(tempConfig);
                }
            }
            else LOG("WARNING: No .rpc config file provided or not valid\n");
        }
        else if ((strcmp(argv[i], "-pn") == 0) || (strcmp(argv[i], "--project-name") == 0))
        {
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                rpcSetText(config, "PROJECT_INTERNAL_NAME", argv[i + 1]);
            }
            else LOG("WARNING: Project internal name provided not valid\n");
        }
        else if ((strcmp(argv[i], "-rn") == 0) || (strcmp(argv[i], "--repo-name") == 0))
        {
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                rpcSetText(config, "PROJECT_REPO_NAME", argv[i + 1]);
            }
            else LOG("WARNING: Project repo name provided not valid\n");
        }
        else if ((strcmp(argv[i], "-cn") == 0) || (strcmp(argv[i], "--commercial-name") == 0))
        {
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                rpcSetText(config, "PROJECT_COMMERCIAL_NAME", argv[i + 1]);
                //rpcSetText(project, "PROJECT_SHORT_NAME", "CoolProject");
            }
            else LOG("WARNING: Project commercial name provided not valid\n");
        }
        else if ((strcmp(argv[i], "-pv") == 0) || (strcmp(argv[i], "--project-version") == 0))
        {
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                rpcSetText(config, "PROJECT_VERSION", argv[i + 1]);
            }
            else LOG("WARNING: Project version provided not valid\n");
        }
        else if (strcmp(argv[i], "--desc") == 0)
        {
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                rpcSetText(config, "PROJECT_DESCRIPTION", argv[i + 1]);
            }
            else LOG("WARNING: Project description provided not valid\n");
        }
        else if (strcmp(argv[i], "--dev") == 0)
        {
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                //rpcSetText(project, "PROJECT_PUBLISHER_NAME", "raylib technologies");
                rpcSetText(config, "PROJECT_DEVELOPER_NAME", argv[i + 1]);
            }
            else LOG("WARNING: Developer name provided not valid\n");
        }
        else if (strcmp(argv[i], "--devurl") == 0)
        {
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                rpcSetText(config, "PROJECT_DEVELOPER_URL", argv[i + 1]);
            }
            else LOG("WARNING: Developer url provided not valid\n");
        }
        else if (strcmp(argv[i], "--devmail") == 0)
        {
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                rpcSetText(config, "PROJECT_DEVELOPER_EMAIL", argv[i + 1]);
            }
            else LOG("WARNING: Developer email provided not valid\n");
        }
        else if (strcmp(argv[i], "--raylib") == 0)
        {
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                if (FileExists(TextFormat("%s/raylib.h", argv[i + 1]))) rpcSetText(config, "RAYLIB_SRC_PATH", argv[i + 1]);
                else LOG("WARNING: raylib source path provided does not contain raylib.h\n");
            }
            else LOG("WARNING: raylib source path parameters provided not valid\n");
        }
        else if (strcmp(argv[i], "--comp") == 0)
        {
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                if (FileExists(TextFormat("%s/gcc.exe", argv[i + 1]))) rpcSetText(config, "PLATFORM_WINDOWS_W64DEVKIT_PATH", argv[i + 1]);
                else LOG("WARNING: Compiler path provided does not contain gcc.exe\n");
            }
            else LOG("WARNING: Compiler path parameters provided not valid\n");
        }
        else if ((strcmp(argv[i], "-o") == 0) || (strcmp(argv[i], "--out") == 0))
        {
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                // TODO: WARNING: Using a global variable for output path
                // Default output path is working directory if not other provided
                strcpy(generationOutPath, argv[i + 1]);
            }
            else LOG("WARNING: Output path provided not valid\n");
        }
    }

    // Generate build projects
    GenerateProject(config, input, generationOutPath);

    rpcUnloadProjectConfig(config);
    rpcUnloadProjectInput(input);

    if (showUsageInfo) ShowCommandLineInfo();
}
#endif // PLATFORM_DESKTOP

//--------------------------------------------------------------------------------------------
// Load/Save/Export functions
//--------------------------------------------------------------------------------------------
//...

//--------------------------------------------------------------------------------------------
// Auxiliar functions
//--------------------------------------------------------------------------------------------

// Load project source/asset file paths bucket
static rpcProjectInput rpcLoadProjectInput(void)
{
    rpcProjectInput input = { 0 };

    input.srcFilePaths = (char **)RL_CALLOC(RPC_MAX_SOURCE_FILES, sizeof(char *));
    input.assetFilePaths = (char **)RL_CALLOC(RPC_MAX_ASSET_FILES, sizeof(char *));

    for (int i = 0; i < RPC_MAX_SOURCE_FILES; i++) input.srcFilePaths[i] = (char *)RL_CALLOC(RPC_SOURCE_PATH_LENGTH, sizeof(char));
    for (int i = 0; i < RPC_MAX_ASSET_FILES; i++) input.assetFilePaths[i] = (char *)RL_CALLOC(RPC_ASSET_PATH_LENGTH, sizeof(char));

    input.requestedBuildSystems[0] = true;  // Scripts
    input.requestedBuildSystems[1] = true;  // Makefile
    input.requestedBuildSystems[2] = true;  // VSCode
    input.requestedBuildSystems[3] = true;  // VS2022
    input.requestedBuildSystems[4] = false;  // CMake
    input.requestedBuildSystems[5] = true;  // GitHub Actions

    return input;
}

// Unload project source/asset file paths bucket
static void rpcUnloadProjectInput(rpcProjectInput input)
{
    for (int i = 0; i < RPC_MAX_SOURCE_FILES; i++) RL_FREE(input.srcFilePaths[i]);
    for (int i = 0; i < RPC_MAX_ASSET_FILES; i++) RL_FREE(input.assetFilePaths[i]);

    RL_FREE(input.srcFilePaths);
    RL_FREE(input.assetFilePaths);
}

// Update input data by selected template
void rpcUpdateProjectInput(rpcProjectInput *input, int selTemplate)
{
    // Clear input data content to refill
    for (int i = 0; i < RPC_MAX_SOURCE_FILES; i++) memset(input->srcFilePaths[i], 0, RPC_SOURCE_PATH_LENGTH);
    for (int i = 0; i < RPC_MAX_ASSET_FILES; i++) memset(input->assetFilePaths[i], 0, RPC_ASSET_PATH_LENGTH);
    input->srcFileCount = 0;
    input->assetFileCount = 0;

    char templatePath[256] = { 0 };
    // NOTE: [template] directory must be in same directory as [rpc] tool
    //strcpy(templatePath, TextFormat("%s/template", GetApplicationDirectory()));
    // WARNING: Instead of copying full template path, assume that the file can be 
    // located in [template] package (next to binary or internal), so no need to specify
    // a full path; full paths are only used for user-provided files
    strcpy(templatePath, "template");

    if (selTemplate == 0)       // Custom
    {
        // NOTE: Filled with user provided source files
        // TODO: Scan required assets and add to the list
    }
    else if (selTemplate == 1)  // Basic Window
    {
        strcpy(input->srcFilePaths[0], TextFormat("%s/src/project_name.c", templatePath));
        input->srcFileCount = 1;

        // No assets required
    }
    else if (selTemplate == 2)  // Screen Manager
    {
        strcpy(input->srcFilePaths[0], TextFormat("%s/src_screen_manager/project_name.c", templatePath));
        strcpy(input->srcFilePaths[1], TextFormat("%s/src_screen_manager/screens.h", templatePath));
        strcpy(input->srcFilePaths[2], TextFormat("%s/src_screen_manager/screen_logo.c", templatePath));
        strcpy(input->srcFilePaths[3], TextFormat("%s/src_screen_manager/screen_title.c", templatePath));
        strcpy(input->srcFilePaths[4], TextFormat("%s/src_screen_manager/screen_options.c", templatePath));
        strcpy(input->srcFilePaths[5], TextFormat("%s/src_screen_manager/screen_gameplay.c", templatePath));
        strcpy(input->srcFilePaths[6], TextFormat("%s/src_screen_manager/screen_ending.c", templatePath));
        input->srcFileCount = 7;

        strcpy(input->assetFilePaths[0], TextFormat("%s/src_screen_manager/resources/coin.wav", templatePath));
        strcpy(input->assetFilePaths[1], TextFormat("%s/src_screen_manager/resources/mecha.png", templatePath));
        strcpy(input->assetFilePaths[2], TextFormat("%s/src_screen_manager/resources/README", templatePath)); // Assets licence
        input->assetFileCount = 3;

        /*
        FilePathList pathList = LoadDirectoryFiles(TextFormat("%s/src_screen_manager/resources", templatePath));
        for (int i = 0; i < pathList.count; i++)
        {
            strcpy(input->assetFilePaths[i], pathList.paths[i]);
            input->assetFileCount++;
        }
        UnloadDirectoryFiles(pathList);
        */
    }
    else if (selTemplate == 3)  // Platform 2D
    {
        strcpy(input->srcFilePaths[0], TextFormat("%s/src_platformer_2d/project_name.c", templatePath));
        input->srcFileCount = 1;

        // No assets required
    }
    else if (selTemplate == 4)  // First Person 3D
    {
        strcpy(input->srcFilePaths[0], TextFormat("%s/src_first_person_3d/project_name.c", templatePath));
        input->srcFileCount = 1;

        strcpy(input->assetFilePaths[0], TextFormat("%s/src_first_person_3d/resources/cubicmap.png", templatePath));
        strcpy(input->assetFilePaths[1], TextFormat("%s/src_first_person_3d/resources/cubicmap_atlas.png", templatePath));
        input->assetFileCount = 2;
    }
    else if (selTemplate == 5)  // raygui Tool
    {
        strcpy(input->srcFilePaths[0], TextFormat("%s/src_raygui_tool/project_name.c", templatePath));
        strcpy(input->srcFilePaths[1], TextFormat("%s/src_raygui_tool/raygui.h", templatePath));
        strcpy(input->srcFilePaths[2], TextFormat("%s/src_raygui_tool/style_cyber.h", templatePath));
        strcpy(input->srcFilePaths[3], TextFormat("%s/src_raygui_tool/style_lavanda.h", templatePath));
        strcpy(input->srcFilePaths[4], TextFormat("%s/src_raygui_tool/style_genesis.h", templatePath));
        strcpy(input->srcFilePaths[5], TextFormat("%s/src_raygui_tool/style_terminal.h", templatePath));
        input->srcFileCount = 6;

        // No assets required
    }
}

// Scan asset paths from a source code file (raylib)
// WARNING: Supported asset file extensions are hardcoded by used file types
// but new examples could require other file extensions to be added
static char **LoadSourceAssetPaths(const char *srcFilePath, int *assetCount)
{
#define RPC_MAX_ASSET_FILES     256
#define RPC_ASSET_PATH_LENGTH   256

    char **paths = (char **)RL_CALLOC(RPC_MAX_ASSET_FILES, sizeof(char **));
    for (int i = 0; i < RPC_MAX_ASSET_FILES; i++) paths[i] = (char *)RL_CALLOC(RPC_ASSET_PATH_LENGTH, sizeof(char));

    int assetCounter = 0;
    char *code = LoadFileText(srcFilePath);

    if (code != NULL)
    {
        // Resources extensions to check
        const char *exts[] = { ".png", ".bmp", ".jpg", ".qoi", ".gif", ".raw", ".hdr", ".ttf", ".fnt", ".wav", ".ogg", ".mp3", ".flac", ".mod", ".xm", ".qoa", ".obj", ".iqm", ".glb", ".m3d", ".vox", ".vs", ".fs", ".txt" };
        const int extCount = sizeof(exts)/sizeof(char *);

        char *ptr = code;
        while ((ptr = strchr(ptr, '"')) != NULL)
        {
            char *start = ptr + 1;
            char *end = strchr(start, '"');
            if (!end) break;

            // WARNING: Some paths could be for saving files, not loading, those "resource" files must be omitted
            // TODO: HACK: Just check previous position from pointer for function name including the string and the index "distance"
            // This is a quick solution, the good one would be getting the data loading function names...
            int functionIndex01 = TextFindIndex(ptr - 40, "ExportImage");       // Check ExportImage()
            int functionIndex02 = TextFindIndex(ptr - 10, "TraceLog");          // Check TraceLog()
            int functionIndex03 = TextFindIndex(ptr - 40, "TakeScreenshot");    // Check TakeScreenshot()
            int functionIndex04 = TextFindIndex(ptr - 40, "SaveFileData");      // Check SaveFileData()
            int functionIndex05 = TextFindIndex(ptr - 40, "SaveFileText");      // Check SaveFileText()

            if (!((functionIndex01 != -1) && (functionIndex01 < 40)) &&  // Not found ExportImage() before ""
                !((functionIndex02 != -1) && (functionIndex02 < 10)) &&  // Not found TraceLog() before ""
                !((functionIndex03 != -1) && (functionIndex03 < 40)) &&  // Not found TakeScreenshot() before ""
                !((functionIndex04 != -1) && (functionIndex04 < 40)) &&  // Not found TakeScreenshot() before ""
                !((functionIndex05 != -1) && (functionIndex05 < 40)))    // Not found SaveFileText() before ""
            {
                int len = (int)(end - start);
                if ((len > 0) && (len < RPC_ASSET_PATH_LENGTH))
                {
                    char buffer[RPC_ASSET_PATH_LENGTH] = { 0 };
                    strncpy(buffer, start, len);
                    buffer[len] = '\0';

                    // Check for known extensions
                    for (int i = 0; i < extCount; i++)
                    {
                        // NOTE: IsFileExtension() expects a NULL terminated fileName string,
                        // it looks for the last '.' and checks "extension" after that
                        if (IsFileExtension(buffer, exts[i]))
                        {
                            // Avoid duplicates
                            bool found = false;
                            for (int j = 0; j < assetCounter; j++)
                            {
                                if (strcmp(paths[j], buffer) == 0) { found = true; break; }
                            }

                            if (!found && (assetCounter < RPC_MAX_ASSET_FILES))
                            {
                                strcpy(paths[assetCounter], buffer);
                                assetCounter++;
                            }

                            break;
                        }
                    }
                }
            }

            ptr = end + 1;
        }

        UnloadFileText(code);
    }

    /*
    // WARNING: Some resources could require linked resources: .fnt --> .png, .mtl --> .png, .gltf --> .png
    // So doing a recursive pass to scan possible files with second resources
    for (int i = 0; i < assetCounter; i++)
    {
        if (IsFileExtension(paths[i], ".fnt;.gltf"))
        {
            int assetCount2 = 0;
            // ERROR: TextFormat() is messing paths due to static buffers
            char **assetPaths2 = LoadSourceAssetPaths(paths[i], &assetCount2);

            for (int j = 0; j < assetCount2; j++)
            {
                strcpy(paths[assetCounter], TextFormat("%s/%s", GetDirectoryPath(paths[i]) + 2, assetPaths2[j]));
                assetCounter++;
            }

            UnloadSourceAssetPaths(assetPaths2);
        }
    }
    */

    *assetCount = assetCounter;
    return paths;
}

// Clear resource paths scanned
static void UnloadSourceAssetPaths(char **assetPaths)
{
    for (int i = 0; i < RPC_MAX_ASSET_FILES; i++) RL_FREE(assetPaths[i]);

    RL_FREE(assetPaths);
}

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
static void GenerateProject(rpcProjectConfig project, rpcProjectInput input, const char *outPath)
{
    char *fileText = NULL;
    char *fileTextUpdated[10] = { 0 };

    // Get template directory
    // TODO: Use embedded template into executable?
    char templatePath[256] = { 0 };

    // NOTE: [template] directory must be in same directory as [rpc] tool
    strcpy(templatePath, TextFormat("%s/template", GetWorkingDirectory()));
    //strcpy(templatePath, TextFormat("%s/template", GetApplicationDirectory()));

    // Security check to validate required template
    if (!DirectoryExists(templatePath) ||
        !DirectoryExists(TextFormat("%s/src", templatePath)) ||
        !DirectoryExists(TextFormat("%s/projects", templatePath)) ||
        !FileExists(TextFormat("%s/project_name.rpc", templatePath)))
    {
        LOG("WARNING: Project generation template required files can not be found\n");
        return;
    }

    rpcProjectConfigTyped *config = rpcLoadProjectConfigTyped(project);
    LOG("\nINFO: Starting project generation: %s\n", config->Project.repoName);
    LOG("-----------------------------------------------------------------\n");

    //mz_bool mz_zip_reader_init_mem(mz_zip_archive *pZip, const void *pMem, size_t size, mz_uint flags); // Read file from memory zip data
    // TODO: Replace LoadFileText(), from template path, by reading text file from zip data, decompressing it...

    if (config->Project.repoName[0] == '\0') strcpy(config->Project.repoName, config->Project.internalName);

    LOG("INFO: Output path: %s/%s\n", outPath, config->Project.repoName);

    // Copy project source file(s) provided
    //--------------------------------------------------------------------------
    LOG("INFO: Copying input source files to project sources path: %s/%s\n", config->Project.repoName, config->Project.sourcePath);

    // Create required output directories (src/external)
    MakeDirectory(TextFormat("%s/%s/%s/external", outPath, config->Project.repoName, config->Project.sourcePath));

    for (int i = 0; i < input.srcFileCount; i++)
    {
        // Get expected destination file path for source input files
        const char *dstFilePath = TextFormat("%s/%s/%s/%s", outPath, config->Project.repoName, config->Project.sourcePath, GetFileName(input.srcFilePaths[i]));

        // Check if source file name contains "project_name"
        if (TextFindIndex(input.srcFilePaths[i], "project_name") > 0)
        {
            // Update project name for destination file to project internal name defined
            FileCopy(input.srcFilePaths[i], TextReplace(dstFilePath, "project_name", config->Project.internalName)); // Copy with new name
        }
        else FileCopy(input.srcFilePaths[i], dstFilePath); // Copy with original name
    }

    LOG("INFO: Copied project source files successfully\n");
    //-------------------------------------------------------------------------------------

    // Copy assets to output resource path (if required)
    //-------------------------------------------------------------------------------------
    if (input.assetFileCount > 0)
    {
        LOG("INFO: Copying input assets files to project resources path: %s/%s\n", config->Project.repoName, config->Project.assetsPath);

        MakeDirectory(TextFormat("%s/%s/%s", outPath, config->Project.repoName, config->Project.assetsPath));

        for (int i = 0; i < input.assetFileCount; i++)
        {
            // Get expected destination file path
            const char *dstFilePath = TextFormat("%s/%s/%s/%s", outPath, config->Project.repoName, config->Project.assetsPath, GetFileName(input.assetFilePaths[i]));
            FileCopy(input.assetFilePaths[i], dstFilePath); // Copy always with original name
        }

        LOG("INFO: Copied project asset files successfully\n");
    }
    //-------------------------------------------------------------------------------------


    // Project configuration file (.rpc)
    // NOTE: This file can be used by [rpb] to build the project
    //-------------------------------------------------------------------------------------
    LOG("INFO: Generating project config file (.rpc): %s/%s.rpc\n", config->Project.repoName, config->Project.internalName);
    // Update project configuration .rpc to defined values by [rpc] tool:
    /*
    PROJECT_INTERNAL_NAME                   "$(project_name)"                   # Project intenal name, used for executable and project files
    PROJECT_REPO_NAME                       "$(repo-name)"                      # Project repository name, used for VCS (GitHub, GitLab)
    PROJECT_COMMERCIAL_NAME                 "$(CommercialName)"                 # Project commercial name, used for docs and web
    PROJECT_SHORT_NAME                      "$(ShortName)"                      # Project short name
    PROJECT_VERSION                         "$(ProjectVersion)"                 # Project version
    PROJECT_DESCRIPTION                     "$(ProjectDescription)"             # Project description
    PROJECT_PUBLISHER_NAME                  "$(PublisherName)"                  # Project publisher name
    PROJECT_DEVELOPER_NAME                  "$(ProjectDeveloper)"               # Project developer name
    PROJECT_DEVELOPER_URL                   "$(DeveloperUrl)"                   # Project developer webpage url
    PROJECT_DEVELOPER_EMAIL                 "$(DeveloperEmail)"                 # Project developer email
    PROJECT_ICON_FILE                       "$(repo-name)/src/$(project_name).ico" # Project icon file
    PROJECT_SOURCE_PATH                     "$(repo-name)/src"                  # Project source directory, including all required code files (C/C++)
    PROJECT_ASSETS_PATH                     "$(repo-name)/src/resources"        # Project assets directory, including all required assets
    PROJECT_ASSETS_OUTPUT_PATH              "$(repo-name)/release/resources"    # Project assets destination path
    */
    rini_data data = rini_load_full(TextFormat("%s/project_name.rpc", templatePath));
    rini_set_value_text(&data, "PROJECT_REPO_NAME", config->Project.repoName, NULL);
    rini_set_value_text(&data, "PROJECT_INTERNAL_NAME", config->Project.internalName, NULL);
    rini_set_value_text(&data, "PROJECT_COMMERCIAL_NAME", config->Project.commercialName, NULL);
    rini_set_value_text(&data, "PROJECT_SHORT_NAME", config->Project.shortName, NULL);
    rini_set_value_text(&data, "PROJECT_VERSION", config->Project.version, NULL);
    rini_set_value_text(&data, "PROJECT_DESCRIPTION", config->Project.description, NULL);
    rini_set_value_text(&data, "PROJECT_PUBLISHER_NAME", config->Project.publisherName, NULL);
    rini_set_value_text(&data, "PROJECT_DEVELOPER_NAME", config->Project.developerName, NULL);
    rini_set_value_text(&data, "PROJECT_DEVELOPER_URL", config->Project.developerUrl, NULL);
    rini_set_value_text(&data, "PROJECT_DEVELOPER_EMAIL", config->Project.developerEmail, NULL);
    rini_set_value_text(&data, "PROJECT_ICON_FILE", config->Project.iconFile, NULL);
    rini_save(data, TextFormat("%s/%s/%s.rpc", outPath, config->Project.repoName, config->Project.internalName));

    /*
    fileText = LoadFileText(TextFormat("%s/project_name.rpc", templatePath));
    fileTextUpdated[0] = TextReplaceAlloc(fileText, "$(project_name)", config->Project.internalName); // rpcGetText(pc, "PROJECT_INTERNAL_NAME")
    fileTextUpdated[1] = TextReplaceAlloc(fileTextUpdated[0], "$(repo-name)", config->Project.repoName);
    fileTextUpdated[2] = TextReplaceAlloc(fileTextUpdated[1], "$(CommercialName)", config->Project.commercialName);
    fileTextUpdated[3] = TextReplaceAlloc(fileTextUpdated[2], "$(ShortName)", config->Project.shortName);
    fileTextUpdated[4] = TextReplaceAlloc(fileTextUpdated[3], "$(ProjectVersion)", config->Project.version);
    fileTextUpdated[5] = TextReplaceAlloc(fileTextUpdated[4], "$(ProjectDescription)", config->Project.description);
    fileTextUpdated[6] = TextReplaceAlloc(fileTextUpdated[5], "$(PublisherName)", config->Project.publisherName);
    fileTextUpdated[7] = TextReplaceAlloc(fileTextUpdated[6], "$(ProjectDeveloper)", config->Project.developerName);
    fileTextUpdated[8] = TextReplaceAlloc(fileTextUpdated[7], "$(DeveloperUrl)", config->Project.developerUrl);
    fileTextUpdated[9] = TextReplaceAlloc(fileTextUpdated[8], "$(DeveloperEmail)", config->Project.developerUrl);
    SaveFileText(TextFormat("%s/%s/%s.rpc", outPath, config->Project.repoName, config->Project.internalName), fileTextUpdated[9]);
    for (int i = 0; i < 10; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
    UnloadFileText(fileText);
    */
    LOG("INFO: Project config file (.rpc) generated successfully\n");
    //-------------------------------------------------------------------------------------

    // Project build system: Scripts
    //-------------------------------------------------------------------------------------
    if (input.requestedBuildSystems[0])
    {
        LOG("INFO: Generating build system: scripts (.bat, .sh)\n");

        // Create required output directories
        MakeDirectory(TextFormat("%s/%s/projects/scripts", outPath, config->Project.internalName));

        // Update src/build.bat (Windows only)
        // TODO: Use CMD/Shell calls directly, current script uses Makefile
        fileText = LoadFileText(TextFormat("%s/projects/scripts/build.bat", templatePath));
        fileTextUpdated[0] = TextReplaceAlloc(fileText, "project_name", config->Project.internalName);
        fileTextUpdated[1] = TextReplaceAlloc(fileTextUpdated[0], "ProjectDescription", config->Project.description);
        fileTextUpdated[2] = TextReplaceAlloc(fileTextUpdated[1], "C:\\raylib\\w64devkit\\bin", config->Platform.Windows.w64devkitPath);
        SaveFileText(TextFormat("%s/%s/projects/scripts/build.bat", outPath, config->Project.repoName), fileTextUpdated[2]);
        for (int i = 0; i < 8; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
        UnloadFileText(fileText);

        // TODO: Add .sh build script

        LOG("INFO: Build system generated successfully: scripts (.bat, .sh)\n");
    }
    //-------------------------------------------------------------------------------------

    // Project build system: Makefile
    //-------------------------------------------------------------------------------------
    if (input.requestedBuildSystems[1])
    {
        LOG("INFO: Generating build system: Makefile\n");

        // Update src/Makefile
        fileText = LoadFileText(TextFormat("%s/src/Makefile", templatePath));

        // Get names for the input source paths (only filename, without path)
        char **srcFileNames = (char **)RL_CALLOC(RPC_MAX_SOURCE_FILES, sizeof(char *));
        for (int i = 0; i < RPC_MAX_SOURCE_FILES; i++) srcFileNames[i] = (char *)RL_CALLOC(RPC_SOURCE_PATH_LENGTH, sizeof(char));

        int srcFileCount = 0;
        for (int j = 0; j < input.srcFileCount; j++)
        {
            if (IsFileExtension(input.srcFilePaths[j], ".c;.cpp"))
            {
                // TODO: WARNING: Some code files could be inside a directory structure,
                // but on copy it will be eliminated
                strcpy(srcFileNames[srcFileCount], GetFileName(input.srcFilePaths[j]));
                srcFileCount++;
            }
        }

        // Add all project required sources concatenated
        fileTextUpdated[0] = TextReplaceAlloc(fileText, "project_name.c", TextJoin(srcFileNames, srcFileCount, " "));

        for (int i = 0; i < RPC_MAX_SOURCE_FILES; i++) RL_FREE(srcFileNames[i]);
        RL_FREE(srcFileNames);

        fileTextUpdated[1] = TextReplaceAlloc(fileText, "project_name", config->Project.internalName);
        fileTextUpdated[2] = TextReplaceAlloc(fileTextUpdated[0], "C:\\raylib\\w64devkit\\bin", config->Platform.Windows.w64devkitPath);
        fileTextUpdated[3] = TextReplaceAlloc(fileTextUpdated[1], "C:/raylib/raylib/src", config->raylib.srcPath);
        SaveFileText(TextFormat("%s/%s/%s/Makefile", outPath, config->Project.repoName, config->Project.sourcePath), fileTextUpdated[3]);
        for (int i = 0; i < 8; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
        UnloadFileText(fileText);

        LOG("INFO: Build system generated successfully: Makefile\n");
    }
    //-------------------------------------------------------------------------------------

    // Project build system: VSCode
    //-------------------------------------------------------------------------------------
    if (input.requestedBuildSystems[2])
    {
        LOG("INFO: Generating build system: VSCode\n");

        // Create required output directories
        MakeDirectory(TextFormat("%s/%s/projects/VSCode/.vscode", outPath, config->Project.repoName));

        // Update projects/VSCode/.vscode/launch.json
        fileText = LoadFileText(TextFormat("%s/projects/VSCode/.vscode/launch.json", templatePath));
        fileTextUpdated[0] = TextReplaceAlloc(fileText, "project_name", config->Project.internalName);
        fileTextUpdated[1] = TextReplaceAlloc(fileTextUpdated[0], "C:/raylib/w64devkit/bin", config->Platform.Windows.w64devkitPath);
        SaveFileText(TextFormat("%s/%s/projects/VSCode/.vscode/launch.json", outPath, config->Project.repoName), fileTextUpdated[1]);
        for (int i = 0; i < 8; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
        UnloadFileText(fileText);

        // Update projects/VSCode/.vscode/c_cpp_properties.json
        fileText = LoadFileText(TextFormat("%s/projects/VSCode/.vscode/c_cpp_properties.json", templatePath));
        fileTextUpdated[0] = TextReplaceAlloc(fileText, "C:/raylib/raylib/src", config->raylib.srcPath);
        fileTextUpdated[1] = TextReplaceAlloc(fileTextUpdated[0], "C:/raylib/w64devkit/bin", config->Platform.Windows.w64devkitPath);
        SaveFileText(TextFormat("%s/%s/projects/VSCode/.vscode/c_cpp_properties.json", outPath, config->Project.repoName), fileTextUpdated[1]);
        for (int i = 0; i < 8; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
        UnloadFileText(fileText);

        // Update projects/VSCode/.vscode/tasks.json
        fileText = LoadFileText(TextFormat("%s/projects/VSCode/.vscode/tasks.json", templatePath));

        // Get names for the input source paths (only filename, without path)
        char **srcFileNames = (char **)RL_CALLOC(RPC_MAX_SOURCE_FILES, sizeof(char *));
        for (int i = 0; i < RPC_MAX_SOURCE_FILES; i++) srcFileNames[i] = (char *)RL_CALLOC(RPC_SOURCE_PATH_LENGTH, sizeof(char));

        int srcFileCount = 0;
        for (int j = 0; j < input.srcFileCount; j++)
        {
            if (IsFileExtension(input.srcFilePaths[j], ".c;.cpp"))
            {
                // TODO: WARNING: Some code files could be inside a directory structure,
                // but on copy it will be eliminated
                strcpy(srcFileNames[srcFileCount], GetFileName(input.srcFilePaths[j]));
                srcFileCount++;
            }
        }

        // Add all project required sources concatenated
        fileTextUpdated[0] = TextReplaceAlloc(fileText, "project_name.c", TextJoin(srcFileNames, srcFileCount, " "));

        for (int i = 0; i < RPC_MAX_SOURCE_FILES; i++) RL_FREE(srcFileNames[i]);
        RL_FREE(srcFileNames);

        fileTextUpdated[1] = TextReplaceAlloc(fileText, "project_name", config->Project.internalName);
        fileTextUpdated[2] = TextReplaceAlloc(fileTextUpdated[1], "C:/raylib/raylib/src", config->raylib.srcPath);
        fileTextUpdated[3] = TextReplaceAlloc(fileTextUpdated[2], "C:/raylib/w64devkit/bin", config->Platform.Windows.w64devkitPath);

        SaveFileText(TextFormat("%s/%s/projects/VSCode/.vscode/tasks.json", outPath, config->Project.repoName), fileTextUpdated[3]);
        for (int i = 0; i < 8; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
        UnloadFileText(fileText);

        // Copy projects/VSCode/.vscode/settings.json
        FileCopy(TextFormat("%s/projects/VSCode/.vscode/settings.json", templatePath),
            TextFormat("%s/%s/projects/VSCode/.vscode/settings.json", outPath, config->Project.repoName));

        // Copy projects/VSCode/main.code-workspace
        FileCopy(TextFormat("%s/projects/VSCode/main.code-workspace", templatePath),
            TextFormat("%s/%s/projects/VSCode/main.code-workspace", outPath, config->Project.repoName));

        // Copy projects/VSCode/README.md
        FileCopy(TextFormat("%s/projects/VSCode/README.md", templatePath),
            TextFormat("%s/%s/projects/VSCode/README.md", outPath, config->Project.repoName));

        LOG("INFO: Build system generated successfully: VSCode\n");
    }
    //-------------------------------------------------------------------------------------

    // Project build system: VS2022
    //-------------------------------------------------------------------------------------
    if (input.requestedBuildSystems[3])
    {
        LOG("INFO: Generating build system: Visual Studio 2022\n");

        // Create required output directories
        MakeDirectory(TextFormat("%s/%s/projects/VS2022/raylib", outPath, config->Project.repoName));
        MakeDirectory(TextFormat("%s/%s/projects/VS2022/%s", outPath, config->Project.repoName, config->Project.internalName));

        // Copy projects/VS2022/raylib/raylib.vcxproj
        fileText = LoadFileText(TextFormat("%s/projects/VS2022/raylib/raylib.vcxproj", templatePath));
        fileTextUpdated[0] = TextReplaceAlloc(fileText, "C:\\raylib\\raylib\\src", config->raylib.srcPath);
        SaveFileText(TextFormat("%s/%s/projects/VS2022/raylib/raylib.vcxproj", outPath, config->Project.repoName, config->Project.internalName), fileTextUpdated[0]);
        for (int i = 0; i < 8; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
        UnloadFileText(fileText);

        // Copy projects/VS2022/raylib/Directory.Build.props
        //fileText = LoadFileText(TextFormat("%s/projects/VS2022/raylib/Directory.Build.props", templatePath));
        //SaveFileText(TextFormat("%s/%s/projects/VS2022/raylib/Directory.Build.props", outPath, config->Project.internalName, config->Project.internalName), fileText);
        //UnloadFileText(fileText);

        // Update projects/VS2022/project_name/config->project_name.vcproj
        fileText = LoadFileText(TextFormat("%s/projects/VS2022/project_name/project_name.vcxproj", templatePath));

        // Get names for the input source paths (only filename, without path)
        char **srcFileNames = (char **)RL_CALLOC(RPC_MAX_SOURCE_FILES, sizeof(char *));
        for (int i = 0; i < RPC_MAX_SOURCE_FILES; i++) srcFileNames[i] = (char *)RL_CALLOC(RPC_SOURCE_PATH_LENGTH, sizeof(char));

        int srcFileCount = 0;
        for (int j = 0; j < input.srcFileCount; j++)
        {
            if (IsFileExtension(input.srcFilePaths[j], ".c;.cpp"))
            {
                // TODO: WARNING: Some code files could be inside a directory structure,
                // but on copy it will be eliminated
                strcpy(srcFileNames[srcFileCount], GetFileName(input.srcFilePaths[j]));
                srcFileCount++;
            }
        }

        // Add all project required sources concatenated
        fileTextUpdated[0] = TextReplaceAlloc(fileText, "project_name.c", srcFileNames[0]); // TODO: Main source code file
        char srcFilesBlock[1024] = { 0 };
        int nextPosition = 0;
        for (int k = 1; k < srcFileCount; k++)
        {
            TextAppend(srcFilesBlock, TextFormat("<ClCompile Include=\"..\\..\\..\\%s\\%s\" />\n    ", 
                config->Project.sourcePath, srcFileNames[k]), &nextPosition);
        }

        fileTextUpdated[1] = TextReplaceAlloc(fileTextUpdated[0], "<!--Additional Compile Items-->", srcFilesBlock);

        for (int i = 0; i < RPC_MAX_SOURCE_FILES; i++) RL_FREE(srcFileNames[i]);
        RL_FREE(srcFileNames);

        fileTextUpdated[2] = TextReplaceAlloc(fileTextUpdated[1], "project_name", config->Project.internalName);
        fileTextUpdated[3] = TextReplaceAlloc(fileTextUpdated[2], "C:\\raylib\\raylib\\src", config->raylib.srcPath);
        SaveFileText(TextFormat("%s/%s/projects/VS2022/%s/%s.vcxproj", outPath, config->Project.repoName, config->Project.internalName, config->Project.internalName), fileTextUpdated[3]);
        for (int i = 0; i < 8; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
        UnloadFileText(fileText);

        // Update projects/VS2022/project_name.sln
        fileText = LoadFileText(TextFormat("%s/projects/VS2022/project_name.sln", templatePath));
        fileTextUpdated[0] = TextReplaceAlloc(fileText, "project_name", config->Project.internalName);
        SaveFileText(TextFormat("%s/%s/projects/VS2022/%s.sln", outPath, config->Project.repoName, config->Project.internalName), fileTextUpdated[0]);
        for (int i = 0; i < 8; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
        UnloadFileText(fileText);

        LOG("INFO: Build system generated successfully: Visual Studio 2022\n");
    }
    //-------------------------------------------------------------------------------------

    // Project build system: CMake
    //-------------------------------------------------------------------------------------
    if (input.requestedBuildSystems[4])
    {
        LOG("INFO: Generating build system: CMake\n");

        // Create required output directories
        MakeDirectory(TextFormat("%s/%s/projects/CMake", outPath, config->Project.internalName));

        // TODO: Add CMake build system

        LOG("INFO: Build system generated successfully: CMake\n");
    }
    //-------------------------------------------------------------------------------------

    // Project build system: GitHub Actions
    // - Windows: Uses VS2022 project
    // - Linux, macOS, WebAssembly: Uses Makefile project
    // WARNING: Expects the PROJECT_NAME to be the repository-name (as generated by default)
    //-------------------------------------------------------------------------------------
    if (input.requestedBuildSystems[5])
    {
        LOG("INFO: Generating build system: GitHub Actions (CI/CD workflows)\n");

        // Create required output directories
        MakeDirectory(TextFormat("%s/%s/.github/workflows", outPath, config->Project.repoName));

        // Copy GitHub workflows: Windows, Linux, macOS, Webassembly
        FileCopy(TextFormat("%s/.github/workflows/build_windows.yml", templatePath),
            TextFormat("%s/%s/.github/workflows/build_windows.yml", outPath, config->Project.repoName));
        FileCopy(TextFormat("%s/.github/workflows/build_linux.yml", templatePath),
            TextFormat("%s/%s/.github/workflows/build_linux.yml", outPath, config->Project.repoName));
        FileCopy(TextFormat("%s/.github/workflows/build_macos.yml", templatePath),
            TextFormat("%s/%s/.github/workflows/build_macos.yml", outPath, config->Project.repoName));
        FileCopy(TextFormat("%s/.github/workflows/build_webassembly.yml", templatePath),
            TextFormat("%s/%s/.github/workflows/build_webassembly.yml", outPath, config->Project.repoName));

        LOG("INFO: Build system generated successfully: GitHub Actions (CI/CD workflows)\n");
    }
    //-------------------------------------------------------------------------------------

    // Update additional files required for product building
    //  - src/project_name.rc   -> Windows: Executable resource file, includes .ico and metadata
    //  - src/project_name.ico  -> Product icon, required for Window resource file
    //  - src/project_name.icns -> macOS: Product icon, required by Info.plist
    //  - src/Info.plist        -> macOS application resource file, includes .icns and metadata
    //  - src/minshell.html     -> Web: Html minimum shell for WebAssembly application, preconfigured
    //-------------------------------------------------------------------------------------
    LOG("INFO: Generating additional files\n");
    // Update src/project_name.rc
    fileText = LoadFileText(TextFormat("%s/src/project_name.rc", templatePath));
    // TODO: Replace "project_name.ico" by GetFileName(config->Project.iconFile) if possible
    fileTextUpdated[0] = TextReplaceAlloc(fileText, "CommercialName", config->Project.commercialName);
    fileTextUpdated[1] = TextReplaceAlloc(fileTextUpdated[0], "project_name", config->Project.internalName);
    fileTextUpdated[2] = TextReplaceAlloc(fileTextUpdated[1], "ProjectDescription", config->Project.description);
    fileTextUpdated[3] = TextReplaceAlloc(fileTextUpdated[2], "ProjectDeveloper", config->Project.developerName);
    fileTextUpdated[4] = TextReplaceAlloc(fileTextUpdated[3], "ProjectYear", TextFormat("%i", currentYear));
    SaveFileText(TextFormat("%s/%s/%s/%s.rc", outPath, config->Project.repoName, config->Project.sourcePath, config->Project.internalName), fileTextUpdated[4]);
    for (int i = 0; i < 8; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
    UnloadFileText(fileText);
    LOG("INFO: Updated Windows resource file successfully: %s/%s.rc\n", config->Project.sourcePath, config->Project.internalName);

    // Copy src/project_name.ico to src/project_name.ico
    // TODO: Generate .ico file from .png if required?
    if (FileExists(config->Project.iconFile))
    {
        const char *iconFilePath = TextFormat("%s/%s/%s/%s", outPath, config->Project.repoName, config->Project.sourcePath, GetFileName(config->Project.iconFile));
        FileCopy(config->Project.iconFile, TextReplace(iconFilePath, "project_name", config->Project.internalName));
        LOG("INFO: Added icon file successfully: %s/%s\n", config->Project.sourcePath, TextReplace(iconFilePath, "project_name", config->Project.internalName));
    }
    else
    {
        FileCopy(TextFormat("%s/src/project_name.ico", templatePath),
            TextFormat("%s/%s/%s/%s.ico", outPath, config->Project.repoName, config->Project.sourcePath, config->Project.internalName));
        LOG("INFO: Added icon file successfully: %s/%s.ico\n", config->Project.sourcePath, config->Project.internalName);
    }

    // Copy src/project_name.icns to src/project_name.icns
    // TODO: Generate .icns from input .ico/.png
    FileCopy(TextFormat("%s/src/project_name.icns", templatePath),
        TextFormat("%s/%s/%s/%s.icns", outPath, config->Project.repoName, config->Project.sourcePath, config->Project.internalName));
    LOG("INFO: Added icon file (.icns) successfully (macOS)\n");

    // Update src/Info.plist
    fileText = LoadFileText(TextFormat("%s/src/Info.plist", templatePath));
    fileTextUpdated[0] = TextReplaceAlloc(fileText, "CommercialName", config->Project.commercialName);
    fileTextUpdated[1] = TextReplaceAlloc(fileTextUpdated[0], "project_name", config->Project.internalName);
    fileTextUpdated[2] = TextReplaceAlloc(fileTextUpdated[1], "ProjectDescription", config->Project.description);
    fileTextUpdated[3] = TextReplaceAlloc(fileTextUpdated[2], "ProjectDeveloper", config->Project.developerName);
    fileTextUpdated[4] = TextReplaceAlloc(fileTextUpdated[3], "project_developer", TextToLower(config->Project.developerName));
    fileTextUpdated[5] = TextReplaceAlloc(fileTextUpdated[4], "ProjectYear", TextFormat("%i", currentYear));
    SaveFileText(TextFormat("%s/%s/%s/Info.plist", outPath, config->Project.repoName, config->Project.sourcePath), fileTextUpdated[5]);
    for (int i = 0; i < 8; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
    UnloadFileText(fileText);
    LOG("INFO: Generated Info.plist successfully (macOS)\n");

    // Update src/minshell.html
    // Review Webpage, links, OpenGraph/X card, keywords...
    fileText = LoadFileText(TextFormat("%s/src/minshell.html", templatePath));
    fileTextUpdated[0] = TextReplaceAlloc(fileText, "CommercialName", config->Project.commercialName);
    fileTextUpdated[1] = TextReplaceAlloc(fileTextUpdated[0], "project_name", config->Project.internalName);
    fileTextUpdated[2] = TextReplaceAlloc(fileTextUpdated[1], "ProjectDescription", config->Project.description);
    fileTextUpdated[3] = TextReplaceAlloc(fileTextUpdated[2], "ProjectDeveloper", config->Project.developerName);
    fileTextUpdated[4] = TextReplaceAlloc(fileTextUpdated[3], "project_developer", TextToLower(config->Project.developerName));
    fileTextUpdated[5] = TextReplaceAlloc(fileTextUpdated[4], "ProjectDeveloperUrl", TextToLower(config->Project.developerUrl));
    SaveFileText(TextFormat("%s/%s/src/minshell.html", outPath, config->Project.repoName), fileTextUpdated[5]);
    for (int i = 0; i < 8; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
    UnloadFileText(fileText);
    LOG("INFO: Generated minshell.html successfully (WebAssembly)\n");
    //-------------------------------------------------------------------------------------

    // Update README.md
    fileText = LoadFileText(TextFormat("%s/README.md", templatePath));
    fileTextUpdated[0] = TextReplaceAlloc(fileText, "CommercialName", config->Project.commercialName);
    fileTextUpdated[1] = TextReplaceAlloc(fileTextUpdated[0], "project_name", config->Project.internalName);
    fileTextUpdated[2] = TextReplaceAlloc(fileTextUpdated[1], "ProjectDescription", config->Project.description);
    fileTextUpdated[3] = TextReplaceAlloc(fileTextUpdated[2], "ProjectDeveloper", config->Project.developerName);
    fileTextUpdated[4] = TextReplaceAlloc(fileTextUpdated[3], "ProjectYear", TextFormat("%i", currentYear));
    SaveFileText(TextFormat("%s/%s/README.md", outPath, config->Project.repoName), fileTextUpdated[4]);
    for (int i = 0; i < 8; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
    UnloadFileText(fileText);
    LOG("INFO: Generated README.md file successfully\n");

    // Update LICENSE, including ProjectDeveloper
    fileText = LoadFileText(TextFormat("%s/LICENSE", templatePath));
    fileTextUpdated[0] = TextReplaceAlloc(fileText, "ProjectDeveloper", config->Project.developerName);
    fileTextUpdated[1] = TextReplaceAlloc(fileTextUpdated[0], "ProjectYear", TextFormat("%i", currentYear));
    SaveFileText(TextFormat("%s/%s/LICENSE", outPath, config->Project.repoName), fileTextUpdated[1]);
    for (int i = 0; i < 8; i++) { MemFree(fileTextUpdated[i]); fileTextUpdated[i] = NULL; }
    UnloadFileText(fileText);
    LOG("INFO: Generated LICENSE file successfully: zlib/libpng\n");

    // Copy from template files that do not require customization: CONVENTIONS.md, .gitignore
    FileCopy(TextFormat("%s/CONVENTIONS.md", templatePath),
        TextFormat("%s/%s/CONVENTIONS.md", outPath, config->Project.repoName));
    LOG("INFO: Generated CONVENTIONS.md file successfully\n");
    FileCopy(TextFormat("%s/.gitignore", templatePath),
        TextFormat("%s/%s/.gitignore", outPath, config->Project.repoName));
    LOG("INFO: Generated .gitignore file successfully\n\n");

    LOG("INFO: Project generated successfully: %s\n", config->Project.internalName);
    LOG("-----------------------------------------------------------------\n");

    rpcUnloadProjectConfigTyped(config);
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
    int textLength = TextLength(text);

    if (text != NULL)
    {
        // Count how many substrings we have on text and point to every one
        for (int i = 0; i < textLength; i++)
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
