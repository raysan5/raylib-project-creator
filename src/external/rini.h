/**********************************************************************************************
*
*   rini v3.0 - A simple and easy-to-use ini-style files reader and writer
*
*   DESCRIPTION:
*       Load and save ini-style files with keys and values
*
*   FEATURES:
*       - Init/Config files reading and writing
*       - Supported value types: int, string
*       - Support comment lines and empty lines
*       - Support custom line comment delimiter
*       - Support custom value delimiters
*       - Support value description comments
*       - Support custom description custom delimiter
*       - Support multi-word text values w/o quote delimiters
*       - Support custom key and value spacings
*       - Customizable maximum values capacity
*       - Minimal C standard lib dependency (optional)
*
*   LIMITATIONS:
*       - [sections] lines not supported
*       - Saving file requires complete rewrite
*
*   POSSIBLE IMPROVEMENTS:
*       - Support disabled key-value entries
*
*   CONFIGURATION:
*       #define RINI_IMPLEMENTATION
*           Generates the implementation of the library into the included file.
*           If not defined, the library is in header only mode and can be included in other headers
*           or source files without problems. But only ONE file should hold the implementation.
*
*       #define RINI_MAX_LINE_SIZE
*           Defines the maximum size of line buffer to read from file.
*           Default value: 512 bytes (considering [key + text + desc] is 256 max size by default)
*
*       #define RINI_MAX_KEY_SIZE
*           Defines the maximum size of value key
*           Default value: 64 bytes
*
*       #define RINI_MAX_TEXT_SIZE
*           Defines the maximum size of value text
*           Default value: 128 bytes
*
*       #define RINI_MAX_DESC_SIZE
*           Defines the maximum size of value description
*           Default value: 128 bytes
*
*       #define RINI_MAX_VALUE_CAPACITY
*           Defines the maximum number of values supported
*           Default value: 128 entries support
*
*       #define RINI_LINE_COMMENT_DELIMITER
*           Define character used to comment lines, placed at beginning of line
*           Most .ini files use semicolon ';' but '#' is also used
*           Default value: '#'
*
*       #define RINI_LINE_SECTION_DELIMITER
*           Defines section lines start character
*           Sections loading is not supported, lines are just skipped for now
*           Default value: '['
*
*       #define RINI_VALUE_DELIMITER
*           Defines a key value delimiter, in case it is defined.
*           Most .ini files use '=' as value delimiter but ':' is also used or just whitespace
*           Default value: ' '
*
*       #define RINI_VALUE_QUOTATION_MARKS
*           Defines quotation marks to be used around text values 
*           Text values are determined checking text with atoi(), only for integer values,
*           in case of float values they are always considered as text
*           Default value: '\"'
*
*       #define RINI_DESCRIPTION_DELIMITER
*           Defines a property line-end comment delimiter
*           This implementation allows adding inline comments after the value.
*           Default value: '#'
*
*   DEPENDENCIES: C standard library:
*       - stdio.h: fopen(), feof(), fgets(), fclose(), fprintf()
*       - stdlib.h: malloc(), calloc(), free()
*       - string.h: memset(), memcpy(), strcmp(), strlen()
*
*   VERSIONS HISTORY:
*       3.0 (xx-May-2025) REDESIGN: BREAKING: Avoid the _config_ in naming
*                         ADDED: Property to set entries as text or value
*                         ADDED: Key and Value spacing defines
*
*       2.0 (26-Jan-2024) ADDED: Support custom comment lines (as config entries)
*                         ADDED: Use of quotation-marks and marks customization
*                         ADDED: rini_set_config_comment_line()
*                         ADDED: rini_get_config_value_fallback(), with fallback return value
*                         ADDED: rini_get_config_text_fallback(), with fallback return value
*                         REMOVED: Config header requirement, use comments entries
*                         REVIEWED: Some configuration default values, capacities
*                         REVIEWED: rini_save_config(), removed parameter
*                         REVIEWED: rini_save_config_from_memory(), removed parameter
*
*       1.0 (18-May-2023) First release, basic read/write functionality
*
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2023-2025 Ramon Santamaria (@raysan5)
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

#ifndef RINI_H
#define RINI_H

#define RINI_VERSION    "3.0"

// Function specifiers in case library is build/used as a shared library (Windows)
// NOTE: Microsoft specifiers to tell compiler that symbols are imported/exported from a .dll
#if defined(_WIN32)
    #if defined(BUILD_LIBTYPE_SHARED)
        #define RINIAPI __declspec(dllexport)     // We are building the library as a Win32 shared library (.dll)
    #elif defined(USE_LIBTYPE_SHARED)
        #define RINIAPI __declspec(dllimport)     // We are using the library as a Win32 shared library (.dll)
    #endif
#endif

// Function specifiers definition
#ifndef RINIAPI
    #define RINIAPI       // Functions defined as 'extern' by default (implicit specifiers)
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
// Allow custom memory allocators
#ifndef RINI_MALLOC
    #define RINI_MALLOC(sz)       malloc(sz)
#endif
#ifndef RINI_CALLOC
    #define RINI_CALLOC(n,sz)     calloc(n,sz)
#endif
#ifndef RINI_FREE
    #define RINI_FREE(p)          free(p)
#endif

// Simple log system to avoid printf() calls if required
// NOTE: Avoiding those calls, also avoids const strings memory usage
#define RINI_SUPPORT_LOG_INFO
#if defined(RINI_SUPPORT_LOG_INFO)
  #define RINI_LOG(...)           printf(__VA_ARGS__)
#else
  #define RINI_LOG(...)
#endif

#if !defined(RINI_MAX_LINE_SIZE)
    #define RINI_MAX_LINE_SIZE              512
#endif

#if !defined(RINI_MAX_KEY_SIZE)
    #define RINI_MAX_KEY_SIZE                64
#endif

#if !defined(RINI_MAX_TEXT_SIZE)
    #define RINI_MAX_TEXT_SIZE              128
#endif

#if !defined(RINI_MAX_DESC_SIZE)
    #define RINI_MAX_DESC_SIZE              128
#endif

#if !defined(RINI_MAX_VALUE_CAPACITY)
    #define RINI_MAX_VALUE_CAPACITY         128
#endif

// Total space reserved for Key,
// Value starts after this spacing
#if !defined(RINI_KEY_SPACING)
    #define RINI_KEY_SPACING                 36
#endif
// Total space reserved for Value,
// Description starts after this spacing
#if !defined(RINI_VALUE_SPACING)
    #define RINI_VALUE_SPACING               32
#endif

// Line comment delimiter (starting string)
#if !defined(RINI_LINE_COMMENT_DELIMITER)
    #define RINI_LINE_COMMENT_DELIMITER     '#'
#endif

// Line section delimiter -NOT USED-
#if !defined(RINI_LINE_SECTION_DELIMITER)
    #define RINI_LINE_SECTION_DELIMITER     '['
#endif

// Value delimiter, separator between key and value
#if !defined(RINI_VALUE_DELIMITER)
    #define RINI_VALUE_DELIMITER            ' '
#endif

// Use quotation marks for text values
// NOTE: Integer values do not use quotation-marks
#define RINI_USE_TEXT_QUOTATION_MARKS         1
// Text value quoation marks
#if !defined(RINI_VALUE_QUOTATION_MARKS)
    #define RINI_VALUE_QUOTATION_MARKS      '\"'
#endif

// Description delimiter, separator between value and description
#if !defined(RINI_DESCRIPTION_DELIMITER)
    #define RINI_DESCRIPTION_DELIMITER      '#'
#endif

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// rini value entry
typedef struct {
    char key[RINI_MAX_KEY_SIZE];    // Value key identifier
    char text[RINI_MAX_TEXT_SIZE];  // Value text
    char desc[RINI_MAX_DESC_SIZE];  // Value description
    bool isText;                    // Value should be considered as text 
} rini_value;

// rini data
typedef struct {
    rini_value *values;         // Values array
    unsigned int count;         // Values count
    unsigned int capacity;      // Values capacity
} rini_data;

#if defined(__cplusplus)
extern "C" {                    // Prevents name mangling of functions
#endif

//------------------------------------------------------------------------------------
// Functions declaration
//------------------------------------------------------------------------------------
RINIAPI rini_data rini_load(const char *file_name);            // Load data from file (*.ini) or create a new rini object (pass NULL)
RINIAPI rini_data rini_load_from_memory(const char *text);     // Load data from text buffer
RINIAPI void rini_save(rini_data data, const char *file_name); // Save data to file, with custom header
RINIAPI char *rini_save_to_memory(rini_data data);           // Save data to text buffer ('\0' EOL)
RINIAPI void rini_unload(rini_data *data);                   // Unload data from memory

RINIAPI int rini_get_value(rini_data data, const char *key); // Get value int for provided key, returns 0 if not found
RINIAPI const char *rini_get_value_text(rini_data data, const char *key); // Get value text for provided key
RINIAPI const char *rini_get_value_description(rini_data data, const char *key); // Get value description for provided key

RINIAPI int rini_get_value_fallback(rini_data data, const char *key, int fallback); // Get value for provided key with default value fallback if not found or not valid
RINIAPI const char *rini_get_value_text_fallback(rini_data data, const char *key, const char *fallback); // Get value text for provided key with fallback if not found or not valid

RINIAPI int rini_set_comment_line(rini_data *data, const char *comment); // Set comment line

// Set value int/text and description for existing key or create a new entry
// NOTE: When setting a text value, if id does not exist, a new entry is automatically created
RINIAPI int rini_set_value(rini_data *data, const char *key, int value, const char *desc);
RINIAPI int rini_set_value_text(rini_data *data, const char *key, const char *text, const char *desc);

// Set value description for existing key
// WARNING: Key must exist to add description, if a description exists, it is updated
RINIAPI int rini_set_value_description(rini_data *data, const char *key, const char *desc);

#ifdef __cplusplus
}
#endif

#endif // RINI_H

/***********************************************************************************
*
*   RINI IMPLEMENTATION
*
************************************************************************************/

#if defined(RINI_IMPLEMENTATION)

#include <stdio.h>          // Required for: fopen(), feof(), fgets(), fclose(), fprintf()
#include <stdlib.h>         // Required for: malloc(), calloc(), free()
#include <string.h>         // Required for: memset(), memcpy(), strcmp(), strlen()

//----------------------------------------------------------------------------------
// Defines and macros
//----------------------------------------------------------------------------------
// ...

//----------------------------------------------------------------------------------
// Global variables definition
//----------------------------------------------------------------------------------
// ...

//----------------------------------------------------------------------------------
// Module Internal Functions Declaration
//----------------------------------------------------------------------------------
static int rini_read_key(const char *buffer, char *key); // Get key from a buffer line containing key-value-(description)
static int rini_read_value_text(const char *buffer, char *text, char *desc); // Get value text (and description) from a buffer line

static int rini_text_to_int(const char *text); // Convert text to int value (if possible), same as atoi()

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
// Load data from file (.ini)
rini_data rini_load(const char *file_name)
{
    rini_data data = { 0 };
    unsigned int value_counter = 0;

    // Init data to max capacity
    data.capacity = RINI_MAX_VALUE_CAPACITY;
    data.values = (rini_value *)RINI_CALLOC(RINI_MAX_VALUE_CAPACITY, sizeof(rini_value));

    if (file_name != NULL)
    {
        FILE *rini_file = fopen(file_name, "rt");

        if (rini_file != NULL)
        {
            char buffer[RINI_MAX_LINE_SIZE] = { 0 };    // Buffer to read every text line

            // First pass to count valid lines
            while (fgets(buffer, RINI_MAX_LINE_SIZE, rini_file))
            {
                // WARNING: fgets() keeps line endings, doesn't have any special options for converting line endings,
                // but on Windows, when reading file 'rt', line endings are converted from \r\n to just \n

                // Skip commented lines and empty lines
                // NOTE: We are also skipping sections delimiters
                if ((buffer[0] != RINI_LINE_COMMENT_DELIMITER) &&
                    (buffer[0] != RINI_LINE_SECTION_DELIMITER) &&
                    (buffer[0] != '\n') && (buffer[0] != '\0')) value_counter++;
            }

            // WARNING: We can't store more values than its max capacity
            data.count = (value_counter > RINI_MAX_VALUE_CAPACITY)? RINI_MAX_VALUE_CAPACITY : value_counter;

            if (data.count > 0)
            {
                rewind(rini_file);
                value_counter = 0;

                // Second pass to read data
                while (fgets(buffer, RINI_MAX_LINE_SIZE, rini_file))
                {
                    // WARNING: fgets() keeps line endings, doesn't have any special options for converting line endings,
                    // but on Windows, when reading file 'rt', line endings are converted from \r\n to just \n

                    // Skip commented lines and empty lines
                    if ((buffer[0] != RINI_LINE_COMMENT_DELIMITER) &&
                        (buffer[0] != RINI_LINE_SECTION_DELIMITER) &&
                        (buffer[0] != '\n') && (buffer[0] != '\0'))
                    {
                        // Get key identifier string
                        memset(data.values[value_counter].key, 0, RINI_MAX_KEY_SIZE);
                        rini_read_key(buffer, data.values[value_counter].key);
                        rini_read_value_text(buffer, data.values[value_counter].text, data.values[value_counter].desc);

                        value_counter++;

                        // Stop reading if first count reached to avoid overflow in case count == RINI_MAX_VALUE_CAPACITY
                        if (value_counter >= data.count) break;
                    }
                }
            }

            fclose(rini_file);
        }
    }

    return data;
}

// Load data from text buffer
// NOTE: Comments and empty lines are ignored
rini_data rini_load_from_memory(const char *text)
{
    #define RINI_MAX_TEXT_LINES     RINI_MAX_VALUE_CAPACITY*2       // Consider possible comments and empty lines

    rini_data data = { 0 };
    unsigned int value_counter = 0;

    // Init data to max capacity
    data.capacity = RINI_MAX_VALUE_CAPACITY;
    data.values = (rini_value *)RINI_CALLOC(RINI_MAX_VALUE_CAPACITY, sizeof(rini_value));

    if (text != NULL)
    {
        // Split text by line-breaks
        const char *lines[RINI_MAX_TEXT_LINES] = { 0 };
        int textSize = (int)strlen(text);
        lines[0] = text;
        int line_counter = 1;

        for (int i = 0, k = 1; (i < textSize) && (line_counter < RINI_MAX_TEXT_LINES); i++)
        {
            if (text[i] == '\n')
            {
                lines[k] = &text[i + 1]; // WARNING: next value is valid?
                line_counter += 1;
                k++;
            }
        }

        // Count possible values in lines
        for (int l = 0; l < line_counter; l++)
        {
            // Skip commented lines and empty lines
            // NOTE: We are also skipping sections delimiters
            if ((lines[l][0] != RINI_LINE_COMMENT_DELIMITER) &&
                (lines[l][0] != RINI_LINE_SECTION_DELIMITER) &&
                (lines[l][0] != '\n') && (lines[l][0] != '\0')) value_counter++;
        }

        // WARNING: We can't store more values than its max capacity
        data.count = (value_counter > RINI_MAX_VALUE_CAPACITY)? RINI_MAX_VALUE_CAPACITY : value_counter;

        // Process lines to get keys and values
        if (data.count > 0)
        {
            value_counter = 0;

            // Second pass to read data
            for (int l = 0; l < line_counter; l++)
            {
                // Skip commented lines and empty lines
                if ((lines[l][0] != RINI_LINE_COMMENT_DELIMITER) &&
                    (lines[l][0] != RINI_LINE_SECTION_DELIMITER) &&
                    (lines[l][0] != '\n') && (lines[l][0] != '\0'))
                {
                    // Get key identifier string
                    memset(data.values[value_counter].key, 0, RINI_MAX_KEY_SIZE);
                    rini_read_key(lines[l], data.values[value_counter].key);
                    rini_read_value_text(lines[l], data.values[value_counter].text, data.values[value_counter].desc);

                    value_counter++;

                    // Stop reading if first count reached to avoid overflow in case count == RINI_MAX_VALUE_CAPACITY
                    if (value_counter >= data.count) break;
                }
            }
        }
    }

    return data;
}

// Save data to file (*.ini)
void rini_save(rini_data data, const char *file_name)
{
    FILE *rini_file = fopen(file_name, "wt");

    if (rini_file != NULL)
    {
        char valuestr[128 + 2] = { 0 }; // Useful for text processing, adding quotation marks if required

        for (unsigned int i = 0; i < data.count; i++)
        {
            if ((data.values[i].key[0] == '\0') && (data.values[i].text[0] == RINI_LINE_COMMENT_DELIMITER))
            {
                if (data.values[i].desc[0] != '\0') fprintf(rini_file, "%c %s\n", RINI_LINE_COMMENT_DELIMITER, data.values[i].desc);
                else fprintf(rini_file, "%c\n", RINI_LINE_COMMENT_DELIMITER);
            }
            else
            {
                memset(valuestr, 0, 130); 
#if RINI_USE_TEXT_QUOTATION_MARKS
                // Add quotation marks if required
                if (data.values[i].isText) snprintf(valuestr, 130, "%c%s%c", RINI_VALUE_QUOTATION_MARKS, data.values[i].text, RINI_VALUE_QUOTATION_MARKS);
#else
                snprintf(valuestr, 130, "%s", data.values[i].text);
#endif
                fprintf(rini_file, "%-*s %c %-*s %c %s\n", RINI_KEY_SPACING, data.values[i].key, RINI_VALUE_DELIMITER,
                    RINI_VALUE_SPACING, data.values[i].isText? valuestr : data.values[i].text, 
                    RINI_DESCRIPTION_DELIMITER, data.values[i].desc);
            }
        }

        fclose(rini_file);
    }
}

// Save data to text buffer ('\0' EOL)
char *rini_save_to_memory(rini_data data)
{
    #define RINI_MAX_TEXT_FILE_SIZE  4096

    // Verify required data size is smaller than memory buffer size
    // NOTE: We add 64 extra possible characters by entry line
    int requiredSize = 0;
    for (unsigned int i = 0; i < data.count; i++) requiredSize += ((int)strlen(data.values[i].key) + (int)strlen(data.values[i].text) + (int)strlen(data.values[i].desc) + 64);
    if (requiredSize > RINI_MAX_TEXT_FILE_SIZE) RINI_LOG("WARNING: Required data.ini size is bigger than max supported memory size, increase RINI_MAX_TEXT_FILE_SIZE\n");

    // NOTE: Using a static buffer to avoid de-allocation requirement on user side
    static char text[RINI_MAX_TEXT_FILE_SIZE] = { 0 };
    memset(text, 0, RINI_MAX_TEXT_FILE_SIZE);
    int offset = 0;

    char valuestr[128 + 2] = { 0 }; // Useful for text processing, adding quotation marks if required

    for (unsigned int i = 0; i < data.count; i++)
    {
        if ((data.values[i].key[0] == '\0') && (data.values[i].text[0] == RINI_LINE_COMMENT_DELIMITER))
        {
            if (data.values[i].desc[0] != '\0') offset += snprintf(text + offset, RINI_MAX_LINE_SIZE, "%c %s\n", RINI_LINE_COMMENT_DELIMITER, data.values[i].desc);
            else offset += snprintf(text + offset, RINI_MAX_LINE_SIZE, "%c\n", RINI_LINE_COMMENT_DELIMITER);
        }
        else
        {
            memset(valuestr, 0, 130); 
#if RINI_USE_TEXT_QUOTATION_MARKS
            // Add quotation marks if required
            if (data.values[i].isText) snprintf(valuestr, 130, "%c%s%c", RINI_VALUE_QUOTATION_MARKS, data.values[i].text, RINI_VALUE_QUOTATION_MARKS);
#else
            snprintf(valuestr, 130, "%s", data.values[i].text);
#endif
            offset += snprintf(text + offset, RINI_MAX_LINE_SIZE, "%-*s %c %-*s %c %s\n", RINI_KEY_SPACING, data.values[i].key, RINI_VALUE_DELIMITER,
                    RINI_VALUE_SPACING, data.values[i].isText? valuestr : data.values[i].text, 
                    RINI_DESCRIPTION_DELIMITER, data.values[i].desc);
        }
    }

    return text;
}

// Unload data
void rini_unload(rini_data *data)
{
    RINI_FREE(data->values);

    data->values = NULL;
    data->count = 0;
    data->capacity = 0;
}

// Get value for provided key, returns 0 if not found or not valid
int rini_get_value(rini_data data, const char *key)
{
    int value = 0;

    for (unsigned int i = 0; i < data.count; i++)
    {
        if (strcmp(key, data.values[i].key) == 0) // Key found
        {
            value = rini_text_to_int(data.values[i].text);
            break;
        }
    }

    return value;
}

// Get value for provided key with default value fallback if not found or not valid
int rini_get_value_fallback(rini_data data, const char *key, int fallback)
{
    int value = fallback;

    for (unsigned int i = 0; i < data.count; i++)
    {
        if (strcmp(key, data.values[i].key) == 0) // Key found
        {
            // TODO: Detect if conversion fails...
            value = rini_text_to_int(data.values[i].text);
            break;
        }
    }

    return value;
}

// Get text for string id
const char *rini_get_value_text(rini_data data, const char *key)
{
    const char *text = NULL;

    for (unsigned int i = 0; i < data.count; i++)
    {
        if (strcmp(key, data.values[i].key) == 0) // Key found
        {
            text = data.values[i].text;
            break;
        }
    }

    return text;
}

// Get value text for provided key with fallback if not found or not valid
RINIAPI const char *rini_get_value_text_fallback(rini_data data, const char *key, const char *fallback)
{
    const char *text = fallback;

    for (unsigned int i = 0; i < data.count; i++)
    {
        if (strcmp(key, data.values[i].key) == 0) // Key found
        {
            text = data.values[i].text;
            break;
        }
    }

    return text;
}

// Get description for string id
const char *rini_get_value_description(rini_data data, const char *key)
{
    const char *desc = NULL;

    for (unsigned int i = 0; i < data.count; i++)
    {
        if (strcmp(key, data.values[i].key) == 0) // Key found
        {
            desc = data.values[i].desc;
            break;
        }
    }

    return desc;
}

// Set comment line
int rini_set_comment_line(rini_data *data, const char *comment)
{
    int result = -1;
    char text[2] = { RINI_LINE_COMMENT_DELIMITER, '\0' };
    
    result = rini_set_value_text(data, NULL, text, comment);
    
    return result;
}

// Set value and description for existing key or create a new entry
int rini_set_value(rini_data *data, const char *key, int value, const char *desc)
{
    int result = -1;
    char value_text[RINI_MAX_TEXT_SIZE] = { 0 };

    snprintf(value_text, RINI_MAX_TEXT_SIZE, "%i", value);

    result = rini_set_value_text(data, key, value_text, desc);

    data->values[data->count - 1].isText = false;

    return result;
}

// Set value text and description for existing key or create a new entry
// NOTE: When setting a text value, if id does not exist, a new entry is automatically created
int rini_set_value_text(rini_data *data, const char *key, const char *text, const char *desc)
{
    int result = -1;

    if ((text == NULL) || (text[0] == '\0')) return result;

    if (key != NULL)
    {
        // Try to find key and update text and description
        for (unsigned int i = 0; i < data->count; i++)
        {
            if (strcmp(key, data->values[i].key) == 0) // Key found
            {
                memset(data->values[i].text, 0, RINI_MAX_TEXT_SIZE);
                memcpy(data->values[i].text, text, strlen(text));

                memset(data->values[i].desc, 0, RINI_MAX_DESC_SIZE);
                if (desc != NULL) memcpy(data->values[i].desc, desc, strlen(desc));
                result = 0;
                break;
            }
        }
    }

    // Key not found, we add a new entry if possible
    if (result == -1)
    {
        if (data->count < data->capacity)
        {
            // NOTE: Supporting comment line entries
            if ((key == NULL) && (text[0] == RINI_LINE_COMMENT_DELIMITER))
            {
                data->values[data->count].key[0] = '\0';
                data->values[data->count].text[0] = RINI_LINE_COMMENT_DELIMITER;
                if (desc != NULL) for (int i = 0; (i < RINI_MAX_DESC_SIZE) && (desc[i] != '\0'); i++) 
                    data->values[data->count].desc[i] = desc[i];
                else data->values[data->count].desc[0] = '\0';
            }
            else
            {
                // NOTE: We do a manual copy to avoid possible overflows on input data
                for (int i = 0; (i < RINI_MAX_KEY_SIZE) && (key[i] != '\0'); i++) data->values[data->count].key[i] = key[i];
                for (int i = 0; (i < RINI_MAX_TEXT_SIZE) && (text[i] != '\0'); i++) data->values[data->count].text[i] = text[i];
                if (desc != NULL) for (int i = 0; (i < RINI_MAX_DESC_SIZE) && (desc[i] != '\0'); i++) data->values[data->count].desc[i] = desc[i];
            }

            data->values[data->count].isText = true;
            data->count++;
            result = 0;
        }
    }

    return result;
}

// Set value description for existing key
// WARNING: Key must exist to add description, if a description exists, it is updated
int rini_set_value_description(rini_data *data, const char *key, const char *desc)
{
    int result = 1;

    for (unsigned int i = 0; i < data->count; i++)
    {
        if (strcmp(key, data->values[i].key) == 0) // Key found
        {
            memset(data->values[i].desc, 0, RINI_MAX_DESC_SIZE);
            if (desc != NULL) memcpy(data->values[i].desc, desc, strlen(desc));
            result = 0;
            break;
        }
    }

    return result;
}

//----------------------------------------------------------------------------------
// Module Internal Functions Declaration
//----------------------------------------------------------------------------------
// Get string id from a buffer line containing id-value pair
static int rini_read_key(const char *buffer, char *key)
{
    int len = 0;
    while ((buffer[len] != '\0') && (buffer[len] != ' ') && (buffer[len] != RINI_VALUE_DELIMITER)) len++;    // Skip keyentifier

    memcpy(key, buffer, len);

    return len;
}

// Get string-value from a buffer line containing id-value pair
static int rini_read_value_text(const char *buffer, char *text, char *desc)
{
    char *buffer_ptr = (char *)buffer;

    // Expected line structure:
    // [key][spaces?][delimiter?][spaces?][quot-mark?][textValue][quot-mark?][spaces?][[;][#]description?]
    // We need to skip spaces, check for delimiter (if required), skip spaces, and get text value

    while ((buffer_ptr[0] != '\0') && (buffer_ptr[0] != ' ')) buffer_ptr++; // Skip keyentifier

    while ((buffer_ptr[0] != '\0') && (buffer_ptr[0] == ' ')) buffer_ptr++; // Skip line spaces before text value or delimiter

#if defined(RINI_VALUE_DELIMITER)
    if (buffer_ptr[0] == RINI_VALUE_DELIMITER)
    {
        buffer_ptr++;       // Skip delimiter

        while ((buffer_ptr[0] != '\0') && (buffer_ptr[0] == ' ')) buffer_ptr++; // Skip line spaces before text value
    }
#endif

    // Now buffer_ptr should be pointing to the start of value

    int len = 0;
    while ((buffer_ptr[len] != '\0') &&
        (buffer_ptr[len] != '\r') &&
        (buffer_ptr[len] != '\n')) len++; // Get text-value and description length (to the end of line)

    // Now we got the length from text-value start to end of line

    int value_len = len;
    int desc_pos = 0;
#if defined(RINI_DESCRIPTION_DELIMITER)
    // Scan text looking for text-value description (if used)
    for (; desc_pos < len; desc_pos++)
    {
        if (buffer_ptr[desc_pos] == RINI_DESCRIPTION_DELIMITER)
        {
            value_len = desc_pos - 1;
            while (buffer_ptr[value_len] == ' ') value_len--;

            value_len++;

            desc_pos++;   // Skip delimiter and following spaces
            while (buffer_ptr[desc_pos] == ' ') desc_pos++;
            break;
        }
    }
#endif

#if RINI_USE_TEXT_QUOTATION_MARKS
    // Remove starting quotation-mark from text (if being used)
    if (buffer_ptr[0] == RINI_VALUE_QUOTATION_MARKS)
    {
        buffer_ptr++; desc_pos--; len--; value_len--;

        // Remove ending quotation-mark from text (if being used)
        if (buffer_ptr[value_len - 1] == RINI_VALUE_QUOTATION_MARKS) { value_len--; }
    }
#endif

    // Clear text buffers to be updated
    memset(text, 0, RINI_MAX_TEXT_SIZE);
    memset(desc, 0, RINI_MAX_DESC_SIZE);

    // Copy value-text and description to provided pointers
    memcpy(text, buffer_ptr, value_len);
    memcpy(desc, buffer_ptr + desc_pos, ((len - desc_pos) > (RINI_MAX_DESC_SIZE - 1))? (RINI_MAX_DESC_SIZE - 1) : (len - desc_pos));

    return len;
}

// Convert text to int value (if possible), same as atoi()
static int rini_text_to_int(const char *text)
{
    int value = 0;
    int sign = 1;

    if ((text[0] == '+') || (text[0] == '-'))
    {
        if (text[0] == '-') sign = -1;
        text++;
    }

    int i = 0;
    for (; ((text[i] >= '0') && (text[i] <= '9')); i++) value = value*10 + (int)(text[i] - '0');

    //if (strlen(text) != i) // Text starts with numbers but contains some text after that...

    return value*sign;
}

#endif  // RINI_IMPLEMENTATION
