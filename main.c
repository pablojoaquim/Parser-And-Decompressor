#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

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

typedef enum cs_parser_tag
{
    CS_LOOKING_FOR_BLOCK_START,
    CS_LOOKING_FOR_BLOCK_NUMBER,
    CS_READ_METADATA,
    CS_READ_DATA_BLOCK
}cs_parser_t;

int main() 
{
    // Control variables
    FILE *file;                 // file* necessary for file operations
    char line[50];              // Buffer to store each line
    unsigned int bytes[16];     // Buffer to perform the data analysis
    int block_seq = 0;          // Check the sequence of block in the file
    int block_counter = 0;      // block counter for statistics
    int block_err_counter = 0;  // block error counter for statistics
    int line_counter = 0;       // line counter for statistics
    int block_number;           // The parsed block number in the label
    int block_number_aux;       // The parsed block number
    int data_counter;
    flashdata_T flashdata;      // The flashdata information from each block
    cs_parser_t cs_parser = CS_LOOKING_FOR_BLOCK_START;  // The state variable for the parser
    
    // Open the file for reading operations
    // The filename is hardcoded. An improvement could be 
    // to receive the name as an args
    file = fopen("INTDATA", "r");
    if (file == NULL) 
    {
        perror("Error opening file");
        return -1;
    }

    // Read every line through the file
    while (fgets(line, sizeof(line), file) != NULL) 
    {
        // Remove trailing newline character if present
        line[strcspn(line, "\n")] = 0; 
        
        // Increment the line counter
        line_counter++;

        // Print the line while parsing
        printf("Parsed line %d: \t\t %s\n", line_counter, line);

        // The parser
        switch(cs_parser)
        {
            case CS_LOOKING_FOR_BLOCK_START:
                // Check for the "BLK" start block identifier
                if(0 == strncmp(line,"BLK", 3))
                {
                    // Count the number of block
                    // could be a good block or a corrupted one
                    block_counter++;

                    // Read the block number in the block start label
                    sscanf(&line[4], "%d", &block_number);
                    //printf("block number is: %d\n", block_number);

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
                    block_err_counter++;
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
                        block_err_counter++;
                        printf("Error: Invalid filename_id.\n");
                        cs_parser = CS_LOOKING_FOR_BLOCK_START;
                    }
                    else if (flashdata.data_length > (2048-14))
                    {
                        // ERROR detected! Discard the whole block!
                        block_err_counter++;
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
                                block_err_counter++;
                                printf("Error: Invalid Block sequence!\n");
                                cs_parser = CS_LOOKING_FOR_BLOCK_START;
                            } 
                        }
                    }
                } 
                else 
                {
                    // ERROR detected! Discard the whole block!
                    block_err_counter++;
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
                            cs_parser = CS_LOOKING_FOR_BLOCK_START;
                            break;
                        }
                    }
                }
                else 
                {
                    // ERROR detected! Discard the whole block!
                    block_err_counter++;
                    printf("Error: Could not read all fields!\n");
                    cs_parser = CS_LOOKING_FOR_BLOCK_START;
                }
                break;   

            default:
                break;
        }
    }

    fclose(file);

    printf("Number of blocks: %d\n", block_counter);
    printf("Number of blocks error: %d\n", block_err_counter);
    return 0;
}