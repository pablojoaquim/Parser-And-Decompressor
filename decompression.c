#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Nibble to character mapping table
char nibble_to_char(int nibble) {
    switch(nibble) {
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

int decompress_data(unsigned char *compressed, int compressed_len, char *output, int max_output_len) {
    int nibbles[compressed_len * 2];
    int nibble_count = 0;
    int output_pos = 0;
    
    // Step 1: Extract nibbles from bytes (low nibble first, high nibble second)
    for(int i = 0; i < compressed_len; i++) {
        nibbles[nibble_count++] = compressed[i] & 0x0F;  // Low nibble
        nibbles[nibble_count++] = (compressed[i] & 0xF0) >> 4;  // High nibble
    }
    
    // Step 2: Remove padding F from the end if present
    if(nibble_count > 0 && nibbles[nibble_count - 1] == 0xF) {
        nibble_count--;
    }
    
    // Step 3: Process nibbles
    int i = 0;
    while(i < nibble_count && output_pos < max_output_len - 1) {
        if(nibbles[i] == 0xF) {
            // Literal character - need two more nibbles
            if(i + 2 >= nibble_count) {
                printf("Error: Incomplete literal character sequence\n");
                break;
            }
            
            // Combine next two nibbles (reversed for Intel endianness)
            unsigned char literal_char = (nibbles[i+2] << 4) | nibbles[i+1];
            
            // Check if it's a compressed space sequence
            if(literal_char >= 0x80) {
                int space_count = literal_char - 0x80;
                for(int j = 0; j < space_count && output_pos < max_output_len - 1; j++) {
                    output[output_pos++] = ' ';
                }
            } else {
                output[output_pos++] = literal_char;
            }
            
            i += 3; // Skip F and two nibbles
        } else {
            // Regular nibble mapping
            char ch = nibble_to_char(nibbles[i]);
            if(ch != 0) {
                output[output_pos++] = ch;
            }
            i++;
        }
    }
    
    output[output_pos] = '\0';
    return output_pos;
}

// Helper function to parse hex string into byte array
int parse_hex_string(const char *hex_str, unsigned char *bytes, int max_bytes) {
    int byte_count = 0;
    int len = strlen(hex_str);
    
    for(int i = 0; i < len && byte_count < max_bytes; i++) {
        if(hex_str[i] == ' ' || hex_str[i] == '\t' || hex_str[i] == '\n') {
            continue; // Skip whitespace
        }
        
        // Read two hex digits
        if(i + 1 < len) {
            unsigned int byte_val;
            if(sscanf(&hex_str[i], "%2x", &byte_val) == 1) {
                bytes[byte_count++] = (unsigned char)byte_val;
                i++; // Skip the second hex digit
            }
        }
    }
    
    return byte_count;
}

int main() {
    // Test cases
    const char *test1 = "ed";
    // const char *test1 = "10 32 54 A6 4F D3 FE";
    // const char *test2 = "10 af 28 f3";
    // const char *test3 = "ef a4 21 a2 00 5a 26 93 3f f8 54 00 02 10 df 08 c9 70 1a b6 62 0a b0 00 3b a4 1f a4 84 01 42 52 46 51 67 d0 fe";
    const char *test2 = "ef a4 21 a2 0 5a 26 93 3f f8 54 0 2 10 df 8 c9 70 1a b6 62 a b0 0 3b a4 1f a4 84 1 42 52 46 51 67 d0 fe";
    const char *test3 = "ef a4 21 a3 0 4a 83 11 3f f8 54 0 2 53 df 8 c9 70 1a b6 31 a b0 31 5b a0 1f a4 84 1 12 8 70 39 43 47 ed";
    const char *test4 = "ef a4 21 a4 0 fa 54 1 4 60 fa 54 1 3 70 df 8 c9 70 1a b6 12 a b0 50 5b a0 94 48 15 2 56 50 11 10 ed";
    const char *test5 = "ef a4 21 a5 0 6a 53 27 3f f8 54 0 8 40 a 27 ef 12 ef 2 f9 2e 40 a c9 70 1a b6 2 a b0 60 5b a8 1f a4 84 1 18 24 35 33 56 80 ed";

    unsigned char compressed[256];
    char output[1024];
    
    printf("Intel CPU Data Decompression Test\n");
    printf("=================================\n\n");
    
    // Test 1
    int len1 = parse_hex_string(test1, compressed, sizeof(compressed));
    int output_len1 = decompress_data(compressed, len1, output, sizeof(output));
    printf("Test 1: %s\n", test1);
    printf("Result: \"%s\"\n", output);
    printf("Length: %d bytes -> %d characters\n\n", len1, output_len1);
    
    // Test 2
    int len2 = parse_hex_string(test2, compressed, sizeof(compressed));
    int output_len2 = decompress_data(compressed, len2, output, sizeof(output));
    printf("Test 2: %s\n", test2);
    printf("Result: \"%s\"\n", output);
    printf("Length: %d bytes -> %d characters\n\n", len2, output_len2);
    
    // Test 3
    int len3 = parse_hex_string(test3, compressed, sizeof(compressed));
    int output_len3 = decompress_data(compressed, len3, output, sizeof(output));
    printf("Test 3: %s\n", test3);
    printf("Result: \"%s\"\n", output);
    printf("Length: %d bytes -> %d characters\n\n", len3, output_len3);

    // Test 4
    int len4 = parse_hex_string(test4, compressed, sizeof(compressed));
    int output_len4 = decompress_data(compressed, len4, output, sizeof(output));
    printf("Test 4: %s\n", test4);
    printf("Result: \"%s\"\n", output);
    printf("Length: %d bytes -> %d characters\n\n", len4, output_len4);
    
    // Test 5
    int len5 = parse_hex_string(test5, compressed, sizeof(compressed));
    int output_len5 = decompress_data(compressed, len5, output, sizeof(output));
    printf("Test 5: %s\n", test5);
    printf("Result: \"%s\"\n", output);
    printf("Length: %d bytes -> %d characters\n\n", len5, output_len3);
    
    return 0;
}