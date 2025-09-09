# Asentria-TechnicalChallenge

## Programming Exercise

NOTE: ALL INFORMATION REQUIRED TO COMPLETE THE TASK IS INCLUDED IN THIS README FILE.

The INTDATA file contains an hex-ASCII dump of binary data, with some header
information for each block. This file was produced by extracting the binary data
from a data collection unit running an Intel CPU, and saving it in human-readable
format. The task is to write a recovery program that operates on the hex-ASCII
representation of the binary data and recovers the data correctly.

Here is a 'c' structure definition which shows, in the data block, the
positions of the items in the block:

struct flashdata
    {
    uint16 filename_id;                     // number for file
    uint32 sequence_in_file;                // file sequence number
    uint16 number_records;                  // recs in block
    uint16 data_length;                     // data length
    uint16 flags;                           // unused presently
    unsigned char compression_type;         // unused presently
    unsigned char spare;                    // unused presently
    unsigned char data[2048-14];            // the data itself
    };

The blocks need to have the binary data extracted from them.  Then check the
block.  If the filename_id is not 0002 then discard the block.  If the block
has obvious errors (data_length>2048-14) then discard.  Otherwise use the
data_length, number_records and data fields to extract the data records.


Record structure:

<length><flags><len2> <data.......>

Length is the length of the record and the 3-byte header.  The other two
bytes of the header can be ignored for this purpose.  The data is the
compressed record data.

The data is compressed in the following manner:

All input data is ASCII, character <128.
Any series of spaces is first compressed into a single character which is
0x80 + the number of spaces.  For example, a series of 5 spaces is compressed
into a single 0x85 character.

Then a second compression method is employed. A table is used to compress
characters into nibbles.  If a character is not in the table then three
nibbles, first F then the two nibbles of the character are used.

The table is shown below:

Character           Nibble
0                   0
1                   1
2                   2
3                   3
4                   4
5                   5
6                   6
7                   7
8                   8
9                   9
space               A       (single space character)
:                   B
/                   C
carraige return     D
linefeed            E
literal character   F

First the low nibble of a byte is used, then the high nibble.
If the last nibble is not used, it should be padded with F to ensure that
it is not used in decompression.


For example,

0123456 C<cr><lf> is compressed as:
10 32 54 A6 4F D3 FE


01          23 is compressed as:
10 af 28 f3

The extraction consists of undoing the nibble compression, then undoing
any space compressions and writing the data to a file.

This data has been previously recovered and will be used to check the
correctness of the data extraction.
