/*******************************************************************************************
*
*   rpc - raylib project config data types and functionality
*
*   NOTE: This header types must be shared by [rpc] and [rpb] tools
*
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2025 raylib technologies (@raylibtech) / Ramon Santamaria (@raysan5)
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

// WARNING: raygui implementation is expected to be defined before including this header

#ifndef RPCONFIG_H
#define RPCONFIG_H

#include "raylib.h"

#define RPC_MAX_PROPERTY_ENTRIES    256

#define RPCAPI

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

// Project image set
typedef struct {
    Image imIcons[9];           // Windows icon images (256, 128, 96, 64, 48, 32, 24, 16, 184)

    Image imGitHubPromo;        // GitHub promo card image (1280x640)
    Image imItchioCover;        // itch.io cover image (315x250)
    Image imItchioPromo;        // itch.io promo image (450x300)
    Image imItchioBanner;       // itch.io banner (960x210)
    Image imTwitterCard;        // Twitter card (800x418)

    //Image imSteamClientImage;           // Steam client_image (16x16)     -> Using imIcon[7]
    //Image imSteamCommunityIcon;         // Steam comunity_icon (184x184)  -> Using imIcon[8]
    Image imSteamStoreCapsuleMain;      // Steam store_capsule_main (616x353)
    Image imSteamStoreCapsuleHeader;    // Steam store_capsule_header (460x215)
    Image imSteamStoreCapsuleSmall;     // Steam store_capsule_small (231x87)
    Image imSteamStoreCapsuleVertical;  // Steam store_capsule_vertical (374x448)
    Image imSteamLibraryCapsule;        // Steam library_capsule (600x900)
    Image imSteamLibraryLogo;           // Steam library_logo_transparent (1280x720)
} rpcProjectImagery;

// Project Configuration
// 
// NOTE 1: It contains all project configurable properties, organized by supported categories: 
//  - PROJECT: Project configuration, required to generate project structure
//  - BUILD: Build configuration properties, generic for all platforms building 
//  - PLATFORM: Platform-specific config properties, required to build project for that platform
//  - DEPLOY: Project deployment options for packaging and project distribution
//  - IMAGERY: Support imagery required on deployment and stores distribution 
//  - raylib: raylib configuration options, for library customization (if desired)
//
// NOTE 2: This structure differs from [rpcProjectConfigRaw] because it not only
// contains all the required properties organized with names (vs raw list of generic properties)
// but it also contains internal properties filled by the tools as required
// i.e. [assetsPath] is automatically scanned to fill [assetFilePaths]
// 
// WARNING: Create only dynamic objects for this selectedSource
typedef struct {
    struct {
        char commercialName[64];        // Project: commercial name, used for docs and web
        char repoName[64];              // Project: repository name, used for VCS (GitHub, GitLab)
        char internalName[64];          // Project: internal name, used for executable and VS2022 project
        char shortName[64];             // Project: short name, used for icons

        // [rpc] year automatically set at project initialization
        int year;                       // Project: year

        char version[16];               // Project: version
        char description[256];          // Project: description
        char publisherName[64];         // Project: publisher name
        char developerName[64];         // Project: developer/company name
        char developerUrl[64];          // Project: developer webpage
        char developerEmail[64];        // Project: developer email (info/support?)
        char iconFile[256];             // Project: icon file path (.ico/.icns), for application

        char sourcePath[256];           // Project: source files path, scanned for (.c/.cpp) files
        char assetsPath[256];           // Project: assets files path, including all project resources
        char assetsOutPath[256];        // Project: assets output path (on project generation)

        // [rpc] scanned from source/assets paths provided
        int sourceFileCount;            // Project: source files count
        char sourceFilePaths[64][256];  // Project: source files path(s) -> MAX_SOURCE_FILES=64
        int assetFileCount;             // Project: assets files count
        char assetFilePaths[256][256];  // Project: assets files paths -> MAX_ASSETS_FILES=256

        // [rpc] internal properties
        int selectedSource;             // Project: selected source (template to start project)
        char generationOutPath[256];    // Project: generation output path

    } Project;
    struct {
        char outputPath[256];           // Build: output path (for VS2022 defaults to 'build' directory)

        bool assetsValidation;          // Build: Flag: request assets validation on building
        bool assetsPackaging;           // Build: Flag: request assets packaging on building (requires source code update)
        char rrpPackagerPath[256];      // Build: Path to [rrespacker] tool

        // [rpc] project generation build system requested
        bool requestedBuildSystems[6];  // Build: Flags: systems required: 0-Script, 1-Makefile, 2-VSCode, 3-VS2022, 4-CMake

        // [rpb] Properties for current automated build
        char targetPlatform[64];        // Build: target platform (Supported: Windows, Linux, macOS, Android, Web)
        char targetArchitecture[64];    // Build: target architecture (Supported: x86-64, Win32, arm64)
        char targetMode[64];            // Build: target mode (Supported: DEBUG, RELEASE, DEBUG_DLL, RELEASE_DLL)

    } Build;
    struct {
        struct {
            char msbuildPath[256];      // Platform: Windows: Path to MSBuild system, required to build VS2022 solution
            char w64devkitPath[256];    // Platform: Windows: Path to w64devkit (GCC), required to use Makefile building
            char signtoolPath[256];     // Platform: Windows: Path to signtool in case program needs to be signed (certificate required)
            char signCertFile[256];     // Platform: Windows: Executable signing certificate

        } Windows;
        struct {
            bool useCrossCompiler;      // Platform: Linux: Flag: request cross-compiler usage
            char crossCompilerPath;     // Platform: Linux: Path to DRM cross-compiler for target ABI

        } Linux;
        struct {
            char bundleInfoFile[256];   // Platform: macOS: Path to macOS bundle options (Info.plist)
            char bundleName[256];       // Platform: macOS: Bundle product name, default to project commercial name
            char bundleVersion[16];     // Platform: macOS: Bundle version
            char bundleIconFile[256];   // Platform: macOS: Bundle icon file (requies .icns)

        } macOS;
        struct {
            char emsdkPath[256];        // Platform: HTML5: Path to emsdk, required for Web building
            char shellFile[256];        // Platform: HTML5: Path to shell file to be used by emscripten
            int heapMemorySize;         // Platform: HTML5: Required heap memory size in MB (required for assets loading)

            bool useAsincify;           // Platform: HTML5: Flag: use ASINCIFY mode on building
            bool useWebGL2;             // Platform: HTML5: Flag: use WebGL2 (OpenGL ES 3.1) instead of default WebGL1 (OpenGL ES 2.0)

        } HTML5;
        struct {
            char sdkPath[256];          // Platform: Android: Path to Android SDK, required for Android App building and support tools
            char ndkPath[256];          // Platform: Android: Path to Android NDK, required for C native building to Android
            char javaSdkPath[256];      // Platform: Android: Path to Java SDK, required for some tools

            char manifestFile[256];     // Platform: Android: Path to Android manifest file, including build options

            char minSdkVersion;         // Platform: Android: Minimum SDK version required
            char targetSdkVersion;      // Platform: Android: Target SDK version

        } Android;
        struct {
            bool useCrossCompiler;      // Platform: DRM: Flag: request cross-compiler usage
            char crossCompilerPath;     // Platform: DRM: Path to DRM cross-compiler for target ABI
        } DRM;
        struct {
            bool placeholder;           // Platform: FreeBSD: Flag: placeholder

        } FreeBSD;
        struct {
            char sdkPath[256];          // Platform: Dreamcast: Path to Dreamcast SDK (KallistiOS), required for Dreamcast building

        } Dreamcast;

    } Platform;
    struct {
        bool zipPackage;                // Deploy: Flag: request package to be zipped for distribution

        bool rifInstaller;              // Deploy: Flag: request installer creation using rInstallFriendly tool
        char rifInstallerPath[256];     // Deploy: Path to [rInstallFriendly] tool

        bool includeREADME;             // Deploy: Flag: request including README file on package
        char readmePath[256];           // Deploy: README file path
        bool includeEULA;               // Deploy: Flag: request including EULA file on package (vs LICENSE file for FOSS)
        char eulaPath[256];             // Deploy: EULA file path

    } Deploy;
    struct {
        char logoFile[256];             // Imagery: logo file path, for imagery (itchio/Steam)
        char splashFile[256];           // Imagery: splash image file path

        // [rpb] Imagery generation internal variables 
        bool genImageryAuto;            // Imagery: generate automatically
        rpcProjectImagery images;       // Imagery: image set for exporting

    } Imagery;
    struct {

        char srcPath[256];              // raylib: source code path
        char version[16];               // raylib: version for the project
        char glVersion[16];             // raylib: OpenGL version requested
        // TODO: raylib config.h options to expose

    } raylib;
} rpcProjectConfig;

// Property category type
typedef enum {
    RPC_CAT_PROJECT = 0,
    RPC_CAT_BUILD,
    RPC_CAT_PLATFORM,
    RPC_CAT_DEPLOY,
    RPC_CAT_IMAGERY,
    RPC_CAT_RAYLIB
} rpcPropertyEntryCategory;

// Property data type
typedef enum {
    RPC_TYPE_BOOL = 0,
    RPC_TYPE_VALUE,
    RPC_TYPE_TEXT,
    RPC_TYPE_TEXT_FILE,
    RPC_TYPE_TEXT_PATH,
} rpcPropertyEntryType;

// Property platform type
typedef enum {
    RPC_PLATFORM_WINDOWS = 0,
    RPC_PLATFORM_LINUX,
    RPC_PLATFORM_MACOS,
    RPC_PLATFORM_HTML5,
    RPC_PLATFORM_ANDROID,
    RPC_PLATFORM_DRM,
    RPC_PLATFORM_SWITCH,
    RPC_PLATFORM_DREAMCAST,
    RPC_PLATFORM_FREEBSD,
    RPC_PLATFORM_ANY
} rpcPlatform;

// Project Config Property Entry data selectedSource
// NOTE: Useful to automatice UI generation,
// every data entry is read from rpc config file
typedef struct {
    char key[64];       // Entry key (as read from .rpc)
    char text[256];     // Entry text data (selectedSource: TEXT, FILE, PATH) - WARNING: Max len defined for rini
    char desc[128];     // Entry data description, useful for tooltips

    // Data extracted from key
    char name[64];      // Entry name label for display, computed from key
    int category;       // Entry category: PROJECT, BUILDING, PLATFORM, DEPLOY, IMAGERY, raylib
    int platform;       // Entry platform: WINDOWS, LINUX, MACOS, HTML5, ANDROID, DRM, SWITCH, DREAMCAST, FREEBSD...
    int type;           // Entry type of data: VALUE (int), BOOL (int), TEXT (string), FILE (string-file), PATH (string-path)
    int value;          // Entry value, integer from text

    // Transient data
    bool editMode;      // Edit mode required for UI text control
} rpcPropertyEntry;

// Project Config Data (generic)
typedef struct {
    int entryCount;     // Number of entries
    rpcPropertyEntry *entries;  // Entries
} rpcProjectConfigRaw;

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------

#if defined(__cplusplus)
extern "C" {    // Prevents name mangling of functions
#endif

RPCAPI rpcProjectConfigRaw LoadProjectConfigRaw(const char *fileName); // Load project config data data from .rpc file
RPCAPI void UnloadProjectConfigRaw(rpcProjectConfigRaw raw); // Unload project data
RPCAPI void SaveProjectConfigRaw(rpcProjectConfigRaw raw, const char *fileName, int flags); // Save project config data to .rpc file

RPCAPI void SyncProjectConfig(rpcProjectConfigRaw src, rpcProjectConfig *dst); // Sync ProjectConfigRaw data --> ProjectConfig data
RPCAPI void SyncProjectConfigRaw(rpcProjectConfig *src, rpcProjectConfigRaw dst); // Sync ProjectConfig data --> ProjectConfigRaw data

#if defined(__cplusplus)
}               // Prevents name mangling of functions
#endif

#endif // RPCONFIG_H

/***********************************************************************************
*
*   RPCONFIG IMPLEMENTATION
*
************************************************************************************/

#if defined(RPCONFIG_IMPLEMENTATION)

#include "rini.h"

#include <string.h>     // Required for: strcpy()
#include <stdlib.h>     // Required for: calloc(), free()

// Load project config raw data from .rpc file
rpcProjectConfigRaw LoadProjectConfigRaw(const char *fileName)
{
    rpcProjectConfigRaw raw = { 0 };

    if (FileExists(fileName))
    {
        rini_data config = { 0 };
        config = rini_load(fileName);

        // Process/organize config data for our application
        raw.entries = (rpcPropertyEntry *)RL_CALLOC(config.count, sizeof(rpcPropertyEntry));
        raw.entryCount = config.count;

        for (int i = 0; i < raw.entryCount; i++)
        {
            strcpy(raw.entries[i].key, config.values[i].key);
            strcpy(raw.entries[i].desc, config.values[i].desc);
            raw.entries[i].platform = RPC_PLATFORM_ANY;

            // Category is parsed from first word on key
            char category[32] = { 0 };
            int categoryLen = 0; //TextFindIndex(config.values[i].key, "_");
            for (int c = 0; c < 128; c++) { if (config.values[i].key[c] != '_') categoryLen++; else break; }
            strncpy(category, config.values[i].key, categoryLen);
            strcpy(raw.entries[i].name, TextReplace(config.values[i].key + categoryLen + 1, "_", " "));

            if (TextIsEqual(category, "PROJECT")) raw.entries[i].category = RPC_CAT_PROJECT;
            else if (TextIsEqual(category, "BUILD")) raw.entries[i].category = RPC_CAT_BUILD;
            else if (TextIsEqual(category, "PLATFORM"))
            {
                raw.entries[i].category = RPC_CAT_PLATFORM;

                // Get platform from key
                char platform[32] = { 0 };
                int platformLen = 0;//TextFindIndex(config.values[i].key + categoryLen + 1, "_");
                for (int c = 0; c < 128; c++) { if (config.values[i].key[c + categoryLen + 1] != '_') platformLen++; else break; }
                memcpy(platform, config.values[i].key + categoryLen + 1, platformLen);

                if (TextIsEqual(platform, "WINDOWS")) raw.entries[i].platform = RPC_PLATFORM_WINDOWS;
                else if (TextIsEqual(platform, "LINUX")) raw.entries[i].platform = RPC_PLATFORM_LINUX;
                else if (TextIsEqual(platform, "MACOS")) raw.entries[i].platform = RPC_PLATFORM_MACOS;
                else if (TextIsEqual(platform, "HTML5")) raw.entries[i].platform = RPC_PLATFORM_HTML5;
                else if (TextIsEqual(platform, "ANDROID")) raw.entries[i].platform = RPC_PLATFORM_ANDROID;
                else if (TextIsEqual(platform, "DRM")) raw.entries[i].platform = RPC_PLATFORM_DRM;
                else if (TextIsEqual(platform, "SWITCH")) raw.entries[i].platform = RPC_PLATFORM_SWITCH;
                else if (TextIsEqual(platform, "DREAMCAST")) raw.entries[i].platform = RPC_PLATFORM_DREAMCAST;
                else if (TextIsEqual(platform, "FREEBSD")) raw.entries[i].platform = RPC_PLATFORM_FREEBSD;

                memset(raw.entries[i].name, 0, 64);
                strcpy(raw.entries[i].name, config.values[i].key + categoryLen + platformLen + 2);
            }
            else if (TextIsEqual(category, "DEPLOY")) raw.entries[i].category = RPC_CAT_DEPLOY;
            else if (TextIsEqual(category, "IMAGERY")) raw.entries[i].category = RPC_CAT_IMAGERY;
            else if (TextIsEqual(category, "RAYLIB")) raw.entries[i].category = RPC_CAT_RAYLIB;
        }

        for (int i = 0; i < raw.entryCount; i++)
        {
            // Type is parsed from key and value
            if (!config.values[i].isText)
            {
                if (TextFindIndex(raw.entries[i].key, "_FLAG")) raw.entries[i].type = RPC_TYPE_BOOL;
                else raw.entries[i].type = RPC_TYPE_VALUE;

                // Get the value
                raw.entries[i].value = TextToInteger(config.values[i].text);
            }
            else // Value is text
            {
                if (TextFindIndex(raw.entries[i].key, "_FILES") > 0)
                {
                    // TODO: How we check if files list includes multiple files,
                    // checking for ';' separator???
                    raw.entries[i].type = RPC_TYPE_TEXT_FILE;
                }
                else if (TextFindIndex(raw.entries[i].key, "_FILE")  > 0) raw.entries[i].type = RPC_TYPE_TEXT_FILE;
                else if (TextFindIndex(raw.entries[i].key, "_PATH")  > 0) raw.entries[i].type = RPC_TYPE_TEXT_PATH;
                else
                {
                    raw.entries[i].type = RPC_TYPE_TEXT;
                }

                strcpy(raw.entries[i].text, config.values[i].text);
            }
        }

        rini_unload(&config);
    }

    return raw;
}

// Unload project data
void UnloadProjectConfigRaw(rpcProjectConfigRaw raw)
{
    RL_FREE(raw.entries);
}

// Save project config data to .rpc file
// NOTE: Same function as [rpc] tool but but adding more data
void SaveProjectConfigRaw(rpcProjectConfigRaw data, const char *fileName, int flags)
{
    rini_data config = rini_load(NULL);   // Create empty config with 32 entries (RINI_MAX_CONFIG_CAPACITY)

    // Define header comment lines
    rini_set_comment_line(&config, NULL);   // Empty comment line, but including comment prefix delimiter
    rini_set_comment_line(&config, "raylib project configuration");
    rini_set_comment_line(&config, NULL);
    rini_set_comment_line(&config, "This file contains all required data to define a raylib C/C++ project");
    rini_set_comment_line(&config, "and allow building it for multiple platforms using [rpb] tool");
    rini_set_comment_line(&config, NULL);
    rini_set_comment_line(&config, "Project configuration is organized in several categories, depending on usage requirements");
    rini_set_comment_line(&config, "CATEGORIES:");
    rini_set_comment_line(&config, "   - PROJECT: Project definition properties, required for project generation");
    rini_set_comment_line(&config, "   - BUILD: Project build properties, required for project building, generic for all platforms");
    rini_set_comment_line(&config, "   - PLATFORM: Platform-specific properies, required for building for that platform");
    rini_set_comment_line(&config, "   - DEPLOY: Deployment properties, required to distribute the generated build");
    rini_set_comment_line(&config, "   - IMAGERY: Project imagery properties, required for distribution on some stores and marketing");
    rini_set_comment_line(&config, NULL);
    rini_set_comment_line(&config, "This file follow certain conventions to be able to display the information in");
    rini_set_comment_line(&config, "an easy-configurable UI manner when loaded through [rpb - raylib project builder] tool");
    rini_set_comment_line(&config, "CONVENTIONS:");
    rini_set_comment_line(&config, "   - ID containing [_FLAG_]: Value is considered a boolean, it displays with a [GuiCheckBox]");
    rini_set_comment_line(&config, "   - ID do not contain "": Value is considered as an integer, it displays as [GuiValueBox]");
    rini_set_comment_line(&config, "   - ID ends with _FILE or _FILES: Value is considered as a text file path, it displays as [GuiTextBox] with a [BROWSE-File] button");
    rini_set_comment_line(&config, "   - ID ends with _PATH: Value is considered as a text directory path, it displays as [GuiTextBox] with a [BROWSE-Dir] button");
    rini_set_comment_line(&config, NULL);
    rini_set_comment_line(&config, "NOTE: The comments/description for each entry is used as tooltip when editing the entry on [rpb]");
    rini_set_comment_line(&config, "\n");

    /*
    char key[64];       // Entry key (as read from .rpc)
    char name[64];      // Entry name label for display, computed from key
    int category;       // Entry category: PROJECT, BUILDING, PLATFORM, DEPLOY, IMAGERY, raylib
    int platform;       // Entry platform-specific
    int type;           // Entry type of data: VALUE (int), BOOL (int), TEXT (string), FILE (string-file), PATH (string-path)
    int value;          // Entry value (type: VALUE, BOOL)
    // TODO: WARNING: rini expects a maximum len for text of 256 chars, multiple files can be longer that that
    char text[256];     // Entry text data (type: TEXT, FILE, PATH) - WARNING: It can include multiple paths
    char desc[128];     // Entry data description, useful for tooltips
    */

    // We are saving data into file organized by categories and platforms,
    // independently of the format it was originally loaded (in case of manual edition)

    // Saving PROJECT category data
    rini_set_comment_line(&config, "Project settings");
    rini_set_comment_line(&config, "------------------------------------------------------------------------------------");
    for (int i = 0; i < data.entryCount; i++)
    {
        rpcPropertyEntry *entry = &data.entries[i];

        if (entry->category == RPC_CAT_PROJECT)
        {
            switch (entry->type)
            {
            case RPC_TYPE_BOOL:
            case RPC_TYPE_VALUE: rini_set_value(&config, entry->key, entry->value, entry->desc); break;
            case RPC_TYPE_TEXT:
            case RPC_TYPE_TEXT_FILE:
            case RPC_TYPE_TEXT_PATH: rini_set_value_text(&config, entry->key, entry->text, entry->desc); break;
            default: break;
            }
        }
    }

    rini_save(config, fileName);
    rini_unload(&config);
}

// Sync ProjectConfigRaw data --> ProjectConfig data
void SyncProjectConfig(rpcProjectConfigRaw src, rpcProjectConfig *dst)
{
    for (int i = 0; i < src.entryCount; i++)
    {
        // PROJECT properties mapping
        if (TextIsEqual(src.entries[i].key, "PROJECT_INTERNAL_NAME")) strcpy(dst->Project.internalName, src.entries[i].text); // Project intenal name, used for executable and project files
        else if (TextIsEqual(src.entries[i].key, "PROJECT_REPO_NAME")) strcpy(dst->Project.repoName, src.entries[i].text); // Project repository name, used for VCS (GitHub, GitLab)
        else if (TextIsEqual(src.entries[i].key, "PROJECT_COMMERCIAL_NAME")) strcpy(dst->Project.commercialName, src.entries[i].text); // Project commercial name, used for docs and web
        else if (TextIsEqual(src.entries[i].key, "PROJECT_SHORT_NAME")) strcpy(dst->Project.shortName, src.entries[i].text); // Project short name
        else if (TextIsEqual(src.entries[i].key, "PROJECT_VERSION")) strcpy(dst->Project.version, src.entries[i].text); // Project version
        else if (TextIsEqual(src.entries[i].key, "PROJECT_DESCRIPTION")) strcpy(dst->Project.description, src.entries[i].text); // Project description
        else if (TextIsEqual(src.entries[i].key, "PROJECT_PUBLISHER_NAME")) strcpy(dst->Project.publisherName, src.entries[i].text); // Project publisher name
        else if (TextIsEqual(src.entries[i].key, "PROJECT_DEVELOPER_NAME")) strcpy(dst->Project.developerName, src.entries[i].text); // Project developer name
        else if (TextIsEqual(src.entries[i].key, "PROJECT_DEVELOPER_URL")) strcpy(dst->Project.developerUrl, src.entries[i].text); // Project developer webpage url
        else if (TextIsEqual(src.entries[i].key, "PROJECT_DEVELOPER_EMAIL")) strcpy(dst->Project.developerEmail, src.entries[i].text); // Project developer email
        else if (TextIsEqual(src.entries[i].key, "PROJECT_ICON_FILE")) strcpy(dst->Project.iconFile, src.entries[i].text); // Project icon file
        else if (TextIsEqual(src.entries[i].key, "PROJECT_SOURCE_PATH")) strcpy(dst->Project.sourcePath, src.entries[i].text); // Project source directory, including all required code files (C/C++)
        else if (TextIsEqual(src.entries[i].key, "PROJECT_ASSETS_PATH")) strcpy(dst->Project.assetsPath, src.entries[i].text); // Project assets directory, including all required assets
        else if (TextIsEqual(src.entries[i].key, "PROJECT_ASSETS_OUTPUT_PATH")) strcpy(dst->Project.assetsOutPath, src.entries[i].text); // Project assets destination path
        // raylib properties mapping
        else if (TextIsEqual(src.entries[i].key, "RAYLIB_SRC_PATH")) strcpy(dst->raylib.srcPath, src.entries[i].text); // Path to raylib source code, to be build for target platform
        else if (TextIsEqual(src.entries[i].key, "RAYLIB_OPENGL_VERSION")) strcpy(dst->raylib.glVersion, src.entries[i].text); // OpenGL version to be used by raylib, WARNING: Platform dependant!
        // BUILD properties mapping
        else if (TextIsEqual(src.entries[i].key, "BUILD_OUTPUT_PATH")) strcpy(dst->Build.outputPath, src.entries[i].text); // Build output path
        else if (TextIsEqual(src.entries[i].key, "BUILD_TARGET_PLATFORM")) strcpy(dst->Build.targetPlatform, src.entries[i].text); // Build target platform (Supported: Windows, Linux, macOS, Android, Web)
        else if (TextIsEqual(src.entries[i].key, "BUILD_TARGET_ARCHITECTURE")) strcpy(dst->Build.targetArchitecture, src.entries[i].text); // Build target architecture (Supported: x86-64, Win32, arm64)
        else if (TextIsEqual(src.entries[i].key, "BUILD_TARGET_MODE")) strcpy(dst->Build.targetMode, src.entries[i].text); // Build target mode (Supported: DEBUG, RELEASE, DEBUG_DLL, RELEASE_DLL)
        else if (TextIsEqual(src.entries[i].key, "BUILD_FLAG_ASSETS_VALIDATION")) dst->Build.assetsValidation = src.entries[i].value; // Flag: request assets validation on building
        else if (TextIsEqual(src.entries[i].key, "BUILD_FLAG_ASSETS_PACKAGING")) dst->Build.assetsPackaging = src.entries[i].value; // Flag: request assets packaging on building
        // PLATFORM properties mapping
        else if (TextIsEqual(src.entries[i].key, "PLATFORM_WINDOWS_MSBUILD_PATH")) strcpy(dst->Platform.Windows.msbuildPath, src.entries[i].text); // Path to MSBuild system, required to build VS2022 solution
        else if (TextIsEqual(src.entries[i].key, "PLATFORM_WINDOWS_W64DEVKIT_PATH")) strcpy(dst->Platform.Windows.w64devkitPath, src.entries[i].text); // Path to w64devkit (GCC), required to use Makefile building
        else if (TextIsEqual(src.entries[i].key, "PLATFORM_WINDOWS_SIGNTOOL_PATH")) strcpy(dst->Platform.Windows.signtoolPath, src.entries[i].text); // Path to signtool in case program needs to be signed (certificate required)
        else if (TextIsEqual(src.entries[i].key, "PLATFORM_WINDOWS_SIGNCERT_FILE")) strcpy(dst->Platform.Windows.signCertFile, src.entries[i].text); // Path to a valid signature certificate to sign executable
        else if (TextIsEqual(src.entries[i].key, "PLATFORM_LINUX_FLAG_CROSS_COMPILE")) dst->Platform.Linux.useCrossCompiler = src.entries[i].value; // Flag: request cross-compiler usage
        else if (TextIsEqual(src.entries[i].key, "PLATFORM_LINUX_CROSS_COMPILER_PATH")) strcpy(dst->Platform.Linux.crossCompilerPath, src.entries[i].text); // Path to GCC compiler (probably not required)
        else if (TextIsEqual(src.entries[i].key, "PLATFORM_MACOS_BUNDLE_INFO_FILE")) strcpy(dst->Platform.macOS.bundleInfoFile, src.entries[i].text); // Path to macOS bundle options (Info.plist)
        else if (TextIsEqual(src.entries[i].key, "PLATFORM_MACOS_BUNDLE_NAME")) strcpy(dst->Platform.macOS.bundleName, src.entries[i].text); // Bundle name
        else if (TextIsEqual(src.entries[i].key, "PLATFORM_MACOS_BUNDLE_VERSION")) strcpy(dst->Platform.macOS.bundleVersion, src.entries[i].text); // Bundle version
        else if (TextIsEqual(src.entries[i].key, "PLATFORM_HTML5_EMSDK_PATH")) strcpy(dst->Platform.HTML5.emsdkPath, src.entries[i].text); // Path to emsdk, required for Web building
        else if (TextIsEqual(src.entries[i].key, "PLATFORM_HTML5_SHELL_FILE")) strcpy(dst->Platform.HTML5.shellFile, src.entries[i].text); // Path to shell file to be used by emscripten
        else if (TextIsEqual(src.entries[i].key, "PLATFORM_HTML5_HEAP_MEMORY_SIZE")) dst->Platform.HTML5.heapMemorySize = src.entries[i].value; // Required heap memory size in MB (required for assets loading)
        else if (TextIsEqual(src.entries[i].key, "PLATFORM_HTML5_FLAG_USE_ASINCIFY")) dst->Platform.HTML5.useAsincify = src.entries[i].value; // Flag: use ASINCIFY mode on building
        else if (TextIsEqual(src.entries[i].key, "PLATFORM_HTML5_FLAG_USE_WEBGL2")) dst->Platform.HTML5.useWebGL2, src.entries[i].value; // Flag: use WebGL2 (OpenGL ES 3.1) instead of default WebGL1 (OpenGL ES 2.0)
        else if (TextIsEqual(src.entries[i].key, "PLATFORM_ANDROID_SDK_PATH")) strcpy(dst->Platform.Android.sdkPath, src.entries[i].text); // Path to Android SDK, required for Android App building and support tools
        else if (TextIsEqual(src.entries[i].key, "PLATFORM_ANDROID_NDK_PATH")) strcpy(dst->Platform.Android.ndkPath, src.entries[i].text); // Path to Android NDK, required for C native building to Android
        else if (TextIsEqual(src.entries[i].key, "PLATFORM_ANDROID_JAVA_SDK_PATH")) strcpy(dst->Platform.Android.javaSdkPath, src.entries[i].text); // Path to Java SDK, required for some tools
        else if (TextIsEqual(src.entries[i].key, "PLATFORM_ANDROID_MANIFEST_FILE")) strcpy(dst->Platform.Android.manifestFile, src.entries[i].text); // Path to Android manifest, including build options
        else if (TextIsEqual(src.entries[i].key, "PLATFORM_ANDROID_MIN_SDK_VERSION")) dst->Platform.Android.minSdkVersion = src.entries[i].value; // Minimum SDK version required
        else if (TextIsEqual(src.entries[i].key, "PLATFORM_ANDROID_TARGET_SDK_VERSION")) dst->Platform.Android.targetSdkVersion = src.entries[i].value; // Target SDK version
        else if (TextIsEqual(src.entries[i].key, "PLATFORM_DRM_FLAG_CROSS_COMPILE")) dst->Platform.DRM.useCrossCompiler = src.entries[i].value; // Flag: request cross-compiler usage
        else if (TextIsEqual(src.entries[i].key, "PLATFORM_DRM_CROSS_COMPILER_PATH")) strcpy(dst->Platform.DRM.crossCompilerPath, src.entries[i].text); // Path to DRM cross-compiler for target ABI
        else if (TextIsEqual(src.entries[i].key, "PLATFORM_DREAMCAST_SDK_PATH")) strcpy(dst->Platform.Dreamcast.sdkPath, src.entries[i].text); // Path to Dreamcast SDK (KallistiOS), required for Dreamcast building
        // DEPLOY properties mapping
        else if (TextIsEqual(src.entries[i].key, "DEPLOY_FLAG_ZIP_PACKAGE")) dst->Deploy.zipPackage = src.entries[i].value; // Flag: request package to be zipped for distribution
        else if (TextIsEqual(src.entries[i].key, "DEPLOY_FLAG_RIF_INSTALLER")) dst->Deploy.rifInstaller = src.entries[i].value; // Flag: request installer creation using rInstallFriendly tool
        else if (TextIsEqual(src.entries[i].key, "DEPLOY_RIF_INSTALLER_PATH")) strcpy(dst->Deploy.rifInstallerPath, src.entries[i].text); // Path to [rInstallFriendly] tool
        else if (TextIsEqual(src.entries[i].key, "DEPLOY_FLAG_INCUDE_README")) dst->Deploy.includeREADME, src.entries[i].value; // Flag: include EULA file on package (vs LICENSE file for FOSS)
        else if (TextIsEqual(src.entries[i].key, "DEPLOY_README_FILE")) strcpy(dst->Deploy.readmePath, src.entries[i].text); // Project README document, contains product information
        else if (TextIsEqual(src.entries[i].key, "DEPLOY_FLAG_INCUDE_EULA")) dst->Deploy.includeEULA = src.entries[i].value; // Flag: include EULA file on package (vs LICENSE file for FOSS)
        else if (TextIsEqual(src.entries[i].key, "DEPLOY_EULA_FILE")) strcpy(dst->Deploy.eulaPath, src.entries[i].text); // Project End-User-License-Agreement
        // IMAGERY properties mapping
        else if (TextIsEqual(src.entries[i].key, "IMAGERY_LOGO_FILE")) strcpy(dst->Imagery.logoFile, src.entries[i].text); // Project logo image, useful for imagery generation
        else if (TextIsEqual(src.entries[i].key, "IMAGERY_SPLASH_FILE")) strcpy(dst->Imagery.splashFile, src.entries[i].text); // Project splash image, useful for imagery generation
        else if (TextIsEqual(src.entries[i].key, "IMAGERY_FLAG_GENERATE")) dst->Imagery.genImageryAuto = src.entries[i].value; // Flag: request project imagery generation: Social Cards, itchio, Steam...
    }
}

// Update property entry value and text
static void UpdateEntryValue(rpcPropertyEntry *entry, int value)
{
    entry->value = value;
    strcpy(entry->text, TextFormat("%i", value));
}

// Sync ProjectConfig data --> ProjectConfigRaw data
void SyncProjectConfigRaw(rpcProjectConfig *src, rpcProjectConfigRaw dst)
{
    // TODO: When updating dst.entries[i].value, should also update dst.entries[i].text?
    //strcpy(dst.entries[i].text, TextFormat("%i", src->Build.assetsValidation));

    for (int i = 0; i < dst.entryCount; i++)
    {
        // PROJECT properties mapping
        if (TextIsEqual(dst.entries[i].key, "PROJECT_INTERNAL_NAME")) strcpy(dst.entries[i].text, src->Project.internalName); // Project intenal name, used for executable and project files
        else if (TextIsEqual(dst.entries[i].key, "PROJECT_REPO_NAME")) strcpy(dst.entries[i].text, src->Project.repoName); // Project repository name, used for VCS (GitHub, GitLab)
        else if (TextIsEqual(dst.entries[i].key, "PROJECT_COMMERCIAL_NAME")) strcpy(dst.entries[i].text, src->Project.commercialName); // Project commercial name, used for docs and web
        else if (TextIsEqual(dst.entries[i].key, "PROJECT_SHORT_NAME")) strcpy(dst.entries[i].text, src->Project.shortName); // Project short name
        else if (TextIsEqual(dst.entries[i].key, "PROJECT_VERSION")) strcpy(dst.entries[i].text, src->Project.version); // Project version
        else if (TextIsEqual(dst.entries[i].key, "PROJECT_DESCRIPTION")) strcpy(dst.entries[i].text, src->Project.description); // Project description
        else if (TextIsEqual(dst.entries[i].key, "PROJECT_PUBLISHER_NAME")) strcpy(dst.entries[i].text, src->Project.publisherName); // Project publisher name
        else if (TextIsEqual(dst.entries[i].key, "PROJECT_DEVELOPER_NAME")) strcpy(dst.entries[i].text, src->Project.developerName); // Project developer name
        else if (TextIsEqual(dst.entries[i].key, "PROJECT_DEVELOPER_URL")) strcpy(dst.entries[i].text, src->Project.developerUrl); // Project developer webpage url
        else if (TextIsEqual(dst.entries[i].key, "PROJECT_DEVELOPER_EMAIL")) strcpy(dst.entries[i].text, src->Project.developerEmail); // Project developer email
        else if (TextIsEqual(dst.entries[i].key, "PROJECT_ICON_FILE")) strcpy(dst.entries[i].text, src->Project.iconFile); // Project icon file
        else if (TextIsEqual(dst.entries[i].key, "PROJECT_SOURCE_PATH")) strcpy(dst.entries[i].text, src->Project.sourcePath); // Project source directory, including all required code files (C/C++)
        else if (TextIsEqual(dst.entries[i].key, "PROJECT_ASSETS_PATH")) strcpy(dst.entries[i].text, src->Project.assetsPath); // Project assets directory, including all required assets
        else if (TextIsEqual(dst.entries[i].key, "PROJECT_ASSETS_OUTPUT_PATH")) strcpy(dst.entries[i].text, src->Project.assetsOutPath); // Project assets destination path
        // raylib properties mapping
        else if (TextIsEqual(dst.entries[i].key, "RAYLIB_SRC_PATH")) strcpy(dst.entries[i].text, src->raylib.srcPath); // Path to raylib source code, to be build for target platform
        else if (TextIsEqual(dst.entries[i].key, "RAYLIB_OPENGL_VERSION")) strcpy(dst.entries[i].text, src->raylib.glVersion); // OpenGL version to be used by raylib, WARNING: Platform dependant!
        // BUILD properties mapping
        else if (TextIsEqual(dst.entries[i].key, "BUILD_OUTPUT_PATH")) strcpy(dst.entries[i].text, src->Build.outputPath); // Build output path
        else if (TextIsEqual(dst.entries[i].key, "BUILD_TARGET_PLATFORM")) strcpy(dst.entries[i].text, src->Build.targetPlatform); // Build target platform (Supported: Windows, Linux, macOS, Android, Web)
        else if (TextIsEqual(dst.entries[i].key, "BUILD_TARGET_ARCHITECTURE")) strcpy(dst.entries[i].text, src->Build.targetArchitecture); // Build target architecture (Supported: x86-64, Win32, arm64)
        else if (TextIsEqual(dst.entries[i].key, "BUILD_TARGET_MODE")) strcpy(dst.entries[i].text, src->Build.targetMode); // Build target mode (Supported: DEBUG, RELEASE, DEBUG_DLL, RELEASE_DLL)
        else if (TextIsEqual(dst.entries[i].key, "BUILD_FLAG_ASSETS_VALIDATION")) UpdateEntryValue(&dst.entries[i], src->Build.assetsValidation); // Flag: request assets validation on building
        else if (TextIsEqual(dst.entries[i].key, "BUILD_FLAG_ASSETS_PACKAGING")) UpdateEntryValue(&dst.entries[i], src->Build.assetsPackaging); // Flag: request assets packaging on building
        // PLATFORM properties mapping
        else if (TextIsEqual(dst.entries[i].key, "PLATFORM_WINDOWS_MSBUILD_PATH")) strcpy(dst.entries[i].text, src->Platform.Windows.msbuildPath); // Path to MSBuild system, required to build VS2022 solution
        else if (TextIsEqual(dst.entries[i].key, "PLATFORM_WINDOWS_W64DEVKIT_PATH")) strcpy(dst.entries[i].text, src->Platform.Windows.w64devkitPath); // Path to w64devkit (GCC), required to use Makefile building
        else if (TextIsEqual(dst.entries[i].key, "PLATFORM_WINDOWS_SIGNTOOL_PATH")) strcpy(dst.entries[i].text, src->Platform.Windows.signtoolPath); // Path to signtool in case program needs to be signed (certificate required)
        else if (TextIsEqual(dst.entries[i].key, "PLATFORM_WINDOWS_SIGNCERT_FILE")) strcpy(dst.entries[i].text, src->Platform.Windows.signCertFile); // Path to a valid signature certificate to sign executable
        else if (TextIsEqual(dst.entries[i].key, "PLATFORM_LINUX_FLAG_CROSS_COMPILE")) UpdateEntryValue(&dst.entries[i], src->Platform.Linux.useCrossCompiler); // Flag: request cross-compiler usage
        else if (TextIsEqual(dst.entries[i].key, "PLATFORM_LINUX_CROSS_COMPILER_PATH")) strcpy(dst.entries[i].text, src->Platform.Linux.crossCompilerPath); // Path to GCC compiler (probably not required)
        else if (TextIsEqual(dst.entries[i].key, "PLATFORM_MACOS_BUNDLE_INFO_FILE")) strcpy(dst.entries[i].text, src->Platform.macOS.bundleInfoFile); // Path to macOS bundle options (Info.plist)
        else if (TextIsEqual(dst.entries[i].key, "PLATFORM_MACOS_BUNDLE_NAME")) strcpy(dst.entries[i].text, src->Platform.macOS.bundleName); // Bundle name
        else if (TextIsEqual(dst.entries[i].key, "PLATFORM_MACOS_BUNDLE_VERSION")) strcpy(dst.entries[i].text, src->Platform.macOS.bundleVersion); // Bundle version
        else if (TextIsEqual(dst.entries[i].key, "PLATFORM_HTML5_EMSDK_PATH")) strcpy(dst.entries[i].text, src->Platform.HTML5.emsdkPath); // Path to emsdk, required for Web building
        else if (TextIsEqual(dst.entries[i].key, "PLATFORM_HTML5_SHELL_FILE")) strcpy(dst.entries[i].text, src->Platform.HTML5.shellFile); // Path to shell file to be used by emscripten
        else if (TextIsEqual(dst.entries[i].key, "PLATFORM_HTML5_HEAP_MEMORY_SIZE")) UpdateEntryValue(&dst.entries[i], src->Platform.HTML5.heapMemorySize); // Required heap memory size in MB (required for assets loading)
        else if (TextIsEqual(dst.entries[i].key, "PLATFORM_HTML5_FLAG_USE_ASINCIFY")) UpdateEntryValue(&dst.entries[i], src->Platform.HTML5.useAsincify); // Flag: use ASINCIFY mode on building
        else if (TextIsEqual(dst.entries[i].key, "PLATFORM_HTML5_FLAG_USE_WEBGL2")) UpdateEntryValue(&dst.entries[i], src->Platform.HTML5.useWebGL2); // Flag: use WebGL2 (OpenGL ES 3.1) instead of default WebGL1 (OpenGL ES 2.0)
        else if (TextIsEqual(dst.entries[i].key, "PLATFORM_ANDROID_SDK_PATH")) strcpy(dst.entries[i].text, src->Platform.Android.sdkPath); // Path to Android SDK, required for Android App building and support tools
        else if (TextIsEqual(dst.entries[i].key, "PLATFORM_ANDROID_NDK_PATH")) strcpy(dst.entries[i].text, src->Platform.Android.ndkPath); // Path to Android NDK, required for C native building to Android
        else if (TextIsEqual(dst.entries[i].key, "PLATFORM_ANDROID_JAVA_SDK_PATH")) strcpy(dst.entries[i].text, src->Platform.Android.javaSdkPath); // Path to Java SDK, required for some tools
        else if (TextIsEqual(dst.entries[i].key, "PLATFORM_ANDROID_MANIFEST_FILE")) strcpy(dst.entries[i].text, src->Platform.Android.manifestFile); // Path to Android manifest, including build options
        else if (TextIsEqual(dst.entries[i].key, "PLATFORM_ANDROID_MIN_SDK_VERSION")) UpdateEntryValue(&dst.entries[i], src->Platform.Android.minSdkVersion); // Minimum SDK version required
        else if (TextIsEqual(dst.entries[i].key, "PLATFORM_ANDROID_TARGET_SDK_VERSION")) UpdateEntryValue(&dst.entries[i], src->Platform.Android.targetSdkVersion); // Target SDK version
        else if (TextIsEqual(dst.entries[i].key, "PLATFORM_DRM_FLAG_CROSS_COMPILE")) UpdateEntryValue(&dst.entries[i], src->Platform.DRM.useCrossCompiler); // Flag: request cross-compiler usage
        else if (TextIsEqual(dst.entries[i].key, "PLATFORM_DRM_CROSS_COMPILER_PATH")) strcpy(dst.entries[i].text, src->Platform.DRM.crossCompilerPath); // Path to DRM cross-compiler for target ABI
        else if (TextIsEqual(dst.entries[i].key, "PLATFORM_DREAMCAST_SDK_PATH")) strcpy(dst.entries[i].text, src->Platform.Dreamcast.sdkPath); // Path to Dreamcast SDK (KallistiOS), required for Dreamcast building
        // DEPLOY properties mapping
        else if (TextIsEqual(dst.entries[i].key, "DEPLOY_FLAG_ZIP_PACKAGE")) UpdateEntryValue(&dst.entries[i], src->Deploy.zipPackage); // Flag: request package to be zipped for distribution
        else if (TextIsEqual(dst.entries[i].key, "DEPLOY_FLAG_RIF_INSTALLER")) UpdateEntryValue(&dst.entries[i], src->Deploy.rifInstaller); // Flag: request installer creation using rInstallFriendly tool
        else if (TextIsEqual(dst.entries[i].key, "DEPLOY_RIF_INSTALLER_PATH")) strcpy(dst.entries[i].text, src->Deploy.rifInstallerPath); // Path to [rInstallFriendly] tool
        else if (TextIsEqual(dst.entries[i].key, "DEPLOY_FLAG_INCUDE_README")) UpdateEntryValue(&dst.entries[i], src->Deploy.includeREADME); // Flag: include EULA file on package (vs LICENSE file for FOSS)
        else if (TextIsEqual(dst.entries[i].key, "DEPLOY_README_FILE")) strcpy(dst.entries[i].text, src->Deploy.readmePath); // Project README document, contains product information
        else if (TextIsEqual(dst.entries[i].key, "DEPLOY_FLAG_INCUDE_EULA")) UpdateEntryValue(&dst.entries[i], src->Deploy.includeEULA); // Flag: include EULA file on package (vs LICENSE file for FOSS)
        else if (TextIsEqual(dst.entries[i].key, "DEPLOY_EULA_FILE")) strcpy(dst.entries[i].text, src->Deploy.eulaPath); // Project End-User-License-Agreement
        // IMAGERY properties mapping
        else if (TextIsEqual(dst.entries[i].key, "IMAGERY_LOGO_FILE")) strcpy(dst.entries[i].text, src->Imagery.logoFile); // Project logo image, useful for imagery generation
        else if (TextIsEqual(dst.entries[i].key, "IMAGERY_SPLASH_FILE")) strcpy(dst.entries[i].text, src->Imagery.splashFile); // Project splash image, useful for imagery generation
        else if (TextIsEqual(dst.entries[i].key, "IMAGERY_FLAG_GENERATE")) UpdateEntryValue(&dst.entries[i], src->Imagery.genImageryAuto); // Flag: request project imagery generation: Social Cards, itchio, Steam...
    }
}

#endif // RPCDATA_IMPLEMENTATION
