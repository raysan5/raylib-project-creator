/*******************************************************************************************
*
*   rpc - raylib project config data types and functionality
*
*   NOTE: This header types and functions must be shared by [rpc] and [rpb] tools for consitency
*
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2025-2026 raylib technologies (@raylibtech) / Ramon Santamaria (@raysan5)
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

#ifndef RPCONFIG_H
#define RPCONFIG_H

#include "raylib.h"     // Required for: Image type

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

// Property category type
typedef enum {
    RPC_CAT_NONE = 0,
    RPC_CAT_PROJECT,
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
    RPC_PLATFORM_NONE = -1,
    RPC_PLATFORM_WINDOWS = 0,
    RPC_PLATFORM_LINUX,
    RPC_PLATFORM_MACOS,
    RPC_PLATFORM_WASM,
    RPC_PLATFORM_ANDROID,
    RPC_PLATFORM_FREEBSD,
    RPC_PLATFORM_DRM,
    RPC_PLATFORM_ESP32,
    RPC_PLATFORM_DREAMCAST,
    RPC_PLATFORM_SWITCH,
    RPC_PLATFORM_ANY
} rpcPlatform;

// Project config property entry
// NOTE: Useful to automatice UI generation,
// every data entry is read from rpc config file
typedef struct {
    char key[64];       // Entry key (as read from .rpc)
    char text[256];     // Entry text data (type: TEXT, FILE, PATH) - WARNING: Max len defined for rini
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
    int capacity;       // Entries capacity
    int entryCount;     // Number of entries used
    rpcPropertyEntry *entries; // Entries
} rpcProjectConfig;

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------

#if defined(__cplusplus)
extern "C" {    // Prevents name mangling of functions
#endif

RPCAPI rpcProjectConfig rpcLoadProjectConfig(const char *fileName); // Load project config data from .rpc file
RPCAPI void rpcUnloadProjectConfig(rpcProjectConfig config); // Unload project config data
RPCAPI void rpcSaveProjectConfig(rpcProjectConfig config, const char *fileName, int flags); // Save project config data to .rpc file

RPCAPI char *rpcGetText(rpcProjectConfig config, const char *key); // Get project config text by key
RPCAPI int rpcSetText(rpcProjectConfig config, const char *key, const char *text); // Set project config text by key
RPCAPI int rpcGetValue(rpcProjectConfig config, const char *key); // Get project config pointer to value by key
RPCAPI int rpcSetValue(rpcProjectConfig config, const char *key, int value); // Set project config value by key
RPCAPI rpcPropertyEntry *rpcGetPropertyEntry(rpcProjectConfig config, const char *key); // Get project property entry from key
RPCAPI int rpcSetPropertyEntry(rpcProjectConfig config, rpcPropertyEntry *entry); // Set project property entry (only if entry->key is found)

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

// NOTE: Requires rini.h being included before this header

#include <string.h>     // Required for: strncpy()
#include <stdlib.h>     // Required for: calloc(), free()

// Load project config data from .rpc file
// NOTE: Data is parsed to organize by type of entry
rpcProjectConfig rpcLoadProjectConfig(const char *fileName)
{
    rpcProjectConfig project = { 0 };

    if (FileExists(fileName))
    {
        rini_data config = { 0 };
        config = rini_load(fileName);

        project.capacity = RPC_MAX_PROPERTY_ENTRIES;
        project.entries = (rpcPropertyEntry *)RL_CALLOC(project.capacity, sizeof(rpcPropertyEntry));
        project.entryCount = config.count;

        for (unsigned int i = 0; i < config.count; i++)
        {
            TextCopy(project.entries[i].key, config.entries[i].key);
            TextCopy(project.entries[i].desc, config.entries[i].desc);
            project.entries[i].platform = RPC_PLATFORM_ANY;

            // Category is parsed from first word on key
            char category[32] = { 0 };
            int categoryLen = 0; //TextFindIndex(config.values[i].key, "_");
            for (int c = 0; c < 128; c++) { if (config.entries[i].key[c] != '_') categoryLen++; else break; }
            strncpy(category, config.entries[i].key, categoryLen);
            TextCopy(project.entries[i].name, TextReplace(config.entries[i].key + categoryLen + 1, "_", " "));

            if (TextIsEqual(category, "PROJECT")) project.entries[i].category = RPC_CAT_PROJECT;
            else if (TextIsEqual(category, "BUILD")) project.entries[i].category = RPC_CAT_BUILD;
            else if (TextIsEqual(category, "PLATFORM"))
            {
                project.entries[i].category = RPC_CAT_PLATFORM;

                // Get platform from key
                char platform[32] = { 0 };
                int platformLen = 0;//TextFindIndex(config.values[i].key + categoryLen + 1, "_");
                for (int c = 0; c < 128; c++) { if (config.entries[i].key[c + categoryLen + 1] != '_') platformLen++; else break; }
                memcpy(platform, config.entries[i].key + categoryLen + 1, platformLen);

                if (TextIsEqual(platform, "WINDOWS")) project.entries[i].platform = RPC_PLATFORM_WINDOWS;
                else if (TextIsEqual(platform, "LINUX")) project.entries[i].platform = RPC_PLATFORM_LINUX;
                else if (TextIsEqual(platform, "MACOS")) project.entries[i].platform = RPC_PLATFORM_MACOS;
                else if (TextIsEqual(platform, "WEB")) project.entries[i].platform = RPC_PLATFORM_WASM;
                else if (TextIsEqual(platform, "ANDROID")) project.entries[i].platform = RPC_PLATFORM_ANDROID;
                else if (TextIsEqual(platform, "DRM")) project.entries[i].platform = RPC_PLATFORM_DRM;
                else if (TextIsEqual(platform, "SWITCH")) project.entries[i].platform = RPC_PLATFORM_SWITCH;
                else if (TextIsEqual(platform, "DREAMCAST")) project.entries[i].platform = RPC_PLATFORM_DREAMCAST;
                else if (TextIsEqual(platform, "FREEBSD")) project.entries[i].platform = RPC_PLATFORM_FREEBSD;

                memset(project.entries[i].name, 0, 64);
                TextCopy(project.entries[i].name, config.entries[i].key + categoryLen + platformLen + 2);
            }
            else if (TextIsEqual(category, "DEPLOY")) project.entries[i].category = RPC_CAT_DEPLOY;
            else if (TextIsEqual(category, "IMAGERY")) project.entries[i].category = RPC_CAT_IMAGERY;
            else if (TextIsEqual(category, "RAYLIB")) project.entries[i].category = RPC_CAT_RAYLIB;
        }

        for (unsigned int i = 0; i < config.count; i++)
        {
            // Type is parsed from key and value
            if (!config.entries[i].is_text)
            {
                if (TextFindIndex(project.entries[i].key, "_FLAG")) project.entries[i].type = RPC_TYPE_BOOL;
                else project.entries[i].type = RPC_TYPE_VALUE;

                // Get the value
                project.entries[i].value = TextToInteger(config.entries[i].text);
            }
            else // Value is text
            {
                if (TextFindIndex(project.entries[i].key, "_FILES") > 0)
                {
                    // TODO: How we check if files list includes multiple files,
                    // checking for ';' separator???
                    project.entries[i].type = RPC_TYPE_TEXT_FILE;
                }
                else if (TextFindIndex(project.entries[i].key, "_FILE")  > 0) project.entries[i].type = RPC_TYPE_TEXT_FILE;
                else if (TextFindIndex(project.entries[i].key, "_PATH")  > 0) project.entries[i].type = RPC_TYPE_TEXT_PATH;
                else
                {
                    project.entries[i].type = RPC_TYPE_TEXT;
                }

                TextCopy(project.entries[i].text, config.entries[i].text);
            }
        }

        rini_unload(&config);
    }

    return project;
}

// Unload project data
void rpcUnloadProjectConfig(rpcProjectConfig config)
{
    RL_FREE(config.entries);
}

// Save project config data to .rpc file
// NOTE: Same function as [rpc] tool but adding more data
void rpcSaveProjectConfig(rpcProjectConfig config, const char *fileName, int flags)
{
    rini_data data = rini_load(NULL);   // Create empty config with RINI_MAX_VALUE_CAPACITY entries

    // Define header comment lines
    rini_set_comment_line(&data, NULL); // Empty comment line, but including comment prefix delimiter
    rini_set_comment_line(&data, "raylib project configuration");
    rini_set_comment_line(&data, NULL);
    rini_set_comment_line(&data, "This file contains all required data to define a raylib C/C++ project");
    rini_set_comment_line(&data, "and allow building it for multiple platforms using [rpb] tool");
    rini_set_comment_line(&data, NULL);
    rini_set_comment_line(&data, "Project configuration is organized in several categories, depending on usage requirements");
    rini_set_comment_line(&data, "CATEGORIES:");
    rini_set_comment_line(&data, "   - PROJECT: Project definition properties, required for project generation");
    rini_set_comment_line(&data, "   - raylib: Library configuration properties, raylib customization for the project");
    rini_set_comment_line(&data, "   - BUILD: Project build properties, required for project building, generic for all platforms");
    rini_set_comment_line(&data, "   - PLATFORM: Platform-specific properies, required for building for that platform");
    rini_set_comment_line(&data, "       Platform identifiers: WINDOWS, LINUX, MACOS, HTML5, ANDROID, DRM, FREEBSD, DREAMCAST");
    rini_set_comment_line(&data, "   - DEPLOY: Deployment properties, required to distribute the generated build");
    rini_set_comment_line(&data, "   - IMAGERY: Project imagery properties, required for distribution on some stores and marketing");
    rini_set_comment_line(&data, NULL);
    rini_set_comment_line(&data, "This file follow certain conventions to be able to display the information in");
    rini_set_comment_line(&data, "an easy-configurable UI manner when loaded through [rpb - raylib project builder] tool");
    rini_set_comment_line(&data, "CONVENTIONS:");
    rini_set_comment_line(&data, "   - ID containing [_FLAG_]: Value is considered a boolean, it displays with a [GuiCheckBox]");
    rini_set_comment_line(&data, "   - ID do not contain "": Value is considered as an integer, it displays as [GuiValueBox]");
    rini_set_comment_line(&data, "   - ID ends with _FILE or _FILES: Value is considered as a text file path, it displays as [GuiTextBox] with a [BROWSE-File] button");
    rini_set_comment_line(&data, "   - ID ends with _PATH: Value is considered as a text directory path, it displays as [GuiTextBox] with a [BROWSE-Dir] button");
    rini_set_comment_line(&data, NULL);
    rini_set_comment_line(&data, "NOTE: The comments/description for each entry is used as tooltip when editing the entry on [rpb]");
    rini_set_comment_line(&data, "\n");

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
    rini_set_comment_line(&data, "Project settings");
    rini_set_comment_line(&data, "------------------------------------------------------------------------------------");
    for (int i = 0; i < config.entryCount; i++)
    {
        rpcPropertyEntry *entry = &config.entries[i];

        if (entry->category == RPC_CAT_PROJECT)
        {
            switch (entry->type)
            {
                case RPC_TYPE_BOOL:
                case RPC_TYPE_VALUE: rini_set_value(&data, entry->key, entry->value, entry->desc); break;
                case RPC_TYPE_TEXT:
                case RPC_TYPE_TEXT_FILE:
                case RPC_TYPE_TEXT_PATH: rini_set_value_text(&data, entry->key, entry->text, entry->desc); break;
                default: break;
            }
        }
    }
    rini_set_comment_line(&data, "------------------------------------------------------------------------------------\n");

    rini_set_comment_line(&data, "raylib config settings");
    rini_set_comment_line(&data, "------------------------------------------------------------------------------------");
    for (int i = 0; i < config.entryCount; i++)
    {
        rpcPropertyEntry *entry = &config.entries[i];

        if (entry->category == RPC_CAT_RAYLIB)
        {
            switch (entry->type)
            {
                case RPC_TYPE_BOOL:
                case RPC_TYPE_VALUE: rini_set_value(&data, entry->key, entry->value, entry->desc); break;
                case RPC_TYPE_TEXT:
                case RPC_TYPE_TEXT_FILE:
                case RPC_TYPE_TEXT_PATH: rini_set_value_text(&data, entry->key, entry->text, entry->desc); break;
                default: break;
            }
        }
    }
    rini_set_comment_line(&data, "------------------------------------------------------------------------------------\n");

    // Saving BUILD category data
    rini_set_comment_line(&data, "Build settings");
    rini_set_comment_line(&data, "NOTE: General settings, common to all platforms");
    rini_set_comment_line(&data, "------------------------------------------------------------------------------------");
    for (int i = 0; i < config.entryCount; i++)
    {
        rpcPropertyEntry *entry = &config.entries[i];

        if (entry->category == RPC_CAT_BUILD)
        {
            switch (entry->type)
            {
                case RPC_TYPE_BOOL:
                case RPC_TYPE_VALUE: rini_set_value(&data, entry->key, entry->value, entry->desc); break;
                case RPC_TYPE_TEXT:
                case RPC_TYPE_TEXT_FILE:
                case RPC_TYPE_TEXT_PATH: rini_set_value_text(&data, entry->key, entry->text, entry->desc); break;
                default: break;
            }
        }
    }
    rini_set_comment_line(&data, "------------------------------------------------------------------------------------\n");

    // Saving PLATFORM category data
    rini_set_comment_line(&data, "Platform-specific build settings");
    rini_set_comment_line(&data, "------------------------------------------------------------------------------------");
    for (int i = 0, prevPlatform = RPC_PLATFORM_WINDOWS; i < config.entryCount; i++)
    {
        rpcPropertyEntry *entry = &config.entries[i];

        if (entry->category == RPC_CAT_PLATFORM)
        {
            // Add line separator beetween platforms
            if ((entry->platform != RPC_PLATFORM_ANY) &&
                (entry->platform != prevPlatform))
            {
                rini_set_comment_line(&data, "");
                prevPlatform = entry->platform;
            }

            switch (entry->type)
            {
                case RPC_TYPE_BOOL:
                case RPC_TYPE_VALUE: rini_set_value(&data, entry->key, entry->value, entry->desc); break;
                case RPC_TYPE_TEXT:
                case RPC_TYPE_TEXT_FILE:
                case RPC_TYPE_TEXT_PATH: rini_set_value_text(&data, entry->key, entry->text, entry->desc); break;
                default: break;
            }
        }
    }
    rini_set_comment_line(&data, "------------------------------------------------------------------------------------\n");

    // Saving DEPLOY category data
    rini_set_comment_line(&data, "Deploy settings");
    rini_set_comment_line(&data, "------------------------------------------------------------------------------------");
    for (int i = 0; i < config.entryCount; i++)
    {
        rpcPropertyEntry *entry = &config.entries[i];

        if (entry->category == RPC_CAT_DEPLOY)
        {
            switch (entry->type)
            {
                case RPC_TYPE_BOOL:
                case RPC_TYPE_VALUE: rini_set_value(&data, entry->key, entry->value, entry->desc); break;
                case RPC_TYPE_TEXT:
                case RPC_TYPE_TEXT_FILE:
                case RPC_TYPE_TEXT_PATH: rini_set_value_text(&data, entry->key, entry->text, entry->desc); break;
                default: break;
            }
        }
    }
    rini_set_comment_line(&data, "------------------------------------------------------------------------------------\n");

    // Saving IMAGERY category data
    rini_set_comment_line(&data, "Imagery settings");
    rini_set_comment_line(&data, "NOTE: Useful for project distribution on several stores");
    rini_set_comment_line(&data, "------------------------------------------------------------------------------------");
    for (int i = 0; i < config.entryCount; i++)
    {
        rpcPropertyEntry *entry = &config.entries[i];

        if (entry->category == RPC_CAT_IMAGERY)
        {
            switch (entry->type)
            {
                case RPC_TYPE_BOOL:
                case RPC_TYPE_VALUE: rini_set_value(&data, entry->key, entry->value, entry->desc); break;
                case RPC_TYPE_TEXT:
                case RPC_TYPE_TEXT_FILE:
                case RPC_TYPE_TEXT_PATH: rini_set_value_text(&data, entry->key, entry->text, entry->desc); break;
                default: break;
            }
        }
    }
    rini_set_comment_line(&data, "------------------------------------------------------------------------------------\n");

    rini_save(data, fileName);
    rini_unload(&data);
}

// Get project config text by key
// NOTE: A pointer to the text is returned to allow modifying it (instead of a text copy)
char *rpcGetText(rpcProjectConfig config, const char *key)
{
    char *text = NULL;

    for (int i = 0; i < config.entryCount; i++)
    {
        if (TextIsEqual(config.entries[i].key, key)) text = config.entries[i].text;
    }

    return text;
}

// Set project config text by key
// WARNING: Only entries[i].text is set but key not parsed to fill additional data
int rpcSetText(rpcProjectConfig config, const char *key, const char *text)
{
    int result = -1;

    for (int i = 0; i < config.entryCount; i++)
    {
        if (TextIsEqual(config.entries[i].key, key) && 
            !TextIsEqual(config.entries[i].text, text))
        {
            memset(config.entries[i].text, 0, 256);
            strcpy(config.entries[i].text, text);
            result = i;
            break;
        }
    }

    return result;
}

// Get project config value by key
int rpcGetValue(rpcProjectConfig config, const char *key)
{
    int value = -1;

    for (int i = 0; i < config.entryCount; i++)
    {
        if (TextIsEqual(config.entries[i].key, key)) value = config.entries[i].value;
    }

    return value;
}

// Set project config value by key
// WARNING: Only entries[i].value/text is set but key not parsed to fill additional data
int rpcSetValue(rpcProjectConfig config, const char *key, int value)
{
    int result = -1;

    for (int i = 0; i < config.entryCount; i++)
    {
        if (TextIsEqual(config.entries[i].key, key))
        {
            config.entries[i].value = value;
            strcpy(config.entries[i].text, TextFormat("%i", value));
            result = i;
            break;
        }
    }

    return result;
}

// Get project property entry
rpcPropertyEntry *rpcGetPropertyEntry(rpcProjectConfig config, const char *key)
{
    for (int i = 0; i < config.entryCount; i++)
    {
        if (TextIsEqual(config.entries[i].key, key)) return &config.entries[i];
    }

    return NULL;
}

// Set project property entry (only if entry->key is found)
int rpcSetPropertyEntry(rpcProjectConfig config, rpcPropertyEntry *entry)
{
    int result = -1;

    for (int i = 0; i < config.entryCount; i++)
    {
        // Update property entry if key found and different content
        if (TextIsEqual(config.entries[i].key, entry->key) &&
            (memcmp(&config.entries[i], entry, sizeof(rpcPropertyEntry)) != 0))
        {
            memset(&config.entries[i], 0, sizeof(rpcPropertyEntry));

            memcpy(config.entries[i].key, entry->key, 64);      // Entry key (as read from .rpc)
            memcpy(config.entries[i].text, entry->text, 256);   // Entry text data (type: TEXT, FILE, PATH) - WARNING: Max len defined for rini
            memcpy(config.entries[i].desc, entry->desc, 128);   // Entry data description, useful for tooltips

            // Data extracted from key
            memcpy(config.entries[i].name, entry->name, 64);    // Entry name label for display, computed from key
            config.entries[i].category = entry->category;       // Entry category: PROJECT, BUILDING, PLATFORM, DEPLOY, IMAGERY, raylib
            config.entries[i].platform = entry->platform;       // Entry platform: WINDOWS, LINUX, MACOS, HTML5, ANDROID, DRM, SWITCH, DREAMCAST, FREEBSD...
            config.entries[i].type = entry->type;               // Entry type of data: VALUE (int), BOOL (int), TEXT (string), FILE (string-file), PATH (string-path)
            config.entries[i].value = entry->value;             // Entry value, integer from text

            result = i;
            break;
        }
    }

    return result;
}

#endif // RPCDATA_IMPLEMENTATION
