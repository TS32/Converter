#include "stdio.h"
#include "stdlib.h"
#include "string.h"

//#include <Commdlg.h>
#include <windows.h>

//Add link to the library, during compile and link, please add option '-l comdlg32' to the linker
#pragma comment(lib, "comdlg32.lib")

/*Read numbers from text file and convert them to hex, then store to a binary file */
int txt2bin(const char *txt_file, const char *bin_file, unsigned int inHex)
{
    FILE *fp_txt, *fp_bin;
    int num;
    unsigned int length = 0;
    char *FMT = inHex ?"%x" : "%d";

    fp_txt = fopen(txt_file, "r");
    if (fp_txt == NULL)
    {
        printf("Error: open file %s failed\n", txt_file);
        return -1;
    }

    fp_bin = fopen(bin_file, "wb");
    if (fp_bin == NULL)
    {
        printf("Error: open file %s failed\n", bin_file);
        return -1;
    }   

    while (fscanf(fp_txt, FMT, &num) != EOF)
    {
        //check if num is in range of 0~255
        if (num >= 0 && num < 256)
        {
            length += 1;
            //print num to console in Hex format with 0X prefix, 16 per line spereated by '\n'                
            //printf("0X%02X ", num);
            //write num to binary file
            fwrite(&num, sizeof(unsigned char), 1, fp_bin);
        }
        else
        {
            printf("Error: number %d is out of range\n", num);
            return -1;
        }
    }
    
    fclose(fp_txt);
    fclose(fp_bin); 
    printf("\nConvert %s to %s successfully, %d bytes converted!\n", txt_file, bin_file, length);

    return length;
}

/*Convert a binary file to text file seperated by space*/
int bin2txt(const char *binFile, const char *txtFile, unsigned int width, unsigned int inHex)
{
    FILE *fp_bin, *fp_txt;
    unsigned char num;
    unsigned int i;
    unsigned int length = 0;

    fp_bin = fopen(binFile, "rb");
    if (fp_bin == NULL)
    {
        printf("Error: open file %s failed\n", binFile);
        return -1;
    }

    fp_txt = fopen(txtFile, "w");
    if (fp_txt == NULL)
    {
        printf("Error: open file %s failed\n", txtFile);
        return -1;
    }

    while (fread(&num, sizeof(unsigned char), 1, fp_bin) != 0)
    {
        length++;
        if (inHex)
        {
            //printf("0X%02X ", num);
            fprintf(fp_txt, "0X%02X ", num);
        }
        else
        {
            //printf("%03d ", num);
            fprintf(fp_txt, "%03d ", num);
        }

        if (length % width == 0)
        {
            //printf("\n");
            fprintf(fp_txt, "\n");
        }
    }
    //printf("\n");
    fclose(fp_bin);
    fclose(fp_txt);
    return length;
}

int bin2Frame(const char *binFile, const char *FrameFile, unsigned int FrameLength,
              unsigned char *openFlag, unsigned int flagLen, unsigned int *compareMask, unsigned int checkerCount,unsigned int FrameID)
{
    FILE *fp_bin, *fp_frame;

    unsigned long totalFrame, actualFrame, uniqueFrame, fSize, fRead, fLeft, fFrame;
    unsigned long i, j;
    unsigned char *pFlag;
    unsigned char *previous_frame;
    unsigned char *next_frame;
    unsigned char *previous_frame_in_mask;
    unsigned char *next_frame_in_mask;
    unsigned int flagDetected, Same;

    //validate input
    if (FrameLength == 0 || flagLen == 0)
    {
        printf("Error: FrameLength or flagLen is 0\n");
        return -1;
    }

    fp_bin = fopen(binFile, "rb");
    if (fp_bin == NULL)
    {
        printf("Error: open file %s failed\n", binFile);
        return -1;
    }

    fp_frame = fopen(FrameFile, "w");
    if (fp_frame == NULL)
    {
        printf("Error: open file %s failed\n", FrameFile);
        return -1;
    }

    previous_frame = (unsigned char *)malloc(FrameLength);
    next_frame = (unsigned char *)malloc(FrameLength);
    pFlag = (unsigned char *)malloc(flagLen);
    //memset pFlag and pFrame
    memset(pFlag, 0, flagLen);
    memset(previous_frame, 0, FrameLength);
    memset(next_frame, 0, FrameLength);

    if (previous_frame == NULL || next_frame == NULL || pFlag == NULL)
    {
        //prompt error detail and exit

        printf("Error: allocate memory for previous_frame or next_frame or pFlag failed\n");
        return -1;
    }

    if (checkerCount) //there are checker bytes, allocate memory for previous_frame_in_mask and next_frame_in_mask
    {
        previous_frame_in_mask = (unsigned char *)malloc(checkerCount);
        next_frame_in_mask = (unsigned char *)malloc(checkerCount);
        memset(previous_frame_in_mask, 0, checkerCount);
        memset(next_frame_in_mask, 0, checkerCount);
        //validate the allocated memory
        if (previous_frame_in_mask == NULL || next_frame_in_mask == NULL)
        {
            printf("Error: allocate memory for previous_frame_in_mask or next_frame_in_mask failed\n");
            return -1;
        }
    }

    fseek(fp_bin, 0, SEEK_END);
    fSize = ftell(fp_bin);
    totalFrame = (unsigned long)(fSize / FrameLength);
    printf("fSize = %ld\n", fSize);
    printf("totalFrame = %ld\n", totalFrame);
    //print input parameters
    printf("FrameLength = %d\n", FrameLength);
    printf("flagLen = %d\n", flagLen);
    //print flag
    printf("open flag = ");
    for (i = 0; i < flagLen; i++)
    {
        printf("%02X ", openFlag[i]);
    }
    printf("\n");
    //print compareMask
    printf("checkerCount = %d\n", checkerCount);
    printf("compareMask = ");
    for (i = 0; i < checkerCount; i++)
    {
        printf("%d ", compareMask[i]);
    }
    printf("\n");       

    if (totalFrame == 0 || fSize <= FrameLength)
    {
        printf("Error: totalFrame is 0 or fSize <= FrameLength\n");
        return -1;
    }

    actualFrame = 0;
    uniqueFrame = 0;
    fRead = 0;
    fLeft = fSize;
    fFrame = 0;
    flagDetected = 0;
    Same = 0;

    //start from the head of the file
    fseek(fp_bin, 0, SEEK_SET);

    //Every search starting from locating the header of the frame, the is the outer loop.
    printf("\n");
    while (fLeft >= FrameLength)
    {
        //locate the first openflag from the beginning of the pbin file
        while (fLeft >= flagLen)
        {
            int bytesRead = fread(pFlag, sizeof(unsigned char), flagLen, fp_bin);            
            if (memcmp(pFlag, openFlag, flagLen) == 0)
            {
                //printf("openflag found [fRead = %d  fLeft=%d]\n",fRead,fLeft);
                flagDetected = 1;
                fRead += flagLen;
                fLeft -= flagLen;
                //printf("[Frame %d Header] Read %d bytes, fRead = %d, fLeft=%d, fSize=%d\n", actualFrame+1, bytesRead, fRead, fLeft, fSize);
                break;
            }
            else
            {
                //printf("openflag not found\n");
                fseek(fp_bin, -(flagLen - 1), SEEK_CUR);
                fRead +=1;
                fLeft -=1;
            }
            //printf("[Frame %d Header] Read %d bytes, fRead = %d, fLeft=%d, fSize=%d\n", actualFrame+1, bytesRead, fRead, fLeft, fSize);
        }
        //check if we detected the flag, or the fLeft is less than flagLen
        if (!flagDetected || fLeft < (FrameLength - flagLen))
        {
            //check if we have already get any frames written to FrameFile, if so, break the loop
            if (actualFrame)
            {

                break;
            }
            else
            {
                printf("Error: openflag not found\n");
                //close files if they're open
                if (fp_bin)
                {
                    fclose(fp_bin);
                }
                if (fp_frame)
                {
                    fclose(fp_frame);
                }
                //free memory if they're allocated
                if (previous_frame)
                {
                    free(previous_frame);
                }
                if (next_frame)
                {
                    free(next_frame);
                }
                if (pFlag)
                {
                    free(pFlag);
                }
                if (previous_frame_in_mask)
                {
                    free(previous_frame_in_mask);
                }
                if (next_frame_in_mask)
                {
                    free(next_frame_in_mask);
                }
                return -1;
            }
        }
        else
        { //flag detected, we need to read (FrameLength-flagLen) bytes to the currentFrame

            //first move the nextFrame to previousFrame
            memcpy(previous_frame, next_frame, FrameLength);

            //also move the nextFrame_in_mask to previousFrame_in_mask
            if (checkerCount)
            {
                memcpy(previous_frame_in_mask, next_frame_in_mask, checkerCount);
            }

            //then fill the flag bytes into the nextFrame
            memcpy(next_frame, pFlag, flagLen);
            //then read the rest of the frame from the bin file
            int bytesRead = fread(next_frame + flagLen, sizeof(unsigned char), FrameLength - flagLen, fp_bin);

            //check if we have read enough bytes
            if (bytesRead < (FrameLength - flagLen))
            {
                if(actualFrame)
                {
                    break;
                }
                else //no actual frame
                {
                printf("Error: read frame failed\n");
                //close files if they're open
                if (fp_bin)
                {
                    fclose(fp_bin);
                }
                if (fp_frame)
                {
                    fclose(fp_frame);
                }
                //free memory if they're allocated
                if (previous_frame)
                {
                    free(previous_frame);
                }
                if (next_frame)
                {
                    free(next_frame);
                }
                if (pFlag)
                {
                    free(pFlag);
                }
                if (previous_frame_in_mask)
                {
                    free(previous_frame_in_mask);
                }
                if (next_frame_in_mask)
                {
                    free(next_frame_in_mask);
                }
                return -1;
               }//no actual frame
            }
            else  //we got enough bytes in this frame
            {

            fRead += (FrameLength - flagLen);
            fLeft -= (FrameLength - flagLen);

            actualFrame++; //everytime when we get a whole frame, we increase the actualFrame by 1

            //check if the frame is the same as the previous frame
            if (checkerCount)
            {
                //mask the checker bytes
                for (j = 0; j < checkerCount; j++)
                    next_frame_in_mask[j] = next_frame[compareMask[j]-1]; //compareMask[j] is the jth checker byte index, index counting from 1

                //check if the checker bytes are the same
                if (memcmp(previous_frame_in_mask, next_frame_in_mask, checkerCount) == 0)
                {
                    Same = 1;
                }
                else
                {
                    Same = 0;
                }
            }
            else //there is no checker bytes, by default we don't run the check, all frames are unique and they should be written to FrameFile
            {
                Same = 0;
            }

            //if the Same is true, we need to skip the frame
            if (Same)
            {
                //printf("Same frame\n");
                continue;
            }
            else
            {
                if(FrameID) //if FrameID is not 0, we need to use the actualFrame as frame ID at the beginning of each frame in the frame file else don't write frame ID 
                {
                    fprintf(fp_frame, "%010d : ", actualFrame);
                }               
                //convert the chars in next_frame to number in hex format, and write to the frame file
                for (i = 0; i < FrameLength; i++)
                {
                    fprintf(fp_frame, "%02X ", next_frame[i]);
                }
                fprintf(fp_frame, "\n");
                uniqueFrame++;
            }
            //continue the loop for the next openFlag and frame
          } //we got enough bytes in this frame
          //printf("[Frame %d data] Read %d bytes, fRead = %d, fLeft=%d, fSize=%d\n", actualFrame, bytesRead, fRead, fLeft, fSize);
        }//flag detected
    }     //while(fLeft >=FrameLength) outer loop

    //close files
    fclose(fp_bin);
    fclose(fp_frame);

    //free memories
    free(previous_frame);
    free(next_frame);
    free(pFlag);
    if (checkerCount)
    {
        free(previous_frame_in_mask);
        free(next_frame_in_mask);
    }

    //Now we print the summary of all the informations including file path and size, total frame, unique frame, and the frame file path
    printf("\n");
    printf(" Bin Filename : %s\n", binFile);
    printf("Bin File size : %ld bytes\n", fSize);
    printf(" Total frames : %ld\n", totalFrame);
    printf("Actual frames : %ld\n", actualFrame);
    printf("Unique frames : %ld\n", uniqueFrame);
    printf("Dropped frames: %ld\n", actualFrame - uniqueFrame);
    printf("   Read Bytes : %ld\n", fRead);
    printf("   Left Bytes : %ld\n", fLeft);
    printf(" Skipped Bytes: %ld\n", fSize-fRead-fLeft);
    printf("\n");
    printf("Frame Filename: %s\n", FrameFile);
    printf("Frame Numbers : %ld\n", uniqueFrame);
    printf("\n");

    //print the framelength, openFlag length  and checkercount value
    printf("Config Data\n");
    printf("Frame length  : %d\n", FrameLength);    
    printf("OpenFlag len  : %d\n", flagLen);
    printf("Open flags    : ");
    for (i = 0; i < flagLen; i++)
    {
        printf("%02X ", openFlag[i]);
    }
    printf("\n");

    printf("Checker count : %d\n", checkerCount);

    //also print the checker bytes, openFlag and compareMask if there are any
    if (checkerCount)
    {
        printf("Checker bytes : ");
        for (i = 0; i < checkerCount; i++)
        {
            printf("%02d ", compareMask[i]);
        }
        printf("\n");

        printf("\n");
    }
    //return
    return uniqueFrame;
}

char *clean_string(char *inStr, char *char_to_remove, int num)
// remove all characters in char_to_remove from inStr and store the result in outStr
//char_to_remove is the array of characters to be removed, num is the number of characters in char_to_remove
//return the pointer to the result string
{
    char *outStr;
    int i, j, k;
    //if num is 0 or char_to_remove is NULL, use predefined char_to_remove
    if (num == 0 || char_to_remove == NULL)
    {
        char_to_remove = "\t";
        num = 1;
    }

    //allocate memory for outStr
    int len = strlen(inStr);
    outStr = (char *)malloc(len);

    for (i = 0, j = 0; i < len; i++)
    {
        for (k = 0; k < num; k++)
        {
            if (inStr[i] == char_to_remove[k])
            {
                break;
            }
        }
        if (k == num)
        {
            outStr[j++] = inStr[i];
        }
    }
    outStr[j] = '\0';
    return outStr;
}

char *single_pattern(char *inStr, char pattern)
//replace the continuous pattern with one pattern character
{
    char *outStr;
    int i, j;
    int len = strlen(inStr);
    outStr = (char *)malloc(len);

    for (i = 0, j = 0; i < len; i++)
    {
        if (inStr[i] != pattern)
        {
            outStr[j++] = inStr[i];
        }
        else
        {
            if (i == 0 || inStr[i - 1] != pattern)
            {
                outStr[j++] = inStr[i];
            }
        }
    }
    outStr[j] = '\0';
    return outStr;
}

//trim leading and tailing space from a string
char *line_trim(char *str)
{
    char *p = str;
    char *q = str;
    char delimter=' ';
    while (*p == delimter )
        p++;
    while (*p != '\0')
        *q++ = *p++;
    *q = '\0';

    //return the trimmed string
    return p;
}

//Check if a number in the array, element is unsigned integer
unsigned int is_in_array(unsigned int *array, unsigned int length, unsigned int element)
{
    unsigned int i;
    for (i = 0; i < length; i++)
    {
        if (array[i] == element)
        {
            return 1;
        }
    }
    return 0;
}

//get the size of the file
unsigned int getFileSize(const char *fileName)
{
    FILE *fp;
    unsigned int size;
    fp = fopen(fileName, "rb");
    if (fp == NULL)
    {
        printf("Error: open file %s failed\n", fileName);
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fclose(fp);
    return size;
}
unsigned int get_num_of_hex(char *str)
{
    char *token;
    int count = 0;
    token = strtok(str, " ");
    while (token != NULL)
    {
        //check if the token is a hex number
        if (strlen(token) == 4 && token[0] == '0' && (token[1] == 'X' || token[1] == 'x'))
        {
            count++;
        }
        count++;
        token = strtok(NULL, " ");
    }
    return count;
}

//get number of integers in a file
int get_num_of_int(const char *fileName, unsigned int retLines)
{
    FILE *fp;
    char line[256];
    char *token;
    int count = 0;
    int lines = 0;
    unsigned char num;

    fp = fopen(fileName, "r");
    if (fp == NULL)
    {
        printf("Error: open file %s failed\n", fileName);
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        token = strtok(line, " ");
        while (token != NULL)
        {
            count++;
            token = strtok(NULL, " ");
        }
        lines++;
    }
    fclose(fp);

    if (retLines) //if return lines, return lines
        return lines;
    else //if return number of integers, return number of integers
        return count;
}

//Check how many columns in a text file , skip the empty lines
int get_columns_count(const char *txtFile)
{
    FILE *fp_txt;
    char line[256], *pline;
    char *token;
    int count = 0;

    fp_txt = fopen(txtFile, "r");
    if (fp_txt == NULL)
    {
        printf("Error: open file %s failed\n", txtFile);
        return -1;
    }

    while (fgets(line, sizeof(line), fp_txt) != NULL)
    {
        pline = &line[0];
        line_trim(pline);
        token = strtok(pline, " ");
        while (token != NULL)
        {
            //check if the current token contains newline
            if (strchr(token, '\n') != NULL)
            {
                //if the token length is 1, just return count, otherwise return count+1
                if (strlen(token) == 1)
                    return count;
                else
                    return count + 1;
            }
            else
            {
                count++;
                token = strtok(NULL, " ");
            }
        }
    }
    return count;
}

/*Get the Nth column from a text file*/
int get_column(const char *txtFile, int column, const char *columnFile)
{
    FILE *fp_txt, *fp_column;
    char line[256];
    char *token;
    int count = 0;
    int index = 0;
    unsigned char num;

    fp_txt = fopen(txtFile, "r");
    if (fp_txt == NULL)
    {
        printf("Error: open file %s failed\n", txtFile);
        return -1;
    }

    fp_column = fopen(columnFile, "w");
    if (fp_column == NULL)
    {
        printf("Error: open file %s failed\n", columnFile);
        return -1;
    }

    while (fgets(line, sizeof(line), fp_txt) != NULL)
    {
        token = strtok(line, " ");
        while (token != NULL)
        {
            count++;
            if (count == column)
            {
                num = atoi(token);
                //fwrite(&token, sizeof(unsigned char), 1, fp_column);
                fprintf(fp_column, "0X%02X\n", num);
                index++;
            }
            token = strtok(NULL, " ");
        }
        count = 0;
    }
    fclose(fp_txt);
    fclose(fp_column);
    return index;
}

/*Get multiple columns from a text file*/
void get_columns(const char *txtFile, int total, int columnList[], const char *outFile)
{
    FILE *fp_txt, *fp_out;
    char line[256], *pline;
    char *token;
    int count = 0;
    int index = 0;
    int i;
    unsigned char num;
    unsigned char delimter = ' ';

    fp_txt = fopen(txtFile, "r");
    if (fp_txt == NULL)
    {
        printf("Error: open file %s failed\n", txtFile);
        return;
    }

    rewind(fp_txt);
    fp_out = fopen(outFile, "w");
    if (fp_out == NULL)
    {
        printf("Error: open file %s failed\n", outFile);
        return;
    }

    while (fgets(line, sizeof(line), fp_txt) != NULL)
    {
        pline = &line[0];
        pline = clean_string(pline, NULL, 0); //remove the '\t'
        pline = single_pattern(pline, delimter);   //remove duplicate space
        pline = single_pattern(pline, '\n');  //remove dulicate newline

        line_trim(pline);

        token = strtok(pline, " ");
        while (token != NULL)
        {
            count++;
            //check if the current token contains newline
            if (strchr(token, '\n') != NULL)
            {
                if (count > 0)
                {
                    fprintf(fp_out, "\n");
                    count = 0;
                    break;
                }
                else //This is empty line
                    break;
            }
            else if (is_in_array(columnList, total, count)) //This column is in the list, should be kept
            {
                num = atoi(token);
                //fprintf(fp_out, "%03d ", num);
                fprintf(fp_out, "%s ", token);
            }

            token = strtok(NULL, " ");
        }
        count = 0;
    }
    fclose(fp_txt);
    fclose(fp_out);
    //make columnList to a string for debug print   
    printf("\nGet %d columns %s to %s successfully!\n",total, txtFile, outFile); 
    printf("Columns copied: ");
    for (i = 0; i < total; i++)
    {
        printf("%d ", columnList[i]);
    }
    printf("\n");

}

/*Delete the some columns from a text file*/
void delete_columns(const char *txtFile, int total, int columnList[], const char *outFile)
{
    FILE *fp_txt, *fp_out;
    char line[256], *pline;
    char *token;
    int count = 0;
    int index = 0;
    int i;
    unsigned char num;

    fp_txt = fopen(txtFile, "r");
    if (fp_txt == NULL)
    {
        printf("Error: open file %s failed\n", txtFile);
        return;
    }

    rewind(fp_txt);
    fp_out = fopen(outFile, "w");
    if (fp_out == NULL)
    {
        printf("Error: open file %s failed\n", outFile);
        return;
    }

    while (fgets(line, sizeof(line), fp_txt) != NULL)
    {
        pline = &line[0];
        line_trim(pline);
        token = strtok(pline, " ");
        while (token != NULL)
        {
            count++;
            //check if the current token contains newline
            if (strchr(token, '\n') != NULL)
            {
                if (count > 0)
                {
                    fprintf(fp_out, "\n");
                    count = 0;
                    break;
                }
                else //This is empty line
                    break;
            }
            else if (!is_in_array(columnList, total, count)) //This column is not in the list, should be kept
            {
                num = atoi(token);
                fprintf(fp_out, "%03d ", num);
            }

            token = strtok(NULL, " ");
        }
        count = 0;
    }
    fclose(fp_txt);
    fclose(fp_out);
}


void dumpTextFile(const char *txtFile)
{
    FILE *fp_txt;
    char line[256];

    fp_txt = fopen(txtFile, "r");
    if (fp_txt == NULL)
    {
        printf("\nError: open file %s failed\n", txtFile);
        return;
    }

    //print filename as title
    printf("\n\nThe contend of %s is : \n", txtFile);

    while (fgets(line, sizeof(line), fp_txt) != NULL)
    {
        printf("%s", line);
    }
    fclose(fp_txt);
}


//dump a binary file in hex format, with a specified number of bytes per line
void dumpBinaryFile(const char *fileName, int bytesPerLine, int linesPerPage, unsigned int debugInfo)
{
    FILE *fp;
    unsigned char buffer[bytesPerLine];
    unsigned int i, j;
    unsigned int totalBytes;
    unsigned int totalLines;
    unsigned int totalPages;
    unsigned int page;
    unsigned int line;
    unsigned int offset;
    unsigned int bytes;

    fp = fopen(fileName, "rb");
    if (fp == NULL)
    {
        printf("Error: open file %s failed\n", fileName);
        return;
    }

    totalBytes = getFileSize(fileName);
    totalLines = totalBytes / bytesPerLine;

    //if line per page is 0, then print all lines
    if (linesPerPage == 0)
        linesPerPage = totalLines;

    totalPages = 1;

    //print absolute filename as title
    printf("\n\nThe contend of %s is : \n", fileName);    
    
    for (page = 0; page < totalPages; page++)
    {
        printf("\nPage %d\n", page);
        //add a header line for column index
        for (line = 0; line < linesPerPage; line++)
        {
            offset = page * linesPerPage * bytesPerLine + line * bytesPerLine;
            fseek(fp, offset, SEEK_SET);
            bytes = fread(buffer, 1, bytesPerLine, fp);
            if (bytes != bytesPerLine)
            {
                printf("Error: read file %s failed\n", fileName);
                return;
            }
            
            if(debugInfo)            
                printf("%08x: ", offset);
                        
            for (i = 0; i < bytesPerLine; i++)
            {
                printf("%02X ", buffer[i]);
            }

            if(debugInfo) 
            {
                printf(" ||  ");
                printf("%010d: ", offset);
                for (i = 0; i < bytesPerLine; i++)
                {
                    printf("%03d ", buffer[i]);
                }
            }
            printf("\n");
        }
    }
   
    fclose(fp);    
    
}

//dump a binary file in hex format to a text file, with a specified number of bytes per line
void dumpBinaryFileToFile(const char *BinfileName, const char *TxtfileName,int bytesPerLine, int linesPerPage)
{
    FILE *fp;
    FILE *fp_txt;
    unsigned char buffer[bytesPerLine];
    unsigned int i, j;
    unsigned int totalBytes;
    unsigned int totalLines;
    unsigned int totalPages;
    unsigned int page;
    unsigned int line;
    unsigned int offset;
    unsigned int bytes;
    unsigned int globalLine;

    fp = fopen(BinfileName, "rb");
    if (fp == NULL)
    {
        printf("Error: open file %s failed\n", BinfileName);
        return;
    }

    fp_txt = fopen(TxtfileName, "w");
    if (fp_txt == NULL)
    {
        printf("Error: open file %s failed\n", TxtfileName);
        return;
    }

    totalBytes = getFileSize(BinfileName);    
    totalLines = totalBytes / bytesPerLine;
    if(linesPerPage==0)  //it means no limit
        linesPerPage = totalLines;
        
    totalPages = totalLines / linesPerPage;

    //print absolute filename as title
    fprintf(fp_txt,"\n%s\n", BinfileName);
    for (page = 0; page < totalPages; page++)
    {
        fprintf(fp_txt,"\nPage %d\n", page);
        //add a header line for column index
        for (line = 0; line < linesPerPage; line++)
        {
            offset = page * linesPerPage * bytesPerLine + line * bytesPerLine;
            globalLine = page * linesPerPage + line + 1;
            fseek(fp, offset, SEEK_SET);
            bytes = fread(buffer, 1, bytesPerLine, fp);
            if (bytes != bytesPerLine)
            {
                printf("Error: read file %s failed\n", BinfileName);
                return;
            }
            fprintf(fp_txt, "%08X: ", offset);
            for (i = 0; i < bytesPerLine; i++)
            {
                fprintf(fp_txt, "%02X ", buffer[i]);
            }
            fprintf(fp_txt, " || ");
            fprintf(fp_txt, "[%08d] || %010d: ",globalLine, offset);
            for (i = 0; i < bytesPerLine; i++)
            {
                fprintf(fp_txt, "%03d ", buffer[i]);
            }
            fprintf(fp_txt, "\n");
        }   
    }

    fclose(fp);
    fclose(fp_txt);
}

//popup an open file dialog and return the file name
char *openFileDialog(const char *title, const char *filter)
{
    char *fileName = NULL;
    OPENFILENAME ofn;
    char szFileName[MAX_PATH] = "";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "txt";

    if (GetOpenFileName(&ofn))
    {
        fileName = (char *)malloc(strlen(szFileName) + 1);
        strcpy(fileName, szFileName);
    }
    return fileName;
}

//get the absolute file name from a full path
char *get_file_name(const char *fullPath)
{
    char *fileName = NULL;
    char *p = NULL;
    int len = 0;

    fileName = (char *)malloc(strlen(fullPath) + 1);
    strcpy(fileName, fullPath);
    p = strrchr(fileName, '\\');
    if (p != NULL)
    {
        len = strlen(p);
        fileName[strlen(fileName) - len] = '\0';
    }
    return fileName;
}

//get the absolute filename from fullpath without extension
char *get_file_name_without_ext(const char *fullPath)
{
    char *fileName = NULL;
    char *p = NULL;
    int len = 0;

    fileName = (char *)malloc(strlen(fullPath) + 1);
    strcpy(fileName, fullPath);
    p = strrchr(fileName, '\\');
    if (p != NULL)
    {
        len = strlen(p);
        strcpy(fileName, p + 1); //skip the '\\'
        fileName[len + 1] = '\0';
    }
    p = strrchr(fileName, '.');
    if (p != NULL)
    {
        len = strlen(p);
        fileName[strlen(fileName) - len] = '\0';
    }
    return fileName;
}

//get the extension of a file
char *get_file_ext(const char *fullPath)
{
    char *p = NULL;
    p = strrchr(fullPath, '.');
    return p;
}

//get the directory of a file
char *get_file_dir(const char *fullPath)
{
    char *fileDir = NULL;
    char *p = NULL;
    int len = 0;

    fileDir = (char *)malloc(strlen(fullPath) + 1);
    strcpy(fileDir, fullPath);
    p = strrchr(fileDir, '\\');
    if (p != NULL)
    {
        len = strlen(p) - 1;
        fileDir[strlen(fileDir) - len] = '\0';
    }
    return fileDir;
}
//replace the extension of a file
char *replace_file_ext(const char *fullPath, const char *newExt)
{
    char *filePath = NULL;
    char *p = NULL;
    int len = 0;

    filePath = (char *)malloc(strlen(fullPath) + 1);
    strcpy(filePath, fullPath);
    p = strrchr(filePath, '.');
    if (p != NULL)
    {
        len = strlen(p);
        filePath[strlen(filePath) - len] = '\0';
    }
    strcat(filePath, newExt);
    return filePath;
}

//make a new filename from directory and file name and extension
char *make_file_path(const char *dir, const char *fileName, const char *ext)
{
    char *filePath = NULL;
    int len = 0;

    filePath = (char *)malloc(strlen(dir) + strlen(fileName) + strlen(ext) + 1);
    strcpy(filePath, dir);
    strcat(filePath, fileName);
    strcat(filePath, ext);
    return filePath;
}

//add prefix to a string
char *str_add_prefix(const char *str, const char *prefix)
{
    char *newStr = NULL;
    int len = 0;

    newStr = (char *)malloc(strlen(str) + strlen(prefix) + 1);
    strcpy(newStr, prefix);
    strcat(newStr, str);
    return newStr;
}

//add suffix to a string
char *str_add_suffix(const char *str, const char *suffix)
{
    char *newStr = NULL;
    int len = 0;

    newStr = (char *)malloc(strlen(str) + strlen(suffix) + 1);
    strcpy(newStr, str);
    strcat(newStr, suffix);

    return newStr;
}

//replace filename in a full path
char *replace_filename(const char *fullPath, const char *newFileName)
{
    char *filePath = NULL;
    char *p = NULL;
    int len = 0;

    char *filedir = get_file_dir(fullPath);
    char *oldfilename = get_file_name_without_ext(fullPath);
    char *oldext = get_file_ext(fullPath);
    char *newfullPath = make_file_path(filedir, newFileName, oldext);

    free(filedir);
    free(oldfilename);

    return newfullPath;
}

//add prefix to the absolute filename and return the new filepath
char *add_prefix_to_filename(const char *fullPath, const char *prefix)
{
    char *filePath = NULL;
    char *p = NULL;
    int len = 0;

    char *filedir = get_file_dir(fullPath);
    char *oldfilename = get_file_name_without_ext(fullPath);
    char *oldext = get_file_ext(fullPath);
    char *newfilename = str_add_prefix(oldfilename, prefix);
    char *newfullPath = make_file_path(filedir, newfilename, oldext);

    free(filedir);
    free(oldfilename);
    free(newfilename);

    return newfullPath;
}

//add surfix to the absolute filename and return the new filepath
char *add_suffix_to_filename(const char *fullPath, const char *suffix)
{
    char *filePath = NULL;
    char *p = NULL;
    int len = 0;  
    char *filedir;

    //check if the fullPath contains a directory
    if (strchr(fullPath, '\\') != NULL)    
        filedir = get_file_dir(fullPath);           
    else            
        filedir = ".\\"; //if there is no directory, use the current directory
           
    printf("filedir: %s\n", filedir);

    char *oldfilename = get_file_name_without_ext(fullPath);
    printf("oldfilename: %s\n", oldfilename);
    char *oldext = get_file_ext(fullPath);
    printf("oldext: %s\n", oldext);
    char *newfilename = str_add_suffix(oldfilename, suffix);
    printf("newfilename: %s\n", newfilename);
    char *newfullPath = make_file_path(filedir, newfilename, oldext);
    printf("newfullPath: %s\n", newfullPath);

    free(filedir);
    free(oldfilename);
    free(newfilename);

    return newfullPath;
}

//check if a file exists, if exist delete it else do nothing
int delete_file(const char *fileName)
{
    FILE *fp;
    if ((fp = fopen(fileName, "r")) == NULL)
    {
        return 0;
    }
    else
    {
        fclose(fp);
        remove(fileName);
        return 1;
    }
}


void generateTestData(const char *filename, unsigned int isTxt, unsigned int length, unsigned int numPerLine,unsigned char delimter, unsigned int isHex, unsigned int Leading0x,unsigned int flagLen,const char *flag)
//generate some test data to file specified by filename.
//isTxt: 1 for text file, 0 for binary file
//length: how many ints should be genrated
//numPerLine: number of ints per line  (for text file only)
//delimter: delimiter between ints (for text file only)
//isHex: 1 for hex, 0 for decimal, if this is 1, it means the number is in hex format, otherwise it is in decimal format (for text file only)
//Leading0x: if this is 1, means all hex ints need to have '0x', otherwise no need to have '0x' at the beginning of every hex ints.
{
    FILE *fp;
    unsigned char num;
    unsigned int i, lineID,colID;

    if ((fp = fopen(filename, "w")) == NULL) 
    {
        printf("Error: cannot open file %s\n", filename);
        return;
    }

    //if flagLen is longer than numPerLine, it means the flag is longer than one line, this is wrong
    if(flagLen>numPerLine)
    {
        printf("Error: flagLen is longer than numPerLine\n");
        return;
    }
    
    lineID=0;
    colID=0;

    for (i = 0; i < length; i++)
    {        
        num = rand() % 256;

        if (isTxt)
        {
            //check if colID is larger than flagLen, if so, num need to be read from flag
            if(colID < flagLen)
            {
                num = flag[colID];
            }
            else if(colID == numPerLine - 1)
            {
                num = lineID % 256;  //for the last line, the last int is the lineID
            }
            else
            {
               //do nothing
            }

            colID++;

            if (isHex)
            {
                if (Leading0x)
                {
                    fprintf(fp, "0x%02x%c", num, delimter);
                }
                else
                {
                    fprintf(fp, "%02x%c", num, delimter);
                }
            }
            else
            {
                fprintf(fp, "%03d%c", num, delimter);
            }
        }
        else
        {
            
            fwrite(&num, sizeof(unsigned char), 1, fp);
        }

        if ((i+1) % numPerLine == 0)
        {
            if (isTxt)
            {
                fprintf(fp, "\n");
                lineID++;
                colID=0;
            }
        }        
    }

    fclose(fp);  
    printf("Generate test data to file %s successfully! %d lines, %d per line\n", filename, lineID, numPerLine);

}
int testCase_Console()
{

    int width = 16;
    int columnsList[3] = {1, 3, 5};
    unsigned int flagLen = 2;
    unsigned char flag[2] = {0x01, 0x02};


    generateTestData(".\\data\\numbers.txt", 1, width*100, width, ' ', 0, 0, flagLen, flag);

    txt2bin(".\\data\\numbers.txt", ".\\data\\numbers.bin",0);
    bin2txt(".\\data\\numbers.bin", ".\\data\\numbers_hex.txt", width, 1);
    bin2txt(".\\data\\numbers.bin", ".\\data\\numbers_dec.txt", width, 0);
    get_column(".\\data\\numbers_hex.txt", 2, ".\\data\\numbers_column2.txt");

    dumpTextFile(".\\data\\numbers_dec.txt");
    delete_columns(".\\data\\numbers_dec.txt", 3, columnsList, ".\\data\\numbers_afterDelete.txt");
    dumpTextFile(".\\data\\numbers_afterDelete.txt");

    int size = getFileSize(".\\data\\numbers_afterDelete.txt");
    int count = get_num_of_int(".\\data\\numbers_afterDelete.txt", 0);
    int lines = get_num_of_int(".\\data\\numbers_afterDelete.txt", 1);
    int columns = get_columns_count(".\\data\\numbers_afterDelete.txt");

    printf("file=%s, size = %d bytes, lines=%d, columns= %d, number_of_int = %d \n", "numbers_afterDelete.txt", size, lines, columns, count);
    return 0;
}

int testCase_GUI()
{
    unsigned int framesNumber=50;
    unsigned int frameLen = 16;    
    unsigned char delimter = ' ';
    unsigned int flagLen = 2;    
    unsigned char flag[2] = {0xEB, 0x90};
    unsigned int totalLength = framesNumber * frameLen;
    
    //txt to bin
    const char *txtFile_dec = ".\\data\\txt_in_dec.txt"; //DEC Text
    const char *txtFile_hex_no0x = ".\\data\\txt_in_hex_no0x.txt"; //HEX Text
    const char *txtFile_hex_with_0x = ".\\data\\txt_in_hex_with_0x.txt"; //HEX Text
    
    const char *binFile_from_dec = replace_file_ext(txtFile_dec,".bin");
    const char *binFile_from_hex_no0x = replace_file_ext(txtFile_hex_no0x,".bin");
    const char *binFile_from_hex_with_0x = replace_file_ext(txtFile_hex_with_0x,".bin");    

    printf("\n- Example 0: generate some test data in text format  -\n");
    //start to convert, convert from a text file in dec format to a binary file
    generateTestData(txtFile_dec, 1, totalLength, frameLen, delimter, 0, 0, flagLen, flag);
    generateTestData(txtFile_hex_no0x, 1, totalLength, frameLen, delimter, 1, 0, flagLen, flag);
    generateTestData(txtFile_hex_with_0x, 1, totalLength, frameLen, delimter, 1, 1, flagLen, flag);
    printf("\n--Example 0 Done--\n"); 

    printf("\n- Example 1: Convert Txt to Bin, source data is in dec format  -\n");
    dumpTextFile(txtFile_dec);
    txt2bin(txtFile_dec, binFile_from_dec, 0);
    dumpBinaryFile(binFile_from_dec, frameLen, 0,0);
    printf("\n--Example 1 Done--\n"); 

    //convert from a text file in hex format to a binary file
    printf("\n- Example 2: Convert Txt to Bin, source data is in hex format, but without 0x  -\n");
    dumpTextFile(txtFile_hex_no0x);
    txt2bin(txtFile_hex_no0x, binFile_from_hex_no0x, 1);
    dumpBinaryFile(binFile_from_hex_no0x, frameLen, 0, 0);
    printf("\n--Example 2 Done--\n");  

    //convert from a text file in hex format with leading 0x to a binary file
    printf("\n- Example 3: Convert Txt to Bin, source data is in hex format, but with 0x  -\n");
    dumpTextFile(txtFile_hex_with_0x);
    txt2bin(txtFile_hex_with_0x, binFile_from_hex_with_0x, 1);
    dumpBinaryFile(binFile_from_hex_with_0x, frameLen, 0, 0);
    printf("\n--Example 3 Done--\n");  

    printf("\n- Example 4: Convert bin file to frames in text- \n");
    //bin to frames
    //get the fullpath of the binary file       
    const char *binFile_from_dec_frame = add_suffix_to_filename(binFile_from_dec,"_frame");
    const char *binFile_from_hex_no0x_frame = add_suffix_to_filename(binFile_from_hex_no0x,"_frame");
    const char *binFile_from_hex_with_0x_frame = add_suffix_to_filename(binFile_from_hex_with_0x,"_frame");

    //convert from a binary file to a frame file
    unsigned int logFrameID  = 0;   //Dont' log frame ID
    unsigned int checkerCount = 0;  // Don't check if frames are duplicate
    unsigned int frame_checker[] ={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    unsigned int *checker = &frame_checker[0];
    
    bin2Frame(binFile_from_dec, binFile_from_dec_frame, frameLen, flag, flagLen, checker, checkerCount, logFrameID);
    bin2Frame(binFile_from_hex_no0x, binFile_from_hex_no0x_frame, frameLen, flag, flagLen, checker, checkerCount, logFrameID);
    bin2Frame(binFile_from_hex_with_0x, binFile_from_hex_with_0x_frame, frameLen, flag, flagLen, checker, checkerCount, logFrameID);

    printf("\n--Example 4 Done--\n");  

    //dump the binary file to debug file, first create filename
    printf("\n- Example 5: Convert bin file to debug comparsion frames in text- \n");
    const char *binFile_from_dec_dump = add_suffix_to_filename(binFile_from_dec,"_dump");
    const char *binFile_from_hex_no0x_dump = add_suffix_to_filename(binFile_from_hex_no0x,"_dump");
    const char *binFile_from_hex_with_0x_dump = add_suffix_to_filename(binFile_from_hex_with_0x,"_dump");

    dumpBinaryFileToFile(binFile_from_dec, binFile_from_dec_dump, frameLen, 0);
    dumpBinaryFileToFile(binFile_from_hex_no0x, binFile_from_hex_no0x_dump, frameLen, 0);
    dumpBinaryFileToFile(binFile_from_hex_with_0x, binFile_from_hex_with_0x_dump, frameLen, 0);

    printf("\n--Example 5 Done--\n"); 

    //get some columns from the txt file    
    printf("\n- Example 6: Get some columns from the txt data file  to a new file -\n");
    const char *txtFile_dec_col = add_suffix_to_filename(txtFile_dec,"_col");
    const char *txtFile_hex_no0x_col = add_suffix_to_filename(txtFile_hex_no0x,"_col");
    const char *txtFile_hex_with_0x_col = add_suffix_to_filename(txtFile_hex_with_0x,"_col");

    unsigned int col_num = 10;
    unsigned int col_list[] ={3,4,5,6,7,8,9,10,11,12};
    
    get_columns(txtFile_dec, col_num, col_list, txtFile_dec_col);    
    dumpTextFile(txtFile_dec_col);

    get_columns(txtFile_hex_no0x, col_num, col_list, txtFile_hex_no0x_col);
    dumpTextFile(txtFile_hex_no0x_col);

    get_columns(txtFile_hex_with_0x, col_num, col_list, txtFile_hex_with_0x_col);
    dumpTextFile(txtFile_hex_with_0x_col);

    //free memory
    free((void *)txtFile_dec_col);
    free((void *)txtFile_hex_no0x_col);
    free((void *)txtFile_hex_with_0x_col);        
    printf("\n--Example 6 Done--\n");

    //free the memory and close files
    free((void *)binFile_from_dec);
    free((void *)binFile_from_hex_no0x);
    free((void *)binFile_from_hex_with_0x);
    free((void *)binFile_from_dec_dump);
    free((void *)binFile_from_hex_no0x_dump);
    free((void *)binFile_from_hex_with_0x_dump);
    free((void *)binFile_from_dec_frame);
    free((void *)binFile_from_hex_no0x_frame);
    free((void *)binFile_from_hex_with_0x_frame);   

    //wait for user input
    printf("\n--All tests Done--\n");        

    return 0;
}

#ifndef _WIN32
int main(void)
{
    unsigned char file_name[] = "test.txt";
    unsigned char file_name_bin[] = "test.bin";
    txt2bin(file_name, file_name_bin,0);
    return 0;
}
#else
//Standard windows main function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  testCase_GUI();
  //testCase_Console();
  return 0;
}
#endif