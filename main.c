/*===========================================================================*/
/**
 * @file main.c
 *
 *  Data parser and decompressor
 *  This is a technical challenge for Asentria
 *
 *------------------------------------------------------------------------------
 *
 *------------------------------------------------------------------------------
 *
 * @section DESC DESCRIPTION:
 *
 * @todo Divide this file content using an abstraction layers concept
 *
 * @section ABBR ABBREVIATIONS:
 *   - @todo List any abbreviations, precede each with a dash ('-').
 *
 * @section TRACE TRACEABILITY INFO:
 *   - Design Document(s):
 *     - @todo Update list of design document(s).
 *
 *   - Requirements Document(s):
 *     - @todo Update list of requirements document(s)
 *
 *   - Applicable Standards (in order of precedence: highest first):
 *     - @todo Update list of other applicable standards
 *
 */
/*==========================================================================*/

/*===========================================================================*
 * Header Files
 *===========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/*===========================================================================*
 * Local Preprocessor #define Constants
 *===========================================================================*/

/*===========================================================================*
 * Local Preprocessor #define MACROS
 *===========================================================================*/

/*===========================================================================*
 * Local Type Declarations
 *===========================================================================*/
typedef struct flashdata_Tag
{
    uint16_t filename_id;                     // number for file
    uint32_t sequence_in_file;                // file sequence number
    uint16_t number_records;                  // recs in block
    uint16_t data_length;                     // data length
    uint16_t flags;                           // unused presently
    unsigned char compression_type;           // unused presently
    unsigned char spare;                      // unused presently
    unsigned char data[2048-14];              // the data itself
}flashdata_T;

typedef enum cs_parser_Tag
{
    CS_LOOKING_FOR_BLOCK_START,
    CS_LOOKING_FOR_BLOCK_NUMBER,
    CS_READ_METADATA,
    CS_READ_DATA_BLOCK
}cs_parser_T;

typedef struct statistics_Tag
{
    uint16_t blockCntr;     // block counter for statistics
    uint16_t blockErrCntr;  // block error counter for statistics
    uint16_t lineCntr;      // line counter for statistics
}statistics_T;

/*===========================================================================*
 * Local Variables Definitions
 *===========================================================================*/
flashdata_T flashdata;      // The flashdata information from each block
statistics_T statistics;    // Information to printout on application finish

/*===========================================================================*
 * Local Function Prototypes
 *===========================================================================*/

/*===========================================================================*
 * Local Inline Function Definitions and Function-Like Macros
 *===========================================================================*/

/*===========================================================================*
 * Function Definitions
 *===========================================================================*/
/***************************************************************************//**
* @fn         nibble_to_char
* @brief      Nibble to character mapping table
* @param [in] nibble - The nibble to convert
* @return     The char value
******************************************************************************/
char nibble_to_char(int nibble) 
{
    switch(nibble) 
    {
        case 0: return '0';
        case 1: return '1';
        case 2: return '2';
        case 3: return '3';
        case 4: return '4';
        case 5: return '5';
        case 6: return '6';
        case 7: return '7';
        case 8: return '8';
        case 9: return '9';
        case 0xA: return ' ';
        case 0xB: return ':';
        case 0xC: return '/';
        case 0xD: return '\r';
        case 0xE: return '\n';
        case 0xF: return 0; // Special case - literal character indicator
        default: return '?';
    }
}

/***************************************************************************//**
* @fn         decompress_data
* @brief      Apply the decompress algorithm
* @param [in] compressed - The byte array of the compressed info
* @param [in] compressed_len - The max length to read from the input
* @param [in] output - The byte array to hold the decompressed output
* @param [in] max_output_len - The limit to hold in the output array
* @return     The actual length of the output
******************************************************************************/
int decompress_data(unsigned char *compressed, int compressed_len, char *output, int max_output_len) {
    int nibbles[compressed_len * 2];
    int nibble_count = 0;
    int output_pos = 0;
    
    // Step 1: Extract nibbles from bytes (low nibble first, high nibble second)
    for(int i = 0; i < compressed_len; i++) 
    {
        nibbles[nibble_count++] = compressed[i] & 0x0F;  // Low nibble
        nibbles[nibble_count++] = (compressed[i] & 0xF0) >> 4;  // High nibble
    }
    
    // Step 2: Remove padding F from the end if present
    if(nibble_count > 0 && nibbles[nibble_count - 1] == 0xF) 
    {
        nibble_count--;
    }
    
    // Step 3: Process nibbles
    int i = 0;
    while(i < nibble_count && output_pos < max_output_len - 1) 
    {
        if(nibbles[i] == 0xF) 
        {
            // Literal character - need two more nibbles
            if(i + 2 >= nibble_count) 
            {
                printf("Error: Incomplete literal character sequence\n");
                break;
            }
            
            // Combine next two nibbles (reversed for Intel endianness)
            unsigned char literal_char = (nibbles[i+2] << 4) | nibbles[i+1];
            
            // Check if it's a compressed space sequence
            if(literal_char >= 0x80)
            {
                int space_count = literal_char - 0x80;
                for(int j = 0; j < space_count && output_pos < max_output_len - 1; j++) 
                {
                    output[output_pos++] = ' ';
                }
            } 
            else 
            {
                output[output_pos++] = literal_char;
            }
            
            i += 3; // Skip F and two nibbles
        } 
        else 
        {
            // Regular nibble mapping
            char ch = nibble_to_char(nibbles[i]);
            if(ch != 0) 
            {
                output[output_pos++] = ch;
            }
            i++;
        }
    }
    
    output[output_pos] = '\0';
    return output_pos;
}

/***************************************************************************//**
* @fn         parse_hex_string
* @brief      Helper function to parse hex string into byte array
* @param [in] hex_str - The input string containing only hex characters
* @param [in] bytes - The array to hold the output
* @param [in] max_bytes - The characters limit to read from the hex_str
* @return     byte_count
******************************************************************************/
int parse_hex_string(const char *hex_str, unsigned char *bytes, int max_bytes) {
    int byte_count = 0;
    int len = strlen(hex_str);
    
    for(int i = 0; i < len && byte_count < max_bytes; i++) 
    {
        if(hex_str[i] == ' ' || hex_str[i] == '\t' || hex_str[i] == '\n') 
        {
            continue; // Skip whitespace
        }
        
        // Read two hex digits
        if(i + 1 < len) 
        {
            unsigned int byte_val;
            if(sscanf(&hex_str[i], "%2x", &byte_val) == 1)
            {
                bytes[byte_count++] = (unsigned char)byte_val;
                i++; // Skip the second hex digit
            }
        }
    }
    
    return byte_count;
}

/***************************************************************************//**
* @fn         process_data_records
* @brief      Process all the data records found in a block.
* @param [in] totalRecords - The number of records in the data
* @param [in] pDataRecords - A pointer to the data content
* @return     0 -Success, -1 -Error
******************************************************************************/
int process_data_records(uint16_t totalRecords, unsigned char* pDataRecords)
{
    int recordCnt = 0;
    int recordDataLength = 0;
    int pos = 0;
    // unsigned char aux;

    for(int recordCnt=0; recordCnt < totalRecords; recordCnt++)
    {
        // Move through the data array
        pos += recordDataLength;
        // Get the length of the record
        recordDataLength = pDataRecords[pos];
        printf("pos: %d", pos);
        printf("recordCnt: %d", recordCnt);
        printf("recordDataLength: %d", recordDataLength);
        // The record data starts at the 3rd position
        for (int i=3; i<recordDataLength; i++)
        {
            printf("%x ", pDataRecords[pos+i]);
        }
        printf("\r\n");
    }
    return 0;
    // FILE *file;
    // file = fopen("aux.data", "w+");
    // if (file == NULL) 
    // {
    //     perror("Error opening file for write");
    //     return -1;
    // }
    // fputs(pFlashdata->data, file);
    // fclose(file);
}

/***************************************************************************//**
* @fn         process_file
* @brief      The file provided is parsed using a state-machine
*             Several error are considered, in such case the function
*             return an error and show a printout describing it
* @param [in] file - A FILE* to the file to be parsed
* @return     0 -Success, -1 -Error
******************************************************************************/
int process_file(FILE* file)
{
    char line[50];              // Buffer to store each line
    unsigned int bytes[16];     // Buffer to perform the data analysis
    int block_seq = 0;          // Check the sequence of block in the file
    int block_number;           // The parsed block number in the label
    int block_number_aux;       // The parsed block number
    int data_counter;
    cs_parser_T cs_parser = CS_LOOKING_FOR_BLOCK_START;  // The state variable for the parser
    
    if (file == NULL)
    {
        return -1;
    }

    // Read every line through the file
    while (fgets(line, sizeof(line), file) != NULL) 
    {
        // Remove trailing newline character if present
        line[strcspn(line, "\n")] = 0; 

        // Print the line while parsing
        statistics.lineCntr++;
        printf("Parsed line %d: \t\t %s\n", statistics.lineCntr, line);

        // The parser
        switch(cs_parser)
        {
            case CS_LOOKING_FOR_BLOCK_START:
                // Check for the "BLK" start block identifier
                if(0 == strncmp(line,"BLK", 3))
                {
                    // Count the number of block
                    statistics.blockCntr++;

                    // Read the block number in the block start label
                    sscanf(&line[4], "%d", &block_number);

                    // Move to the next state of the parser
                    cs_parser = CS_LOOKING_FOR_BLOCK_NUMBER;
                }
                break;

            case CS_LOOKING_FOR_BLOCK_NUMBER:
                sscanf(&line[0], "%d", &block_number_aux);
                if (block_number_aux == block_number)
                {
                    cs_parser = CS_READ_METADATA;
                }
                else
                {
                    // ERROR detected! Discard the whole block!                    
                    statistics.blockErrCntr++;
                    printf("Error: Invalid Block number!\n");
                    cs_parser = CS_LOOKING_FOR_BLOCK_START;
                }
                break;

            case CS_READ_METADATA:
                if (16 == sscanf(line, "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
                                    &bytes[0], &bytes[1], &bytes[2], &bytes[3], &bytes[4], &bytes[5], &bytes[6], &bytes[7], &bytes[8],
                                    &bytes[9], &bytes[10], &bytes[11], &bytes[12], &bytes[13], &bytes[14], &bytes[15])) 
                {
                    // The data is little-endian, so we combine the bytes in reverse order.
                    flashdata.filename_id = (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
                    flashdata.sequence_in_file = (uint32_t)bytes[2] | ((uint32_t)bytes[3] << 8) | ((uint32_t)bytes[4] << 16) | ((uint32_t)bytes[5] << 24);
                    flashdata.number_records = (uint16_t)bytes[6] | ((uint16_t)bytes[7] << 8);
                    flashdata.data_length = (uint16_t)bytes[8] | ((uint16_t)bytes[9] << 8);
                    flashdata.flags = (uint16_t)bytes[10] | ((uint16_t)bytes[11] << 8);
                    flashdata.compression_type = (uint8_t)bytes[12];
                    flashdata.spare = (uint8_t)bytes[13];
                    
                    printf("Filename ID: %u (0x%X)\n", flashdata.filename_id, flashdata.filename_id);
                    printf("Sequence in File: %u (0x%X)\n", flashdata.sequence_in_file, flashdata.sequence_in_file);
                    printf("Number of Records: %u (0x%X)\n", flashdata.number_records, flashdata.number_records);
                    printf("Data Length: %u (0x%X)\n", flashdata.data_length, flashdata.data_length);
                    printf("Flags: %u (0x%X)\n", flashdata.flags, flashdata.flags);
                    printf("Compression Type: %u (0x%X)\n", flashdata.compression_type, flashdata.compression_type);
                    printf("Spare: %u (0x%X)\n", flashdata.spare, flashdata.spare);

                    if (flashdata.filename_id != 0x02)
                    {
                        // ERROR detected! Discard the whole block!
                        statistics.blockErrCntr++;
                        printf("Error: Invalid filename_id.\n");
                        cs_parser = CS_LOOKING_FOR_BLOCK_START;
                    }
                    else if (flashdata.data_length > (2048-14))
                    {
                        // ERROR detected! Discard the whole block!
                        statistics.blockErrCntr++;
                        printf("Error: Invalid data_length.\n");
                        cs_parser = CS_LOOKING_FOR_BLOCK_START;
                    }
                    else
                    {
                        if (block_seq == 0)
                        {
                            block_seq = flashdata.sequence_in_file;
                            // Read the last 2 bytes of the first row
                            flashdata.data[0] = bytes[14];
                            flashdata.data[1] = bytes[15];
                            data_counter = 2;
                            cs_parser = CS_READ_DATA_BLOCK;
                        }
                        else
                        {
                            if (flashdata.sequence_in_file > block_seq )
                            {
                                block_seq = flashdata.sequence_in_file;
                                // Read the last 2 bytes of the first row
                                flashdata.data[0] = bytes[14];
                                flashdata.data[1] = bytes[15];
                                data_counter = 2;
                                cs_parser = CS_READ_DATA_BLOCK;
                            }
                            else
                            {
                                // ERROR detected! Discard the whole block!                    
                                statistics.blockErrCntr++;
                                printf("Error: Invalid Block sequence!\n");
                                cs_parser = CS_LOOKING_FOR_BLOCK_START;
                            } 
                        }
                    }
                } 
                else 
                {
                    // ERROR detected! Discard the whole block!
                    statistics.blockErrCntr++;
                    printf("Error: Could not read all fields!\n");
                    cs_parser = CS_LOOKING_FOR_BLOCK_START;                    
                }
                break;     

            case CS_READ_DATA_BLOCK:
                if (16 == sscanf(line, "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
                                    &bytes[0], &bytes[1], &bytes[2], &bytes[3], &bytes[4], &bytes[5], &bytes[6], &bytes[7], &bytes[8],
                                    &bytes[9], &bytes[10], &bytes[11], &bytes[12], &bytes[13], &bytes[14], &bytes[15])) 
                {
                    for(int i=0; i<16; i++)
                    {
                        flashdata.data[data_counter] = bytes[i];
                        data_counter++;
                        // Check for the end of block condition
                        if(data_counter >= flashdata.data_length)
                        {
                            // All the records has been read, go for the next block
                            if(-1 == process_data_records(flashdata.number_records, flashdata.data))
                            {
                                return -1;
                            }
                            cs_parser = CS_LOOKING_FOR_BLOCK_START;
                            break;
                        }
                    }
                }
                else 
                {
                    // ERROR detected! Discard the whole block!
                    statistics.blockErrCntr++;
                    printf("Error: Could not read all fields!\n");
                    cs_parser = CS_LOOKING_FOR_BLOCK_START;
                }
                break;   

            default:
                break;
        }
    }

    return 0;
}

/***************************************************************************//**
* @fn         main
* @brief      The main entry point
* @param [in] void
* @return     0 -success, -1 -Error
******************************************************************************/
int main() 
{
    // Control variables
    FILE *fInput;
    
    // Open the file for reading operations
    // The filename is hardcoded. An improvement could be 
    // to receive the name as an args
    fInput = fopen("INTDATA", "r");
    if (fInput == NULL) 
    {
        perror("Error opening file");
        return -1;
    }

    int ret = process_file(fInput);

    fclose(fInput);
    
    printf("Number of blocks: %d\n", statistics.blockCntr);
    printf("Number of blocks error: %d\n", statistics.blockErrCntr);

    return ret;
}